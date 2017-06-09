#include "../easyunit/testharness.h"

#include <nuttx/config.h>
#include <nuttx/init.h>
#include <nuttx/arch.h>

#include <stdio.h>

extern "C" int smoothie_main(int argc, char *argv[])
{
	// do C++ initialization for static constructors first
    up_cxxinitialize();

    printf("Starting tests...\n");

    TestRegistry::runAndPrint();

    printf("Done\n");

    return 1;
}
