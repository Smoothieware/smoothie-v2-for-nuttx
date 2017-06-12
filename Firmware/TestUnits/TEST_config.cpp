#include "../easyunit/test.h"
#include <sstream>

#include "ConfigReader.h"

TEST(Config,load_switches)
{
    std::string str("[switch]\nfan.enable = true\nfan.input_on_command = M106\nfan.input_off_command = M107\n\
fan.output_pin = 2.6\nfan.output_type = pwm\nmisc.enable = true\nmisc.input_on_command = M42\nmisc.input_off_command = M43\n\
misc.output_pin = 2.4\nmisc.output_type = digital\nmisc.value = 123.456\nmisc.ivalue= 123\npsu.enable = false\n\
[dummy]\nenable = false");
    std::stringstream ss(str);

    ConfigReader cr;
    ConfigReader::sub_section_map_t ssmap;
    if(!cr.get_sub_sections(ss, "switch", ssmap)) {
        FAIL_M("no section found");
    }

    ASSERT_TRUE(cr.get_current_section() == "switch");

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
                ASSERT_TRUE(pin == "2.6");
                ASSERT_TRUE(input_on_command == "M106");
                ASSERT_TRUE(input_off_command == "M107");
                ASSERT_TRUE(output_on_command.empty());
                ASSERT_TRUE(output_off_command.empty());
                ASSERT_TRUE(type == "pwm");
                ASSERT_TRUE(ipb == "momentary");
                ASSERT_EQUALS_V(0, iv);
                ASSERT_EQUALS_V(0.0F, fv);
                fanok= true;

            } else if(name == "misc") {
                ASSERT_TRUE(pin == "2.4");
                ASSERT_TRUE(input_on_command == "M42");
                ASSERT_TRUE(input_off_command == "M43");
                ASSERT_TRUE(output_on_command.empty());
                ASSERT_TRUE(output_off_command.empty());
                ASSERT_TRUE(type == "digital");
                ASSERT_TRUE(ipb == "momentary");
                ASSERT_EQUALS_V(123, iv);
                ASSERT_EQUALS_V(123.456F, fv);
                miscok= true;

            } else if(name == "psu") {
                psuok= true;

            } else {
                FAIL_M("unexpected switch name");
            }
        }
    }
    ASSERT_TRUE(fanok);
    ASSERT_TRUE(miscok);
    ASSERT_TRUE(!psuok);
}
