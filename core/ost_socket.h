#ifndef OST_SOCKET_H
#define OST_SOCKET_H

#include <inttypes.h>
#include "ost_segment.h"
#include "timer_fifo.h"
#include "../../swic.h"

static const micros_t DURATION_RETRANSMISSON = 2000000; // 2 secs

//static const uint16_t WINDOW_SZ = 10;
#define WINDOW_SZ 10

enum TransportLayerEvent;

typedef enum SocketMode
{
    CONNECTIONLESS = 0,
    CONNECTION_ACTIVE,
    CONNECTION_PASSIVE
} SocketMode;

typedef enum
{
    CLOSED = 0,
    LISTEN,
    SYN_SENT,
    SYN_RCVD,
    OPEN,
    CLOSE_WAIT,
} SocketState;

typedef struct OstSocket
{
    struct OstNode *ost;
    /* data */
    SocketMode mode;
    SocketState state;
    uint8_t to_address;
    uint8_t self_address;
    uint8_t self_port;

    uint8_t to_retr;
    uint8_t tx_window_bottom;
    uint8_t tx_window_top;
    uint8_t rx_window_bottom;
    uint8_t rx_window_top;
    OstSegment tx_buffer[WINDOW_SZ];
    OstSegment rx_buffer[WINDOW_SZ];
    int8_t acknowledged[WINDOW_SZ];
    TimerFifo queue;

    void (*application_receive_callback) (uint8_t, uint8_t, OstSegment *);

    SWIC_SEND spw_layer;
} OstSocket;

int8_t open(OstSocket* const sk, int8_t mode);
int8_t close(OstSocket* const sk);
int8_t send(OstSocket* const sk, OstSegment* seg);
int8_t receive(OstSocket* const sk, OstSegment* seg);

int8_t socket_event_handler(OstSocket* const sk, const enum TransportLayerEvent e, OstSegment* seg, uint8_t seq_n);
int8_t add_to_tx(OstSocket* const sk, const OstSegment* const seg, uint8_t * const seq_n);

#endif
