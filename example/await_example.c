#include <assert.h>
#include "../src/await.h"
#include <stdlib.h>
#include <stdio.h>

// Fibonacci number
typedef struct fibonacci {
    await_t await_t;
    unsigned int n;             // arg
    unsigned int v;             // return value
    struct fibonacci *fib_n1;   // local variable, f(n-1)
    struct fibonacci *fib_n2;   // local variable, f(n-2)
} fibonacci_t;

#define FIBONACCI(N) ((fibonacci_t){    \
    .await_t = AWAIT(fibonacci),        \
    .n = (N),                           \
})

void fibonacci(fibonacci_t *co)
{
    co_begin(co);

    if (co->n == 0) {           // f(0) = 0
        co->v = 0;
    } else if (co->n == 1) {    // f(1) = 1
        co->v = 1;
    } else {                    // f(n) = f(n-1) + f(n-2)
        co->fib_n1 = malloc(sizeof *co->fib_n1);
        co->fib_n2 = malloc(sizeof *co->fib_n1);
        assert(co->fib_n1);
        assert(co->fib_n2);
        *co->fib_n1 = FIBONACCI(co->n - 1);
        *co->fib_n2 = FIBONACCI(co->n - 2);

        co_await(co, co->fib_n1);  // wait f(n-1) to finish
        co_await(co, co->fib_n2);  // wait f(n-2) to finish
        co->v = co->fib_n1->v + co->fib_n2->v;

        free(co->fib_n1);
        free(co->fib_n2);
    }

    co_end(co);
}

int main(void)
{
    fibonacci_t f = FIBONACCI(9);
    await_run(&f);
    printf("%u\n", f.v);

    return 0;
}