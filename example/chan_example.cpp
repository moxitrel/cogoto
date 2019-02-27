#include <assert.h>
#include "../src/cogo.h"
#include <iostream>
#include <time.h>

// send a signal to <reader> when writer finished
class writer_t : public co_t {
    chan_t<0> &chan;
    unsigned long i;

    void operator()()
    {
        co_begin();

        for (; i > 0; i--) {        // countdown
            co_yield();
        }
        co_chan_write(chan, this);  // send finish signal

        co_end();
    }

public:
    writer_t(chan_t<0> &chan, unsigned long i)
        : chan(chan)
        , i(i)
    {}
};


// wait <writer>s finished
class reader_t : public co_t {
    chan_t<0> &chan;
    writer_t   writers[2];
    void      *msg;
    time_t     t;

    void operator()()
    {
        co_begin();

        // create 2 new coroutines
        co_start(writers[0]);
        co_start(writers[1]);

        // wait finish
        co_chan_read(chan, msg);    // receive finish signal
        std::cout << msg << " finished: " << clock() * 1.0 / CLOCKS_PER_SEC << "s\n";
        co_chan_read(chan, msg);    // receive finish signal
        std::cout << msg << " finished: " << clock() * 1.0 / CLOCKS_PER_SEC << "s\n";

        co_end();
    }

public:
    reader_t(chan_t<0> &&chan)
        : chan(chan)
        , writers{writer_t(chan, 1ul << 26), writer_t(chan, 1ul << 27)}
    {}
};

int main()
{
    reader_t(chan_t<0>()).run();
}