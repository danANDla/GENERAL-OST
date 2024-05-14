#ifndef OST_NODE_H
#define OST_NODE_H

#include <inttypes.h>

#include "timer_fifo.h"
#include "ost_segment.h"
#include "spw_packet.h"
#include "ost_socket.h"

static const uint8_t PORTS_NUMBER = 1;

/**
 * \defgroup ost Open SpaceWire Transport Layer Node
 * This section documents the API of the transport-layer node of spw network.
 */
typedef enum {
    PACKET_ARRIVED_FROM_NETWORK = 0,
    APPLICATION_PACKET_READY,
    RETRANSMISSION_INTERRUPT,
    TRANSPORT_CLK_INTERRUPT,
} TransportLayerEvent;

typedef struct {
    uint8_t self_address;
    void (*rx_cb) (uint8_t, OstSegment *);
    void (*timeout_cb) (uint8_t);
    OstSocket *ports[PORTS_NUMBER];
    OstSegment* that_arrived;
} OstNode;

int8_t start(OstNode *const node, int8_t socket_mode);
void shutdown(OstNode *const node);
int8_t event_handler(OstNode *const node, const TransportLayerEvent e);

int8_t open_connection(OstNode *const node, uint8_t address, int8_t mode);
int8_t close_connection(OstNode *const node, uint8_t address);
int8_t send_packet(OstNode *const node, int8_t address, const uint8_t *buffer, uint32_t size);
int8_t receive_packet(OstNode *const node, const uint8_t *buffer, uint32_t *received_sz);
int8_t get_socket(OstNode *const node, uint8_t address, OstSocket *const socket);

// Ptr<SpWDevice> GetSpWLayer();

#endif