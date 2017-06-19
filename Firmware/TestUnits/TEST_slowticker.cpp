#include "../Unity/src/unity.h"
#include "TestRegistry.h"

#include "SlowTicker.h"

static volatile int timer_cnt= 0;

static void timer_callback(void)
{
    ++timer_cnt;
}

REGISTER_TEST(SlowTicker, test_20_hz)
{
    SlowTicker& slt= SlowTicker::getInstance();
    slt.set_frequency(1); // set to 1hz
    TEST_ASSERT_TRUE(slt.start());

    int n= slt.attach(20, timer_callback);

    for (int i = 0; i < 10; ++i) {
        usleep(1000000);
        printf("time %d seconds, timer %d calls\n", i, timer_cnt);
    }

    slt.detach(n);
    slt.stop();
}
