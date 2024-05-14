#include "ost_node.h"

// #define LOOPBACK_MODE
#define TRY 100
#define SWIC_SPEED 100

void send_to_application(const OstNode* const node, OstSegment* segment);
int8_t get_packet_from_application(OstNode* const node);
void add_packet_to_rx(OstNode* const node, OstSegment* segment);

int8_t send_to_physical(const OstNode* const node, SegmentFlag t, uint8_t seq_n);
int8_t get_packet_from_physical(const OstNode* const node);
int8_t tx_sliding_window_have_space(const OstNode* const node);
int8_t in_tx_window(const OstNode* const node, uint8_t);
int8_t in_rx_window(const OstNode* const node, uint8_t);
int8_t hw_timer_handler(uint8_t);

int8_t network_layer_handler(OstNode* const node, const SpWPacket* const pkt);

int8_t aggregate_socket(uint8_t address);
int8_t delete_socket(uint8_t address);

void event_name(TransportLayerEvent e);

int8_t start(OstNode *const node, int8_t socket_mode) {
    return spw_hw_init(node);
}

void shutdown(OstNode *const node) {
    // clear timer table
    uint8_t i = 0;
    for(i = 0; i < PORTS_NUMBER; ++i) {
        if(node->ports[i]->to_address != node->self_address && node->ports[i]->state != CLOSED) {
            close(node->ports[i]);
        }
    }
}

int8_t event_handler(OstNode *const node, const TransportLayerEvent e) {

}

int8_t send_packet(OstNode *const node, int8_t address, const uint8_t *buffer, uint32_t size);
int8_t receive_packet(OstNode *const node, const uint8_t *buffer, uint32_t *received_sz);

int8_t open_connection(OstNode *const node, uint8_t address, int8_t mode);
int8_t close_connection(OstNode *const node, uint8_t address);
int8_t get_socket(OstNode *const node, uint8_t address, OstSocket *const socket);


// void swic_test(void) {
//   unsigned char src[SIZE] __attribute__((aligned(8))) = {
//       0,
//   };
//   unsigned char dst[SIZE] __attribute__((aligned(8))) = {
//       0,
//   };
//   unsigned int desc[2] __attribute__((aligned(8))) = {
//       0,
//   };
//   unsigned int i = 0;

// #ifdef LOOPBACK_MODE
//   unsigned int flag0 = 0;
//   unsigned int flag1 = 0;
// #endif

//   base_init();

// #ifdef LOOPBACK_MODE
//   // SWIC 1
//   for (i = 0; i <= TRY; i++) {
//     swic_init_loopback(1);

//     if (swic_is_connected(1) == 1)
//       break;
//     debug_printf("SWIC 1 MODE: %x \n", GIGASPWR_COMM_SPW_MODE(1));
//     debug_printf("SWIC 1 STATUS: %x \n", GIGASPWR_COMM_SPW_STATUS(1));
//     delay(1000);
//   }
//   flag1 = swic_is_connected(1);
//   debug_printf("SWIC 1 CONNECT: %x \n", flag1);

//   // SWIC0
//   i = 0;
//   for (i = 0; i <= TRY; i++) {
//     swic_init_loopback(0);

//     if (swic_is_connected(0) == 1)
//       break;
//     debug_printf("SWIC 0 MODE: %x \n", GIGASPWR_COMM_SPW_MODE(0));
//     debug_printf("SWIC 0 STATUS: %x \n", GIGASPWR_COMM_SPW_STATUS(0));
//     delay(1000);
//   }
//   flag0 = swic_is_connected(0);
//   debug_printf("SWIC 0 CONNECT: %x \n", flag0);

//   i = 0;
//   if ((flag0 == 0) & (flag1 == 0)) {
//     debug_printf("ALL SWIC CONNECTION FAIL! \n");
//     return;
//   }

// #endif

int8_t spw_hw_init(OstNode* const node) {
#ifndef LOOPBACK_MODE
  // CABLE SWIC0 <-> SWIC1
  uint32_t i;
  for (i = 0; i <= TRY; i++) {
    swic_init(1);
    swic_init(0);
    delay(1000);
    if (swic_is_connected(1) == 1)
      break;
    debug_printf("SWIC 1 MODE: %x \n", GIGASPWR_COMM_SPW_MODE(1));
    debug_printf("SWIC 0 MODE: %x \n", GIGASPWR_COMM_SPW_MODE(0));
    debug_printf("SWIC 1 STATUS: %x \n", GIGASPWR_COMM_SPW_STATUS(1));
    debug_printf("SWIC 0 STATUS: %x \n", GIGASPWR_COMM_SPW_STATUS(0));
  }

  if (swic_is_connected(1) == 0) {
    debug_printf("OST SWIC CONNECTION FAIL! \n");
    return -1;
  }

  debug_printf("TRY SPEED_SET: %u \n", SWIC_SPEED);

  for (i = 0; i <= (TRY - 1); i++) {
    swic_set_tx_speed(0, SWIC_SPEED);
    swic_set_tx_speed(1, SWIC_SPEED);

    if (swic_is_connected(1) == 1)
      break;

    swic_init(1);
    swic_init(0);
    delay(1000);
    swic_set_tx_speed(0, SWIC_SPEED);
    swic_set_tx_speed(1, SWIC_SPEED);
  }

  if (swic_is_connected(1) == 0) {
    debug_printf("OST hardware Init FAIL! \n");
    return;
  }

  debug_printf("OST 0 RX_SPEED: %x \n", swic_get_rx_speed(0));
  debug_printf("OST 1 RX_SPEED: %x \n", swic_get_rx_speed(1));
#endif
}