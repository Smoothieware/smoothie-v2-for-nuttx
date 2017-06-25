#include "../Unity/src/unity.h"
#include <sstream>

#include "ConfigReader.h"
#include "TestRegistry.h"

#include "prettyprint.hpp"
#include <iostream>

static std::string str("[switch]\nfan.enable = true\nfan.input_on_command = M106 # comment\nfan.input_off_command = M107\n\
fan.output_pin = 2.6 # pin to use\nfan.output_type = pwm\nmisc.enable = true\nmisc.input_on_command = M42\nmisc.input_off_command = M43\n\
misc.output_pin = 2.4\nmisc.output_type = digital\nmisc.value = 123.456\nmisc.ivalue= 123\npsu.enable = false\n\
[dummy]\nenable = false #set to true\n#ignore comment\n");

REGISTER_TEST(ConfigTest, get_sections)
{
    std::stringstream ss1(str);
    ConfigReader cr(ss1);

    ConfigReader::sections_t sections;
    //systime_t st = clock_systimer();
    TEST_ASSERT_TRUE(cr.get_sections(sections));
    //systime_t en = clock_systimer();
    //printf("elapsed time %d us\n", TICK2USEC(en-st));

    TEST_ASSERT_TRUE(sections.find("switch") != sections.end());
    TEST_ASSERT_TRUE(sections.find("dummy") != sections.end());
    TEST_ASSERT_TRUE(sections.find("none") == sections.end());
}

REGISTER_TEST(ConfigTest, load_section)
{
    std::stringstream ss1(str);
    ConfigReader cr(ss1);
    //cr.reset();

    ConfigReader::section_map_t m;
    //systime_t st = clock_systimer();
    bool b= cr.get_section("dummy", m);
    //systime_t en = clock_systimer();
    //printf("elapsed time %d us\n", TICK2USEC(en-st));
    std::cout << m << "\n";

    TEST_ASSERT_TRUE(b);
    TEST_ASSERT_EQUAL_INT(1, m.size());
    TEST_ASSERT_TRUE(m.find("enable") != m.end());
    TEST_ASSERT_EQUAL_STRING("false", m["enable"].c_str());
    TEST_ASSERT_FALSE(cr.get_bool(m, "enable", true));
}

REGISTER_TEST(ConfigTest, load_sub_sections)
{
    std::stringstream ss1(str);
    ConfigReader cr(ss1);

    ConfigReader::sub_section_map_t ssmap;
    TEST_ASSERT_TRUE(ssmap.empty());

    systime_t st = clock_systimer();
    TEST_ASSERT_TRUE(cr.get_sub_sections("switch", ssmap));
    systime_t en = clock_systimer();
    printf("elapsed time %d us\n", TICK2USEC(en-st));

    TEST_ASSERT_EQUAL_STRING("switch", cr.get_current_section().c_str());
    TEST_ASSERT_EQUAL_INT(3, ssmap.size());

    bool fanok= false;
    bool miscok= false;
    bool psuok= false;
    for(auto& i : ssmap) {
        // foreach switch
        std::string name= i.first;
        auto& m= i.second;
        if(cr.get_bool(m, "enable", false)) {
            const char* pin= cr.get_string(m, "output_pin", "nc");
            const char* input_on_command = cr.get_string(m, "input_on_command", "");
            const char* input_off_command = cr.get_string(m, "input_off_command", "");
            const char* output_on_command = cr.get_string(m, "output_on_command", "");
            const char* output_off_command = cr.get_string(m, "output_off_command", "");
            const char* type = cr.get_string(m, "output_type", "");
            const char* ipb = cr.get_string(m, "input_pin_behavior", "momentary");
            int iv= cr.get_int(m, "ivalue", 0);
            float fv= cr.get_float(m, "value", 0.0F);

            if(name == "fan") {
                TEST_ASSERT_EQUAL_STRING("2.6", pin);
                TEST_ASSERT_EQUAL_STRING("M106", input_on_command);
                TEST_ASSERT_EQUAL_STRING("M107", input_off_command);
                TEST_ASSERT_TRUE(output_on_command[0] == 0);
                TEST_ASSERT_TRUE(output_off_command[0] == 0);
                TEST_ASSERT_EQUAL_STRING(type, "pwm");
                TEST_ASSERT_EQUAL_STRING(ipb, "momentary");
                TEST_ASSERT_EQUAL_INT(0, iv);
                TEST_ASSERT_EQUAL_FLOAT(0.0F, fv);
                fanok= true;

            } else if(name == "misc") {
                TEST_ASSERT_EQUAL_STRING("2.4", pin);
                TEST_ASSERT_EQUAL_STRING("M42", input_on_command);
                TEST_ASSERT_EQUAL_STRING("M43", input_off_command);
                TEST_ASSERT_TRUE(output_on_command[0] == 0);
                TEST_ASSERT_TRUE(output_off_command[0] == 0);
                TEST_ASSERT_EQUAL_STRING("digital", type);
                TEST_ASSERT_EQUAL_STRING("momentary", ipb);
                TEST_ASSERT_EQUAL_INT(123, iv);
                TEST_ASSERT_EQUAL_FLOAT(123.456F, fv);
                miscok= true;

            } else if(name == "psu") {
                psuok= true;

            } else {
                TEST_FAIL();
            }
        }
    }
    TEST_ASSERT_TRUE(fanok);
    TEST_ASSERT_TRUE(miscok);
    TEST_ASSERT_FALSE(psuok);
}
