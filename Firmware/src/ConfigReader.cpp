#include "ConfigReader.h"
#include "StringUtils.h"

// just extract the key/values from the specified section
bool ConfigReader::get_section(const char *section, section_map_t& config)
{
    reset();
    current_section=  section;
    bool in_section= false;
    while (!is.eof()) {
        std::string s;
        std::getline(is, s);

        if(!is.good()) break;
        s= stringutils::trim(s);

        // only check lines that are not blank and are not all comments
        if (s.size() > 0 && s[0] != '#') {
            std::smatch match;

            if (std::regex_search(s, match, section_test)) {
                // if this is the section we are looking for
                if(match[1] == section) {
                    in_section= true;

                }else if(in_section) {
                    // we are no longer in the section we want
                    break;
                }
            }

            if(in_section) {
                // extract all key/values from this section
                if (std::regex_search(s, match, value_test)) {
                    // set this as a key value pair on the current name
                    config[match[1]] = stringutils::trim(match[2]);
                }

            }
        }
    }

    return !config.empty();
}

// just extract the key/values from the specified section and split them into sub sections
bool ConfigReader::get_sub_sections(const char *section, sub_section_map_t& config)
{
    reset();
    current_section=  section;
    bool in_section= false;
    while (!is.eof()) {
        std::string s;
        std::getline(is, s);

        if(!is.good()) break;
        s= stringutils::trim(s);

        // only check lines that are not blank and are not all comments
        if (s.size() > 0 && s[0] != '#') {
            std::smatch match;

            if (std::regex_search(s, match, section_test)) {
                // if this is the section we are looking for
                if(match[1] == section) {
                    in_section= true;

                }else if(in_section) {
                    // we are no longer in the section we want
                    break;
                }
            }

            if(in_section) {
                // extract all key/values from this section
                // and split them into subsections
                if (std::regex_search(s, match, sub_value_test)) {
                    // set this as a key value pair on the current name
                    config[match[1]][match[2]] = stringutils::trim(match[3]);
                }

            }
        }
    }
    return !config.empty();
}

// just extract the sections
bool ConfigReader::get_sections(sections_t& config)
{
    reset();
    current_section=  "";

    while (!is.eof()) {
        std::string s;
        std::getline(is, s);

        if(!is.good()) break;
        s= stringutils::trim(s);

        // only check lines that are not blank and are not all comments
        if (s.size() > 0 && s[0] != '#') {
            std::smatch match;

            if (std::regex_search(s, match, section_test)) {
                config.insert(match[1]);
            }
        }
    }

    return !config.empty();
}

const char *ConfigReader::get_string(const section_map_t& m, const char *key, const char *def) const
{
    auto s= m.find(key);
    if(s != m.end()) {
        return s->second.c_str();
    }

    return def;
}

float ConfigReader::get_float(const section_map_t& m, const char *key, float def)
{
    auto s= m.find(key);
    if(s != m.end()) {
        return std::stof(s->second);
    }

    return def;
}

bool ConfigReader::get_bool(const section_map_t& m, const char *key, bool def)
{
    auto s= m.find(key);
    if(s != m.end()) {
        return s->second == "true";
    }

    return def;
}

int ConfigReader::get_int(const section_map_t& m, const char *key, int def)
{
    auto s= m.find(key);
    if(s != m.end()) {
        return std::stoi(s->second);
    }

    return def;
}

#if 0
#include "../TestUnits/prettyprint.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char const *argv[])
{

    std::fstream fs;
    fs.open(argv[1], std::fstream::in);
    if(!fs.is_open()) {
        std::cout << "Error opening file: " << argv[1] << "\n";
        return 0;
    }

    ConfigReader cr(fs);
    if(argc == 2) {
        ConfigReader::sections_t sections;
        if(cr.get_sections(sections)) {
            std::cout << sections << "\n";
        }

        for(auto& i : sections) {
            cr.reset();
            std::cout << i << "...\n";
            ConfigReader::section_map_t config;
            if(cr.get_section(i.c_str(), config)) {
                std::cout << config << "\n";
            }
        }

    }else if(argc == 3) {
        ConfigReader::section_map_t config;
        if(cr.get_section(argv[2], config)) {
            std::cout << config << "\n";
        }

        // see if we have sub sections
        bool is_sub_section= false;
        for(auto& i : config) {
            if(i.first.find_first_of('.') != std::string::npos) {
                is_sub_section= true;
                break;
            }
        }

        if(is_sub_section) {
            cr.reset();
            std::cout << "\nSubsections...\n";
            ConfigReader::sub_section_map_t ssmap;
            // dump sub sections too
            if(cr.get_sub_sections(argv[2], ssmap)) {
                std::cout << ssmap << "\n";
            }

            for(auto& i : ssmap) {
                std::string ss= i.first;
                std::cout << ss << ":\n";
                for(auto& j : i.second) {
                    std::cout << "  " << j.first << ": " << j.second << "\n";
                }
            }
        }
    }

    fs.close();

    return 0;
}

#else
#if 0
#include "../TestUnits/prettyprint.hpp"
#include <iostream>
#include <sstream>

int main(int argc, char const *argv[])
{

    std::string str("[switch]\nfan.enable = true\nfan.input_on_command = M106\nfan.input_off_command = M107\n\
fan.output_pin = 2.6\nfan.output_type = pwm\nmisc.enable = true\nmisc.input_on_command = M42\nmisc.input_off_command = M43\n\
misc.output_pin = 2.4\nmisc.output_type = digital\nmisc.value = 123.456\npsu.enable = false\n\
[dummy]\nenable = false");

    std::stringstream ss(str);
    ConfigReader cr(ss);
    ConfigReader::sub_section_map_t ssmap;
    if(!cr.get_sub_sections("switch", ssmap)) {
        std::cout << "no switch section found\n";
        exit(0);
    }

    for(auto& i : ssmap) {
        // foreach switch
        std::string name= i.first;
        auto& m= i.second;
        if(cr.get_bool(m, "enable", false)) {
            std::cout << "Found switch: " << name << "\n";
            //std::string input_on_command = cr.value("switch", name, "input_on_command").by_default("").as_string();
            std::string input_on_command = cr.get_string(m, "input_on_command", "");
            std::string input_off_command = cr.get_string(m, "input_off_command", "");
            std::string pin= cr.get_string(m, "output_pin", "nc");
            std::string type= cr.get_string(m, "output_type", "");
            float value= cr.get_float(m, "value", 0.0F);

            std::cout << "input_on_command: " << input_on_command << ", ";
            std::cout << "input_off_command: " << input_off_command << ", ";
            std::cout << "pin: " << pin << ", ";
            std::cout << "type: " << type << "\n";
            std::cout << "value: " << value << "\n";
        }

    }
}

#endif

#endif
