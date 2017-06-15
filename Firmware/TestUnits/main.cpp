#include <nuttx/config.h>
#include <nuttx/init.h>
#include <nuttx/arch.h>

#include <stdio.h>

#include <vector>
#include <tuple>

#include "../Unity/src/unity.h"
#include "TestRegistry.h"

int test_runner(void)
{
    auto tests= TestRegistry::instance().get_tests();
    printf("There are %d registered tests...\n", tests.size());
    for(auto& i : tests) {
        printf("  %s\n", std::get<1>(i));
    }

    UnityBegin("TestUnits.new");

    for(auto i : tests) {
        UnityTestFunction fnc= std::get<0>(i);
        const char *name= std::get<1>(i);
        int ln= std::get<2>(i);
        UnityDefaultTestRun(fnc, name, ln);
    }

    return (UnityEnd());
}

int run_tests(int argc, char *argv[])
{
    // do C++ initialization for static constructors first
    up_cxxinitialize();

    printf("Starting tests...\n");
    int ret = test_runner();
    printf("Done\n");
    return ret;
}

extern "C" int smoothie_main(int argc, char *argv[])
{

    task_create("tests", SCHED_PRIORITY_DEFAULT,
                10000,
                (main_t)run_tests,
                (FAR char * const *)NULL);


    return 1;
}
