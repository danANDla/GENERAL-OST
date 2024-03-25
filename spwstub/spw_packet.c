
#include "spw_packet.h"
#include "../utils/bit_io.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const SpWCharcter NULL_CHARACTER = {.b=0x74};
static const SpWCharcter FCT_CHARACTER = {.b=0x40};

const Packet NULL_PACKET = {
    .s_header = { .s_payload_len = 1},
    .s_payload[0] = NULL_CHARACTER
};

const Packet FCT_PACKET = {
    .s_header = { .s_payload_len = 1},
    .s_payload[0] = FCT_CHARACTER
};

CharacterType get_charater_type(const SpWCharcter* const ch) {
    if((ch->b & 0x40) == 0) return DATA_CH;
    else if((ch->b & 0x74) << 1 == 0xE8) return NULL_CH;
    else if((ch->b & 0x40) << 1 == 0x80) return FCT_CH;
    return BAD_CH;
}

void decode_character(char* const str, const SpWCharcter* const ch) {
    switch (get_charater_type(ch)) {
    case FCT_CH:
        strcpy(str, "FCT");
        break;
    case EOP_CH:
        strcpy(str, "FCT");
        break;
    case EEP_CH:
        strcpy(str, "EEP");
        break;
    case ESC_CH:
        strcpy(str, "ESC");
        break;
    case NULL_CH:
        strcpy(str, "NLL");
        break;
    case TIME_CH:
        strcpy(str, "TME");
        break;
    case DATA_CH:
        strcpy(str, "DTA");
        break;
    case BAD_CH:
        strcpy(str, "BAD");
        break;
    }
}

void decode_packet(Packet packet){
    char* dec = malloc(sizeof(char) * 3);
    decode_character(dec, &packet.s_payload[0]);
    printf("%s ", dec);
    for(int i = 0; i < sizeof(PacketHeader); ++i) {
        print_bits(*(((char*)&packet) + i));
        printf(" ");
    }
    printf("| ");
    for(int i = 0; i < packet.s_header.s_payload_len; ++i) {
        print_bits(*(((char*)&packet.s_payload) + i));
        printf(" ");
    }
    printf("\n");
    free(dec);
}