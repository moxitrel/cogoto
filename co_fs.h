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
co_step_begin   (co_t*)                 : return co_step_t
co_step         (co_step_t*)            : run the current coroutine until yield or finished

co_msg_t                                : channel message type
co_chan_t                               : channel type
CO_CHAN_MAKE (size_t)                   : return a channel with capacity size_t
CO_CHAN_WRITE(co_chan_t*, co_msg_t*)    : send a message to channel
CO_CHAN_READ (co_chan_t*, co_msg_t*)    : receive a message from channel, the result stored in co_msg_t.next

*/
#ifndef MOXITREL_COGO_CO_IMPL_H_
#define MOXITREL_COGO_CO_IMPL_H_

#include "co.h"
#include <stddef.h>

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
// COGO_QUEUE_PUSH (co_t) (      COGO_QUEUE_T(co_t)*, co_t*)
//
#undef  COGO_QUEUE_ITEM_T
#undef  COGO_QUEUE_ITEM_NEXT
#define COGO_QUEUE_ITEM_T           co_t
#define COGO_QUEUE_ITEM_NEXT(P)     ((P)->next)
#include "queue_fs.inc"

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
    if (co != NULL) {
        COGO_QUEUE_PUSH(co_t)(&((co_sch_t*)sch)->q, (co_t*)co);
    }
    return 1;
}

// implement cogo_sch_pop()
COGO_INLINE cogo_co_t* cogo_sch_pop(cogo_sch_t* sch)
{
    COGO_ASSERT(sch);
    return (cogo_co_t*)COGO_QUEUE_POP(co_t)(&((co_sch_t*)sch)->q);
}

COGO_INLINE void co_run(void* co)
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

//
// COGO_QUEUE_T    (co_msg_t)
// COGO_QUEUE_EMPTY(co_msg_t) (const COGO_QUEUE_T(co_msg_t)*)
// COGO_QUEUE_POP  (co_msg_t) (      COGO_QUEUE_T(co_msg_t)*)
// COGO_QUEUE_PUSH (co_msg_t) (      COGO_QUEUE_T(co_msg_t)*, co_msg_t*)
//
#undef  COGO_QUEUE_ITEM_T
#undef  COGO_QUEUE_ITEM_NEXT
#define COGO_QUEUE_ITEM_T           co_msg_t
#define COGO_QUEUE_ITEM_NEXT(P)     ((P)->next)
#include "queue_fs.inc"

typedef struct {
    // rq, wq: all coroutines blocked by this channel
    COGO_QUEUE_T(co_t) rq;
    COGO_QUEUE_T(co_t) wq;

    // message queue
    COGO_QUEUE_T(co_msg_t) mq;
    ptrdiff_t size;
    ptrdiff_t cap;
} co_chan_t;

#define CO_CHAN_MAKE(N)    ((co_chan_t){.cap = (N),})

// CO_CHAN_READ(co_chan_t*, co_msg_t*);
#define CO_CHAN_READ(CHAN, MSG_NEXT)                                                \
do {                                                                                \
    if (cogo_chan_read((co_t*)(CO_THIS), (CHAN), (MSG_NEXT)) != 0) {                \
        CO_YIELD;                                                                   \
    }                                                                               \
} while (0)
COGO_INLINE int cogo_chan_read(co_t* co, co_chan_t* chan, co_msg_t* msg_next)
{
    COGO_ASSERT(co);
    COGO_ASSERT(chan);
    COGO_ASSERT(chan->size > PTRDIFF_MIN);
    COGO_ASSERT(msg_next);

    if (chan->size <= 0) {
        // sleep in background
        COGO_QUEUE_PUSH(co_t)(&chan->rq, co);       // append to blocking queue
        ((cogo_co_t*)co)->sch->stack_top = NULL;    // remove from scheduler

        COGO_QUEUE_PUSH(co_msg_t)(&chan->mq, msg_next);
    } else {
        msg_next->next = COGO_QUEUE_POP(co_msg_t)(&chan->mq);
    }
    chan->size--;
    // wake up a writer if exists
    return cogo_sch_push(((cogo_co_t*)co)->sch, (cogo_co_t*)COGO_QUEUE_POP(co_t)(&chan->wq))
        || ((cogo_co_t*)co)->sch->stack_top == NULL;
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
    COGO_ASSERT(chan->size < PTRDIFF_MAX);
    COGO_ASSERT(msg);

    if (chan->size < 0) {
        COGO_QUEUE_POP(co_msg_t)(&chan->mq)->next = msg;
    } else {
        if (chan->size >= chan->cap) {
            // sleep in background
            COGO_QUEUE_PUSH(co_t)(&chan->wq, co);
            ((cogo_co_t*)co)->sch->stack_top = NULL;
        }
        COGO_QUEUE_PUSH(co_msg_t)(&chan->mq, msg);
    }
    chan->size++;
    // wake up a reader if exists
    return cogo_sch_push(((cogo_co_t*)co)->sch, (cogo_co_t*)COGO_QUEUE_POP(co_t)(&chan->rq))
        || ((cogo_co_t*)co)->sch->stack_top == NULL;
}

#undef CO_DECLARE
#define CO_DECLARE(NAME, ...)                                   \
    COGO_IFNIL(__VA_ARGS__)(                                    \
            COGO_DECLARE(NAME, co_t co),                        \
            COGO_DECLARE(NAME, co_t co, __VA_ARGS__)            \
        )


#undef CO_MAKE
#define CO_MAKE(NAME, ...)                                      \
    ((NAME){                                                    \
        {{.func = (void(*)(cogo_co_t*))(NAME##_func)}},         \
        __VA_ARGS__                                             \
    })


typedef co_sch_t co_step_t;

COGO_INLINE co_step_t co_step_begin(void* co)
{
    return (co_sch_t){
        .cogo_sch = {
            .stack_top = (cogo_co_t*)co,
        },
    };
}

COGO_INLINE co_t* co_step(co_step_t* thiz)
{
    COGO_ASSERT(thiz);
    return (co_t*)cogo_sch_step((cogo_sch_t*)thiz);
}

#endif  // MOXITREL_COGO_CO_IMPL_H_
