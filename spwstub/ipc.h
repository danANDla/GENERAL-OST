#ifndef __IFMO_DISTRIBUTED_CLASS_IPC__H
#define __IFMO_DISTRIBUTED_CLASS_IPC__H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "spw_packet.h"

//------------------------------------------------------------------------------

typedef int8_t local_id;
typedef int16_t timestamp_t;
typedef int32_t pipe_fd;

enum {
    MESSAGE_MAGIC = 0xAFAF,
    MAX_MESSAGE_LEN = 16384,
    PARENT_ID = 0,
    MAX_PROCESS_ID = 15, 
    USECOND_TX_DELAY = 100
};

typedef enum {
    PARENT_CONTROL = 0,
    CONSOLE_CONTROL,
    LINK,
    STOP,
    START,
    NOTHING
} MessageType;

typedef enum {
    IDLE_STATE = 0,
    STARTED_STATE,
    STOPED_STATE
} ProcessState;


typedef struct {
    uint16_t     s_payload_len;  ///< length of payload
    int16_t      s_type;         ///< type of the message
} __attribute__((packed)) MessageHeader;

enum {
    MAX_PAYLOAD_LEN = MAX_MESSAGE_LEN - sizeof(MessageHeader)
};

typedef struct {
    MessageHeader s_header;
    char s_payload[MAX_PAYLOAD_LEN]; ///< Must be used as a buffer, unused "tail"
                                     ///< shouldn't be transfered
} __attribute__((packed)) Message;

extern Message DEFAULT_START_MESSAGE;
extern Message DEFAULT_STOP_MESSAGE;

//------------------------------------------------------------------------------

/** Send multicast message.
 *
 * Send msg to all other processes including parrent.
 * Should stop on the first error.
 * 
 * @param self    Any data structure implemented by students to perform I/O
 * @param msg     Message to multicast.
 *
 * @return 0 on success, any non-zero value on error
 */
int send_multicast(void * self, const Message * msg);

//------------------------------------------------------------------------------

/** Receive a message from any process. If cable (process) is a reciever, it will
 * listen for messages both from parent process and outer node. 
 *
 * Receive a message from any process, in case of blocking I/O should be used
 * with extra care to avoid deadlocks.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param msg     Message structure allocated by the caller
 *
 * @return 0 on success, any non-zero value on error
 */
int receive_any(void * self, Message * msg);

int32_t write_pipe(pipe_fd to, const Message* const msg);
int32_t read_pipe(pipe_fd from, Message* const msg);

//------------------------------------------------------------------------------

typedef struct {
    ProcessState state;
    bool is_rx;
    uint64_t pid;
    uint64_t ppid;
    int64_t last_send;
    pipe_fd from_parent_read;
    pipe_fd to_parent_write;
    pipe_fd outer;
} ChildProcess;

int32_t read_rx_pipe(pipe_fd from, Packet* const packet);
int32_t write_tx_pipe(ChildProcess* const pr, const Packet* const packet);
int32_t is_available_to_write(pipe_fd to, int32_t to_write);
int32_t init_pipe_cap(pipe_fd from);

#endif // __IFMO_DISTRIBUTED_CLASS_IPC__H
