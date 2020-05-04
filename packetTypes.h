#ifndef PHOTON_PACKETTYPES_H
#define PHOTON_PACKETTYPES_H

// Packet sizes for sending routine
#define SIZE_OPEN_CONNECTION 9
#define SIZE_STATUS_RESPONSE 11
#define SIZE_DATA_DUMP 129

#define CURRENT_VERSION 0
#define PACKET_ID 0xBA

typedef enum {
    OPEN_CONNECTION,
    OPEN_RESPONSE,
    STATUS,
    STATUS_RESPONSE,
    DATA_DUMP,
    UNKNOWN
} subtype_t;

struct __attribute__((__packed__)) pHeader {
    uint8_t ID          = PACKET_ID;
    uint8_t version     = CURRENT_VERSION;
};

struct __attribute__((__packed__)) pOpenConnection {
    struct pHeader head;
    subtype_t subtype = OPEN_CONNECTION;
    uint8_t mac[6];
};

struct __attribute__((__packed__)) pStatusResponse {
    struct pHeader head;
    subtype_t subtype = STATUS_RESPONSE;
    int16_t pmean;
    uint16_t urms;
    int16_t powerf;
    uint16_t freq;
};

struct __attribute__((__packed__)) dataChunk {
    int16_t pmean = 0;
    uint32_t t = 0;
};

struct __attribute__((__packed__)) pDataDump {
    struct pHeader head;
    subtype_t subtype = DATA_DUMP;
    struct dataChunk chunk[21];
};

#endif //PHOTON_PACKETTYPES_H
