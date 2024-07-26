#include "ost_socket.h"
#include "ost_node.h"
#include "data_fifo.h"
#include "spw_layer.h"
#include "ost_segment.h"

#if (defined(TARGET_MC30SF6))
#include "../swic.h"
#endif

int8_t init_socket(OstSocket *const sk);
int8_t peek_from_transmit_fifo(OstSocket *const sk);
int8_t tx_sliding_window_have_space(const OstSocket *const sk);
int8_t socket_send_spw(OstSocket *const sk, const OstSegment *const p);
int8_t in_tx_window(const OstSocket *const sk, uint8_t seq_n);
int8_t in_rx_window(const OstSocket *const sk, uint8_t seq_n);
int8_t mark_packet_ack(OstSocket *const sk, uint8_t seq_n);
int8_t mark_packet_receipt(OstSocket *const sk, uint8_t seq_n, const OstSegment *const p);
int8_t send_to_physical(OstSocket *const sk, SegmentFlag f, uint8_t seq_n);
int8_t segment_arrival_event_socket_handler(OstSocket *const sk, const OstSegment *const seg);
int8_t add_packet_to_tx(OstSocket *const sk, OstSegment *const seg);

int8_t send_rejection(const uint8_t seq_n)
{
    // not implemented
    return -1;
}
int8_t send_syn(const uint8_t seq_n)
{
    // not implemented
    return -1;
}
int8_t send_syn_confirm(const uint8_t seq_n)
{
    // not implemented
    return -1;
}
int8_t send_confirm(const uint8_t seq_n)
{
    // not implemented
    return -1;
}
int8_t full_states_handler(seg)
{
    // not implemented
    return -1;
}

int8_t send_to_application(OstSocket* const sk, const OstSegment *const seg)
{
    if(VerifyArray(seg->payload, 0, seg->header.payload_length, seg->header.seq_number)) sk->verified_received++;
    return 0;
}

int8_t
open(OstSocket *const sk, SocketMode sk_mode)
{
    sk->mode = sk_mode;
    if (sk->mode == CONNECTIONLESS)
    {
        sk->state = OPEN;
        // sk->spw_layer = sk->ost->spw_layer;
        init_socket(sk);
        init_timer_queue(&sk->queue);
        return 1;
    }
    else
    {
        return -1;
    }
}

int8_t
close(OstSocket *const sk)
{
    if (sk->mode != CONNECTIONLESS)
    {
        sk->state = CLOSE_WAIT;
        send_rejection(0);
    }
    else
    {
        sk->state = CLOSED;
    }
    sk->tx_window_bottom = 0;
    sk->tx_window_top = 0;
    sk->rx_window_bottom = 0;
    sk->rx_window_top = 0;
    sk->to_retr = 0;
    return 1;
}

int8_t init_socket(OstSocket *const sk)
{
    sk->tx_window_bottom = 0;
    sk->tx_window_top = 0;
    sk->rx_window_bottom = 0;
    sk->rx_window_top = WINDOW_SZ;
    sk->to_retr = 0;
    int8_t i;
    for (i = 0; i < WINDOW_SZ; ++i)
    {
        sk->acknowledged[i] = 0;
        sk->received[i] = 0;
    }
}

int8_t send(OstSocket *const sk, const uint8_t *buffer, uint32_t size)
{
    if (sk->state != OPEN)
    {
        return -1;
    }

    if (size > 1024 * 64)
        return -2;
    OstSegment segment = (OstSegment){
        .header = {
            .payload_length = size,
            .source_addr = sk->ost->self_address},
    	.payload = buffer
    };
    set_flag(&segment, DTA);
    int r = data_fifo_enqueue(&sk->data, &segment);
    if(r != 1) return -1;
    return peek_from_transmit_fifo(sk);
}

int8_t peek_from_transmit_fifo(OstSocket *const sk)
{
    while (!data_fifo_is_empty(&sk->data) && tx_sliding_window_have_space(sk))
    {
        OstSegment *p;
        if (data_fifo_peek(&sk->data, &p))
        {
            if (add_packet_to_tx(sk, p) != -1)
            {
                data_fifo_dequeue(&sk->data);
                if (spw_is_ready_to_transmit(sk->ost->spw_layer))
                    socket_event_handler(sk, APPLICATION_PACKET_READY, 0, 0);
            }
        }
    }
    return 1;
}

int8_t
socket_event_handler(OstSocket *const sk, const enum TransportLayerEvent e, OstSegment *seg, uint8_t seq_n)
{
    switch (e)
    {
    case PACKET_ARRIVED_FROM_NETWORK:
        return segment_arrival_event_socket_handler(sk, seg);
    case APPLICATION_PACKET_READY:
        return send_to_physical(sk, DTA, (sk->tx_window_top + MAX_SEQ_N - 1) % MAX_SEQ_N);
    case RETRANSMISSION_INTERRUPT:
        return send_to_physical(sk, DTA, seq_n);
    case SPW_READY:
        peek_from_transmit_fifo(sk);
        return 1;
    default:
        return -1;
    }
    return 1;
}

int8_t
add_packet_to_tx(OstSocket *const sk, OstSegment *const seg)
{
    if (tx_sliding_window_have_space(sk))
    {
        sk->tx_window[sk->tx_window_top].payload = malloc(seg->header.payload_length);
        memcpy(sk->tx_window[sk->tx_window_top].payload, seg->payload, seg->header.payload_length);
        sk->tx_window[sk->tx_window_top].header.seq_number = sk->tx_window_top;
        sk->tx_window[sk->tx_window_top].header.payload_length = seg->header.payload_length;
        sk->tx_window_top = (sk->tx_window_top + 1) % MAX_SEQ_N;
        return 1;
    }
    return -1;
}

