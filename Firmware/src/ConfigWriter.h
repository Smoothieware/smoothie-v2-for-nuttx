#pragma once

#include <string>
#include <map>
#include <set>
#include <istream>

class ConfigWriter
{
public:
    ConfigWriter(std::iostream& iost) : ios(iost) {};
    ~ConfigWriter(){};

    bool write(const char *section, const char* key, const char *value);

private:
    std::iostream& ios;
};
