#include <assert.h>
#include "co_fs.h"
#include "co_intf.h"
#include "gtest/gtest.h"

CO_DECLARE(Recv, co_chan_t* c, co_msg_t msgNext)
{
CO_BEGIN:

    CO_CHAN_READ(CO_THIS->c, &CO_THIS->msgNext);

CO_END:;
}

CO_DECLARE(Send, co_chan_t* c, co_msg_t msg)
{
CO_BEGIN:

    CO_CHAN_WRITE(CO_THIS->c, &CO_THIS->msg);

CO_END:;
}

CO_DECLARE(Entry, Recv recv1, Send send1)
{
CO_BEGIN:

    CO_START(&CO_THIS->recv1);
    CO_START(&CO_THIS->send1);

CO_END:;
}

TEST(Chan, ReadWrite)
{
    auto c0 = CO_CHAN_MAKE(0);
    auto entry = CO_MAKE(Entry,
            CO_MAKE(Recv, &c0),
            CO_MAKE(Send, &c0));
    co_run((co_t*)&entry);
    EXPECT_EQ(&entry.send1.msg, entry.recv1.msgNext.next);
}
