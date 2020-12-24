/*
COGO_QUEUE_ITEM_T       : the element type of queue
COGO_QUEUE_ITEM_NEXT()  : return the next element

*/
#include <stdbool.h>
#include <stddef.h>

#define COGO_QUEUE_T(T)         COGO_QUEUE_T1(T)
#define COGO_QUEUE_T1(T)        T##_queue_t

#define COGO_QUEUE_EMPTY(T)     COGO_QUEUE_EMPTY1(T)
#define COGO_QUEUE_EMPTY1(T)    T##_queue_empty

#define COGO_QUEUE_POP(T)       COGO_QUEUE_POP1(T)
#define COGO_QUEUE_POP1(T)      T##_queue_pop

#define COGO_QUEUE_PUSH(T)      COGO_QUEUE_PUSH1(T)
#define COGO_QUEUE_PUSH1(T)     T##_queue_push

#define COGO_Q_T                COGO_QUEUE_T(COGO_QUEUE_ITEM_T)
#define COGO_Q_EMPTY            COGO_QUEUE_EMPTY(COGO_QUEUE_ITEM_T)
#define COGO_Q_POP              COGO_QUEUE_POP(COGO_QUEUE_ITEM_T)
#define COGO_Q_PUSH             COGO_QUEUE_PUSH(COGO_QUEUE_ITEM_T)

typedef struct {
    COGO_QUEUE_ITEM_T* head;
    COGO_QUEUE_ITEM_T* tail;
} COGO_Q_T;


COGO_INLINE bool COGO_Q_EMPTY(const COGO_Q_T* thiz)
{
    return thiz->head == NULL;
}


/* dequeue, return NULL if empty */
COGO_INLINE COGO_QUEUE_ITEM_T* COGO_Q_POP(COGO_Q_T* thiz)
{
    COGO_QUEUE_ITEM_T* node = thiz->head;
    if (!COGO_Q_EMPTY(thiz)) {
        thiz->head = COGO_QUEUE_ITEM_NEXT(thiz->head);
    }
    return node;
}


/* enqueue */
COGO_INLINE void COGO_Q_PUSH(COGO_Q_T* thiz, COGO_QUEUE_ITEM_T* node)
{
    if (COGO_Q_EMPTY(thiz)) {
        thiz->head = node;
    } else {
        COGO_QUEUE_ITEM_NEXT(thiz->tail) = node;
    }
    thiz->tail = node;
    COGO_QUEUE_ITEM_NEXT(node) = NULL;
}

#undef COGO_Q_T
#undef COGO_Q_EMPTY
#undef COGO_Q_POP
#undef COGO_Q_PUSH
