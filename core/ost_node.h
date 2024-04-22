#ifndef OST_NODE_H
#define OST_NODE_H

#include <inttypes.h>

#include "timer_fifo.h"
#include "ost_segment.h"
#include "spw_packet.h"

#define WINDOW_SZ 1

/**
 * \defgroup ost Open SpaceWire Transport Layer Node
 * This section documents the API of the transport-layer node of spw network.
 */

typedef enum {
    PACKET_ARRIVED_FROM_NETWORK = 0,
    APPLICATION_PACKET_READY,
    CHECKSUM_ERR,
    RETRANSMISSION_INTERRUPT,
    TRANSPORT_CLK_INTERRUPT,
} TransportLayerEvent;
int8_t add_packet_to_tx(OstNode* const node, void* data, uint8_t sz);

typedef struct {
    uint8_t tx_window_top;
    uint8_t tx_window_bottom;
    uint8_t rx_window_top;
    uint8_t rx_window_bottom;
    uint8_t to_retr;

    OstSegment tx_buffer[MAX_UNACK_PACKETS];
    OstSegment rx_buffer[MAX_UNACK_PACKETS];
    int8_t acknowledged[MAX_UNACK_PACKETS];

    TimerFifo* queue;

    void* spw_layer;
} OstNode;

int8_t event_handler(OstNode * const node, const TransportLayerEvent e);
int8_t add_packet_to_tx(OstNode* const node, void* data, uint8_t sz);

#endif