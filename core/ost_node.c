#include "ost_node.h"
#include "ost_socket.h"
#include "../../swic.h"

#include <stdlib.h>

// #define LOOPBACK_MODE
#define TRY 100
#define SWIC_SPEED 100

void fill_segment(OstSegment *seg, unsigned int len, int first);
int8_t spw_hw_init(OstNode* const node);
void print_event(const OstNode* const node, const TransportLayerEvent e);

int8_t start(OstNode *const node, const enum SocketMode socket_mode) {
    node->ports[0] = malloc(sizeof(OstSocket));
    node->ports[0]->queue.upper_handler = node->timeout_cb;
    return spw_hw_init(node);
}

void shutdown(OstNode *const node) {
    uint8_t i = 0;
    for(i = 0; i < PORTS_NUMBER; ++i) {
        if(node->ports[i]->to_address != node->self_address && node->ports[i]->state != CLOSED) {
            close(node->ports[i]);
            free(node->ports[i]);
        }
    }
}

int8_t event_handler(OstNode *const node, const TransportLayerEvent e) {
    print_event(node, e);
    switch (e)
    {
        case PACKET_ARRIVED_FROM_NETWORK: {
            if(node->that_arrived)
                socket_event_handler(&node->ports[0], e, node->that_arrived, 0);
            free(node->that_arrived);
            break;
        }
        case APPLICATION_PACKET_READY: {
            OstSegment seg;
            fill_segment(&seg, 100, 1);
            uint8_t seg_n;
            if(add_to_tx(&node->ports[0], &seg, &seg_n)) {
                socket_event_handler(&node->ports[0], e, 0, seg_n);
            }
            free(seg.payload);
            break;
        }
        default:
            break;
    }
    return 0;
}

void fill_segment(OstSegment *seg, unsigned int len, int first) {
  seg->payload = malloc(len);
  unsigned int i;
  for (i = 0; i < len; i++) {
    seg->payload[i] = ((i + first) % 256);
  }
  seg->header.payload_length = len;
}

int8_t send_packet(OstNode *const node, int8_t address, const uint8_t *buffer, uint32_t size) {
    if(node->ports[0]->to_address != address) return -1;
    OstSegment seg = {.header = {.payload_length = size}, .payload = buffer};
    return send(node->ports[0], &seg);
}

int8_t receive_packet(OstNode *const node, const uint8_t *buffer, uint32_t *received_sz);

int8_t open_connection(OstNode *const node, uint8_t address, int8_t mode) {
    node->ports[0]->self_port = 0;
    node->ports[0]->to_address = address;
    open(node->ports[0], mode);
}

int8_t close_connection(OstNode *const node, uint8_t address) {
    close(node->ports[0]);
}
int8_t get_socket(OstNode *const node, uint8_t address, OstSocket *const socket);

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
    return -1;
  }

  debug_printf("OST 0 RX_SPEED: %x \n", swic_get_rx_speed(0));
  debug_printf("OST 1 RX_SPEED: %x \n", swic_get_rx_speed(1));
#endif
}

void print_event(const OstNode* const node, const TransportLayerEvent e) {
    switch (e)
    {
    case PACKET_ARRIVED_FROM_NETWORK:
        debug_printf("NODE[%d] event: PACKET_ARRIVED_FROM_NETWORK", node->self_address);
        break;
    case RETRANSMISSION_INTERRUPT:
        debug_printf("NODE[%d] event: RETRANSMISSION_INTERRUPT", node->self_address);
        break;
    case APPLICATION_PACKET_READY:
        debug_printf("NODE[%d] event: APPLICATION_PACKET_READY", node->self_address);
        break;
    default:
        debug_printf("NODE[%d] event: unk", node->self_address);
        break;
    }
}
