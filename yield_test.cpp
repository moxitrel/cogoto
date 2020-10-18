#include <assert.h>

#if defined(COGO_CASE)
#   include "yield_case.h"
#elif defined(COGO_LABEL_VALUE)
#   include "yield_label_value.h"
#endif

#include "yield.h"
#include "gtest/gtest.h"

CO_DECLARE(CogoYield, int v)
{
CO_BEGIN:

    CO_THIS->v++;
    CO_YIELD;
    CO_THIS->v++;

CO_END:;
}

TEST(cogo_yield_t, state_after_yield)
{
    CogoYield cogoYield = {.v = 0};
    EXPECT_EQ(CO_STATE(&cogoYield), 0);     // init

    CogoYield_func(&cogoYield);
    EXPECT_GT(CO_STATE(&cogoYield), 0);     // running
    EXPECT_EQ(cogoYield.v, 1);

    CogoYield_func(&cogoYield);
    EXPECT_EQ(CO_STATE(&cogoYield), -1);    // end
    EXPECT_EQ(cogoYield.v, 2);
}


CO_DECLARE(CogoReturn, int v)
{
CO_BEGIN:

    CO_THIS->v++;
    CO_RETURN;
    CO_THIS->v++;

CO_END:;
}

TEST(cogo_yield_t, state_after_return)
{
    CogoReturn cogoReturn = {.v = 0};
    EXPECT_EQ(CO_STATE(&cogoReturn), 0);    // init

    CogoReturn_func(&cogoReturn);
    EXPECT_EQ(CO_STATE(&cogoReturn), -1);   // end
    EXPECT_EQ(cogoReturn.v, 1);
}


CO_DECLARE(OutLabel, int begin, int end)
{
    CO_THIS->begin++;

CO_BEGIN:
    CO_YIELD;
    CO_YIELD;
CO_END:

    CO_THIS->end++;
}

TEST(cogo_yield_t, always_run_before_or_after)
{
    OutLabel outLabel = {.begin=0, .end=0};

    while(CO_STATE(&outLabel) >= 0) {
        OutLabel_func(&outLabel);
    }

    EXPECT_EQ(outLabel.begin, 3);
    EXPECT_EQ(outLabel.end, 3);
}
