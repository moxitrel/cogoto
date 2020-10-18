#include <stdbool.h>
#include <stddef.h>

#define COGO_QUEUE_T(T)         COGO_QUEUE_T_(T)
#define COGO_QUEUE_T_(T)        T##_queue_t

#define COGO_QUEUE_EMPTY(T)     COGO_QUEUE_EMPTY_(T)
#define COGO_QUEUE_EMPTY_(T)    T##_queue_empty

#define COGO_QUEUE_POP(T)       COGO_QUEUE_POP_(T)
#define COGO_QUEUE_POP_(T)      T##_queue_pop

#define COGO_QUEUE_PUSH(T)      COGO_QUEUE_PUSH_(T)
#define COGO_QUEUE_PUSH_(T)     T##_queue_push

typedef struct {
    COGO_QUEUE_ITEM_T* head;
    COGO_QUEUE_ITEM_T* tail;
} COGO_QUEUE_T(COGO_QUEUE_ITEM_T);


COGO_INLINE bool COGO_QUEUE_EMPTY(COGO_QUEUE_ITEM_T)(const COGO_QUEUE_T(COGO_QUEUE_ITEM_T)* thiz)
{
    return thiz->head == NULL;
}


/* dequeue, return NULL if empty */
COGO_INLINE COGO_QUEUE_ITEM_T* COGO_QUEUE_POP(COGO_QUEUE_ITEM_T)(COGO_QUEUE_T(COGO_QUEUE_ITEM_T)* thiz)
{
    COGO_QUEUE_ITEM_T* node = thiz->head;
    if (!COGO_QUEUE_EMPTY(COGO_QUEUE_ITEM_T)(thiz)) {
        thiz->head = COGO_QUEUE_ITEM_NEXT(thiz->head);
    }
    return node;
}


/* enqueue */
COGO_INLINE void COGO_QUEUE_PUSH(COGO_QUEUE_ITEM_T)(COGO_QUEUE_T(COGO_QUEUE_ITEM_T)* thiz, COGO_QUEUE_ITEM_T* node)
{
    if (COGO_QUEUE_EMPTY(COGO_QUEUE_ITEM_T)(thiz)) {
        thiz->head = node;
    } else {
        COGO_QUEUE_ITEM_NEXT(thiz->tail) = node;
    }
    thiz->tail = node;
    COGO_QUEUE_ITEM_NEXT(node) = NULL;
}
