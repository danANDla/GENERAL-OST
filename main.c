#include "core/segment.h"
#include "utils/payload_io.h"

int main() {
    struct Segment s = {.seq_number = 0, .payload_length = 10, .payload = 0};
    generate_payload(&s);
    print_payload(&s);
    print_is_ack(&s);
}