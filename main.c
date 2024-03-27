#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "spwstub/ipc.h"
#include "spwstub/spw_interface.h"

int8_t get_id(int argc, char** argv) {
    if(argc < 3) {
        return -1;
        fprintf(stderr, "bad format of args\n");
    }
    int8_t id;
    int opt;
    while((opt = getopt(argc, argv, "i:")) != -1){
        if(opt == 'i'){
            id = atoi(optarg);
        }
    }
    return id;
}

void enter_spw(pipe_fd* tx, pipe_fd* rx, int8_t id) {
    if(id == 1) {
        char* to_read = "/tmp/spw_2_to_1";
        mkfifo(to_read, 0666);
        *rx = open(to_read, O_RDONLY | O_NONBLOCK);
        for(size_t i = 0; i < 10e6; ++i) {
            uint8_t anything;
            int32_t r = read(*rx, &anything, 1);
            if(r != 1) continue;
            char* write = "/tmp/spw_1_to_2";
            mkfifo(write, 0666);
            *tx = open(write,  O_WRONLY | O_NONBLOCK);
            break;
        }
    } else {
        char* read = "/tmp/spw_1_to_2";
        mkfifo(read, 0666);
        *rx = open(read, O_RDONLY | O_NONBLOCK);
        char* to_write = "/tmp/spw_2_to_1";
        mkfifo(to_write, 0666);
        *tx = open(to_write,O_WRONLY | O_NONBLOCK);
        int8_t anything = 7;
        write(*tx, &anything, 1); 
    }
    init_pipe_cap(*rx);
}

int main(int argc, char** argv) {
    int8_t id = get_id(argc, argv);
    if(id == -1) return 1;

    SpWInterface interface = {
        .state = OFF,
        .auto_start = false,
    };
    printf("%d \n", id);

    pipe_fd tx = -1, rx = -1;
    enter_spw(&tx, &rx, id);
    printf("tx=%2d rx=%2d\n", tx, rx);
    powerup_link(&interface, tx, rx);
    start_link(&interface);
}