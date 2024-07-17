#include "data_fifo.h"

inline void data_fifo_move_head(DataFifo* const fifo) {
    fifo->head = (fifo->head + 1)  % (DATA_FIFO_MAX_SZ);
}

inline void data_fifo_move_tail(DataFifo* const fifo) {
    fifo->tail = (fifo->tail + 1)  % (DATA_FIFO_MAX_SZ);
}

int8_t data_fifo_has_space(const DataFifo* const fifo){
    return (fifo->head + 1) % (DATA_FIFO_MAX_SZ) != fifo->tail;
}

int8_t data_fifo_is_empty(const DataFifo* const fifo) {
    return fifo->head == fifo->tail;
}

int8_t data_fifo_enqueue(DataFifo* const fifo, const OstSegment* const seg) {
    if(!data_fifo_has_space(fifo)) return -1;
    fifo->data[fifo->head]->header = seg->header;
    if(seg->header.payload_length > 0){
        fifo->data[fifo->head]->payload = (uint8_t *) malloc(seg->header.payload_length);
        memcpy(fifo->data[fifo->head]->payload, seg->payload, seg->header.payload_length);
    }
    data_fifo_move_head(fifo);
    return 1;
}

int8_t data_fifo_dequeue(DataFifo* const fifo) {
    if(data_fifo_is_empty(fifo)) return -1;
    free(fifo->data[fifo->tail]->payload);
    data_fifo_move_tail(fifo);
    return 1;
}

int8_t data_fifo_peek(const DataFifo* const fifo, OstSegment** const seg) {
    if(data_fifo_is_empty(fifo)) return -1;
    *seg = fifo->data[fifo->tail];
    return 1;
}
