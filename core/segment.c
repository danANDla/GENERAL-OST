#include "segment.h"

bool is_ack(const struct Segment* const segment){
    return segment->flag & 0x80;
}