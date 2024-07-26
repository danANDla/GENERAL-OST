#include "timer_fifo.h"

#if (defined(TARGET_MC30SF6))
#include "../risc_timer.h"
#include "../cpu.h"
#include "../risc_interrupt.h"
#include "../system.h"
#include "../debug_printf.h"

RISC_INTERRUPT_TYPE int_t = INTH_80000180;
#endif

void move_head(TimerFifo *const q);
void rmove_head(TimerFifo *const q);
void move_tail(TimerFifo *const q);
int8_t is_queue_have_space(const TimerFifo *const q);
int8_t get_number_of_timers(const TimerFifo *const q);
int8_t activate_timer(TimerFifo *const q, const uint32_t duration);
void hw_timer_stop(TimerFifo *const q);

int8_t push_timer(TimerFifo *const q, const uint8_t seq_n, const uint32_t duration, uint32_t *const duration_to_set);
int8_t pop_timer(TimerFifo *const q, const uint8_t seq_n, uint32_t *const durtaion_to_set);

uint32_t get_hard_timer_left_tics(const TimerFifo *const q);


void move_head(TimerFifo *const q)
{
    q->head = (q->head + 1) % (Q_SZ + 1);
}

void rmove_head(TimerFifo *const q)
{
    if (q->head == 0)
        q->head = Q_SZ;
    else
        q->head -= 1;
}

void move_tail(TimerFifo *const q)
{
    q->tail = (q->tail + 1) % (Q_SZ + 1);
}

int8_t is_queue_have_space(const TimerFifo *const q)
{
    return ((q->head + 1) % (Q_SZ + 1)) != q->tail;
}

int8_t get_number_of_timers(const TimerFifo *const q)
{
    if (q->head < q->tail)
        return Q_SZ - q->tail + q->head;
    return q->head - q->tail;
}

int8_t push_timer(TimerFifo *const q, const uint8_t seq_n, const uint32_t duration_tics, uint32_t *const duration_to_set)
{
    if (!is_queue_have_space(q) || duration_tics > MAX_TIMER_DURATION)
    {
        return -1;
    }
    if (q->head == q->tail)
    {
        q->data[q->head] = (Timer){.for_packet = seq_n, .tics = duration_tics};
        *duration_to_set = duration_tics;
    }
    else
    {
        micros_t left = get_hard_timer_left_tics(q);
        q->data[q->head] = (Timer){.for_packet = seq_n, .tics = duration_tics - left - q->timers_sum};
        q->timers_sum += q->data[q->head].tics;
        *duration_to_set = 0;
    }
    move_head(q);
    return 1;
}

int8_t pop_timer(TimerFifo *const q, const uint8_t seq_n, uint32_t *const duration_to_set)
{
    if (q->tail == q->head)
    {
        return -1;
    }

    if (seq_n == q->data[q->tail].for_packet)
    {
        if (((q->tail > q->head) && q->tail == Q_SZ && q->head == 0) || ((q->tail < q->head) && q->head - q->tail == 1))
        {
            move_tail(q);
            *duration_to_set = 0;
        }
        else
        {
            micros_t r = get_hard_timer_left_tics(q);
            move_tail(q);
            q->timers_sum -= q->data[q->tail].tics;
            q->data[q->tail].tics += r;
            *duration_to_set = q->data[q->tail].tics;
        }
    }
    else
    {
        *duration_to_set = 0;

        uint8_t t_id = q->tail;

        while (q->data[t_id].for_packet != seq_n && t_id != q->head)
            t_id = (t_id + 1) % (Q_SZ + 1);
        if (t_id == q->head)
            return -1;

        micros_t r = q->data[t_id].tics;
        uint8_t i = t_id;
        while ((i + 1) % (Q_SZ + 1) != q->head)
        {
            q->data[i] = q->data[(i + 1) % (Q_SZ + 1)];
            i = (i + 1) % (Q_SZ + 1);
        }
        rmove_head(q);
        if (t_id == q->head)
        {
            q->timers_sum -= r;
        }
        else
        {
            q->data[t_id].tics += r;
        }
    }
    return 1;
}

uint32_t tics_from_micros(const micros_t ms)
{
    return ms * TIMER_MIN_STEP;
}


void init_timer_queue(TimerFifo *const q) {

}

int8_t add_new_timer(TimerFifo *const q, uint8_t seq_n, const micros_t duration)
{
    uint32_t to_set;
    int8_t r = push_timer(q, seq_n, tics_from_micros(duration), &to_set);
    if (r != 1)
        return -1;
    if (to_set != 0)
    {
        return activate_timer(q, to_set);
    }
    return 1;
}

