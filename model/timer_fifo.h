#ifndef TIMER_FIFO_H
#define TIMER_FIFO_H

#include <inttypes.h>

#define Q_SZ 255

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @ingroup ost
     * \defgroup Timer Timer structures and functions
     */

    /**
     * @ingroup Timer
     * @typedef micros_t
     * @brief Целочисленный беззнаковый тип микросекунд.
     */
    typedef uint32_t micros_t;

    /**
     * @ingroup Timer
     * @brief основная частота в кГц
     */
    static const uint32_t CPU_FREQ = 32768;

    static const uint32_t MAX_TIMER_DURATION = 1048560000000;
    static const micros_t TIMER_MIN_STEP = 31;

    /**
     * @ingroup Timer
     * @struct Timer
     * @brief Представление таймера в очерди.
     *
     * @var Timer::for_packet
     * Номер пакета для которого запущен таймер
     *
     * @var Timer::for_packet
     * Продолжительность в тиках, на которую должен быть запущен аппаратный таймер, когда данный таймер
     * окажется в начачле очереди.
     */
    typedef struct
    {
        uint8_t for_packet;
        uint32_t tics;
    } Timer;

    /**
     * @ingroup Timer
     * @struct HardwareTimer
     * @brief Implementation (elvees 1892VM15F) of hardware timer.
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
    typedef struct
    {
        uint8_t ITCR;
        uint32_t ITPERIOD;
        uint32_t ITCOUNT;
        uint8_t ITSCALE;
    } HardwareTimer;

/**
 * \ingroup Timer
 * \struct TimerFifo
 * \brief FIFO oчередь, позволяющая хранить несколько таймеров при использовании одного аппаратного.
 *
 * Хранит значение времени через которое должен сработать следующий таймер. При срабатывании
 * прерывания удаляется верхний элемент, новый таймер запускается с значением верхнего элемента
 * очереди. При добавлении нового таймера (который должен сработать через new_timer секунд)
 * происходит обращение к состоянию аппаратного таймера, определяется сколько времени прошло
 * с момента запуска таймера (time_passed). Новый таймер добавляется в конец очереди
 * с значением равным t = new_timer - timers_sum - time_passed.
 *
 * @note Ожидается что таймеры имеют одинаковую длительность,
 * должно выполняться условие : new_timer > timers_sum - time_passed.
 *
 * @var TimerFifo::data
 * Массив времён в @ref ost::MicroSeconds "наносекундах"
 *
 * @var TimerFifo::head
 * Индекс первого элемента очереди
 *
 * @var TimerFifo::tail
 * Индекс последнего элемента очереди
 *
 * @var TimerFifo::timers_sum
 * Текущая сумма таймеров (в тиках) - время, через которое срабатает самый последний таймер,
 * при условии что самый первый в очереди только что был запущен. Если прошло какое-то время в
 *
 * @var TimerFifo::last_timer
 * Продлжительность таймера (в тиках), который сейчас тикает.
 *
 * @var TimerFifo::interrupt_counter
 * Отладочная информация о количестве срабатываний таймера.
 *
 * @var TimerFifo::fifo_id
 * Идентификатор очереди таймеров. Одна очередь обслуживается одним аппаратным таймером.
 */
#if (defined(TARGET_NS3))
	#include "ns3/event-id.h"
	#include "ns3/callback.h"

	typedef struct
	{
		/* data */
		uint32_t val;
		EventId e_id;
	} NS3_TIMER;

	typedef struct
	{
		Timer data[Q_SZ + 1];
		uint8_t head;
		uint8_t tail;
		uint32_t timers_sum;
		NS3_TIMER last_timer;
		uint32_t interrupt_counter;
		uint8_t fifo_id;
	} TimerFifo;
#else
    typedef struct
    {
        Timer data[Q_SZ + 1];
        uint8_t head;
        uint8_t tail;
        uint32_t timers_sum;
        uint32_t last_timer;
        uint32_t interrupt_counter;
        uint8_t fifo_id;
    } TimerFifo;
#endif

    void init_timer_queue(TimerFifo *const q);

    /**
     * @relates TimerFifo
     * @fn add_new_timer(TimerFifo *const queue, uint8_t seq_n, const micrios_t duration)
     * @brief Добавить таймер в очередь.
     * @param queue очерeдь таймеров.
     * @param seq_n номер пакета, к которому относится таймер.
     * @param duration время в миллисекундах, через которое должен сработать таймер.
     * @returns 1 в случае успешного выполнения.
     */
    int8_t add_new_timer(TimerFifo *const queue, uint8_t seq_n, const micros_t duration);

    /**
     * @relates TimerFifo
     * @fn cancel_timer(TimerFifo *const queue, const uint8_t seq_n)
     * @brief Удалить таймер пакета из очереди.
     * @param queue очерeдь таймеров.
     * @param seq_n номер пакета, к которому относится таймер.
     * @returns 1 в случае успешного выполнения.
     */
    int8_t cancel_timer(TimerFifo *const queue, const uint8_t seq_n);

    /**
     * @relates TimerFifo
     * @fn timer_interrupt_handler(int a)
     * @brief Обработчик прерывания аппаратного таймера.
     * @param a
     *
     * Сигнатура функции согласно документации процессора. В имплементации для 1892ВМ15АФ
     * функция помещена в main.c.
     */
    void timer_interrupt_handler(int a);

    /**
     * @relates TimerFifo
     * @fn get_number_of_timers(const TimerFifo *const q)
     * @param queue очерeдь таймеров.
     * @returns количество таймеров в очереди.
     */
    int8_t get_number_of_timers(const TimerFifo *const q);

    /**
     * @relates TimerFifo
     * @fn queue_empty(const TimerFifo *const q)
     * @param queue очерeдь таймеров.
     * @returns 1, если в очереди нет таймеров.
     */
    int8_t queue_empty(const TimerFifo *const q);

    /**
     * @relates TimerFifo
     * @fn clean_queue(const TimerFifo *const q)
     * @param queue очерeдь таймеров.
     * @brief Удалить все таймеры из очереди.
     * @returns 1 в случае успешного выполнения.
     */
    int8_t clean_queue(TimerFifo *const q);

    /**
     * @relates TimerFifo
     * @fn print_timers(const TimerFifo *const q)
     * @param queue очерeдь таймеров.
     * @brief Удалить все таймеры из очереди.
     * @returns 1 в случае успешного выполнения.
     */
    void print_timers(const TimerFifo *const q);


    /**
     * @relates TimerFifo
     * @fn rtc_timer_start()
     * @brief Начать измерение времени.
     * Для измерения используется аппаратный watchdog таймер в режиме интервального таймера.
     */
    void rtc_timer_start();

    /**
     * @relates TimerFifo
     * @fn rtc_timer_start()
     * @brief Остановить измерение времени.
     * Через UART выводится время прошедшее с момента старта в милиисекндах и тактах процессора.
     */
    void rtc_timer_stop();

#ifdef __cplusplus
}
#endif
#endif
