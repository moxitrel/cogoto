#include <assert.h>
#include "co_intf.h"
#include "gtest/gtest.h"
#include <stdlib.h>

// put a coroutine into queue
COGO_INLINE int cogo_sch_push(cogo_sch_t* sch, cogo_co_t* co)
{
    assert(sch);
    assert(sch->stack_top);
    assert(co);
    if (co != sch->stack_top) {
        cogo_co_await(sch->stack_top, co);
    }
    return 1;
}

// fetch the next coroutine to be run
COGO_INLINE cogo_co_t* cogo_sch_pop(cogo_sch_t* sch)
{
    return sch->stack_top;
}

static void cogo_co_run(cogo_co_t* main)
{
    cogo_sch_t sch = {.stack_top = main};
    while (cogo_sch_step((cogo_sch_t*)&sch))
    {}
}

unsigned int fibonacci(unsigned int n)
{
    switch (n) {
    case 0:
        return 0;
    case 1:
        return 1;
    default:
        return fibonacci(n-1) + fibonacci(n-2);
    }
}

CO_DECLARE(Fibonacci, unsigned int n, unsigned int v, Fibonacci* fib_n1, Fibonacci* fib_n2)
{
CO_BEGIN:

    switch (CO_THIS->n) {
    case 0:     // f(0) = 0
        CO_THIS->v = 0;
        break;
    case 1:     // f(1) = 1
        CO_THIS->v = 1;
        break;
    default:    // f(n) = f(n-1) + f(n-2)
        CO_THIS->fib_n1 = (Fibonacci*)malloc(sizeof(*CO_THIS->fib_n1));
        CO_THIS->fib_n2 = (Fibonacci*)malloc(sizeof(*CO_THIS->fib_n2));
        ASSERT_NE(CO_THIS->fib_n1, nullptr);
        ASSERT_NE(CO_THIS->fib_n2, nullptr);
        *CO_THIS->fib_n1 = CO_MAKE(Fibonacci, CO_THIS->n - 1);
        *CO_THIS->fib_n2 = CO_MAKE(Fibonacci, CO_THIS->n - 2);

        CO_AWAIT(CO_THIS->fib_n1);  // eval f(n-1)
        CO_AWAIT(CO_THIS->fib_n2);  // eval f(n-2)

        CO_THIS->v = CO_THIS->fib_n1->v + CO_THIS->fib_n2->v;
        free(CO_THIS->fib_n1);
        free(CO_THIS->fib_n2);
        break;
    }

CO_END:;
}

TEST(CO, Step)
{
    struct {
        Fibonacci fib;
        unsigned int value;
    } example[] = {
        {CO_MAKE(Fibonacci, 0)  , fibonacci(0)},
        {CO_MAKE(Fibonacci, 1)  , fibonacci(1)},
        {CO_MAKE(Fibonacci, 11) , fibonacci(11)},
        {CO_MAKE(Fibonacci, 23) , fibonacci(23)},
        {CO_MAKE(Fibonacci, 29) , fibonacci(29)},
    };

    for (size_t i = 0; i < sizeof(example) / sizeof(example[0]); i++) {
        cogo_co_run((cogo_co_t*)&example[i].fib);
        ASSERT_EQ(example[i].fib.v, example[i].value);
    }
}
