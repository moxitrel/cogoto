/*
void       : the element type of queue
COGO_QUEUE_ITEM_NEXT()  : return the next element
*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//#define COGO_QUEUE_T(T)         COGO_QUEUE_T1(T)
//#define COGO_QUEUE_T1(T)        T##_queue_t

//#define COGO_QUEUE_EMPTY(T)     COGO_QUEUE_EMPTY1(T)
//#define COGO_QUEUE_EMPTY1(T)    T##_queue_empty

//#define COGO_QUEUE_POP(T)       COGO_QUEUE_POP1(T)
//#define COGO_QUEUE_POP1(T)      T##_queue_pop

//#define COGO_QUEUE_PUSH(T)      COGO_QUEUE_PUSH1(T)
//#define COGO_QUEUE_PUSH1(T)     T##_queue_push

//#define co_queue_t                COGO_QUEUE_T(COGO_QUEUE_ITEM_T)
//#define co_queue_empty            COGO_QUEUE_EMPTY(void)
//#define co_queue_pop              COGO_QUEUE_POP(void)
//#define co_queue_push             COGO_QUEUE_PUSH(void)

typedef struct {
    void* head;
    void* tail;
} co_queue_t;

#define COGO_Q_NEXT(Q,N)    (*(void**)((intptr_t)(Q) + (N)))

static inline bool co_queue_empty(const co_queue_t* thiz)
{
    return thiz->head == NULL;
}


/* dequeue, return NULL if empty */
static inline void* co_queue_pop(co_queue_t* thiz, ptrdiff_t next)
{
    void* node = thiz->head;
    if (!co_queue_empty(thiz)) {
        thiz->head = COGO_Q_NEXT(thiz->head, next);
    }
    return node;
}


/* enqueue */
static inline void co_queue_push(co_queue_t* thiz, ptrdiff_t next, void* node)
{
    if (co_queue_empty(thiz)) {
        thiz->head = node;
    } else {
        COGO_Q_NEXT(thiz->tail, next) = node;
    }
    thiz->tail = node;
    COGO_Q_NEXT(node, next) = NULL;
}

//#undef co_queue_t
//#undef co_queue_empty
//#undef co_queue_pop
//#undef co_queue_push
