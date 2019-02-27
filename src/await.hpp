/*

* API
- co_await(await_t &)   :: call another coroutine.
- co_await(await_t *)   :: ~

- .run()                :: run the coroutine until finish.

*/
#ifndef COGO_AWAIT_H
#define COGO_AWAIT_H

#include "gen.hpp"

class await_t;
class await_sch_t;
class co_t;

// awaitable context: add call stack support, behave like function
//  .run(): keep running until finish.
class await_t : protected gen_t {
    // the coroutine function
    virtual void operator()() = 0;

    // the lower call stack frame
    await_t *caller = nullptr;

    // scheduler, inited by co_await() or .run()
    await_sch_t *sch;

protected:
    friend await_sch_t;
    friend co_t;

    // co_await(): call another coroutine, push callee into call stack
    void _await(await_t &) noexcept;
    void _await(await_t *) noexcept;

public:
    virtual ~await_t() = default;

    // run until finish
    void run();
};

// scheduler for await_t
class await_sch_t {
    // call stack top, the coroutine run by scheduler
    await_t *stack_top; // inited by await_t.run()

    friend await_t;
    friend co_t;

    // run until yield
    void step();
};


//
// await_t
//
inline void await_t::_await(await_t &callee) noexcept
{
    assert(sch);

    // call stack push
    callee.caller = this;
    callee.sch = sch;
    callee.sch->stack_top = &callee;    // set new stack top
}

inline void await_t::_await(await_t *callee) noexcept
{
    assert(callee);
    _await(*callee);
}

// await_t::co_await(await_t &)
// await_t::co_await(await_t *)
#define co_await(CO)                        \
do {                                        \
    _await(CO);                             \
    co_yield();                             \
} while (0)


inline void await_t::run()
{
    await_sch_t await_sch;

    await_sch.stack_top = this;
    sch = &await_sch;

    while (await_sch.stack_top) {
        await_sch.step();
    }
}


//
// await_sch_t
//
inline void await_sch_t::step()
{
    if (stack_top->state() < 0) {
        // call stack pop
        stack_top = stack_top->caller;
    } else {
        // run
        stack_top->operator()();
    }
}

#endif // COGO_AWAIT_H
