#include "timer_fifo.h"

void move_head(TimerFifo *const q);
void rmove_head(TimerFifo *const q);
void move_tail(TimerFifo *const q);
int8_t is_queue_have_space(const TimerFifo *const q);
int8_t get_number_of_timers(const TimerFifo *const q);
int8_t activate_itimer(TimerFifo *const q, const nsecs_t duration);
int8_t is_queue_have_space(const TimerFifo* const q);

int8_t push_timer(TimerFifo* const q, const uint8_t seq_n, nsecs_t duration, nsecs_t* duration_to_set);
int8_t pop_timer(TimerFifo* const q, const uint8_t seq_n, nsecs_t* durtaion_to_set);

nsecs_t get_hard_timer_left_timer();

void hw_timer_start(nsecs_t);
void hw_timer_stop();

void move_head(TimerFifo* const q) {
    q->head = (q->head + 1) % (MAX_UNACK_PACKETS + 1);
}

void rmove_head(TimerFifo* const q) {
    if(q->head == 0) q->head = MAX_UNACK_PACKETS;
    else q->head -= 1;
}

void move_tail(TimerFifo* const q) {
    q->tail = (q->tail + 1) % (MAX_UNACK_PACKETS + 1);
}

int8_t is_queue_have_space(const TimerFifo* const q) {
    return ((q->head + 1) % (MAX_UNACK_PACKETS + 1)) != q->tail;
}

int8_t get_number_of_timers(const TimerFifo* const q) {
    if(q->head < q->tail)
        return MAX_UNACK_PACKETS - q->tail + q->head;
    return q->head - q->tail;
}

int8_t push_timer(TimerFifo* const q, uint8_t seq_n, nsecs_t duration, nsecs_t* duration_to_set) {
    if (!is_queue_have_space(q) || duration > MAX_TIMER_DURATION) {
        return -1;
    }
    if (q->head == q->tail) {
        q->data[q->head] = (Timer) { .for_packet = seq_n, .val = duration};
        *duration_to_set = duration;
    } else {
        nsecs_t left = get_hard_timer_left_time();
        q->data[q->head] = (Timer) {.for_packet = seq_n, .val = duration - left - q->timers_sum};
        q->timers_sum += q->data[q->head].val;
        *duration_to_set = 0;
    }
    nsecs_t r = q->data[q->head].val;
    move_head(q);
    return 0;
}

int8_t pop_timer(TimerFifo* const q, uint8_t seq_n, nsecs_t* duration_to_set) {
    if (q->tail == q->head) {
        return -1;
    }

    if(seq_n == q->data[q->tail].for_packet) {
        NS_LOG_DEBUG("timer is on tail");
        if(((q->tail > q->head) && q->tail == MAX_UNACK_PACKETS && q->head == 0) || q->head - q->tail == 1) {
            NS_LOG_DEBUG("timer is the only");
            move_tail(q);
            *duration_to_set = 0;
        } else {
            nsecs_t r = get_hard_timer_left_time(q);
            move_tail(q);
            q->timers_sum -= q->data[q->tail].val;
            q->data[q->tail].val += r;
            *duration_to_set = q->data[q->tail].val;
        }
    } else {
        NS_LOG_DEBUG("timer is smwhere else");
        *duration_to_set = 0;

        uint8_t t_id = q->tail;

        while(q->data[t_id].for_packet != seq_n && t_id != q->head) t_id = (t_id + 1) % (MAX_UNACK_PACKETS + 1);
        if(t_id == q->head) return -1;

        nsecs_t r = q->data[t_id].val;
        uint8_t i = t_id;
        while((i + 1) % (MAX_UNACK_PACKETS + 1) != q->head) {
            q->data[i] = q->data[(i + 1) % (MAX_UNACK_PACKETS + 1)];
            i = (i + 1) % (MAX_UNACK_PACKETS + 1);
        }
        rmove_head(q);
        if(t_id == q->head) {
            q->timers_sum -= r;
        }
        else {
            q->data[t_id].val += r;
        }
    }
    return 0;
}

int8_t add_new_timer(TimerFifo* const q, uint8_t seq_n, const nsecs_t duration) {
    nsecs_t to_set;
    int8_t r = push_timer(q, seq_n, duration, &to_set);
    if(r != 0) return -1;
    if(to_set != 0) {
        activate_timer(to_set);
    }
    return 0;
}

int8_t cancel_timer(TimerFifo* const q, uint8_t seq_n) {
    int8_t was_in_hw = seq_n == q->data[q->tail].val; // if is timer that was on hw, remove from hw first
    nsecs_t to_set;
    int8_t r = pop_timer(q, seq_n, to_set);
    if(r != 0) return -1;

    if(was_in_hw) {
        hw_timer_stop();
    }
    if(to_set != 0) {
        activate_timer(to_set);
    }

    return 0;
} 

void timer_interrupt_handler(TimerFifo* const q) {
        uint8_t seq_n = q->data[q->tail].for_packet;
        //NS_LOG_LOGIC("timer is up (" << std::to_string(seq_n) <<  ")");
        nsecs_t to_set;
        int8_t r = pop_timer(q, seq_n, to_set);
        if(r == 0 && to_set != 0) {
            activate_timer(q, &to_set);
        }
        //upper_handler(seq_n);
} 

