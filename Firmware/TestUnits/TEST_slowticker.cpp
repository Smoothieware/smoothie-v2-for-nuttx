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
    TEST_ASSERT_TRUE(slt.start());

    int n= slt.attach(20, timer_callback);

    for (int i = 0; i < 5; ++i) {
        usleep(1000000);
        printf("time %d seconds, timer %d calls\n", i, timer_cnt);
    }

    slt.detach(n);
    slt.stop();

    TEST_ASSERT_INT_WITHIN(2, 100, timer_cnt);
}

#include "nuttx/clock.h"
#include <semaphore.h>
static sem_t g_sem;

static void timer_callback2(void)
{
    sem_post(&g_sem);
    timer_cnt++;
}

REGISTER_TEST(SlowTicker, test_10_hz)
{
    sem_init(&g_sem, 0, 0);
    sem_setprotocol(&g_sem, SEM_PRIO_NONE); // as it is a signalling instead of locking
    timer_cnt= 0;

    // test where the interrupt triggers a semaphore and the thread is waiting on a semaphore
    SlowTicker& slt= SlowTicker::getInstance();
    TEST_ASSERT_TRUE(slt.start());

    int n= slt.attach(10, timer_callback2);

    // allow 5 seconds of events
    systime_t st = clock_systimer();
    for (int i = 0; i < 50; ++i) {
        // wait for timer to trigger us to wakeup
        while (sem_wait(&g_sem) < 0);
        // do stuff here, ie call the timeouts
    }
    systime_t en = clock_systimer();

    printf("elapsed time %dus, timer %d calls\n", TICK2USEC(en-st), timer_cnt);

    slt.detach(n);
    slt.stop();
    sem_destroy(&g_sem);

    TEST_ASSERT_EQUAL_INT(50, timer_cnt);
    TEST_ASSERT_INT_WITHIN(100000, 5000000, TICK2USEC(en-st));
}
