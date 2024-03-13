#include "payload_io.h"
#include <stdio.h>
#include <stdlib.h>


void generate_payload(struct Segment* const segment) {
    uint8_t* data = (uint8_t*) malloc(sizeof(uint8_t) * segment->payload_length);
    for(uint8_t i = 0; i < segment->payload_length; ++i) {
        data[i] = i;
    }
    segment->payload = data;
}

void print_payload(struct Segment* segment) {
    for(uint8_t i = 0; i < segment->payload_length; ++i) {
        printf("%u ", segment->payload[i]);
    }
    printf("\n");
}

void print_is_ack(const struct Segment* const s){
    if(is_ack(s)) {
        printf("is ack packet\n");
    } else {
        printf("is not ack packet\n");
    }
}