int8_t cancel_timer(TimerFifo *const q, const uint8_t seq_n)
{
    int8_t was_in_hw = seq_n == q->data[q->tail].for_packet;
    uint32_t to_set;
    int8_t r = pop_timer(q, seq_n, &to_set);
    if (r != 1)
        return -1;

    if (was_in_hw)
    {
        hw_timer_stop(q);
    }
    if (to_set != 0)
    {
        return activate_timer(q, to_set);
    }

    return 1;
}

void timer_interrupt_handler(int a)
{
}

int8_t activate_timer(TimerFifo *const q, const uint32_t tics)
{
#if (defined(TARGET_MC30SF6))
    ITSCALE0 = 0;
    ITPERIOD0 = tics;
    ITCSR0 = (2 << 3);
    ITCSR0 |= 1;
    risc_enable_interrupt(RISC_INT_IT0, 0);
    q->last_timer = tics;
#elif (defined(TARGET_NS3))
    q->last_timer.e_id = Simulator::Schedule(
        MicroSeconds(duration),
        &TimerFifo::timer_interrupt_handler,
        this);
    q->last_timer.val = tics;
#endif
    return 1;
}

uint32_t get_hard_timer_left_tics(const TimerFifo *const q)
{
#if (defined(TARGET_MC30SF6))
    return ITCOUNT0;
#elif (defined(TARGET_NS3))
    int64_t nanos = Simulator::GetDelayLeft(q->last_timer.e_id).GetNanoSeconds();
    int64_t micros = Simulator::GetDelayLeft(q->last_timer.e_id).GetMicroSeconds();
    if (nanos)
        return micros + 1;
    return micros;
#endif
    return 0;
}

void hw_timer_stop(TimerFifo *const q)
{
#if (defined(TARGET_MC30SF6))
        ITCSR0 &= 0xfe;
#elif (defined(TARGET_NS3))
        Simulator::Cancel(q->last_timer.e_id);
    #endif
}

void print_timers(const TimerFifo *const q)
{
    //	debug_printf("%d timers\n", get_number_of_timers(q));

    int i;
    for (i = 0; i < Q_SZ; ++i)
    {
        if (i == q->tail && i == q->head)
            debug_printf("[]");
        else if (i == q->tail)
            debug_printf("[");

        if ((q->tail > q->head && (i >= q->tail || i < q->head)) || (q->head > q->tail && i >= q->tail && i < q->head))
        {
            debug_printf("%5d {%2d} ", q->data[i].tics, q->data[i].for_packet);
        }
        else
        {
            if (i == q->head && i != q->tail)
                debug_printf("] ");
            else if (i != Q_SZ)
                debug_printf(" NaN ");
        }
    }
    debug_printf("\n tail=%d, head=%d, sum=%d, left_in_hw=%u\n", q->tail, q->head, q->timers_sum, get_hard_timer_left_tics(q));
}

int8_t queue_empty(const TimerFifo *const q)
{
    return get_number_of_timers(q) == 0;
}

int8_t clean_queue(TimerFifo *const q)
{
    hw_timer_stop(q);
    q->head = 0;
    q->tail = 0;
    q->timers_sum = 0;
    q->last_timer = 0;
    uint16_t i;
    for (i = 0; i < Q_SZ; ++i)
        q->data[i] = (Timer){.tics = 0, .for_packet = 0};
}

#if (defined(TARGET_MC30SF6))
int rtc_timer_counter = 0;
void rtc_timer_handler(int a)
{
    if ((WTCSR & 0xfdff) == 2)
    {
        WTCSR |= (1 << 8); // start timer
    }
    rtc_timer_counter += 1;
}

void rtc_timer_start()
{
    WTCSR &= 0xfeff;    // disable timer
    WTCSR |= (1 << 10); // switch to itm

    risc_enable_interrupt(RISC_INT_WDT, 0);
    risc_register_interrupt(rtc_timer_handler, RISC_INT_WDT);

    WTSCALE = 0;
    WTPERIOD = 0xffffffff;
    WTCSR |= (1 << 8); // start timer
}

void rtc_timer_print_elapsed()
{
    //	debug_printf("%u interrupts, %u tics in timer\n", timer_counter, ITCOUNT0);
    //	debug_printf("%u tics processed \n", 0xffffffff - ITCOUNT0);
    float ms = ((float)(0xffffffff - WTCOUNT) * 1000 + (float)rtc_timer_counter * 0xffffffff) / (float)get_cpu_clock();
    debug_printf("%.2f milliseconds, %u tics, %u interrupts\n", ms, 0xffffffff - WTCOUNT, rtc_timer_counter);
}

void rtc_timer_stop()
{
    WTCSR &= 0xfeff;
    rtc_timer_print_elapsed();
    risc_disable_interrupt(RISC_INT_WDT, 0);
}
#endif
