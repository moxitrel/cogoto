/*

* API
CO_BEGIN                : ...
CO_END                  : ...
CO_YIELD                : ...
CO_RETURN               : ...
CO_THIS                 : ...
CO_STATE  (GEN)         : ...
CO_DECLARE(NAME, ...)   : ...
CO_DEFINE (NAME)        : ...
CO_AWAIT(cogo_co_t*)    : ...
CO_START(cogo_co_t*)    : ...

co_t                                    : coroutine type to be inherented
co_run(co_t*)                           : run the coroutine until all finished

co_msg_t                                : channel message type
co_chan_t                               : channel type
CO_CHAN_MAKE (size_t)                   : return a channel with capacity size_t
CO_CHAN_WRITE(co_chan_t*, co_msg_t* )   : send a message to channel
CO_CHAN_READ (co_chan_t*, co_msg_t**)   : receive a message from channel

*/
#ifndef MOXITREL_COGO_CO_IMPL_H_
#define MOXITREL_COGO_CO_IMPL_H_

#include "co_intf.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct co       co_t;
typedef struct co_sch   co_sch_t;
typedef struct co_msg   co_msg_t;

struct co {
    // inherit cogo_co_t
    cogo_co_t cogo_co;

    // build coroutine queue
    co_t* next;
};

//
// COGO_QUEUE_T    (co_t)
// COGO_QUEUE_EMPTY(co_t) (const COGO_QUEUE_T(co_t)*)
// COGO_QUEUE_POP  (co_t) (      COGO_QUEUE_T(co_t)*)
// COGO_QUEUE_PUSH (co_t) (      COGO_QUEUE_T(co_t)*, T*)
//
#undef  COGO_QUEUE_ITEM_T
#undef  COGO_QUEUE_ITEM_NEXT
#define COGO_QUEUE_ITEM_T           co_t
#define COGO_QUEUE_ITEM_NEXT(P)     ((P)->next)
#include "queue_template.inc"

struct co_sch {
    // inherent cogo_sch_t
    cogo_sch_t cogo_sch;

    // coroutine queue run concurrently
    COGO_QUEUE_T(co_t) q;
};

// implement cogo_sch_push()
COGO_INLINE int cogo_sch_push(cogo_sch_t* sch, cogo_co_t* co)
{
    COGO_ASSERT(sch);
    if (co) {
        COGO_QUEUE_PUSH(co_t)(&((co_sch_t*)sch)->q, (co_t*)co);
    }
    return 0;
}

// implement cogo_sch_pop()
COGO_INLINE cogo_co_t* cogo_sch_pop(cogo_sch_t* sch)
{
    COGO_ASSERT(sch);
    return (cogo_co_t*)COGO_QUEUE_POP(co_t)(&((co_sch_t*)sch)->q);
}

COGO_INLINE void co_run(co_t* main)
{
    co_sch_t sch = {
        .cogo_sch = {.stack_top = (cogo_co_t*)main},
    };
    while (cogo_sch_step((cogo_sch_t*)&sch))
    {}
}

// channel message
struct co_msg {
    co_msg_t* next;
};

//
// COGO_QUEUE_T    (co_msg_t)
// COGO_QUEUE_EMPTY(co_msg_t) (const COGO_QUEUE_T(co_msg_t)*)
// COGO_QUEUE_POP  (co_msg_t) (      COGO_QUEUE_T(co_msg_t)*)
// COGO_QUEUE_PUSH (co_msg_t) (      COGO_QUEUE_T(co_msg_t)*, T*)
//
#undef  COGO_QUEUE_ITEM_T
#undef  COGO_QUEUE_ITEM_NEXT
#define COGO_QUEUE_ITEM_T           co_msg_t
#define COGO_QUEUE_ITEM_NEXT(P)     ((P)->next)
#include "queue_template.inc"

typedef struct {
    // rq, wq: all coroutines blocked by this channel
    COGO_QUEUE_T(co_t) rq;
    COGO_QUEUE_T(co_t) wq;

    // message queue
    COGO_QUEUE_T(co_msg_t) mq;
    size_t size;
    size_t cap;
} co_chan_t;

