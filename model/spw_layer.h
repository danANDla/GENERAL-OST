#ifndef SPW_LAYER_H
#define SPW_LAYER_H

#include "../swic.h"
#include <inttypes.h>

#define MAX_WORMHOLE_ADDRESS_LENGTH 4
#define TRY 100
#define SWIC_SPEED 10

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @relates SpwDevice
 * @fn spw_is_ready_to_transmit(const SWIC_SEND interface);
 * @brief Готов ли интерфейс к передаче
 * @param interface Идентификатор интерфейса
 * @returns 1 в случае готовности.
 */
int8_t spw_is_ready_to_transmit(const SWIC_SEND interface);

/**
 * @relates SpwDevice
 * @fn spw_send_data(const SWIC_SEND interface, const uint8_t* const data, const uint32_t sz);
 * @brief Отправить данные 
 * @param interface Идентификатор интерфейса
 * @param data Буффер с данными 
 * @param sz Размер данных для отправки 
 * @returns 1 в случае успешного выполнения.
 */
int8_t spw_send_data(const SWIC_SEND interface, const uint8_t* const data, const uint32_t sz);

/**
 * @relates SpwDevice
 * @fn spw_hw_init();
 * @brief Запустить интерфейсы. Для каждого: выполнить установление соединения с противоположным 
 * узлом, установить скорость передачи (200 Мбит/c).
 * @returns 1 в случае успешного выполнения.
 * @returns -1 если соединение не установлено или не удалось установить необходимую скорость передачи.
 */
int8_t spw_hw_init();

/**
 * @relates SpwDevice
 * @fn spw_receive_run(void* dst, const uint32_t dst_area_size, unsigned int* desc_area, const uint32_t desc_number);
 * @brief Передать контроллеру dma области для хранения принятых пакетов и их дескрипторов. 
 * И начать принимать данные.
 * @param dst Выделенная память для принимаемых пакетов
 * @param dst_area_size Размер области под пакеты
 * @param desc_area Выделенная память для дескрипторов принимаемых пакетов
 * @param desc_number Количество дескрипторов пакетов, котороые вмещает область под дескрипторы
 * @returns 1 в случае успешного выполнения.
 * @returns -2 Если не выровнена память
 */
int8_t spw_receive_run(void* dst, const uint32_t dst_area_size, unsigned int* desc_area, const uint32_t desc_number);

/**
 * @relates SpwDevice
 * @fn spw_recieve_wait(unsigned int *desc, unsigned char desc_to_receive);
 * @brief Перейти в ожидание пока не придет новый пакет.
 * @param desc Указатель на дескриптор принятого пакета в случае успешного выполнения
 * @param desc_to_recieve Порядковый номер пакета, который необходимо ожидать
 * @returns 1 в случае успешного выполнения.
 * @returns -1 в случае ошибки в принятом пакете (если он оканчивается символом EEP)
 */
int8_t spw_recieve_wait(unsigned int *desc, unsigned char desc_to_receive);

#ifdef __cplusplus
}
#endif

#endif // SPW_LAYER_H
