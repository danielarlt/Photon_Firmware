#include "packetTypes.h"

// Sample rate in seconds
#define SAMPLE_RATE 180

// Uncomment this to get various debug messages output over serial
#define SERIAL_DEBUG

void sendPacket(void* toSend, uint16_t size);
void packetHandler(char* buf);
void takeSample(void);
void TCP_Connect(void);

int16_t endianswap_16_signed(int16_t val);
uint16_t endianswap_16_unsigned(uint16_t val);
uint32_t endianswap_32_unsigned(uint32_t val);

// This disables automatic network connecting and a few other things for more dev control
SYSTEM_MODE(MANUAL);

UDP Udp;
TCPClient client;
int led = D7;

// This address needs to reflect the IP of the TCP server
IPAddress serverIP(192,168,68,123);
// This port needs to reflect the port of the TCP server
int TCPort = 7060;

void setup() {
    WiFi.setListenTimeout(30);
    // Connect to the WiFi network
    WiFi.connect();

    // Set LED for output
    pinMode(led, OUTPUT);

    // Begin Serial and setup SPI1
    Serial.begin(9600);
    SPI1.begin(SPI_MODE_MASTER);

    // Wait until WiFi is connected
    while(WiFi.connecting()){}
    // Connect to the TCP server
    TCP_Connect();
}

void loop() {
    // For keeping track of sample times
    static uint32_t oldTime, newTime = 0;

    // Ensure that the TCP server is connected
    if (!client.connected()) {
        // If the TCP server isn't connected, disable LED and try to connect
        digitalWrite(led, LOW);
        TCP_Connect();
    }
    else {
        // Read buffer for TCP
        char buf[50];
        // Read from the TCP handler
        int bytesRead = client.read((uint8_t*)buf, sizeof(buf));
        // If we get bytes from the TCP server then handle the packet
        if (bytesRead > 0) {
            packetHandler(buf);
            // Clear buffer
            for (int i = 0; i < bytesRead; i++) {
                buf[i] = '\0';
            }
        }
    }

    // Get new time for sample check
    newTime = Time.now();
    // If we're at or over our sample rate then take a sample
    if ((newTime - oldTime) >= SAMPLE_RATE) {
        oldTime = newTime;
        takeSample();
    }
}


/**
 * Handle the packet provided
 *
 * @param char* buf whose value is the bytes from the TCP read
 * @return None
 */
void packetHandler(char* buf) {
    // Get packet header details
    uint8_t pID = buf[0];
    uint8_t pVersion = buf[1];
    subtype_t pSubtype = (subtype_t)buf[2];

    // Verify that packet is correct version and ID
    if (pID != PACKET_ID || pVersion != CURRENT_VERSION) {
        #ifdef SERIAL_DEBUG
            Serial.println("Error: Incorrect packet ID or version!");
        #endif

        return;
    }
    // Packet is valid, process based on subtype
    else {
        switch (pSubtype) {
            case OPEN_RESPONSE: {     // Open response packet - time
                #ifdef SERIAL_DEBUG
                    Serial.println("Open Response receieved.");
                #endif

                // Get time from Open Response packet and set system time
                unsigned long int t;
                t = (buf[3] << 24) | (buf[4] << 16) | (buf[5] << 8) | (buf[6]);
                Time.setTime(t);
            }
            break;

            case STATUS: {
                #ifdef SERIAL_DEBUG
                    Serial.println("Status receieved. Building Status Response.");
                #endif

                // Prepare to send Status Response packet
                struct pStatusResponse toSend;
                // SPI transmit/receive buffers
                char rx[2] = {0,0};
                char tx[2] = {0,0};
                int16_t sStore;
                uint16_t uStore;
                // Read registers and pack
                tx[1] = 74; // Pmean register at 4AH
                SPI1.transfer(tx, rx, sizeof(rx), NULL);
                sStore = (rx[0] << 8) | rx[1];
                toSend.pmean = endianswap_16_signed(sStore);

                #ifdef SERIAL_DEBUG
                    Serial.print("Pmean SPI read: ");
                    Serial.print(sStore);
                #endif

                rx[0], rx[1] = 0;
                tx[1] = 73; // Urms register at 49H
                SPI1.transfer(tx, rx, sizeof(rx), NULL);
                uStore = (rx[0] << 8) | rx[1];
                toSend.urms = endianswap_16_unsigned(uStore);

                #ifdef SERIAL_DEBUG
                    Serial.print("Urms SPI read: ");
                    Serial.print(uStore);
                #endif

                rx[0], rx[1] = 0;
                tx[1] = 77; // Powerf register at 4DH
                SPI1.transfer(tx, rx, sizeof(rx), NULL);
                sStore = (rx[0] << 8) | rx[1];
                toSend.powerf = endianswap_16_signed(sStore);

                #ifdef SERIAL_DEBUG
                    Serial.print("PowerF SPI read: ");
                    Serial.print(sStore);
                #endif

                rx[0], rx[1] = 0;
                tx[1] = 76; // Freq register at 4CH
                SPI1.transfer(tx, rx, sizeof(rx), NULL);
                uStore = (rx[0] << 8) | rx[1];
                toSend.freq = endianswap_16_unsigned(uStore);

                #ifdef SERIAL_DEBUG
                    Serial.print("Freq SPI read: ");
                    Serial.print(uStore);
                #endif

                // Send fully built Status Response packet
                sendPacket(&toSend, SIZE_STATUS_RESPONSE);
            }
            break;

            default: {
                #ifdef SERIAL_DEBUG
                    Serial.println("Unknown Packet");
                #endif
            }
            break;
        }
    }
}


