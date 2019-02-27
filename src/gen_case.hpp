/*

* API
- co_begin ()   :: mark coroutine begin. List with line numbers of co_yield() and co_return().
- co_end   ()   :: mark coroutine end.
- co_yield ()   :: yield from coroutine.
- co_return()   :: return with ending coroutine.

- .state() -> int   :: get the current running state.

*/
#ifndef COGO_GEN_H
#define COGO_GEN_H

#ifndef assert
#   define assert(...)  /* nop */
#endif

// gen_t: generator context with yield support
//  .state() -> int: return the current running state.
class gen_t {
protected:
    // start point where coroutine continue to run.
    //   0: inited
    //  >0: running
    //  <0: stopped (-1: success)
    int _pc = 0;
public:
    // get the current running state
    int state() const noexcept
    {
        return _pc;
    }
};

// gen_t::co_begin();
#define co_begin(...)                                   \
    switch (gen_t::_pc) {                               \
    default:                    /* invalid _pc      */  \
        assert(((void)"_pc isn't valid.", false));      \
        goto CO_END;                                    \
    case -1:                    /* coroutine end    */  \
        goto CO_END;                                    \
    case  0:                    /* coroutine begin  */  \


// gen_t::co_yield();
#define co_yield(...)                                                                           \
do {                                                                                            \
        gen_t::_pc = __LINE__;  /* 1. save the restore point, at label case __LINE__ */         \
        goto CO_END;            /* 2. return */                                                 \
    case __LINE__:;             /* 3. put a case after each return as restore point */          \
} while (0)


// gen_t::co_return();
#define co_return(...)                                  \
do {                                                    \
        goto CO_RETURN;     /* return */                \
} while (0)


// gen_t::co_end();
#define co_end()                                        \
    CO_RETURN:                                          \
        gen_t::_pc = -1;   /* finish successfully */    \
    CO_END:;                                            \
    }

#endif // COGO_GEN_H
