#include "../Unity/src/unity.h"
#include <sstream>

#include "ConfigReader.h"
#include "TestRegistry.h"

static std::string str("[switch]\nfan.enable = true\nfan.input_on_command = M106\nfan.input_off_command = M107\n\
fan.output_pin = 2.6\nfan.output_type = pwm\nmisc.enable = true\nmisc.input_on_command = M42\nmisc.input_off_command = M43\n\
misc.output_pin = 2.4\nmisc.output_type = digital\nmisc.value = 123.456\nmisc.ivalue= 123\npsu.enable = false\n\
[dummy]\nenable = false");

static ConfigReader::sections_t sections;
static std::stringstream ss1(str);
static ConfigReader cr(ss1);
REGISTER_TEST(ConfigTest, sections)
{
    cr.reset();
    TEST_ASSERT_TRUE(cr.get_sections(sections));
    TEST_ASSERT_TRUE(sections.find("switch") != sections.end());
    TEST_ASSERT_TRUE(sections.find("dummy") != sections.end());
    TEST_ASSERT_TRUE(sections.find("none") == sections.end());
}


static ConfigReader::sub_section_map_t ssmap;
REGISTER_TEST(ConfigTest, load_switches)
{
    cr.reset();
    TEST_ASSERT_TRUE(ssmap.empty());
    TEST_ASSERT_TRUE(cr.get_sub_sections("switch", ssmap));
    TEST_ASSERT_EQUAL_STRING("switch", cr.get_current_section().c_str());

    bool fanok= false;
    bool miscok= false;
    bool psuok= false;
    for(auto& i : ssmap) {
        // foreach switch
        std::string name= i.first;
        auto& m= i.second;
        if(cr.get_bool(m, "enable", false)) {
            std::string pin= cr.get_string(m, "output_pin", "nc");
            std::string input_on_command = cr.get_string(m, "input_on_command", "");
            std::string input_off_command = cr.get_string(m, "input_off_command", "");
            std::string output_on_command = cr.get_string(m, "output_on_command", "");
            std::string output_off_command = cr.get_string(m, "output_off_command", "");
            std::string type = cr.get_string(m, "output_type", "");
            std::string ipb = cr.get_string(m, "input_pin_behavior", "momentary");
            int iv= cr.get_int(m, "ivalue", 0);
            float fv= cr.get_float(m, "value", 0.0F);

            if(name == "fan") {
                TEST_ASSERT_EQUAL_STRING("2.6", pin.c_str());
                TEST_ASSERT_EQUAL_STRING("M106", input_on_command.c_str());
                TEST_ASSERT_EQUAL_STRING("M107", input_off_command.c_str());
                TEST_ASSERT_TRUE(output_on_command.empty());
                TEST_ASSERT_TRUE(output_off_command.empty());
                TEST_ASSERT_EQUAL_STRING(type.c_str(), "pwm");
                TEST_ASSERT_EQUAL_STRING(ipb.c_str(), "momentary");
                TEST_ASSERT_EQUAL_INT(0, iv);
                TEST_ASSERT_EQUAL_FLOAT(0.0F, fv);
                fanok= true;

            } else if(name == "misc") {
                TEST_ASSERT_EQUAL_STRING("2.4", pin.c_str());
                TEST_ASSERT_EQUAL_STRING("M42", input_on_command.c_str());
                TEST_ASSERT_EQUAL_STRING("M43", input_off_command.c_str());
                TEST_ASSERT_TRUE(output_on_command.empty());
                TEST_ASSERT_TRUE(output_off_command.empty());
                TEST_ASSERT_EQUAL_STRING("digital", type.c_str());
                TEST_ASSERT_EQUAL_STRING("momentary", ipb.c_str());
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
