#include "ConfigWriter.h"
#include "ConfigReader.h"
#include "StringUtils.h"

#include <cstring>

bool ConfigWriter::write(const char *section, const char* key, const char *value)
{
    // find the section we want
    bool found_section= false;
    std::iostream::pos_type pos= std::iostream::pos_type(-1);

    while (!ios.eof()) {
        std::string s;
        std::getline(ios, s);

        if(!ios.good()) break;
        s = stringutils::trim(s);

        // only check lines that are not blank and are not all comments
        if (s.size() > 0 && s[0] != '#') {
            ConfigReader::strip_comments(s);

            std::string sec;

            if (ConfigReader::match_section(s.c_str(), sec)) {
                // if this is the section we are looking for
                if(sec == section) {
                    found_section = true;

                } else if(found_section) {
                    // we are no longer in the section we want
                    pos= ios.tellg();
                    break;
                }
            }

        }
    }

    // when we get here we are either at the end of the file or we are at the end of the section we want


    return false;
}
