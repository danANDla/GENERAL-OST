
#ifndef SPW_PACKET_H
#define SPW_PACKET_H

#define MAX_PACKET_LEN 8192

#include <inttypes.h>

typedef struct {
    uint8_t b;
} SpWCharcter; 


typedef enum {
    FCT_CH = 0,
    EOP_CH,
    EEP_CH,
    ESC_CH,
    NULL_CH,
    TIME_CH,
    DATA_CH, 
    BAD_CH
} CharacterType;

typedef struct {
    uint16_t     s_payload_len;  ///< length of payload
} __attribute__((packed)) PacketHeader;


enum {
    MAX_PACKET_PAYLOAD_LEN = MAX_PACKET_LEN - sizeof(PacketHeader)
};

typedef struct {
    PacketHeader s_header;
    SpWCharcter s_payload[MAX_PACKET_PAYLOAD_LEN]; ///< Must be used as a buffer, unused "tail"
                                     ///< shouldn't be transfered
} __attribute__((packed)) Packet;

extern const Packet NULL_PACKET;
extern const Packet FCT_PACKET;

CharacterType get_charater_type(const SpWCharcter* const ch);
void decode_character(char* const str, const SpWCharcter* const ch);
void decode_packet(Packet packet);

#endif