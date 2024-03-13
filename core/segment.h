#include <inttypes.h>
#include <stdbool.h>

#ifndef segment_hpp
#define segment_hpp

typedef uint8_t FlagOctet;

struct Segment {
    uint8_t seq_number;
    FlagOctet flag;
    uint8_t payload_length;
    uint8_t* payload;
};

bool is_ack(const struct Segment* const segment);

#endif