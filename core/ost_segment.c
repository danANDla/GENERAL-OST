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

void set_type(OstSegmentHeader* const header, const SegmentType t) {

}

int8_t data_to_ost_segment(OstSegment* const seg, void* data, uint8_t sz){
    return 1;
}