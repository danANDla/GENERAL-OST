#include "ost_segment.h"

#include <string.h>

bool is_ack(const OstSegmentHeader* const header){
    return header->flag & 0x80;
}

int8_t peekHeader(const SpWPacket* const packet, OstSegmentHeader* const header) {
    if(packet->sz < sizeof(OstSegmentHeader)) return -1;
    if(memcpy(header, packet->data, sizeof(OstSegmentHeader)) != sizeof(OstSegmentHeader)) return -1;
    return 0;    
}