#include "../core/segment.h"

#ifndef PAYLOAD_HPP
#define PAYLOAD_HPP

void generate_payload(struct Segment* const segment);
void print_payload(struct Segment* segment);
void print_is_ack(const struct Segment* const s);

#endif