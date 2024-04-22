#include <inttypes.h>
#include <stdbool.h>

#include "spw_packet.h"

#ifndef OST_SEGMENT_H
#define OST_SEGMENT_H

typedef uint8_t FlagOctet;

typedef enum {
    DATA = 0,
    ACK = 1
} SegmentType;

typedef struct {
    FlagOctet flag;
    uint8_t seq_number;
    uint8_t source_addr;
    uint8_t payload_length;
} OstSegmentHeader;

typedef struct { 
    OstSegmentHeader header;
    uint8_t* payload;
} OstSegment;

int8_t data_to_ost_segment(OstSegment* const seg, void* data, uint8_t sz);
int8_t peekHeader(const SpWPacket* const packet, OstSegmentHeader* const header);
void set_type(OstSegmentHeader* const header, const SegmentType t);
bool is_ack(const OstSegmentHeader* const header);

#endif