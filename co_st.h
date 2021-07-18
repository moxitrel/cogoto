/*

* API
CO_BEGIN                : ...
CO_END                  : ...
CO_YIELD                : ...
CO_RETURN               : ...
CO_THIS                 : ...
CO_STATE    (CO)        : ...
CO_DECLARE  (NAME, ...) : ...
CO_DEFINE   (NAME)      : ...
CO_AWAIT    (cogo_co_t*): ...
CO_START    (cogo_co_t*): ...

co_t                                    : coroutine type to be inherited
co_run          (co_t*)                 : run the coroutine until all finished

co_msg_t                                : channel message type
co_chan_t                               : channel type
CO_CHAN_MAKE (size_t)                   : return a channel with capacity size_t
CO_CHAN_WRITE(co_chan_t*, co_msg_t*)    : send a message to channel
CO_CHAN_READ (co_chan_t*, co_msg_t*)    : receive a message from channel, the result stored in co_msg_t.next

*/
#ifndef MOXITREL_COGO_CO_IMPL_H_
#define MOXITREL_COGO_CO_IMPL_H_

#include "co.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void* head;
    void* tail;
} co_queue_t;

#define CO_QUEUE_NEXT(Q,N)    (*(void**)((intptr_t)(Q) + (N)))

static inline bool co_queue_empty(const co_queue_t* thiz)
{
    return thiz->head == NULL;
}

/* dequeue */
static inline void* co_queue_pop(co_queue_t* thiz, ptrdiff_t next)
{
    void* node = thiz->head;
    if (!co_queue_empty(thiz)) {
        thiz->head = CO_QUEUE_NEXT(thiz->head, next);
    }
    return node;
}

/* enqueue */
static inline void co_queue_push(co_queue_t* thiz, ptrdiff_t next, void* node)
{
    if (co_queue_empty(thiz)) {
        thiz->head = node;
    } else {
        CO_QUEUE_NEXT(thiz->tail, next) = node;
    }
    thiz->tail = node;
    CO_QUEUE_NEXT(node, next) = NULL;
}

typedef struct co       co_t;
typedef struct co_sch   co_sch_t;
typedef struct co_msg   co_msg_t;

struct co {
    // inherit cogo_co_t
    cogo_co_t cogo_co;

    // build coroutine queue
    co_t* next;
};

struct co_sch {
    // inherent cogo_sch_t
    cogo_sch_t cogo_sch;
    // coroutine queue run concurrently
    co_queue_t q;
};

// implement cogo_sch_push()
inline int cogo_sch_push(cogo_sch_t* sch, cogo_co_t* co)
{
    COGO_ASSERT(sch);
    COGO_ASSERT(co);
    co_queue_push(&((co_sch_t*)sch)->q, offsetof(co_t, next), (co_t*)co);
    return 1;   // switch context
}

// implement cogo_sch_pop()
inline cogo_co_t* cogo_sch_pop(cogo_sch_t* sch)
{
    COGO_ASSERT(sch);
    return (cogo_co_t*)co_queue_pop(&((co_sch_t*)sch)->q, offsetof(co_t, next));
}

static inline void co_run(void* co)
{
    co_sch_t sch = {
        .cogo_sch = {
            .stack_top = (cogo_co_t*)co,
        },
    };
    while (cogo_sch_step((cogo_sch_t*)&sch))
    {}
}

// channel message
struct co_msg {
    co_msg_t* next;
};

typedef struct {
    // all coroutines blocked by this channel
    co_queue_t cq;
    // message queue
    co_queue_t mq;
    // current size
    ptrdiff_t size;
    // max size
    const ptrdiff_t cap;
} co_chan_t;

#define CO_CHAN_MAKE(N)    ((co_chan_t){.cap = (N),})

// CO_CHAN_READ(co_chan_t*, co_msg_t*);
// MSG_NEXT: the read message sit in MSG_NEXT->next
#define CO_CHAN_READ(CHAN, MSG_NEXT)                                                \
do {                                                                                \
    if (cogo_chan_read((co_t*)(CO_THIS), (CHAN), (MSG_NEXT)) != 0) {                \
        CO_YIELD;                                                                   \
    }                                                                               \
} while (0)
inline int cogo_chan_read(co_t* co, co_chan_t* chan, co_msg_t* msg_next)
{
  //COGO_ASSERT(co);
    COGO_ASSERT(chan);
    COGO_ASSERT(chan->size > PTRDIFF_MIN);
    COGO_ASSERT(msg_next);

    ptrdiff_t chan_size = chan->size--;
    if (chan_size <= 0) {
        co_queue_push(&chan->mq, offsetof(co_msg_t, next), msg_next);
        // sleep in background
        co_queue_push(&chan->cq, offsetof(co_t, next), co);     // append to blocking queue
        ((cogo_co_t*)co)->sch->stack_top = NULL;                // remove from scheduler
        return 1;
    } else {
        msg_next->next = (co_msg_t*)co_queue_pop(&chan->mq, offsetof(co_msg_t, next));
        // wake up a writer if exists
        if (chan_size >= chan->cap) {
            cogo_co_t* writer = (cogo_co_t*)co_queue_pop(&chan->cq, offsetof(co_t, next));
            return cogo_sch_push(((cogo_co_t*)co)->sch, writer);
        }
        return 0;
    }
}

// CO_CHAN_WRITE(co_chan_t*, co_msg_t*);
#define CO_CHAN_WRITE(CHAN, MSG)                                                                \
do {                                                                                            \
    if (cogo_chan_write((co_t*)(CO_THIS), (CHAN), (co_msg_t*)(MSG)) != 0) {                     \
        CO_YIELD;                                                                               \
    }                                                                                           \
} while (0)
inline int cogo_chan_write(co_t* co, co_chan_t* chan, co_msg_t* msg)
{
  //COGO_ASSERT(co);
    COGO_ASSERT(chan);
    COGO_ASSERT(chan->size < PTRDIFF_MAX);
    COGO_ASSERT(msg);

    ptrdiff_t chan_size = chan->size++;
    if (chan_size < 0) {
        ((co_msg_t*)co_queue_pop(&chan->mq, offsetof(co_msg_t, next)))->next = msg;
        // wake up a reader
        cogo_co_t* reader = (cogo_co_t*)co_queue_pop(&chan->cq, offsetof(co_t, next));
        return cogo_sch_push(((cogo_co_t*)co)->sch, reader);
    } else {
        co_queue_push(&chan->mq, offsetof(co_msg_t, next), msg);
        if (chan_size >= chan->cap) {
            // sleep in background
            co_queue_push(&chan->cq, offsetof(co_t, next), co);
            ((cogo_co_t*)co)->sch->stack_top = NULL;
            return 1;
        }
        return 0;
    }
}

#undef CO_DECLARE
#define CO_DECLARE(NAME, ...)                           \
    COGO_DECLARE(NAME, co_t co, __VA_ARGS__)

#undef CO_MAKE
#define CO_MAKE(NAME, ...)                              \
    ((NAME){                                            \
        .co = {.cogo_co = {.func = NAME##_func}},       \
        __VA_ARGS__                                     \
    })

#endif  // MOXITREL_COGO_CO_IMPL_H_
