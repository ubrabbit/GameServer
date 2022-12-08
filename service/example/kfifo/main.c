#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include "../../../common/kfifo/kfifo.h"

#define KFIFO_TEST_LOOP_COUNT (100 * 10000)

typedef struct ST_FIFO_UNIT
{
    int x;
    int y;
    int z;
    char buffer[100];
}ST_FIFO_UNIT;

DECLARE_KFIFO(g_fifobuf, ST_FIFO_UNIT, 256);

void* write_thread(void* arg) {
    printf("write...\n");
    int i=0;
    ST_FIFO_UNIT stUnit = {0};
    while(i < KFIFO_TEST_LOOP_COUNT)
    {
        if (kfifo_is_full(&g_fifobuf)) {
            printf("kfifo_is_full, wait.\n");
            usleep(100);
            continue;
        }
        if (kfifo_avail(&g_fifobuf) < 1) {
            printf("kfifo_avail no enough space, wait.\n");
            usleep(100);
            continue;
        }

        i++;
        stUnit.x = i;
        stUnit.y = i+1;
        stUnit.z = i+2;
        sprintf(stUnit.buffer, "str_%d", i);

        kfifo_in(&g_fifobuf, &stUnit, 1);
    }
    printf("write over: %d\n", i);
    return NULL;
}

void* read_thread(void* arg) {
    printf("read...\n");
    int i = 0;
    ST_FIFO_UNIT stUnit = {0};
    while(i < KFIFO_TEST_LOOP_COUNT)
    {
        if (kfifo_is_empty(&g_fifobuf)) {
            usleep(100);
            continue;
        }
        if (kfifo_len(&g_fifobuf) < 1) {
            usleep(100);
            continue;
        }
        kfifo_out(&g_fifobuf, &stUnit, 1);

        i++;
        assert(stUnit.x == i);
        assert(stUnit.y == i+1);
        assert(stUnit.z == i+2);
        char buffer[100] = {0};
        sprintf(buffer, "str_%d", i);
        assert(strcmp(buffer, stUnit.buffer) == 0);
    }
    printf("read over: %d\n", stUnit.x);
    return NULL;
}

int main() {
    INIT_KFIFO(g_fifobuf);
    int n = kfifo_avail(&g_fifobuf);
    printf("hello kfifo, avail: %d\n", n);

    pthread_t r, w;
    pthread_create(&w, NULL, write_thread, NULL);
    printf("..............\n");
    pthread_create(&r, NULL, read_thread, NULL);
    pthread_join(r, NULL);
    pthread_join(w, NULL);

    printf("test success\n");
    return 0;
}
