#ifndef PHOTON_PACKET_H
#define PHOTON_PACKET_H

#define CURRENT_VERSION 0
#define PACKET_ID 0xBA

#include <cstdint>
#include <vector>

using namespace std;

typedef enum {
    OPEN_CONNECTION,
    OPEN_RESPONSE,
    STATUS,
    STATUS_RESPONSE,
    DATA_DUMP,
    UNKNOWN
} subtype_t;

class packet {
private:
    uint8_t     ID = PACKET_ID;
    uint8_t     version = CURRENT_VERSION;
    subtype_t   subtype;
    int         size;
    bool        send;
    vector<char>       data;
    vector<char>       packetArray;

public:
    uint8_t getID();
    void setID(uint8_t store);

    uint8_t getVersion();
    void setVersion(uint8_t store);

    subtype_t getSubtype();
    void setSubtype(subtype_t store);

    int getSize();
    void setSize(int store);

    bool getSend();
    void setSend(bool store);

    vector<char> getData();
    void setData(char* store);

    vector<char> getPacketArray();

    // Constructors
    packet();
    packet(subtype_t);

    // Destructor
    ~packet();
    // Copy constructor
    // packet(const packet &p2);

    char* asArray();

};

#endif //PHOTON_PACKET_H
