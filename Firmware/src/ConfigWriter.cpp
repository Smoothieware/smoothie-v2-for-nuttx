#include "ConfigWriter.h"
#include "ConfigReader.h"
#include "StringUtils.h"

#include <cstring>

/*
 * We create a new config.ini file
 * copy all lines to the new file until we hit a match for the line we want to change
 * write the new line to the file then copy the rest of the lines
 * then rename current file to config.ini.bak
 * rename the new file to config.ini
 *
 * if we do not find the line we want we insert it at the end of the section
 * then copy the rest of the file
 *
 * This is easier than trying to insert or update the line in place
 *
 */

bool ConfigWriter::write(const char *section, const char* key, const char *value)
{
    bool in_section = false;
    bool left_section = false;
    bool changed_line = false;

    // first find the section we want
    while (!is.eof()) {
        std::string s;

        std::getline(is, s);

        if(!is.good()) {
            if(is.eof()) {
                // make sure we don't need to add a new section
                if(!changed_line) {
                    std::string new_line;
                    if(!in_section) {
                        // need to add new section at the end of the file, unless we were in the last section
                        new_line.push_back('[');
                        new_line.append(section);
                        new_line.append("]\n");
                    }
                    new_line.append(key);
                    new_line.append(" = ");
                    new_line.append(value);
                    new_line.push_back('\n');
                    os.write(new_line.c_str(), new_line.size());
                    return os.good();
                }
                return true;
            }
            return false;
        }

        if(changed_line) {
            // just copy the lines. no need to check anymore
            s.push_back('\n');
            os.write(s.c_str(), s.size());
            if(!os.good()) return false;
            continue;
        }

        std::string sn = stringutils::trim(s);

        // only check lines that are not blank and are not all comments
        if (sn.size() > 0 && sn[0] != '#') {
            ConfigReader::strip_comments(sn);

            std::string sec;

            if (ConfigReader::match_section(sn.c_str(), sec)) {
                // if this is the section we are looking for
                if(sec == section) {
                    in_section = true;

                } else if(in_section) {
                    // we are no longer in the section we want
                    left_section = true;
                    in_section = false;
                }
            }

            if(in_section) {
                std::string k;
                std::string v;
                // extract key/value from this line and check if it is the one we want
                if(ConfigReader::extract_key_value(sn.c_str(), k, v) && k == key) {
                    // this is the one we want to change
                    // write the new key value
                    // then copy the rest of the file
                    std::string new_line(key);
                    new_line.append(" = ");
                    new_line.append(value);
                    new_line.push_back('\n');
                    os.write(new_line.c_str(), new_line.size());
                    if(!os.good()) return false;
                    changed_line = true;

                } else {
                    // in section but not the key/value we want or not a key/value just write it out
                    s.push_back('\n');
                    os.write(s.c_str(), s.size());
                    if(!os.good()) return false;
                }

            } else if(left_section) {
                // we just left the section, and we did not find the key/value so we write the new one
                // TODO this leaves a space after the last entry if there was one in the source file
                std::string new_line(key);
                new_line.append(" = ");
                new_line.append(value);
                new_line.append("\n\n");
                // add the last read section header
                new_line.append(s);
                new_line.push_back('\n');
                os.write(new_line.c_str(), new_line.size());
                if(!os.good()) return false;
                changed_line = true;

            } else {
                // not in the section of interest so just copy lines
                s.push_back('\n');
                os.write(s.c_str(), s.size());
                if(!os.good()) return false;
            }

        } else {
            // not a line of interest so just copy line
            s.push_back('\n');
            os.write(s.c_str(), s.size());
            if(!os.good()) return false;
        }
    }

    return true;;
}

#if 1
#include <iostream>
#include <fstream>

int main(int argc, char const *argv[])
{
    if(argc < 6) {
        std::cout << "Usage: infile outfile section key value\n";
        return 0;
    }

    std::fstream fsin;
    std::fstream fsout;
    fsin.open(argv[1], std::fstream::in);
    if(!fsin.is_open()) {
        std::cout << "Error opening file: " << argv[1] << "\n";
        return 0;
    }

    fsout.open(argv[2], std::fstream::out);
    if(!fsout.is_open()) {
        std::cout << "Error opening file: " << argv[2] << "\n";
        fsin.close();
        return 0;
    }

    ConfigWriter cw(fsin, fsout);

    const char *section = argv[3];
    const char *key = argv[4];
    const char *value = argv[5];

    if(cw.write(section, key, value)) {
        std::cout << "added ok\n";
    } else {
        std::cout << "failed to add\n";
        std::cout << "fsin: good()=" << fsin.good();
        std::cout << " eof()=" << fsin.eof();
        std::cout << " fail()=" << fsin.fail();
        std::cout << " bad()=" << fsin.bad() << "\n";

        std::cout << "failed to add\n";
        std::cout << "fsout: good()=" << fsout.good();
        std::cout << " eof()=" << fsout.eof();
        std::cout << " fail()=" << fsout.fail();
        std::cout << " bad()=" << fsout.bad();
    }

    fsin.close();
    fsout.close();

    return 1;
}
#endif
