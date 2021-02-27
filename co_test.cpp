#include <assert.h>
#include "co.h"
#include "gtest/gtest.h"
#include <stdlib.h>

// put a coroutine into queue
inline int cogo_sch_push(cogo_sch_t* sch, cogo_co_t* co)
{
    assert(sch);
    assert(sch->stack_top);
    assert(co);
    if (co != sch->stack_top) {
        co->caller = sch->stack_top;
        sch->stack_top = co;
    }
    return 1;
}

// fetch the next coroutine to be run
inline cogo_co_t* cogo_sch_pop(cogo_sch_t* sch)
{
    assert(sch);
    return sch->stack_top;
}

inline void cogo_co_run(void* co)
{
    cogo_sch_t sch = {
        .stack_top = (cogo_co_t*)co,
    };
    while (cogo_sch_step(&sch))
    {}
}

extern inline cogo_co_t* cogo_sch_step(cogo_sch_t* sch);
extern inline int cogo_sch_push(cogo_sch_t* sch, cogo_co_t* co);
extern inline cogo_co_t* cogo_sch_pop(cogo_sch_t* sch);
extern inline void cogo_co_run(void* co);


CO_DECLARE(static F3)
{
CO_BEGIN:
    CO_YIELD;
    CO_RETURN;
    CO_YIELD;    // never run
CO_END:;
}

CO_DECLARE(static F2, F3 f3)
{
CO_BEGIN:
    CO_YIELD;
    CO_AWAIT(&((F2*)CO_THIS)->f3);
CO_END:;
}

CO_DECLARE(static F1, F2 f2)
{
CO_BEGIN:
    CO_AWAIT(&((F1*)CO_THIS)->f2);
CO_END:;
}

TEST(cogo_co_t, Step)
{
    F1 f1_ = CO_MAKE(F1, CO_MAKE(F2, CO_MAKE(F3)));
    auto& f2_ = f1_.f2;
    auto& f3_ = f2_.f3;

    cogo_sch_t sch = {
        .stack_top = (cogo_co_t*)&f1_,
    };
    ASSERT_EQ(CO_STATE(&f1_), 0);
    ASSERT_EQ(CO_STATE(&f2_), 0);
    ASSERT_EQ(CO_STATE(&f3_), 0);

    // F2 yield
    auto co = cogo_sch_step(&sch);
    EXPECT_EQ(co, (cogo_co_t*)&f2_);
    EXPECT_GT(CO_STATE(&f1_), 0);
    EXPECT_GT(CO_STATE(&f2_), 0);
    EXPECT_EQ(CO_STATE(&f3_), 0);

    // F3 first yield
    co = cogo_sch_step(&sch);
    EXPECT_EQ(co, (cogo_co_t*)&f3_);
    EXPECT_GT(CO_STATE(&f1_), 0);
    EXPECT_GT(CO_STATE(&f2_), 0);
    EXPECT_GT(CO_STATE(&f3_), 0);

    // F3 co_return
    co = cogo_sch_step(&sch);
    EXPECT_EQ(CO_STATE(&f1_), -1);
    EXPECT_EQ(CO_STATE(&f2_), -1);
    EXPECT_EQ(CO_STATE(&f3_), -1);
}

static unsigned fibonacci(unsigned n)
{
    switch (n) {
    case 0:
        return 1;
    case 1:
        return 1;
    default:
        return fibonacci(n-1) + fibonacci(n-2);
    }
}

CO_DECLARE(static Fibonacci, unsigned n, unsigned v, Fibonacci* fib_n1, Fibonacci* fib_n2)
{
    auto* thiz = (Fibonacci*)CO_THIS;
    auto& n = thiz->n;
    auto& v = thiz->v;
    auto& fib_n1 = thiz->fib_n1;
    auto& fib_n2 = thiz->fib_n2;
CO_BEGIN:

    switch (n) {
    case 0:     // f(0) = 1
        v = 1;
        CO_RETURN;
    case 1:     // f(1) = 1
        v = 1;
        CO_RETURN;
    default:    // f(n) = f(n-1) + f(n-2)
        fib_n1 = (Fibonacci*)malloc(sizeof(*fib_n1));
        fib_n2 = (Fibonacci*)malloc(sizeof(*fib_n2));
        ASSERT_NE(fib_n1, nullptr);
        ASSERT_NE(fib_n2, nullptr);

        *fib_n1 = CO_MAKE(Fibonacci, n - 1);
        *fib_n2 = CO_MAKE(Fibonacci, n - 2);
        CO_AWAIT(fib_n1);  // eval f(n-1)
        CO_AWAIT(fib_n2);  // eval f(n-2)
        v = fib_n1->v + fib_n2->v;

        free(fib_n1);
        free(fib_n2);
        CO_RETURN;
    }

CO_END:;
}

TEST(cogo_co_t, Run)
{
    struct {
        Fibonacci fib;
        unsigned value;
    } example[] = {
        {CO_MAKE(Fibonacci, 0)  , fibonacci(0)},
        {CO_MAKE(Fibonacci, 1)  , fibonacci(1)},
        {CO_MAKE(Fibonacci, 11) , fibonacci(11)},
        {CO_MAKE(Fibonacci, 23) , fibonacci(23)},
        {CO_MAKE(Fibonacci, 29) , fibonacci(29)},
    };

    for (size_t i = 0; i < sizeof(example) / sizeof(example[0]); i++) {
        cogo_co_run(&example[i].fib);
        ASSERT_EQ(example[i].fib.v, example[i].value);
    }
}
