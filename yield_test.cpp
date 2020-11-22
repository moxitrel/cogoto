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

TEST(cogo_yield_t, Yield)
{
    CogoYield cogoYield = {
        .v = 0,
    };
    EXPECT_EQ(CO_STATE(&cogoYield), 0);     // init
    EXPECT_EQ(cogoYield.v, 0);

    CogoYield_func(&cogoYield);
    EXPECT_GT(CO_STATE(&cogoYield), 0);     // running
    EXPECT_EQ(cogoYield.v, 1);

    CogoYield_func(&cogoYield);
    EXPECT_EQ(CO_STATE(&cogoYield), -1);    // end
    EXPECT_EQ(cogoYield.v, 2);

    // noop when coroutine finished
    CogoYield_func(&cogoYield);
    EXPECT_EQ(CO_STATE(&cogoYield), -1);
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

TEST(cogo_yield_t, Return)
{
    CogoReturn cogoReturn = {
        .v = 0,
    };
    EXPECT_EQ(CO_STATE(&cogoReturn), 0);    // init
    EXPECT_EQ(cogoReturn.v, 0);

    CogoReturn_func(&cogoReturn);
    EXPECT_EQ(CO_STATE(&cogoReturn), -1);   // end
    EXPECT_EQ(cogoReturn.v, 1);

    // noop
    CogoReturn_func(&cogoReturn);
    EXPECT_EQ(CO_STATE(&cogoReturn), -1);
    EXPECT_EQ(cogoReturn.v, 1);
}


CO_DECLARE(ProEpiLogue, int enter, int exit)
{
    CO_THIS->enter++;

CO_BEGIN:
    CO_YIELD;
    CO_YIELD;
CO_END:

    CO_THIS->exit++;
}

TEST(cogo_yield_t, PrologueEpilogue)
{
    ProEpiLogue proEpiLogue = {
        .enter = 0,
        .exit = 0,
    };

    while(CO_STATE(&proEpiLogue) >= 0) {
        ProEpiLogue_func(&proEpiLogue);
    }

    EXPECT_EQ(proEpiLogue.enter, 3);
    EXPECT_EQ(proEpiLogue.exit, 3);
}
