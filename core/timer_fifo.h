#ifndef TIMER_FIFO_H
#define TIMER_FIFO_H
 
/**
* @file
* @brief
*/


#define Q_SZ 255

#include <inttypes.h>

/**
 * @ingroup ost
 * \defgroup Timer Timer structures and functions
 */

/**
* @ingroup Timer
* @typedef NanoSeconds
* @brief Целочисленный беззнаковый тип наносекунды
*/
typedef uint32_t micros_t;

/**
* @ingroup Timer
* @brief основная частота в кГц
*/
static const uint32_t CPU_FREQ = 32768; 

static const uint32_t MAX_TIMER_DURATION = 1048560000000;
static const micros_t TIMER_MIN_STEP = 31;

typedef struct {
    uint8_t for_packet;
    uint32_t tics;
} Timer;

/**
 * @ingroup Timer
 * @struct HardwareTimer
 * @brief Implementation (elvees 1892VM15F) of hardware timer.
 * 
 *
 * @var HardwareTimer::ITCR
 * Регистр управления (182F_5000). 5 разрядов.
 *
 * @var HardwareTimer::ITPERIOD
 * Регистр периода работы таймера (182F_5004). 32 разрядов.
 *
 * @var HardwareTimer::ITCOUNT
 * Регистр счетчика (182F_5008). 32 разрядов.
 *
 * @var HardwareTimer::ITSCALE
 * Регистр предделителя (182F_500C). 8 разрядов.
 */
typedef struct {
    uint8_t ITCR;
    uint32_t ITPERIOD;
    uint32_t ITCOUNT;
    uint8_t ITSCALE;
} HardwareTimer;

/**
 * \ingroup Timer
 * \struct TimerFifo
 * \brief Data structure for helding retransmission timers.
 * 
 * FIFO oчередь хранящая значение времени через которое должен сработать
 * следующий таймер. При срабатывании прерывания удаляется верхний элемент,
 * новый таймер запускается с значением верхнего элемента очереди. При добавлении
 * нового таймера (который должен сработать через new_timer секунд) происходит
 * обращение к состоянию аппаратного таймера, определяется сколько времени прошло
 * с момента запуска таймера (time_passed). Новый таймер добавляется
 * в конец очереди с значением равным t = new_timer - timers_sum - time_passed.
 * 
 * @note Ожидается что таймеры имеют одинаковую длительность,
 * должно выполняться условие : new_timer > timers_sum - time_passed.
 *
 * @var TimerFifo::data
 * Массив времён в @ref ost::NanoSeconds "наносекундах"
 *
 * @var TimerFifo::head
 * Индекс первого элемента очереди
 *
 * @var TimerFifo::tail
 * Индекс последнего элемента очереди
 *
 * @var TimerFifo::timers_sum
 * Текущая сумма таймеров (в тиках) - время, через которое срабатает самый правый таймер
 * при условии что левой только что был запущен.

 * @var TimerFifo::last_timer
 * Продлжительность таймера (в тиках), который сейчас тикает.
 */
typedef struct {
    Timer data[Q_SZ + 1];
    uint8_t head;
    uint8_t tail;
    uint8_t window_sz;
    uint32_t timers_sum;
    uint32_t last_timer;
    // HardwareTimer* hw;
    void (*upper_handler) (uint8_t);
} TimerFifo;

/**
 * @relates TimerFifo
 * @fn int8_t add_new_timer(TimerFifo *const queue, const nsecs_t to_wait)
 * @brief Обработать прерывание от аппаратного таймера, завести новый из очереди.
 * @param queue очерeдь таймеров.
 * @param seq_n номер пакета, к которому относится таймер.
 * @param duration время через, которое должен сработать таймер.
 * @returns 0 в случае успешного выполнения.
 */
int8_t add_new_timer(TimerFifo *const queue, uint8_t seq_n, const micros_t duration);

/**
 * @relates TimerFifo
 * @fn int8_t cancel_timer(TimerFifo *const queue, const uint8_t seq_n)
 * @brief Удалить таймер пакета из очереди.
 * @param queue очерeдь таймеров.
 * @param seq_n номер пакета, к которому относится таймер.
 * @returns 0 в случае успешного выполнения.
 */
int8_t cancel_timer(TimerFifo *const queue, uint8_t seq_n);

/**
 * @relates TimerFifo
 * @fn int8_t timer_interrupt_handler(TimerFifo *const queue)
 * @brief Обработать прерывание от аппаратного таймера, завести новый из очереди.
 * @param queue очерeдь таймеров.
 * @returns Возвращает значение согласно требованию обработчика прерывыния. Пока
 * что 0 в случае успешного выполнения.
 */
void timer_interrupt_handler(int a);

void print_timers(const TimerFifo* const q);
int8_t get_number_of_timers(const TimerFifo *const q);
int8_t queue_empty(const TimerFifo *const q);
int8_t clean_queue(TimerFifo *const q);
void init_hw_timer(TimerFifo* main_fi, int* int_counter);

#endif