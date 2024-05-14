#include "ost_socket.h"
#include "timer_fifo.h"
#include "ost_node.h"

// #include "SWIC.h"

#include <stdlib.h>
#include <string.h>

int8_t segment_arrival_event_socket_handler(OstSocket* const sk, OstSegment* seg);
void set_state(OstSocket *const sk, SocketState);
void state_name(SocketState);

void send_rejection(OstSocket *const sk, uint8_t seq_n);
void send_syn(OstSocket *const sk, uint8_t seq_n);
void send_syn_confirm(OstSocket *const sk, uint8_t seq_n);
void send_confirm(OstSocket *const sk, uint8_t seq_n);
void send_spw(void *spw_layer, OstSegment *seg);

void start_close_wait_timer(OstSocket *const sk);
void stop_close_wait_timer(OstSocket *const sk);
void dealloc(OstSocket *const sk);

void send_to_application(OstSocket *const sk, OstSegment *seg);

int8_t in_tx_window(const OstSocket *const sk, uint8_t);
int8_t in_rx_window(const OstSocket *const sk, uint8_t);
int8_t tx_sliding_window_have_space(const OstSocket* const sk);

int8_t event_handler(OstSocket* const sk, const TransportLayerEvent e, OstSegment* seg, uint8_t seq_n)
{
    switch (e)
    {
        case PACKET_ARRIVED_FROM_NETWORK:
            return segment_arrival_event_socket_handler(sk, seg);
        case APPLICATION_PACKET_READY:
        case RETRANSMISSION_INTERRUPT:
            return send_to_physical(sk, DTA, seq_n);
        default:
            return -1;
    }
    return 1;
}

int8_t open(OstSocket *const sk, int8_t mode)
{
    sk->mode = mode;
    if (mode == CONNECTIONLESS)
    {

        return 1;
    }
    else
    {
        return -1; // not implemented
    }
}

int8_t close(OstSocket *const sk)
{
    if (sk->mode != CONNECTIONLESS)
    {
        sk->state = CLOSE_WAIT;
        send_rejection(sk, 0);
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
    sk->self_address = -1;
}

int8_t send(OstSocket *const sk, OstSegment *seg)
{
    if(sk->mode == OPEN) {
        debug_printf("socket must be in open state\n");
        return -1; 
    }

    if(!tx_sliding_window_have_space(sk)) {
        debug_printf("no free space in tx_buffer\n");
        return -1;
    }

    sk->acknowledged[sk->tx_window_top] = false;
    int8_t* buff = malloc(sizeof(int8_t*) * seg->header.payload_length);
    sk->tx_buffer[sk->tx_window_top] = (OstSegment) {
        .header = {
            .payload_length = seg->header.payload_length,
            .seq_number = sk->tx_window_top,
            .source_addr = sk->ost->self_address,
        },
        .payload = buff
    };
    set_flag(&sk->tx_buffer[sk->tx_window_top].header, DTA);

    sk->tx_window_top = (sk->tx_window_top + 1) % WINDOW_SZ;
}

int8_t receive(OstSocket *const sk, OstSegment *seg)
{

}

int8_t add_to_tx(OstSocket* const sk, const OstSegment* const seg, uint8_t * const seq_n) {
    if(tx_sliding_window_have_space(sk)) {
        sk->acknowledged[sk->tx_window_top] = false;

        sk->tx_buffer[sk->tx_window_top].header.payload_length = seg->header.payload_length;
        sk->tx_buffer[sk->tx_window_top].header.seq_number = sk->tx_window_top;
        memcpy(sk->tx_buffer[sk->tx_window_top].payload, seg, sizeof(OstSegmentHeader) + seg->header.payload_length);
        *seq_n = sk->tx_window_top;

        sk->tx_window_top = (sk->tx_window_top + 1) % WINDOW_SZ;

        return 1;
    }
    return -1;
}

int8_t segment_arrival_event_socket_handler(OstSocket *const sk, OstSegment *seg)
{ if (sk->mode == CONNECTIONLESS)
    {
        OstSegmentHeader header;

        peek_header(&seg, &header);

        if (is_ack(&header))
        {
            if (in_tx_window(sk, get_seq_number(&header)))
            {
                if (!sk->acknowledged[get_seq_number(&header)])
                {
                    sk->acknowledged[get_seq_number(&header)] = true;
                    cancel_timer(&sk->queue, get_seq_number(&header));
                    while (sk->acknowledged[sk->tx_window_bottom])
                    {
                        free(sk->tx_buffer[sk->tx_window_bottom].payload);
                        sk->tx_window_bottom = (sk->tx_window_bottom + 1) % WINDOW_SZ;
                    }
                }
            }
        }
        else
        {
            if (in_rx_window(sk, get_seq_number(&header)))
            {
                memcpy(&sk->rx_buffer[header.seq_number], seg, sizeof(OstSegmentHeader) + seg->header.payload_length);

                send_to_application(sk, &sk->rx_buffer[sk->rx_window_bottom]);
                sk->rx_window_bottom = (sk->rx_window_bottom + 1) % WINDOW_SZ;
                sk->rx_window_top = (sk->rx_window_top + 1) % WINDOW_SZ;
            }
            send_to_physical(sk, ACK, get_seq_number(&header));
        }
        return 1;
    }
    return -1;
}

int8_t send_to_physical(OstSocket *const sk, const SegmentFlag t, const uint8_t seq_n)
{
    OstSegmentHeader header;
    if (t == ACK)
    {
        set_payload_len(&header, 0);
        set_seq_number(&header, seq_n);
        set_flag(&header, ACK);
        set_src_addr(&header, sk->ost->self_address);
        OstSegment seg = (OstSegment){.header = header};
        send_spw(sk->spw_layer, &seg);
    }
    else
    {
        header = sk->tx_buffer[seq_n].header;
        if (header.payload_length == 0 ||
            !is_dta(&header) ||
            header.seq_number != seq_n ||
            header.source_addr != sk->ost->self_address)
            return -1;

        send_spw(sk->spw_layer, &sk->tx_buffer[seq_n]);
        if (add_new_timer(&sk->queue, seq_n, DURATION_RETRANSMISSON) != 0)
            NS_LOG_ERROR("timer error");
    }
    return 0;
}

void send_spw(void *spw_layer, OstSegment *seg)
{
    swic_send_packege(SWIC0, seg, sizeof(OstSegmentHeader) + seg->header.payload_length);
}

int8_t tx_sliding_window_have_space(const OstSocket* const sk)
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