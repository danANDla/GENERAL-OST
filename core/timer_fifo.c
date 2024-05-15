#include "timer_fifo.h"

#define ON_MC30SF

#ifdef ON_MC30SF

#include "../../risc_timer.h"
#include "../../cpu.h"
#include "../../risc_interrupt.h"
#include "../../system.h"

RISC_INTERRUPT_TYPE int_t = INTH_80000180;

#endif

void move_head(TimerFifo *const q);
void rmove_head(TimerFifo *const q);
void move_tail(TimerFifo *const q);
int8_t is_queue_have_space(const TimerFifo *const q);
int8_t get_number_of_timers(const TimerFifo *const q);
int8_t activate_timer(TimerFifo *const q, const micros_t duration);
void hw_timer_stop();

int8_t push_timer(TimerFifo* const q, const uint8_t seq_n, uint32_t duration_tics, uint32_t* duration_to_set);
int8_t pop_timer(TimerFifo* const q, const uint8_t seq_n, uint32_t* durtaion_to_set);

micros_t get_hard_timer_left_time(const TimerFifo* const q);

TimerFifo* main_timer_fifo;
int* int_counter;

void move_head(TimerFifo* const q) {
    q->head = (q->head + 1) % (Q_SZ + 1);
}

void rmove_head(TimerFifo* const q) {
    if(q->head == 0) q->head = Q_SZ;
    else q->head -= 1;
}

void move_tail(TimerFifo* const q) {
    q->tail = (q->tail + 1) % (Q_SZ + 1);
}

int8_t is_queue_have_space(const TimerFifo* const q) {
    return ((q->head + 1) % (Q_SZ + 1)) != q->tail;
}

int8_t get_number_of_timers(const TimerFifo* const q) {
    if(q->head < q->tail)
        return Q_SZ - q->tail + q->head;
    return q->head - q->tail;
}

int8_t push_timer(TimerFifo* const q, uint8_t seq_n, uint32_t duration_tics, uint32_t* duration_to_set) {
    if (!is_queue_have_space(q) || duration_tics > MAX_TIMER_DURATION) {
        return -1;
    }
    if (q->head == q->tail) {
        q->data[q->head] = (Timer) { .for_packet = seq_n, .tics = duration_tics};
        *duration_to_set = duration_tics;
    } else {
        uint32_t left = get_hard_timer_left_time(q);
        q->data[q->head] = (Timer) {.for_packet = seq_n, .tics = duration_tics - left - q->timers_sum};
        q->timers_sum += q->data[q->head].tics;
        *duration_to_set = 0;
    }
    micros_t r = q->data[q->head].tics;
    move_head(q);
    return 0;
}

int8_t pop_timer(TimerFifo* const q, uint8_t seq_n, uint32_t* duration_to_set) {
    if (q->tail == q->head) {
        return -1;
    }

    if(seq_n == q->data[q->tail].for_packet) {
        if(((q->tail > q->head) && q->tail == Q_SZ && q->head == 0) || q->head - q->tail == 1) {
            move_tail(q);
            *duration_to_set = 0;
        } else {
        	uint32_t r = get_hard_timer_left_time(q);
        	if ( IT0.ITCSR.bits.EN ) r = 0;
            move_tail(q);
            q->timers_sum -= q->data[q->tail].tics;
            q->data[q->tail].tics += r;
            *duration_to_set = q->data[q->tail].tics;
        }
    } else {
        *duration_to_set = 0;

        uint8_t t_id = q->tail;

        while(q->data[t_id].for_packet != seq_n && t_id != q->head) t_id = (t_id + 1) % (Q_SZ + 1);
        if(t_id == q->head) return -1;

        micros_t r = q->data[t_id].tics;
        uint8_t i = t_id;
        while((i + 1) % (Q_SZ + 1) != q->head) {
            q->data[i] = q->data[(i + 1) % (Q_SZ + 1)];
            i = (i + 1) % (Q_SZ + 1);
        }
        rmove_head(q);
        if(t_id == q->head) {
            q->timers_sum -= r;
        }
        else {
            q->data[t_id].tics += r;
        }
    }
    return 0;
}

