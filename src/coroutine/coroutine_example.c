#include "coroutine.h"  // 1. include coroutine.h
#include <stdio.h>

// a coroutine print 0x7ffee80:0, 0x7ffee80:1, ... 0x7ffee80:6
typedef struct {
    co_t co;            // 2. inherit co_t (as first field)
    int i;              // declare coroutine local variable as struct field
} co_print_t;

void co_print(co_print_t *co)   // 3. define coroutine function which has the type "void (co_t *)"
{
    co_begin(co, 17);   // 4. coroutine begin; 17: line number of co_return(), co_call(), co_sched()
 // co_begin(co);       //    you can omit line numbers if enable gnuc extension

    for (co->i = 0; co->i < 7; co->i++) {
        printf("%p:%d\n", co, co->i);
        co_return(co);  // 5. yield
    }

    co_end(co);         // 4. coroutine end
}
// co_print_t constructor
#define CO_PRINT() ((co_print_t){.co = CO(co_print),})


typedef struct {
    co_t co;                // inherit co_t
    co_print_t coroutine1;  // declare coroutine local variable
    co_print_t coroutine2;  // declare coroutine local variable
} coroutine_example_t;

// a coroutine run two co_print coroutine concurrently
void coroutine_example(coroutine_example_t *co) // define coroutine function
{
    co_begin(co, 37, 38);

    co_sched(co, &co->coroutine1);  // run coroutine1 concurrently
    co_sched(co, &co->coroutine2);  // run coroutine2 concurrently

    co_end(co);
}

// coroutine_example_t constructor
#define COROUTINE_EXAMPLE() ((coroutine_example_t){ \
    .co = CO(coroutine_example),                    \
    .coroutine1 = CO_PRINT(),                       \
    .coroutine2 = CO_PRINT(),                       \
})

int main()
{
    // Run until finish all coroutines.
    // Print:
    //  0x8f0:0
    //  0x8f0:1
    //  0x918:0
    //  0x8f0:2
    //  0x918:1
    //  0x8f0:3
    //  ...
    coroutine_example_t co = COROUTINE_EXAMPLE();
    co_run((co_t *)&co);
}

// clang -std=c17 coroutine_example.c -o /tmp/coroutine_example && /tmp/coroutine_example
