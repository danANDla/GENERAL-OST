#include "ost_node.h"

void send_to_application(const OstSegment* const node);
int8_t get_packet_from_application();
int8_t send_to_physical(OstNode* const node, SegmentType t, uint8_t seq_n);
int8_t spw_layer_send(void* spw_layer, const OstSegment* const seg);
int8_t get_packet_from_physical();

bool tx_sliding_window_have_space();
bool in_tx_window(const OstNode* const node, uint8_t);
bool in_rx_window(const OstNode* const node, uint8_t);
bool hw_timer_handler(OstNode* const node, uint8_t);

void send_to_application(const OstSegment* const s) {
    // some output
}

int8_t add_packet_to_tx(OstNode* const node, void* data, uint8_t sz) {
    if(tx_sliding_window_have_space(node) && sz <= UINT8_MAX) {
        node->acknowledged[node->tx_window_top] = false;
        data_to_ost_segment(&node->tx_buffer[node->tx_window_top], data, sz);
        node->tx_window_top = (node->tx_window_top + 1)  % MAX_UNACK_PACKETS;
        return 0;
    }
    return 1;
}

bool tx_sliding_window_have_space(const OstNode* const node) {
    if(node->tx_window_top >= node->tx_window_bottom) {
        return node->tx_window_top - node->tx_window_bottom < WINDOW_SZ;
    } else {
        return MAX_UNACK_PACKETS - node->tx_window_top + 1 + node->tx_window_bottom < WINDOW_SZ;
    }
}

bool in_tx_window(const OstNode* const node, uint8_t seq_n) {
    return (node->tx_window_top >= node->tx_window_bottom && seq_n >= node->tx_window_bottom && seq_n < node->tx_window_top) ||
           (node->tx_window_bottom > node->tx_window_top && (seq_n >= node->tx_window_bottom || seq_n < node->tx_window_top));
}

bool in_rx_window(const OstNode* const node, uint8_t seq_n) {
    return (node->rx_window_top >= node->rx_window_bottom && seq_n >= node->rx_window_bottom && seq_n < node->rx_window_top) ||
           (node->rx_window_bottom > node->rx_window_top && (seq_n >= node->rx_window_bottom || seq_n < node->rx_window_top));
}

int8_t send_to_physical(OstNode* const node, SegmentType t, uint8_t seq_n) {
    if(t == DATA) {
        node->tx_buffer[seq_n].header.seq_number = seq_n;
        set_type(&node->tx_buffer[seq_n].header, DATA);
        node->tx_buffer[seq_n].header.source_addr = -1;

        spw_layer_send(node->spw_layer, &node->tx_buffer[seq_n]);
        add_new_timer(node->queue, node->tx_window_bottom, 100);
    } else if (t == ACK) {
        OstSegment seg = { 
            .header = {
                .payload_length = 0,
                .seq_number = seq_n,
                .source_addr = -1
            }
        };
        set_type(&seg.header, ACK);
        spw_layer_send(node->spw_layer, &seg);
    }
    return 0;
}

bool hw_timer_handler(OstNode* const node, uint8_t seq_n) {
    node->to_retr = seq_n;
    event_handler(node, RETRANSMISSION_INTERRUPT);
    return true;
}

int8_t event_handler(OstNode* const node, const TransportLayerEvent e) {
    switch (e)
    {
        case PACKET_ARRIVED_FROM_NETWORK:
            get_packet_from_physical();
            break;
        case APPLICATION_PACKET_READY:
            send_to_physical(node, DATA, (node->tx_window_top + MAX_UNACK_PACKETS - 1) % MAX_UNACK_PACKETS);
            break;
        case RETRANSMISSION_INTERRUPT:
            send_to_physical(node, DATA, node->to_retr);
            break;
        default:
            break;
    }

    return 0;
}