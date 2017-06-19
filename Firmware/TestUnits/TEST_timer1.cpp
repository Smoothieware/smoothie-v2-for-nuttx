#include "../Unity/src/unity.h"
#include "TestRegistry.h"

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/boardctl.h>

#include <nuttx/timers/timer.h>

static volatile int timer_cnt= 0;

static bool timer_handler(FAR uint32_t *next_interval_us)
{
    ++timer_cnt;
    return true;
}

#define TIMER_DEVNAME "/dev/timer1"
#define TIMER_INTERVAL 1000000/20

REGISTER_TEST(TimerTest, test_20_hz)
{
    struct timer_sethandler_s handler;
    int ret;
    int fd;

    ret = boardctl(BOARDIOC_INIT, 0);
    TEST_ASSERT_EQUAL_INT(OK, ret);

    /* Open the timer device */

    printf("Open %s\n", TIMER_DEVNAME);

    fd = open(TIMER_DEVNAME, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Failed to open %s: %d\n",
                TIMER_DEVNAME, errno);
        TEST_FAIL();
    }

    /* Set the timer interval */
    printf("Set timer interval to %lu\n",
           (unsigned long)TIMER_INTERVAL);

    ret = ioctl(fd, TCIOC_SETTIMEOUT, TIMER_INTERVAL);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Failed to set the timer interval: %d\n", errno);
        close(fd);
        TEST_FAIL();
    }

    /* Attach the timer handler
     *
     */

    printf("Attach timer handler\n");

    handler.newhandler = timer_handler;
    handler.oldhandler = NULL;

    ret = ioctl(fd, TCIOC_SETHANDLER, (unsigned long)((uintptr_t)&handler));
    if (ret < 0) {
        fprintf(stderr, "ERROR: Failed to set the timer handler: %d\n", errno);
        close(fd);
        TEST_FAIL();
    }

    /* Start the timer */

    printf("Start the timer\n");

    ret = ioctl(fd, TCIOC_START, 0);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Failed to start the timer: %d\n", errno);
        close(fd);
        TEST_FAIL();
    }

    for (int i = 0; i < 10; ++i) {
        usleep(1000000);
        printf("time %d seconds, timer %d calls\n", i, timer_cnt);
    }


    /* Stop the timer */

    printf("Stop the timer\n");

    ret = ioctl(fd, TCIOC_STOP, 0);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Failed to stop the timer: %d\n", errno);
        close(fd);
        TEST_FAIL();
    }


    /* Close the timer driver */
    printf("Finished\n");
    close(fd);
}
