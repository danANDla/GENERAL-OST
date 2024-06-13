#ifndef OST_NODE_H
#define OST_NODE_H

#include <inttypes.h>

#include "timer_fifo.h"
#include "ost_segment.h"

#define PORTS_NUMBER 1

enum SocketMode;

/**
 * \defgroup ost Open SpaceWire Transport Layer
 * Документация программного интерфейса протокла OST для SpaceWire.
 */

/**
 * @ingroup ost
 * \defgroup OstNode
 */

/**
 * @ingroup ost
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
    RETRANSMISSION_INTERRUPT
} TransportLayerEvent;

/**
 * \ingroup OstNode
 * \struct OstNode
 * \brief Представление узла на транспортном уровне.
 *
 * Относится к одному интерфейсу SpaceWire.
 *
 * @var OstNode::self_address
 * SpaceWire адрес, который присвоен интерфейсу. Представляет собой идентификатор интерфейса
 * относительно всего проекта. При этом при передаче будет использована путевая адресация.
 * Для идентификации маршрутов до других узлов и будет использован этот идентификатор.
 *
 * @var OstNode::ports
 * Список соекетов (логических соединений с другими узлами) данного узла. 
 *
 * @var OstNode::that_arrived
 * Указатель на сегмент транспортного уровня который был получен из сети. Буферизация сегмента
 * перед обработкой сокетом.
 */
typedef struct OstNode
{
    uint8_t self_address;
    struct OstSocket *ports[PORTS_NUMBER];
    OstSegment *that_arrived;
} OstNode;

/**
 * @relates OstNode 
 * @fn start(OstNode *const node, const enum SocketMode socket_mode, uint8_t hw_timer_id)
 * @brief Запустить транспортный уровень. Выполняется инициализация интерфейса SpaceWire.
 * @param node Транспортный узел.
 * @param hw_timer_id Идентификатор аппаратного таймера, который будет использован
 * для измерения таймаутов данного узла
 * @returns 1 в случае успешного выполнения.
 */
int8_t start(OstNode *const node, uint8_t hw_timer_id);

/**
 * @relates OstNode 
 * @fn shutdown(OstNode *const node)
 * @brief Выключить транспортный уровень. Освобождение памяти выделенный для сокетов.
 * @param node Транспортный узел.
 */
void shutdown(OstNode *const node);

/**
 * @relates OstNode 
 * @fn event_handler(OstNode *const node, const TransportLayerEvent e)
 * @brief Обработчик событий уровня узла. Передает события соотсветствующему сокету.
 * @param node Транспортный узел.
 * @param e событие.
 */
int8_t event_handler(OstNode *const node, const TransportLayerEvent e);

/**
 * @relates OstNode 
 * @fn open_connection(OstNode *const node, uint8_t address, int8_t mode)
 * @brief Выполнить подключение с другим узлом. Окрытие сокета с другим узлом.
 * @param node Транспортный узел.
 * @param address Идентификатор узла назначения.
 * @param mode Режим работы: 1 - с установкой соединеия, 0 - без.
 * @returns 1 в случае успешного выполнения.
 */
int8_t open_connection(OstNode *const node, uint8_t address, int8_t mode);

/**
 * @relates OstNode 
 * @fn close_connection(OstNode *const node, uint8_t address)
 * @brief Разорвать соединение с другим узлом.
 * @param node Транспортный узел.
 * @param address Идентификатор узла назначения.
 * @returns 1 в случае успешного выполнения.
 */
int8_t close_connection(OstNode *const node, uint8_t address);

/**
 * @relates OstNode 
 * @fn send_packet(OstNode *const node, int8_t address, const uint8_t *buffer, uint32_t size)
 * @brief Отправить данные из буфера другому узлу.
 * @param node Транспортный узел.
 * @param address Идентификатор узла назначения.
 * @param buffer Указатель на данные.
 * @param size Размер данных.
 * @returns 1 в случае успешного выполнения.
 */
int8_t send_packet(OstNode *const node, int8_t address, const uint8_t *buffer, uint32_t size);

#endif // OST_NODE_H
