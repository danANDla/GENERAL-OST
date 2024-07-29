#ifndef OST_COMMON_H
#define OST_COMMON_H

#include <inttypes.h>

#if (defined(TARGET_MC30SF6))
#include "../swic.h"
#else
typedef uint8_t SWIC_SEND;
#endif

/**
 * @ingroup OstNode
 * @enum TransportLayerEvent
 * @brief События которые обрабатываются транспортным уровнем.
 * @var TransportLayerEvent::PACKET_ARRIVED_FROM_NETWORK
 * получен пакета интерфейсом SpaceWire
 * @var TransportLayerEvent::APPLICATION_PACKET_READY
 * готов пакет для отправки
 * @var TransportLayerEvent::RETRANSMISSION_INTERRUPT
 * прерывание аппаратного таймера
 */
typedef enum TransportLayerEvent
{
    PACKET_ARRIVED_FROM_NETWORK = 0,
    APPLICATION_PACKET_READY,
    RETRANSMISSION_INTERRUPT,
    SPW_READY
} TransportLayerEvent;

#endif //OST_COMMON_H