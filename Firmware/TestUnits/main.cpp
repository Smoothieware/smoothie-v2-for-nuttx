#include "../easyunit/testharness.h"

#include <nuttx/config.h>
#include <nuttx/init.h>
#include <nuttx/arch.h>

#include <stdio.h>

int run_tests(int argc, char *argv[])
{
    // do C++ initialization for static constructors first
    up_cxxinitialize();

    printf("Starting tests...\n");
    TestRegistry::runAndPrint();
    printf("Done\n");
    return 1;
}

extern "C" int smoothie_main(int argc, char *argv[])
{

    task_create("tests", SCHED_PRIORITY_DEFAULT,
                    10000,
                    (main_t)run_tests,
                    (FAR char * const *)NULL);


    return 1;
}
