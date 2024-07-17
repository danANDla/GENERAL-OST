#ifndef OST_SEGMENT_H
#define OST_SEGMENT_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef uint8_t FlagOctet;

typedef enum
{
    ACK,
    SYN,
    RST,
    DTA // virtual
} SegmentFlag;

typedef struct
{
    FlagOctet flags;
    uint8_t seq_number;
    uint8_t source_addr;
    uint16_t payload_length;
} OstSegmentHeader;

typedef struct OstSegment
{
    OstSegmentHeader header;
    uint8_t *payload;
} OstSegment;

int8_t data_to_ost_segment(OstSegment *const seg, void *data, uint8_t sz);
void print_header(const OstSegmentHeader* const header);

void set_seq_number(OstSegment *const header, uint8_t);
uint8_t get_seq_number(const OstSegment *const header);

void set_src_addr(OstSegment *const header, uint8_t);
uint8_t get_src_addr(const OstSegment *const header);

void set_payload_len(OstSegment *const header, uint8_t);
uint8_t get_payload_len(const OstSegment *const header);

void set_flag(OstSegment *const header, SegmentFlag flag);
void unset_flag(OstSegment *const header, SegmentFlag flag);

int8_t is_ack(const OstSegment* const header);
int8_t is_syn(const OstSegment* const header);
int8_t is_rst(const OstSegment* const header);
int8_t is_dta(const OstSegment* const header);

void segment_type_name(SegmentFlag t);


#ifdef __cplusplus
}
#endif

#endif