/**
 * Send the provided packet
 *
 * @param void* toSend whose contents are sent out over TCP
 * @param uint16_t size whose value reflects the size of the provided packet
 * @return None
 */
void sendPacket(void* toSend, uint16_t size) {
    // Make sure we didn't lose TCP connection
    if (client.connected()) {
        // Send packet to TCP server
        client.write((uint8_t*)toSend, size);

        #ifdef SERIAL_DEBUG
            Serial.println("Sent packet!");

            int err = client.getWriteError();
            if (err != 0) {
                Serial.print("Error: ");
                Serial.print(err);
            }
        #endif
    }
    else {
        #ifdef SERIAL_DEBUG
            Serial.println("Failed to send. No connection to TCP server.");
        #endif
    }
}


/**
 * Reads Pmean over SPI and stores the sample with the time in the EEPROM.
 * If a dataDump packet can be filled, then it is filled and sent
 *
 * @param None
 * @return None
 */
void takeSample(void) {
    static int eeAddress = 0;
    static uint16_t sampleCount = 0;

    // SPI transmit/receive buffers
    char rx[2] = {0,0};
    char tx[2] = {0,0};
    int16_t sStore;

    #ifdef SERIAL_DEBUG
        Serial.print("Taking sample: ");
        Serial.println(sampleCount);
    #endif

    tx[1] = 74; // Pmean register at 4AH
    SPI1.transfer(tx, rx, sizeof(rx), NULL);
    sStore = (rx[0] << 8) | rx[1];

    #ifdef SERIAL_DEBUG
        Serial.print("Pmean SPI read: ");
        Serial.println(sStore);
    #endif

    // sStore = 0x0AAA;

    // Get dataChunk to store sample
    struct dataChunk data;
    data.pmean = endianswap_16_signed(sStore);
    data.t = endianswap_32_unsigned(Time.now());

    // Store dataChunk in the EEPROM at the specified address
    EEPROM.put(eeAddress, data);
    // Increment EEPROM address to prevent data overwriting
    eeAddress += sizeof(struct dataChunk);
    sampleCount++;

    // Check if we have enough samples to send to the server
    if (sampleCount == 20) {
        #ifdef SERIAL_DEBUG
            Serial.println("Building dataDump");
        #endif

        // Reset statics
        eeAddress = 0;
        sampleCount = 0;

        // Get dataDump for storing samples from EEPROM
        struct pDataDump toSend;
        for (int j = 0; j < 20; j++) {
            // Get all the samples and store in packet
            EEPROM.get(eeAddress, toSend.chunk[j]);
            eeAddress += sizeof(struct dataChunk);
        }
        // Append an empty sample for software parsing
        struct dataChunk empty;
        toSend.chunk[21] = empty;
        // Send filled dataDump packet
        sendPacket(&toSend, SIZE_DATA_DUMP);
        // Clear EEPROM and reset the address one more time for future collection
        EEPROM.clear();
        eeAddress = 0;
    }
}


/**
 * Attempt to connect to the TCP server and send an Open Connection packet
 *
 * @param None
 * @return None
 */
void TCP_Connect(void) {
    if (client.connect(serverIP, TCPort)) {
        #ifdef SERIAL_DEBUG
            Serial.println("Connected to TCP server.");
        #endif

        // Enable LED to indicate TCP connection
        digitalWrite(led, HIGH);

        // Get device MAC address
        byte mac[6];
        WiFi.macAddress(mac);

        // Build Open Connection packet
        struct pOpenConnection toSend;
        memcpy(&toSend.mac, &mac, 6);
        // Send filled Open Connection packet
        sendPacket(&toSend, SIZE_OPEN_CONNECTION);
    }
    else {
        #ifdef SERIAL_DEBUG
            Serial.println("TCP connection failed.");
        #endif
    }
}


/**
 * Swap endian of a 16 bit signed integer
 *
 * @param int16_t val whose value will have its endian swapped
 * @return int16_t swapped whose value reflects the endian swapped result
 */
int16_t endianswap_16_signed(int16_t val) {
    int16_t swapped = ((val >> 8) & 0x00ff) | ((val & 0x00ff) << 8);
    return swapped;
}


/**
 * Swap endian of a 16 bit unsigned integer
 *
 * @param uint16_t val whose value will have its endian swapped
 * @return uint16_t swapped whose value reflects the endian swapped result
 */
uint16_t endianswap_16_unsigned(uint16_t val) {
    uint16_t swapped = ((val >> 8) & 0x00ff) | ((val & 0x00ff) << 8);
    return swapped;
}


/**
 * Swap endian of a 32 bit unsigned integer
 *
 * @param uint32_t val whose value will have its endian swapped
 * @return uint32_t swapped whose value reflects the endian swapped result
 */
uint32_t endianswap_32_unsigned(uint32_t val) {
    uint32_t b0, b1, b2, b3;
    b0 = (val & 0x000000ff) << 24u;
    b1 = (val & 0x0000ff00) << 8u;
    b2 = (val & 0x00ff0000) >> 8u;
    b3 = (val & 0xff000000) >> 24u;

    return (b0 | b1 | b2 | b3);
}
