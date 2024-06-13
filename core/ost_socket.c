#include "ost_socket.h"
#include "timer_fifo.h"
#include "ost_node.h"
#include "swic.h"

#include <stdlib.h>
#include <string.h>

int8_t segment_arrival_event_socket_handler(OstSocket *const sk, OstSegment *seg);
void set_state(OstSocket *const sk, SocketState);
void state_name(SocketState);

void send_rejection(OstSocket *const sk, uint8_t seq_n);
void send_syn(OstSocket *const sk, uint8_t seq_n);
void send_syn_confirm(OstSocket *const sk, uint8_t seq_n);
void send_confirm(OstSocket *const sk, uint8_t seq_n);
int8_t send_to_physical(OstSocket *const sk, const SegmentFlag t, const uint8_t seq_n);
void send_spw(SWIC_SEND spw_layer, OstSegment *seg);

void start_close_wait_timer(OstSocket *const sk);
void stop_close_wait_timer(OstSocket *const sk);
void dealloc(OstSocket *const sk);

void send_to_application(OstSocket *const sk, OstSegment *seg);

int8_t in_tx_window(const OstSocket *const sk, uint8_t);
int8_t in_rx_window(const OstSocket *const sk, uint8_t);
int8_t tx_sliding_window_have_space(const OstSocket *const sk);

int8_t socket_event_handler(OstSocket *const sk, const enum TransportLayerEvent e, OstSegment *seg, uint8_t seq_n)
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
    sk->verified_received = 0;
    if (mode == CONNECTIONLESS)
    {
        sk->state = OPEN;
        init_socket(sk);
        debug_printf("opened socket [%d:%d] to %d\n", sk->self_address, sk->self_port, sk->to_address);
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
    return 1;
}

int8_t send(OstSocket *const sk, const uint8_t *buffer, uint32_t size)
{
    if (sk->state != OPEN)
    {
        debug_printf("socket[%d:%d] must be in open state\n", sk->self_address, sk->self_port);
        return -1;
    }

    if (!tx_sliding_window_have_space(sk))
    {
        debug_printf("no free space in tx_window\n");
        return -1;
    }

    sk->acknowledged[sk->tx_window_top] = 0;
    int8_t *buff = malloc(sizeof(int8_t *) * size);
    sk->tx_window[sk->tx_window_top] = (OstSegment){
        .header = {
            .payload_length = size,
            .seq_number = sk->tx_window_top,
            .source_addr = sk->ost->self_address,
        },
        .payload = buff};
    set_flag(&sk->tx_window[sk->tx_window_top].header, DTA);
    memcpy(sk->tx_window[sk->tx_window_top].payload, buffer, size);
    int8_t r = send_to_physical(sk, DTA, sk->tx_window_top);
    if (r != 1)
        return -1;
    sk->tx_window_top = (sk->tx_window_top + 1) % WINDOW_SZ;
    return 1;
}

int8_t receive(OstSocket *const sk, OstSegment *seg)
{
}

int8_t add_to_tx(OstSocket *const sk, const OstSegment *const seg, uint8_t *const seq_n)
{
    if (tx_sliding_window_have_space(sk))
    {
        sk->acknowledged[sk->tx_window_top] = 0;

        sk->tx_window[sk->tx_window_top].header.payload_length = seg->header.payload_length;
        sk->tx_window[sk->tx_window_top].header.seq_number = sk->tx_window_top;
        memcpy(sk->tx_window[sk->tx_window_top].payload, seg, sizeof(OstSegmentHeader) + seg->header.payload_length);
        *seq_n = sk->tx_window_top;

        sk->tx_window_top = (sk->tx_window_top + 1) % WINDOW_SZ;

        return 1;
    }
    return -1;
}

