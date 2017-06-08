#include "../easyunit/testharness.h"

#include <nuttx/config.h>
#include <nuttx/init.h>
#include <nuttx/arch.h>

#include <stdio.h>

//#include <iostream>

extern "C" int smoothie_main(int argc, char *argv[])
{
//	auto toEnsureInitialization= new std::ios_base::Init;

	// do C++ initialization for static constructors first
    up_cxxinitialize();

    printf("Starting tests...\n");

    TestRegistry::runAndPrint();

    printf("Done\n");

    return 1;
}