void add_packet_to_transmit_fifo(OstSocket *const sk, OstSegment *p)
{
    data_fifo_enqueue(&sk->data, p);
}

int8_t
segment_arrival_event_socket_handler(OstSocket *const sk, const OstSegment *const seg)
{
    if (sk->mode == CONNECTIONLESS)
    {
        OstSegmentHeader *header = &seg->header;
        if (is_ack(seg))
        {
            if (in_tx_window(sk, header->seq_number) && !sk->acknowledged[header->seq_number])
            {
                mark_packet_ack(sk, header->seq_number);
            }
        }
        else
        {
            if (in_rx_window(sk, header->seq_number) && !sk->received[header->seq_number])
            {
                mark_packet_receipt(sk, header->seq_number, seg);
            }
            send_to_physical(sk, ACK, header->seq_number);
        }
        return 1;
    }
    else
    {
        full_states_handler(seg);
    }
    return -1;
}

int8_t
send_to_physical(OstSocket *const sk, SegmentFlag f, uint8_t seq_n)
{
    if (f == ACK)
    {
        OstSegment seg = (OstSegment){
            .header = (OstSegmentHeader){
                .payload_length = 0,
                .seq_number = seq_n,
                .source_addr = sk->ost->self_address}};
        set_flag(&seg, ACK);
        socket_send_spw(sk, &seg);
    }
    else
    {
        OstSegmentHeader *header = &sk->tx_window[seq_n].header;
        if (header->payload_length == 0 || !is_dta(&sk->tx_window[seq_n]) || header->seq_number != seq_n ||
            header->source_addr != sk->ost->self_address)
        {
            return -1;
        }
        socket_send_spw(sk, &sk->tx_window[seq_n]);
        if (add_new_timer(&sk->queue, seq_n, DURATION_RETRANSMISSON) != 1)
        {
            return -1;
        }
    }
    return 1;
}
int8_t
mark_packet_ack(OstSocket *const sk, uint8_t seq_n)
{
    if (!sk->acknowledged[seq_n])
    {
        sk->acknowledged[seq_n] = 1;
        if (cancel_timer(&sk->queue, seq_n) != 1)
        {
            // NS_LOG_ERROR("error removing timer from queue");
            return -1;
        };
        while (sk->acknowledged[sk->tx_window_bottom] && (sk->tx_window_bottom + 1) % MAX_SEQ_N <= sk->tx_window_top)
        {
            free(sk->tx_window[sk->tx_window_bottom].payload);
            sk->tx_window_bottom = (sk->tx_window_bottom + 1) % MAX_SEQ_N;
        }
        peek_from_transmit_fifo(sk);
    }
    return 1;
}

int8_t
mark_packet_receipt(OstSocket *const sk, uint8_t seq_n, const OstSegment *const p)
{
    if (!sk->received[seq_n])
    {
        sk->received[seq_n] = 1;
        sk->rx_window[seq_n].header = p->header;
        sk->rx_window[seq_n].payload = malloc(p->header.payload_length);
        memcpy(sk->rx_window[seq_n].payload, p->payload, p->header.payload_length);
        while (sk->received[sk->rx_window_bottom])
        {
            send_to_application(sk, &sk->rx_window[sk->rx_window_bottom]);
            free(sk->rx_window[sk->rx_window_bottom].payload);
            sk->received[sk->rx_window_top] = 0;
            sk->rx_window_bottom = (sk->rx_window_bottom + 1) % MAX_SEQ_N;
            sk->rx_window_top = (sk->rx_window_top + 1) % MAX_SEQ_N;
        }
    }
    return 1;
}

int8_t socket_send_spw(OstSocket *const sk, const OstSegment *const p)
{
    uint8_t address_size = 0;
    uint8_t* address_buffer;
    unsigned char src[64 * 1024] __attribute__((aligned(8))) = {
        0,
    };
    if (address_size > 0)
    {
        if (address_size > MAX_WORMHOLE_ADDRESS_LENGTH)
            return -1;
        memcpy(src, address_buffer, address_size);
    }
    memcpy(address_size + src, p, sizeof(OstSegmentHeader));
    memcpy(address_size + src + sizeof(OstSegmentHeader), p->payload, p->header.payload_length);
    int8_t r = spw_send_data(sk->ost->spw_layer, src, address_size + sizeof(OstSegmentHeader) + p->header.payload_length);

    return r;
}

int8_t
in_tx_window(const OstSocket *const sk, uint8_t seq_n)
{
    return (sk->tx_window_top >= sk->tx_window_bottom && seq_n >= sk->tx_window_bottom &&
            seq_n < sk->tx_window_top) ||
           (sk->tx_window_bottom > sk->tx_window_top &&
            (seq_n >= sk->tx_window_bottom || seq_n < sk->tx_window_top));
}

int8_t
in_rx_window(const OstSocket *const sk, uint8_t seq_n)
{
    return (sk->rx_window_top >= sk->rx_window_bottom && seq_n >= sk->rx_window_bottom &&
            seq_n < sk->rx_window_top) ||
           (sk->rx_window_bottom > sk->rx_window_top &&
            (seq_n >= sk->rx_window_bottom || seq_n < sk->rx_window_top));
}

int8_t
tx_sliding_window_have_space(const OstSocket *const sk)
{
    if (sk->tx_window_top >= sk->tx_window_bottom)
    {
        return sk->tx_window_top - sk->tx_window_bottom < WINDOW_SZ;
    }
    else
    {
        return WINDOW_SZ - sk->tx_window_top + 1 + sk->tx_window_bottom < WINDOW_SZ;
    }
}
