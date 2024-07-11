#include "ost_node.h"
#include "ost_socket.h"
#include <stdlib.h>
#include <stdio.h>

/**
 * @relates OstNode
 * @fn get_socket_by_address(OstNode *const node, uint8_t address, OstSocket** sk);
 * @brief Доступ к сокету по адресу.
 * @param node Транспортный узел.
 * @param address Идентификатор узла назначения.
 * @param sk Указатель на сокет.
 * @returns 1 в случае успешного выполнения.
 * @returns -1 при отсутсвии инициированного сокета по этому адресу.
 */
int8_t get_socket_by_address(OstNode *const node, uint8_t address, OstSocket **sk);

/**
 * @relates OstNode
 * @fn aggregate_socket(OstNode *const node, uint8_t address);
 * @brief Иницировать сокет для работы с конкретным узлом
 * @param node Транспортный узел.
 * @param address Идентификатор узла назначения.
 * @returns Индекс проинициализрованного сокета в случае успешного выполнения
 * @returns -1 в случае отсутствия свободных сокетов.
 */
int8_t occupy_socket(OstNode *const node, uint8_t address);

int8_t
start(OstNode *const node, uint8_t hw_timer_id)
{
    return 0;
}

int8_t event_handler(OstNode *const node, const TransportLayerEvent e)
{
    switch (e)
    {
    case PACKET_ARRIVED_FROM_NETWORK:
        if (node->that_arrived)
        {
            socket_event_handler(node->ports[0], PACKET_ARRIVED_FROM_NETWORK, node->that_arrived, 0);
        }
        break;
    case APPLICATION_PACKET_READY:
        break;
    default:
        break;
    }
    return 0;
}

int8_t open_connection(OstNode *const node, uint8_t address, int8_t mode)
{
    if (address == node->self_address)
    {
        return -2;
    }

    OstSocket *sk;
    int8_t r = GetSocket(node, address, &sk);
    if (r != 1) // create new
    {
        r = AggregateSocket(node, address);
        if (r != -1)
            return open(node->ports[r], CONNECTIONLESS);
    }
    else
    {
        if (get_socket_state(sk) == OPEN)
            return 0; // already opened
        return (sk, CONNECTIONLESS);
    }
    return 1;
}

int8_t close_connection(OstNode *const node, uint8_t address)
{
    if (address == node->self_address)
        return -1;

    for (int i = 0; i < PORTS_NUMBER; ++i)
    {
        if (node->ports[i]->to_address == address)
        {
            close(node->ports[i]);
            return 1;
        }
    }
    return 0;
}

int8_t send_packet(OstNode *const node, int8_t address, const uint8_t *buffer, uint32_t size)
{
    if (node->ports[0]->to_address != address || node->ports[0]->state != OPEN)
        return -1;
    return send(node->ports[0], buffer, size);
}

int8_t
occupy_socket(OstNode *const node, uint8_t address)
{
    for (int i = 0; i < PORTS_NUMBER; ++i)
    {
        if (!node->ports[i]->is_occupied)
        {
            node->ports[i]->to_address = address;
            node->ports[i]->is_occupied = 1;
            return i;
        }
    }
    return -1;
}

int8_t
get_socket_by_address(OstNode *const node, uint8_t address, OstSocket **sk)
{
    for (int i = 0; i < PORTS_NUMBER; ++i)
    {
        if (node->ports[i]->is_occupied && node->ports[i]->to_address == address)
        {
            *sk = node->ports[i];
            return 1;
        }
    }
    return -1;
}