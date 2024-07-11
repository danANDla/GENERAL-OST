#ifndef DATA_FIFO_H
#define DATA_FIFO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include "ost_segment.h"

#define DATA_FIFO_MAX_SZ 10

typedef struct DataFifo{
    OstSegment* data[DATA_FIFO_MAX_SZ];
    uint8_t head;
    uint8_t tail;
} DataFifo;

int8_t data_fifo_enqueue(DataFifo* const fifo, const OstSegment* const seg);
int8_t data_fifo_dequeue(DataFifo* const fifo);
int8_t data_fifo_empty(DataFifo* const fifo);
int8_t data_fifo_peek(DataFifo* const fifo, OstSegment** const seg);

#ifdef __cplusplus
}
#endif

#endif // DATA_FIFO_H