int8_t segment_arrival_event_socket_handler(OstSocket *const sk, OstSegment *seg)
{
    if (sk->mode == CONNECTIONLESS)
    {
        if (is_ack(&seg->header))
        {
            if (in_tx_window(sk, seg->header.seq_number))
            {
                if (!sk->acknowledged[seg->header.seq_number])
                {
                    sk->acknowledged[seg->header.seq_number] = 1;
                    cancel_timer(&sk->queue, seg->header.seq_number);
                    sk->queue.interrupt_counter = 0;
                    while (sk->acknowledged[sk->tx_window_bottom])
                    {
                        free(sk->tx_window[sk->tx_window_bottom].payload);
                        sk->tx_window_bottom = (sk->tx_window_bottom + 1) % WINDOW_SZ;
                    }
                }
            }
        }
        else
        {
            if (in_rx_window(sk, get_seq_number(&seg->header)))
            {
                sk->rx_window[seg->header.seq_number].payload = (uint8_t *)malloc(seg->header.payload_length);
                memcpy(sk->rx_window[seg->header.seq_number].payload, seg->payload, seg->header.payload_length);
                sk->rx_window[seg->header.seq_number].header = seg->header;

                send_to_application(sk, &sk->rx_window[sk->rx_window_bottom]);
                sk->rx_window_bottom = (sk->rx_window_bottom + 1) % WINDOW_SZ;
                sk->rx_window_top = (sk->rx_window_top + 1) % WINDOW_SZ;
            }
            send_to_physical(sk, ACK, seg->header.seq_number);
        }
        return 1;
    }
    return -1;
}

void send_rejection(OstSocket *const sk, uint8_t seq_n)
{
    OstSegmentHeader header;
    set_payload_len(&header, 0);
    set_seq_number(&header, seq_n);
    set_flag(&header, RST);
    set_src_addr(&header, sk->ost->self_address);
    OstSegment seg = (OstSegment){.header = header};
    send_spw(sk->spw_layer, &seg);
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
        header = sk->tx_window[seq_n].header;
        if (header.payload_length == 0 ||
            !is_dta(&header) ||
            header.seq_number != seq_n ||
            header.source_addr != sk->ost->self_address)
            return -1;
        send_spw(sk->spw_layer, &sk->tx_window[seq_n]);
//		if (add_new_timer(&sk->queue, seq_n, DURATION_RETRANSMISSON) != 1)
//			return -1;
    }
    return 1;
}

void send_spw(SWIC_SEND spw_layer, OstSegment *seg)
{
	unsigned char addr_sz = 0;

//	unsigned char addr;
//	addr_sz = 1;
//	if(spw_layer == SWIC0)
//		addr = 0b00000001;
//	else if(spw_layer == SWIC1)
//		addr = 0b00000010;

    unsigned char src[64*1024 + 6] __attribute__((aligned(8))) = {
        0,
    };
    unsigned int sz = sizeof(OstSegmentHeader) + seg->header.payload_length;
    int i = 0;
	memcpy(src + addr_sz, &seg->header, sizeof(OstSegmentHeader));
	memcpy(src + addr_sz + sizeof(OstSegmentHeader), seg->payload, seg->header.payload_length);
//	src[0] = addr;
	swic_send_packege(spw_layer, src, sz + addr_sz);
}

void send_to_application(OstSocket *const sk, OstSegment *seg)
{
    if (VerifyArray(seg->payload, 0, seg->header.payload_length, sk->rx_window_bottom) == 1)
        sk->verified_received++;
    free(seg->payload);
}

int8_t in_tx_window(const OstSocket *const sk, uint8_t seq_n)
{
    return (sk->tx_window_top >= sk->tx_window_bottom && seq_n >= sk->tx_window_bottom &&
            seq_n < sk->tx_window_top) ||
           (sk->tx_window_bottom > sk->tx_window_top &&
            (seq_n >= sk->tx_window_bottom || seq_n < sk->tx_window_top));
}

int8_t in_rx_window(const OstSocket *const sk, uint8_t seq_n)
{
    if (sk->rx_window_bottom > sk->rx_window_top)
    {
        return seq_n >= sk->rx_window_bottom || seq_n <= sk->rx_window_top;
    }
    return seq_n >= sk->rx_window_bottom && seq_n <= sk->rx_window_top;
}

int8_t tx_sliding_window_have_space(const OstSocket *const sk)
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

void init_socket(OstSocket *const sk)
{
    sk->to_retr = 0;
    sk->tx_window_top = 0;
    sk->tx_window_bottom = 0;
    sk->rx_window_top = 0;
    sk->rx_window_bottom = 0;
    uint8_t i;
    for (i = 0; i < WINDOW_SZ; ++i)
    {
        sk->acknowledged[i] = 0;
    }
}

void print_header(const OstSegmentHeader *const header)
{
    debug_printf("flags: %d, seq_n: %d, length: %d bytes, src_addr: %d\n", header->flags, header->seq_number, header->payload_length, header->source_addr);
}
