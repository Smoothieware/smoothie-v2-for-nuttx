#pragma once

#include "../Unity/src/unity.h"

#include <vector>
#include <tuple>

class TestRegistry
{
public:
    TestRegistry(){};
    ~TestRegistry(){};

    // setup the Singleton instance
    static TestRegistry& instance()
    {
        static TestRegistry instance;
        return instance;
    }

    void add_test(UnityTestFunction fnc, const char *name, int ln)
    {
        test_fncs.push_back(std::make_tuple(fnc, name, ln));
    }

    std::vector<std::tuple<UnityTestFunction, const char *, int>>& get_tests() { return test_fncs; }

private:
    std::vector<std::tuple<UnityTestFunction, const char *, int>> test_fncs;

};

#define REGISTER_TEST(testCaseName, testName)\
  class testCaseName##testName##Test{ \
    public: testCaseName##testName##Test() { TestRegistry::instance().add_test(testCaseName##testName##Test::test, #testCaseName "-" #testName, __LINE__);}\
    static void test(void); };\
    static testCaseName##testName##Test testCaseName##testName##Instance; \
    void testCaseName##testName##Test::test(void)
