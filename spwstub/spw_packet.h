
#ifndef SPW_PACKET_H
#define SPW_PACKET_H

#define MAX_PACKET_LEN 8192

#include <inttypes.h>

typedef struct {
    uint16_t     s_payload_len;  ///< length of payload
} __attribute__((packed)) PacketHeader;

enum {
    MAX_PACKET_PAYLOAD_LEN = MAX_PACKET_LEN - sizeof(PacketHeader)
};

typedef struct {
    PacketHeader s_header;
    char s_payload[MAX_PACKET_PAYLOAD_LEN]; ///< Must be used as a buffer, unused "tail"
                                     ///< shouldn't be transfered
} __attribute__((packed)) Packet;

#endif