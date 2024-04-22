#include "timer_fifo.h"

void hw_timer_start(nsecs_t);
void hw_timer_stop();

nsecs_t push_timer(TimerFifo* const q, uint8_t seq_n, nsecs_t duration);
nsecs_t pop_timer(TimerFifo* const q, uint8_t seq_n);

int8_t is_queue_have_space(const TimerFifo* const q) {
    return ((q->head + 1) % MAX_UNACK_PACKETS) != q->tail;
}

int8_t add_new_timer(TimerFifo* const q, const uint8_t seq_n, const nsecs_t duration) {
    if(!is_queue_have_space(q)) return -1;
    nsecs_t r = push_timer(q, seq_n, duration);
    if(r == 0) {
        return -1;
    }

    hw_timer_start(duration);
    q->last_timer = duration;

    return 0;
}

int8_t cancel_timer(TimerFifo* const q, uint8_t seq_n) {
    if(seq_n == q->data[q->tail].for_packet) {
        hw_timer_stop();
    }
    nsecs_t r = pop_timer(q, seq_n);
    if(r != -1) {
        if(r != 0) {
            hw_timer_start(r);
            q->last_timer = r;
        }
    }
    return 0;
} 

nsecs_t push_timer(TimerFifo* const q, uint8_t seq_n, nsecs_t duration) {
    if (((q->head + 1) % MAX_UNACK_PACKETS) == q->tail ||
        duration > MAX_TIMER_DURATION) {
        return 0;
    }
    if (q->head == q->tail) {
        q->data[q->head].for_packet = seq_n;
        q->data[q->head].val = duration;
    } else {
        nsecs_t left = get_hard_timer_left_time();
        q->data[q->head].for_packet = seq_n;
        q->data[q->head].val = duration - left - q->timers_sum;
        q->timers_sum += q->data[q->head].val;
    }
    nsecs_t r = q->data[q->head].val;
    q->head = (q->head + 1) % MAX_UNACK_PACKETS;
    return r;
}


nsecs_t pop_timer(TimerFifo* const q, uint8_t seq_n) {
    if (q->tail == q->head) {
        return -1;
    }

    if(seq_n == q->data[q->tail].for_packet) {
        if(((q->tail > q->head) && q->tail == MAX_UNACK_PACKETS && q->head == 0) || q->head - q->tail == 1) {
            q->tail = (q->tail + 1) % MAX_UNACK_PACKETS;
            return 0;
        }
        nsecs_t r = get_hard_timer_left_time();
        q->tail = (q->tail + 1) % MAX_UNACK_PACKETS;
        q->timers_sum -= q->data[q->tail].val;
        q->data[q->tail].val += r;
        return r;
    } else {
        uint8_t t_id = q->tail;
        while(q->data[t_id].for_packet != seq_n) t_id = (t_id + 1) % MAX_UNACK_PACKETS;

        nsecs_t r = q->data[t_id].val;
        if(q->tail > q->head) {
            for(uint16_t i = t_id; i < MAX_UNACK_PACKETS - 1; ++i) {
                q->data[i] = q->data[i + 1];
            }
            if(q->head > 0)
                q->data[MAX_UNACK_PACKETS - 1] = q->data[0];
            for(uint16_t i = 0; i < q->head - 1; ++i) {
                q->data[i] = q->data[i + 1];
            }
            q->data[t_id].val += r;
        } else {
            for(uint16_t i = t_id; i < q->head - 1; ++i) {
                q->data[i] = q->data[i + 1];
            }
        }
        if((seq_n + 1 ) % MAX_UNACK_PACKETS) q->timers_sum -= q->data[t_id].val;
        
        if(q->head == 0) q->head = MAX_UNACK_PACKETS - 1;
        else q->head -= 1;
        return 0;
    }
}