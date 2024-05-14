#include "ost_segment.h"

#include <string.h>

int8_t peek_header(const SpWPacket *const packet, OstSegmentHeader *const header)
{
    if (packet->sz < sizeof(OstSegmentHeader))
        return -1;
    if (memcpy(header, packet->data, sizeof(OstSegmentHeader)) != sizeof(OstSegmentHeader))
        return -1;
    return 0;
}

int8_t data_to_ost_segment(OstSegment *const seg, void *data, uint8_t sz)
{
    return 1;
}
void set_seq_number(OstSegment *const seg, uint8_t n)
{
    seg->header.seq_number = n;
}
uint8_t get_seq_number(const OstSegment *const seg)
{
    return seg->header.seq_number;
}

void set_src_addr(OstSegment *const seg, uint8_t addr)
{
    seg->header.source_addr = addr;
}

uint8_t get_src_addr(const OstSegment *const seg)
{
    return seg->header.source_addr;
}

void set_payload_len(OstSegment *const seg, uint8_t l)
{
    seg->header.payload_length = l;
}

uint8_t get_payload_len(const OstSegment *const seg)
{
    return seg->header.payload_length;
}

void set_flag(OstSegment *const seg, SegmentFlag flag)
{
    switch (flag)
    {
    case ACK:
        seg->header.flags |= 0b00000001;
        break;
    case SYN:
        seg->header.flags |= 0b00000010;
        break;
    case RST:
        seg->header.flags |= 0b00000100;
        break;
    default:
        seg->header.flags &= 0b11111000;
        break;
    }
}

int8_t is_ack(const OstSegment *const seg)
{
    return seg->header.flags & 0b00000001;
}

int8_t is_syn(const OstSegment *const seg)
{
    return seg->header.flags & 0b00000010;
}

int8_t is_rst(const OstSegment *const seg)
{
    return seg->header.flags & 0b00000100;
}

int8_t is_dta(const OstSegment *const seg)
{
    return seg->header.flags & 0b00000000 == 0;
}