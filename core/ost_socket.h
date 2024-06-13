#ifndef OST_SOCKET_H
#define OST_SOCKET_H

#include <inttypes.h>
#include "ost_segment.h"
#include "timer_fifo.h"
#include "swic.h"

static const micros_t DURATION_RETRANSMISSON = 1000; // 1 secs

#define WINDOW_SZ 10

enum TransportLayerEvent;

/**
 * @ingroup ost
 * \defgroup OstSocket
 */


/**
 * @ingroup OstSocket
 * @enum SocketMode
 * @brief Режим работы сокета.
 * @var SocketMode::CONNECTIONLESS
 * без установки соединения 
 * @var SocketMode::CONNECTION_ACTIVE
 * с активным установлением соединения (переход из closed по вызову open)
 * @var SocketMode::CONNECTION_PASSIVE
 * с пассвным установлением соединения (переход из closed по полученному SYN)
 */
typedef enum SocketMode
{
    CONNECTIONLESS = 0,
    CONNECTION_ACTIVE,
    CONNECTION_PASSIVE
} SocketMode;


/**
 * @ingroup OstSocket
 * @enum SocketState
 * @brief Режим работы сокета. Основано на RFC-908
 * @var SocketState::CLOSED
 * закрыт
 * @var SocketState::LISTEN
 * включен режим с пассивным соединением; ожидает запроса на подключение
 * @var SocketState::SYN_SENT
 * включен режим с активным соединением; ожидает подтверждения запроса на подключение
 * @var SocketState::SYN_RCVD
 * был получен syn; ожидает подтверждения
 * @var SocketState::OPEN
 * открыт
 * @var SocketState::CLOSE_WAIT
 * отправлен RST; ожидает подтверждение или таймаут
 */
typedef enum
{
    CLOSED = 0,
    LISTEN,
    SYN_SENT,
    SYN_RCVD,
    OPEN,
    CLOSE_WAIT,
} SocketState;

/**
 * \ingroup OstSocket
 * \struct OstSocket
 * \brief Хранение информации о подключении.
 *
 * @var OstSocket::ost
 * Указатель на узел к которому относится сокет

 * @var OstSocket::mode
 * Режим работы сокета
 *
 * @var OstSocket::state
 * Текущее состоянисе сокета
 *
 * @var OstSocket::to_address
 * Адрес назначения
 *
 * @var OstSocket::self_address
 * Совпадает с адресом присвоенным узлу
 *
 * @var OstSocket::self_port
 * Индекс в списке сокетов узла
 * 
 * @var OstSocket::to_retr
 * Номер пакета который должен быть повторно отправлен. Изменяется обработчиком прерывания таймера
 * 
 * @var OstSocket::tx_window_bottom
 * Нижняя граница скользящего окна отправления - самый старый пакет, которые был добавлен в окно 
 * и для которго не было еще получено подтверждение.
 * 
 * @var OstSocket::tx_window_top
 * Верхняя граница скользящего окна отправления - самый новый пакет, которые был добавлен в окно 
 * и для которго не было еще получено подтверждение.
 * 
 * @var OstSocket::rx_window_bottom
 * Нижняя граница скользящего окна приёма - первый в очереди пакет, который будет 
 * передан уровню приложения при его получении
 * 
 * @var OstSocket::rx_window_top
 * Верхняя граница скользящего окна приёма - последний в очереди пакет, который будет 
 * передан уровню приложения при получениии
 * 
 * @var OstSocket::tx_buffer
 * Окно передачи - пакеты, для которых не было получено подтверждение
 * 
 * @var OstSocket::rx_buffer
 * Окно приёма - пакеты, которые не были переданы уровню приложения
 * 
 * @var OstSocket::acknowledged
 * Было ли получено подтверждение для пакета
 * 
 * @var OstSocket::queue
 * Очередь таймеров
 * 
 * @var OstSocket::spw_layer
 * Физический интерфейс который привязан к сокету
 */

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
    OstSegment tx_window[WINDOW_SZ];
    OstSegment rx_window[WINDOW_SZ];
    int8_t acknowledged[WINDOW_SZ];
    TimerFifo queue;

    int8_t verified_received;
    void (*application_receive_callback)(uint8_t, uint8_t, OstSegment *);
    SWIC_SEND spw_layer;
} OstSocket;

/**
 * @relates OstSocket
 * @fn open(OstSocket *const sk, int8_t mode)
 * @brief Открыть сокет, выполнить подключение
 * @param sk Сокет
 * @param mode режим сокета
 * @returns 1 в случае успешного выполнения.
 */
int8_t open(OstSocket *const sk, int8_t mode);

/**
 * @relates OstSocket
 * @fn close(OstSocket *const sk)
 * @brief Закрыть сокет, выполнить разрыв соединения
 * @param sk Сокет
 * @returns 1 в случае успешного выполнения.
 */
int8_t close(OstSocket *const sk);

/**
 * @relates OstSocket
 * @fn send(OstSocket *const sk, const uint8_t *buffer, uint32_t size)
 * @brief Отправить данные из буфера
 * @param sk Сокет
 * @param buffer Буфер с данными
 * @param size Размер данных для отправки
 * @returns 1 в случае успешного выполнения.
 */
int8_t send(OstSocket *const sk, const uint8_t *buffer, uint32_t size);

/**
 * @relates OstSocket
 * @fn send(OstSocket *const sk, const uint8_t *buffer, uint32_t size)
 * @brief Отправить данные из буфера
 * @param sk Сокет
 * @param buffer Буфер с данными
 * @param size Размер данных для отправки
 * @returns 1 в случае успешного выполнения.
 */
int8_t receive(OstSocket *const sk, OstSegment *seg);

/**
 * @relates OstSocket
 * @fn socket_event_handler(OstSocket *const sk, const enum TransportLayerEvent e, OstSegment *seg, uint8_t seq_n);
 * @brief Обработчик событий
 * @param sk Сокет
 * @param e Событие
 * @param seg Указатель на сегмент который пришел с низкого уровня
 * @param seq_n Номер пакета который должен быть повторно отправлен
 * @returns 1 в случае успешного выполнения.
 */
int8_t socket_event_handler(OstSocket *const sk, const enum TransportLayerEvent e, OstSegment *seg, uint8_t seq_n);

/**
 * @relates OstSocket
 * @fn add_to_tx(OstSocket *const sk, const OstSegment *const seg, uint8_t *const seq_n)
 * @brief Добавить пакет в очередь для отправки
 * @param sk Сокет
 * @param seg Указатель на сегмент который будет отправлен
 * @param seq_n Номер который присвоен пакету
 * @returns 1 в случае успешного выполнения.
 */
int8_t add_to_tx(OstSocket *const sk, const OstSegment *const seg, uint8_t *const seq_n);

#endif
