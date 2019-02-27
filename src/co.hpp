#ifndef COGO_CO_H
#define COGO_CO_H

#include "await.hpp"
#include <cstddef>

class co_t;
class co_queue_t;
class co_sch_t;

template <std::size_t> class chan_t;

// co_t: support concurrency
//  .run(): keep running until all coroutines finished
class co_t : protected await_t {
    // build coroutine queue (run concurrently)
    co_t *next;

    // cache channel message
    // when write blocked, the stored value type is void *
    // when read  blocked, the stored value type is void **
    void *chan_msg;

protected:
    friend co_queue_t;
    friend co_sch_t;

    // only allow co_t (forbid await_t)
    void _await(co_t &) noexcept;   // hide await_t::_await(co_t &)
    void _await(co_t *) noexcept;   // hide await_t::_await(co_t *)

    // co_start(): add a new coroutine to the scheduler to run.
    void _start(co_t &) noexcept;
    void _start(co_t *) noexcept;

    // co_chan_read(): receive a message from channel
    void _chan_read(chan_t<1> &, void *&) noexcept;
    // co_chan_write(): send a message to channel
    void _chan_write(chan_t<1> &, void *) noexcept;

public:
    // run until finish all
    void run(); // hide await_t::run()
};

// coroutine queue
class co_queue_t {
    co_t *head = nullptr;
    co_t *tail;

public:
    bool empty() const noexcept
    {
        return head == nullptr;
    }

    // dequeue, delete head node and return. If empty, return NULL.
    co_t *pop() noexcept
    {
        co_t *node = head;
        if (head != nullptr) {
            head = head->next;
        }
        return node;
    }

    // enqueue, append a coroutine to the end of queue.
    void push(co_t &co) noexcept
    {
        if (empty()) {
            head = &co;
        } else {
            tail->next = &co;
        }
        tail = &co;
        tail->next = nullptr;
    }

    // join another co_queue_t to the end.
    void merge(co_queue_t &q) noexcept
    {
        if (q.empty()) {
            return;
        }
        if (empty()) {
            *this = q;
            return;
        }
        tail->next = q.head;
        tail = q.tail;
    }
};

// co_t scheduler
class co_sch_t : await_sch_t {
    // coroutines in this queue run concurrently
    co_queue_t q = {};

    friend co_t;
};

// channel
template<std::size_t N>
class chan_t {
    /* rq, wq: all coroutines blocked by this channel */
    co_queue_t rq;  /* blocked coroutines when read   */
    co_queue_t wq;  /* blocked coroutines when write  */

    /* message buffer queue */
    std::size_t head = 0;
    std::size_t end  = 0;   /* just behind tail */
    std::size_t cap  = N;   /* the max number of messages can be buffered */
    void *msg[N];

    friend co_t;

    std::size_t len() const noexcept
    {
        return end - head;
    }

    void push(void *x) noexcept
    {
        msg[end++] = x;
        if (end >= cap) {
            end = 0;
        }
    }

    void *pop() noexcept
    {
        void *v = msg[head++];
        if (head >= cap) {
            head = 0;
        }
        return v;
    }
};


//
// co_t
//
inline void co_t::_await(co_t &co) noexcept
{
    await_t::_await(co);
}

inline void co_t::_await(co_t *co) noexcept
{
    assert(co);
    _await(*co);
}

inline void co_t::_start(co_t &co) noexcept
{
    co.sch = sch;
    static_cast<co_sch_t *>(sch)->q.push(co);
}

inline void co_t::_start(co_t *co) noexcept
{
    assert(co);
    _start(*co);
}

void co_t::_chan_read(chan_t<1> &chan, void *&msg) noexcept
{
    co_t *writer;   // coroutine write to channel blocked

    if (chan.len() > 0) {
        msg = chan.pop();
        if ((writer = chan.wq.pop()) != nullptr) {
            // push cached message
            chan.push(writer->chan_msg);
            // wake up the writer
            static_cast<co_sch_t *>(sch)->q.push(*writer);
        }
    } else if ((writer = chan.wq.pop()) != nullptr) {
        // read cached message from writer
        msg = writer->chan_msg;
        // wake up the writer
        static_cast<co_sch_t *>(sch)->q.push(*writer);
    } else {
        // save message address for writing
        chan_msg = &msg;                    // NOTE: the stored value type is void**
        // sleep in background
        chan.rq.push(*this);                // put into blocking queue
        sch->stack_top = nullptr;   // remove from scheduler
    }
}

void co_t::_chan_write(chan_t<1> &chan, void *msg) noexcept
{
    co_t *reader;   // coroutine read from channel blocked

    // 1. feed blocked reader first
    // 2. feed message buffer
    // 3. cache message in coroutine
    if ((reader = chan.rq.pop()) != nullptr) {
        // send message to reader, the stored value type of chan_msg should be void**
        *static_cast<void **>(reader->chan_msg) = msg;
        // wake up the reader
        static_cast<co_sch_t *>(sch)->q.push(*reader);
    } else if (chan.cap > chan.len()) {
        chan.push(msg);
    } else {
        // cache message for reading
        chan_msg = msg;
        // sleep in background
        chan.wq.push(*this);
        sch->stack_top = nullptr;
    }
}

// co_start(co_t &)
// co_start(co_t *)
#define co_start(CO)            \
do {                            \
    co_t::_start(CO);           \
    co_yield();                 \
} while (0)

#define co_chan_write(CHAN,MSG)                                     \
do {                                                                \
    co_t::_chan_write(reinterpret_cast<chan_t<1> &>(CHAN),(MSG));   \
    co_yield();                                                     \
} while (0)

#define co_chan_read(CHAN,MSG)                                      \
do {                                                                \
    co_t::_chan_read(reinterpret_cast<chan_t<1> &>(CHAN),(MSG));    \
    co_yield();                                                     \
} while (0)

// FIXME: scheduler stop successfully while expected blocking forever by channel.
inline void co_t::run()
{
    co_sch_t co_sch;

    co_sch.stack_top = this;
    sch = &co_sch;

    for (; co_sch.stack_top; co_sch.stack_top = co_sch.q.pop()) {
        co_sch.step();
        if (co_sch.stack_top) {
            co_sch.q.push(*static_cast<co_t *>(co_sch.stack_top));
        }
    }
}


#endif //COGO_CO_H
