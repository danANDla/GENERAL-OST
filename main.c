#include "spwstub/spw_node.h"

int main() {
    struct SpWInterface interface = {
        .state = OFF,
        .auto_start = false
    };
    powerup_link(&interface, -1, -1);
    start_link(&interface);
}