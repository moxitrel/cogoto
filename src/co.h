/*

* API
- co_start     (co_t *, co_t *)             :: run a new coroutine concurrently.
- co_chan_write(co_t *, chanptr_t, void *)  :: send a message to channel.
- co_chan_read (co_t *, chanptr_t, void **) :: receive a message from channel.

- CO(void(*)(co_t *)) -> co_t   :: co_t constructor.
- co_run(co_t *)                :: run until all coroutine finished.

*/
#ifndef COGO_CO_H
#define COGO_CO_H

#include "await.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct co_t       co_t;
typedef struct co_queue_t co_queue_t;
typedef struct co_sch_t   co_sch_t;

// co_t: support concurrency
struct co_t {
    // inherit await_t
    await_t await_t;

    // build coroutine queue (run concurrently)
    co_t *next;

    // cache channel message
    // when write blocked, the stored value type is void *
    // when read  blocked, the stored value type is void **
    void *chan_msg;
};

// co_t queue
struct co_queue_t {
    co_t *head;
    co_t *tail;
};

// co_t scheduler
struct co_sch_t {
    // inherent await_sch_t
    await_sch_t await_sch_t;

    // all coroutines that run concurrently
    co_queue_t q;
};


//
// co_queue_t
//
inline static bool co_queue_empty(const co_queue_t *q)
{
    assert(q);
    return q->head == NULL;
}

// dequeue
inline static co_t *co_queue_pop(co_queue_t *q)
{
    assert(q);

    co_t *node = q->head;
    if (q->head != NULL) {
        q->head = q->head->next;
    }
    return node;
}

// enqueue
inline static void co_queue_push(co_queue_t *q, co_t *co)
{
    assert(q);
    assert(co);

    if (co_queue_empty(q)) {
        q->head = co;
    } else {
        q->tail->next = co;
    }
    q->tail = co;
    q->tail->next = NULL;
}

inline static void co_queue_merge(co_queue_t *q, const co_queue_t *q1)
{
    assert(q);
    assert(q1);

    if (co_queue_empty(q1)) {
        return;
    }
    if (co_queue_empty(q)) {
        *q = *q1;
        return;
    }
    q->tail->next = q1->head;
    q->tail = q1->tail;
}


//
// co_t
//

// co_t CO(void(*)(co_t *)): co_t constructor.
#define CO(FUN) ((co_t){                                    \
    /* await_t.sch inited by co_start() or await_run()*/    \
    .await_t = AWAIT(FUN),                                  \
})

// add co2 to the coroutine queue
inline static co_t *co__start(co_t *co, co_t *co1)
{
    assert(co);
    assert(co1);

    co1->await_t.sch = co->await_t.sch;
    co_queue_push(&((co_sch_t *)co->await_t.sch)->q, co1);
    return co;
}

// co_start(co_t *, co_t *): add a new coroutine to the scheduler.
#define co_start(CO, CO2)  co_yield(co__start((co_t *)(CO), (co_t *)(CO2)))

// run co as the entry coroutine, until all finished.
// FIXME: scheduler stop successfully while expected blocking forever by channel.
static void co_run(void *co)
{
    assert(co);

    // associate coroutine with a scheduler
    co_sch_t sch = {
        .await_sch_t = {.stack_top = (await_t *)co},
    };
    ((await_t *)co)->sch = &sch.await_sch_t;

    for (; sch.await_sch_t.stack_top; sch.await_sch_t.stack_top = (await_t *)co_queue_pop(&sch.q)) {
        sch_step(&sch.await_sch_t);
        if (sch.await_sch_t.stack_top) {
            co_queue_push(&sch.q, (co_t *)sch.await_sch_t.stack_top);
        }
    }
}


//
// channel
//

#define chan_t(N)                                                   \
struct {                                                            \
    /* rq, wq: all coroutines blocked by this channel */            \
    co_queue_t rq;  /* blocked coroutines when read   */            \
    co_queue_t wq;  /* blocked coroutines when write  */            \
                                                                    \
    /* message buffer queue */                                      \
    size_t msg_head;                                                \
    size_t msg_end; /* just behind msg_tail */                      \
    size_t cap;     /* the max number of messages can be buffered */\
    void *msg[N];                                                   \
}

typedef chan_t(1)   *chanptr_t;
#define CHANPTR(N)  ((chanptr_t)&(chan_t(N)){.cap = (N)})
#define CHAN_INIT(P,N)  (*(chanptr_t)(P) = *(chanptr_t)&(chan_t(1)){.cap = (N)})

// co_chan_write(co_t *, chanptr_t, void *);
#define co_chan_write(CO,CHAN,MSG)  co_yield(chan__write((co_t *)(CO), (chanptr_t)(CHAN), (MSG)))

// co_chan_read(co_t *, chanptr_t, void **);
#define co_chan_read(CO,CHAN,MSG)   co_yield(chan__read((co_t *)(CO), (chanptr_t)(CHAN), (MSG)))


inline static size_t chan_len(chanptr_t chan)
{
    assert(chan);
    return chan->msg_end - chan->msg_head;
}

// enqueue
inline static void chan__push(chanptr_t chan, void *msg)
{
    chan->msg[chan->msg_end++] = msg;
    if (chan->msg_end >= chan->cap) {
        chan->msg_end = 0;
    }
}

// dequeue
inline static void chan__pop(chanptr_t chan, void **msg_ptr)
{
    *msg_ptr = chan->msg[chan->msg_head++];
    if (chan->msg_head >= chan->cap) {
        chan->msg_head = 0;
    }
}

inline static co_t *chan__read(co_t *co, chanptr_t chan, void **msg_ptr)
{
    assert(co);
    assert(chan);
    assert(msg_ptr);

    co_t *writer;   // coroutine write to channel blocked

    if (chan_len(chan) > 0) {
        chan__pop(chan, msg_ptr);
        if ((writer = co_queue_pop(&chan->wq)) != NULL) {
            // push cached message
            chan__push(chan, writer->chan_msg);
            // wake up the writer
            co_queue_push(&((co_sch_t *)co->await_t.sch)->q, writer);
        }
    } else if ((writer = co_queue_pop(&chan->wq)) != NULL) {
        // read cached message from writer
        *msg_ptr = writer->chan_msg;
        // wake up the writer
        co_queue_push(&((co_sch_t *)co->await_t.sch)->q, writer);
    } else {
        // save message address for writing
        co->chan_msg = msg_ptr;             // NOTE: the stored value type is void**
        // sleep in background
        co_queue_push(&chan->rq, co);       // put into blocking queue
        co->await_t.sch->stack_top = NULL;  // remove from scheduler
    }

    return co;
}

inline static co_t *chan__write(co_t *co, chanptr_t chan, void *msg)
{
    assert(co);
    assert(chan);

    co_t *reader;   // coroutine read from channel blocked

    // 1. feed blocked reader first
    // 2. feed message buffer
    // 3. cache message in coroutine
    if ((reader = co_queue_pop(&chan->rq)) != NULL) {
        // send message to reader, the stored value type of chan_msg should be void**
        *(void **)reader->chan_msg = msg;
        // wake up the reader
        co_queue_push(&((co_sch_t *)co->await_t.sch)->q, reader);
    } else if (chan->cap > chan_len(chan)) {
        chan__push(chan, msg);
    } else {
        // cache message for reading
        co->chan_msg = msg;
        // sleep in background
        co_queue_push(&chan->wq, co);
        co->await_t.sch->stack_top = NULL;
    }

    return co;
}

#endif //COGO_CO_H
