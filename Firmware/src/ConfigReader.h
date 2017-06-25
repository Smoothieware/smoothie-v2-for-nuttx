#pragma once

#include <string>
#include <map>
#include <set>
#include <istream>

class ConfigReader
{
public:
    ConfigReader(std::istream& ist) : is(ist) {};
    ~ConfigReader(){};

    void reset() { is.clear(); is.seekg (0); }
    using section_map_t = std::map<std::string, std::string>;
    using sub_section_map_t =  std::map<std::string, section_map_t>;
    using sections_t = std::set<std::string>;
    bool get_sections(sections_t& sections);
    bool get_section(const char *section, section_map_t& config);
    bool get_sub_sections(const char *section, sub_section_map_t& config);

    const std::string& get_current_section() const { return current_section; }

    const char *get_string(const section_map_t&, const char *key, const char *def="") const;
    float get_float(const section_map_t&, const char *key, float def=0.0F);
    int get_int(const section_map_t&, const char *key, int def=0);
    bool get_bool(const section_map_t&, const char *key, bool def=false);

private:
    bool match_section(const char *line, std::string& section_name);
    bool extract_key_value(const char *line, std::string& key, std::string& value);
    bool extract_sub_key_value(const char *line, std::string& key1, std::string& key2, std::string& value);

    std::istream& is;
    std::string current_section;
};
