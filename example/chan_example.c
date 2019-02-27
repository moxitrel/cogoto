#include <assert.h>
#include "../src/co.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>

//
// send a signal to <reader> when writer finished
//
typedef struct {
    co_t      co_t;
    chanptr_t chan;
    uintmax_t i;
} writer_t;

void writer(writer_t *co)
{
    co_begin(co);

    for (; co->i > 0; co->i--) {
        co_yield(co);
    }
    // send finish signal
    co_chan_write(co, co->chan, co);

    co_end(co);
}

#define WRITER(C,N) ((writer_t){    \
    .co_t = CO(writer),             \
    .chan = (C),                    \
    .i    = (N),                    \
})


//
// wait <writer>s finished
//
typedef struct {
    co_t      co_t;
    chanptr_t chan;
    writer_t  writers[3];
    void     *msg;
} reader_t;

void reader(reader_t *co)
{
    co_begin(co);

    // create 3 new coroutines
    co_start(co, &co->writers[0]);
    co_start(co, &co->writers[1]);
    co_start(co, &co->writers[2]);

    // wait finish
    co_chan_read(co, co->chan, &co->msg);
    printf("%p is finished at %s", co->msg, ctime(&(time_t){time(NULL)}));
    co_chan_read(co, co->chan, &co->msg);
    printf("%p is finished at %s", co->msg, ctime(&(time_t){time(NULL)}));
    co_chan_read(co, co->chan, &co->msg);
    printf("%p is finished at %s", co->msg, ctime(&(time_t){time(NULL)}));

    co_end(co);
}

#define READER(C) ((reader_t){          \
    .co_t = CO(reader),                 \
    .chan = (C),                        \
    .writers = {                        \
        WRITER(C, (uintmax_t)1 << 25),  \
        WRITER(C, (uintmax_t)1 << 26),  \
        WRITER(C, (uintmax_t)1 << 27),  \
    },                                  \
})

int main(void)
{
    chanptr_t c = CHANPTR(0);
    co_run(&READER(c));
}