uint32_t tics_from_micros(const micros_t ms) {
	uint32_t tics;
	if(ms < TIMER_MIN_STEP) return tics = 1;
	else {
		tics = ms / TIMER_MIN_STEP;
		if (ms % TIMER_MIN_STEP != 0) tics += 1;
	}
	return tics;
}

int8_t add_new_timer(TimerFifo* const q, uint8_t seq_n, const micros_t duration) {
    uint32_t to_set;
    int8_t r = push_timer(q, seq_n, tics_from_micros(duration), &to_set);
    print_timers(q);
    if(r != 0) return -1;
    if(to_set != 0) {
        activate_timer(q, to_set);
    }
    return 0;
}

int8_t cancel_timer(TimerFifo* const q, uint8_t seq_n) {
    int8_t was_in_hw = seq_n == q->data[q->tail].tics; // if is timer that was on hw, remove from hw first
    uint32_t to_set;
    int8_t r = pop_timer(q, seq_n, &to_set);
    if(r != 0) return -1;

    if(was_in_hw) {
        hw_timer_stop();
    }
    if(to_set != 0) {
        activate_timer(q, to_set);
    }

    return 0;
}



int8_t activate_timer(TimerFifo* const q, const uint32_t tics) {
	debug_printf("starting timer for %u tics\n", tics);
	risc_it_setup(tics, 2);
    risc_it_start();
    q->last_timer = tics;
    return 1;
}

uint32_t get_hard_timer_left_time(const TimerFifo* const q) {
    #ifdef ON_MC30SF
        //unsigned int clk = q->last_timer - ITCOUNT0;
        unsigned int clk = IT0.ITCOUNT.data;
        return clk;
    #endif // ON_MC30SF
    return 0;
}

void hw_timer_stop() {
	risc_it_stop();
}


void timer_interrupt_handler(int a) {
    #ifdef ON_MC30SF
		if ( IT0.ITCSR.bits.EN ){
            debug_printf("Timer beps\n");
            *int_counter += 1; //инкрементируем счетчик прерываний
            risc_it_stop();
            if(*int_counter >= 3) {
                risc_disable_interrupt(RISC_INT_IT0, 0);
            }
            else {
                TimerFifo* q = main_timer_fifo;
                uint8_t seq_n = q->data[q->tail].for_packet;
                micros_t to_set;
                int8_t r = pop_timer(q, seq_n, &to_set);
                print_timers(q);
                if(r == 0 && to_set != 0) {
                    activate_timer(q, to_set);
                }
            }
        }
    #endif // ON_MC30SF
}

void init_hw_timer(TimerFifo* main_fi, int* int_cnt) {
    #ifdef ON_MC30SF
        main_timer_fifo = main_fi;
        int_counter = int_cnt;
        risc_set_interrupts_vector(int_t);
        risc_enable_interrupt(RISC_INT_IT0,0);
//        enum ERL_ERROR error_status =
        		risc_register_interrupt(timer_interrupt_handler, RISC_INT_IT0);
    #endif // ON_MC30SF
}

void print_timers(const TimerFifo* const q) {
//	debug_printf("%d timers\n", get_number_of_timers(q));

    int i;
    for(i = 0; i < 10; ++i) {
        if(i == q->tail && i == q->head) debug_printf("[]");
        else if(i == q->tail) debug_printf("[");

        if((q->tail > q->head && (i >= q->tail || i < q->head)) || (q->head > q->tail && i >= q->tail && i < q->head)) {
            debug_printf("%5d {%2d} ", q->data[i].tics, q->data[i].for_packet);
        }
        else {
            if(i == q->head && i != q->tail) debug_printf("] ");
            else if (i != Q_SZ) debug_printf(" NaN ");
        }
    }
    debug_printf("\n tail=%d, head=%d, sum=%d, left_in_hw=%u\n", q->tail, q->head, q->timers_sum, get_hard_timer_left_time(q));
}

int8_t queue_empty(const TimerFifo *const q) {
    return get_number_of_timers(q) == 0;
}

int8_t clean_queue(TimerFifo *const q) {
    hw_timer_stop();
    q->head = 0;
    q->tail = 0;
    q->timers_sum = 0;
    q->last_timer = 0;
    uint16_t i;
    for(i = 0; i < Q_SZ; ++i)
        q->data[i] = (Timer) {.tics = 0, .for_packet = 0};
}
