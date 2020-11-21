#include <assert.h>
#include "co_fs.h"
#include "co_intf.h"
#include "gtest/gtest.h"

CO_DECLARE(F3)
{
CO_BEGIN:
    CO_YIELD;
    CO_RETURN;
    CO_YIELD;    // never run
CO_END:;
}

CO_DECLARE(F2, F3 f3)
{
CO_BEGIN:
    CO_YIELD;
    CO_AWAIT(&CO_THIS->f3);
CO_END:;
}

CO_DECLARE(F1, F2 f2)
{
CO_BEGIN:
    CO_AWAIT(&CO_THIS->f2);
CO_END:;
}

CO_DECLARE(F0, F1 f1);
CO_DEFINE(F0)
{
CO_BEGIN:
    CO_AWAIT(&CO_THIS->f1);
CO_END:;
}

class Co : public ::testing::Test {
protected:
    F0 f0_ = CO_MAKE(F0, CO_MAKE(F1, CO_MAKE(F2, CO_MAKE(F3))));
    F1& f1_ = f0_.f1;
    F2& f2_ = f1_.f2;
    F3& f3_ = f2_.f3;

    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_F(Co, InitState)
{
    ASSERT_EQ(CO_STATE(&f0_), 0);
    ASSERT_EQ(CO_STATE(&f1_), 0);
    ASSERT_EQ(CO_STATE(&f2_), 0);
    ASSERT_EQ(CO_STATE(&f3_), 0);
}

TEST_F(Co, Step)
{
    co_sch_t sch = {};
    cogo_sch_push((cogo_sch_t*)&sch, (cogo_co_t*)&f0_);
    cogo_sch_step((cogo_sch_t*)&sch);

    // F2 yield
    cogo_co_t* co = cogo_sch_step((cogo_sch_t*)&sch);
    EXPECT_EQ(co, (cogo_co_t*)&f2_);
    EXPECT_GT(CO_STATE(&f0_), 0);
    EXPECT_GT(CO_STATE(&f1_), 0);
    EXPECT_GT(CO_STATE(&f2_), 0);
    EXPECT_EQ(CO_STATE(&f3_), 0);

    // F3 first yield
    co = cogo_sch_step((cogo_sch_t*)&sch);
    EXPECT_EQ(co, (cogo_co_t*)&f3_);
    EXPECT_GT(CO_STATE(&f0_), 0);
    EXPECT_GT(CO_STATE(&f1_), 0);
    EXPECT_GT(CO_STATE(&f2_), 0);
    EXPECT_GT(CO_STATE(&f3_), 0);

    // F3 co_return
    co = cogo_sch_step((cogo_sch_t*)&sch);
    EXPECT_EQ(CO_STATE(&f0_), -1);
    EXPECT_EQ(CO_STATE(&f1_), -1);
    EXPECT_EQ(CO_STATE(&f2_), -1);
    EXPECT_EQ(CO_STATE(&f3_), -1);
}

TEST_F(Co, Run)
{
    co_run((co_t*)&f0_);
    EXPECT_EQ(CO_STATE(&f0_), -1);
    EXPECT_EQ(CO_STATE(&f1_), -1);
    EXPECT_EQ(CO_STATE(&f2_), -1);
    EXPECT_EQ(CO_STATE(&f3_), -1);
}


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
