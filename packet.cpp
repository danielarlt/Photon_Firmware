#include "packet.h"

using namespace std;

uint8_t packet::getID() {
    return this->ID;
}

void packet::setID(uint8_t store) {
    ID = store;
    return;
}

uint8_t packet::getVersion() {
    return this->version;
}

void packet::setVersion(uint8_t store) {
    version = store;
    return;
}

subtype_t packet::getSubtype() {
    return this->subtype;
}

void packet::setSubtype(subtype_t store) {
    subtype = store;
    return;
}

int packet::getSize() {
    return this->size;
}

void packet::setSize(int store) {
    size = store;
    return;
}

bool packet::getSend() {
    return this->send;
}

void packet::setSend(bool store) {
    send = store;
    return;
}

vector<char> packet::getData() {
    return this->data;
}

void packet::setData(char* store) {
    vector<char> newVect(store, store + (size - 3));
    data = newVect;
    return;
}

vector<char> packet::getPacketArray() {
    return this->packetArray;
}

packet::packet() {
    subtype = UNKNOWN;
    size = 0;
    send = false;
}

packet::packet(subtype_t pSubtype) {
    subtype = pSubtype;
    switch (subtype) {
        case OPEN_CONNECTION:     // Open connection
            size = 9;
            break;
        case STATUS_RESPONSE:     // Status response
            size = 11;
            break;
        case DATA_DUMP:     // Data dump (340 points)
            size = 2043;
            break;
        default:
            size = 4;
            break;
    }
    data.resize(size - 3);
    packetArray.resize(size);
    send = false;
}

packet::~packet() {
}

// packet::packet(const packet &p2) {
//     ID = p2.ID;
//     version = p2.version;
//     subtype = p2.subtype;
//     size = p2.size;
//     send = p2.send;
//     data = p2.data;
//     packetArray = p2.packetArray;
// }


char* packet::asArray() {
    packetArray[0] = ID;
    packetArray[1] = version;
    packetArray[2] = subtype;
    for (int i = 3; i < size; i++) packetArray[i] = data[i - 3];
    return this->packetArray.data();
}
