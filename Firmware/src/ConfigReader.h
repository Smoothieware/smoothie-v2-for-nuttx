#pragma once

#include <string>
#include <map>
#include <set>
#include <istream>

class ConfigReader
{
public:
    ConfigReader(){};
    ~ConfigReader(){};

    using section_map_t = std::map<std::string, std::string>;
    using sub_section_map_t =  std::map<std::string, std::map<std::string, std::string>>;
    using sections_t = std::set<std::string>;
    bool get_sections(std::istream& is, sections_t& sections);
    bool get_section(std::istream& is, const char *section, section_map_t& config);
    bool get_sub_sections(std::istream& is, const char *section, sub_section_map_t& config);

    const std::string& get_current_section() const { return current_section; }

    const std::string get_string(const section_map_t&, const char *key, const char *def="");
    float get_float(const section_map_t&, const char *key, float def=0.0F);
    int get_int(const section_map_t&, const char *key, int def=0);
    bool get_bool(const section_map_t&, const char *key, bool def=false);

private:
    std::string current_section;

};
