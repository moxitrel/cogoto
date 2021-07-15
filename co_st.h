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
#include "queue_st.inc"

struct co_sch {
    // inherent cogo_sch_t
    cogo_sch_t cogo_sch;
    // coroutine queue run concurrently
    COGO_QUEUE_T(co_t) q;
};

// implement cogo_sch_push()
inline int cogo_sch_push(cogo_sch_t* sch, cogo_co_t* co)
{
    COGO_ASSERT(sch);
    COGO_ASSERT(co);
    COGO_QUEUE_PUSH(co_t)(&((co_sch_t*)sch)->q, (co_t*)co);
    return 1;   // switch context
}

// implement cogo_sch_pop()
inline cogo_co_t* cogo_sch_pop(cogo_sch_t* sch)
{
    COGO_ASSERT(sch);
    return (cogo_co_t*)COGO_QUEUE_POP(co_t)(&((co_sch_t*)sch)->q);
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
#include "queue_st.inc"

typedef struct {
    // all coroutines blocked by this channel
    COGO_QUEUE_T(co_t) cq;
    // message queue
    COGO_QUEUE_T(co_msg_t) mq;
    // current size
    ptrdiff_t sz;
    // max size
    const ptrdiff_t mz;
} co_chan_t;

#define CO_CHAN_MAKE(N)    ((co_chan_t){.mz = (N),})

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
    COGO_ASSERT(chan->sz > PTRDIFF_MIN);
    COGO_ASSERT(msg_next);

    ptrdiff_t chan_size = chan->sz--;
    if (chan_size <= 0) {
        COGO_QUEUE_PUSH(co_msg_t)(&chan->mq, msg_next);
        // sleep in background
        COGO_QUEUE_PUSH(co_t)(&chan->cq, co);       // append to blocking queue
        ((cogo_co_t*)co)->sch->stack_top = NULL;    // remove from scheduler
        return 1;
    } else {
        msg_next->next = COGO_QUEUE_POP(co_msg_t)(&chan->mq);
        // wake up a writer if exists
        if (chan_size >= chan->mz) {
            cogo_co_t* writer = (cogo_co_t*)COGO_QUEUE_POP(co_t)(&chan->cq);
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
    COGO_ASSERT(chan->sz < PTRDIFF_MAX);
    COGO_ASSERT(msg);

    ptrdiff_t chan_size = chan->sz++;
    if (chan_size < 0) {
        COGO_QUEUE_POP(co_msg_t)(&chan->mq)->next = msg;
        // wake up a reader
        cogo_co_t* reader = (cogo_co_t*)COGO_QUEUE_POP(co_t)(&chan->cq);
        return cogo_sch_push(((cogo_co_t*)co)->sch, reader);
    } else {
        COGO_QUEUE_PUSH(co_msg_t)(&chan->mq, msg);
        if (chan_size >= chan->mz) {
            // sleep in background
            COGO_QUEUE_PUSH(co_t)(&chan->cq, co);
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