#define CO_CHAN_MAKE(N)    ((co_chan_t){.cap = (N),})

COGO_INLINE bool cogo_chan_empty(const co_chan_t* thiz)
{
    COGO_ASSERT(thiz);
    return thiz->size == 0;
}

COGO_INLINE bool cogo_chan_full(const co_chan_t* thiz)
{
    COGO_ASSERT(thiz);
    return thiz->size >= thiz->cap;
}

// enqueue
COGO_INLINE void cogo_chan_push(co_chan_t* chan, co_msg_t* msg)
{
    COGO_ASSERT(chan);
    COGO_ASSERT(msg);
    COGO_QUEUE_PUSH(co_msg_t)(&chan->mq, msg);
    chan->size++;
}

// dequeue
COGO_INLINE co_msg_t* cogo_chan_pop(co_chan_t* chan)
{
    COGO_ASSERT(chan);
    if (chan->size > 0) {
        chan->size--;
    }
    return COGO_QUEUE_POP(co_msg_t)(&chan->mq);
}

// CO_CHAN_READ(co_chan_t*, const co_msg_t**);
#define CO_CHAN_READ(CHAN, MSG_PTR)                                                             \
do {                                                                                            \
    while (cogo_chan_read((co_t*)(CO_THIS), (CHAN), (const co_msg_t**)(MSG_PTR)) >= 2) {        \
        CO_YIELD;                                                                               \
    }                                                                                           \
} while (0)
COGO_INLINE unsigned cogo_chan_read(co_t* co, co_chan_t* chan, const co_msg_t** msg_ptr)
{
    COGO_ASSERT(co);
    COGO_ASSERT(chan);
    COGO_ASSERT(msg_ptr);

    if (cogo_chan_empty(chan)) {
        // sleep in background
        COGO_QUEUE_PUSH(co_t)(&chan->rq, co);       // append to blocking queue
        ((cogo_co_t*)co)->sch->stack_top = NULL;    // remove from scheduler
        return 2;
    } else {
        *msg_ptr = cogo_chan_pop(chan);
        COGO_ASSERT(*msg_ptr);
        // wake up a writer if exists
        return cogo_sch_push(((cogo_co_t*)co)->sch, (cogo_co_t*)COGO_QUEUE_POP(co_t)(&chan->wq))
            ? 1
            : 0;
    }
}

// CO_CHAN_WRITE(co_chan_t*, co_msg_t*);
#define CO_CHAN_WRITE(CHAN, MSG)                                                                \
do {                                                                                            \
    if (cogo_chan_write((co_t*)(CO_THIS), (CHAN), (co_msg_t*)(MSG)) != 0) {                     \
        CO_YIELD;                                                                               \
    }                                                                                           \
} while (0)
COGO_INLINE int cogo_chan_write(co_t* co, co_chan_t* chan, co_msg_t* msg)
{
    COGO_ASSERT(co);
    COGO_ASSERT(chan);
    COGO_ASSERT(msg);

    if (cogo_chan_full(chan)) {
        // sleep in background
        COGO_QUEUE_PUSH(co_t)(&chan->wq, co);
        ((cogo_co_t*)co)->sch->stack_top = NULL;
    }
    cogo_chan_push(chan, msg);
    // wake up a reader if exists
    return cogo_sch_push(((cogo_co_t*)co)->sch, (cogo_co_t*)COGO_QUEUE_POP(co_t)(&chan->rq))
        || ((cogo_co_t*)co)->sch->stack_top == NULL;
}

#undef CO_DECLARE
#define CO_DECLARE(NAME, ...)                                       \
    IFNIL(__VA_ARGS__)(                                             \
            COGO_DECLARE(NAME, co_t co),                            \
            COGO_DECLARE(NAME, co_t co, __VA_ARGS__)                \
        )


#undef CO_MAKE
#define CO_MAKE(NAME, ...)                                      \
    ((NAME){                                                    \
        {{.func = (void(*)(void*))(NAME##_func)}},              \
        __VA_ARGS__                                             \
    })

#endif  // MOXITREL_COGO_CO_IMPL_H_
