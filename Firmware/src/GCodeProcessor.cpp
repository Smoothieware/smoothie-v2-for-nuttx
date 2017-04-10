// gcode processor and dispatcher

#include "GCodeProcessor.h"
#include "GCode.h"

#include <map>
#include <vector>
#include <stdint.h>
#include <ctype.h>
#include <cmath>
#include <cstdlib>
#include <cstring>


using namespace std;

static tuple<uint16_t, uint16_t, float> parseCode(const char *&p)
{
	int a= 0, b= 0;
	float f= strtof(p, nullptr);
	while(*p && isdigit(*p)) {
		a = (a*10) + (*p-'0');
		++p;
	}
	if(*p == '.') {
		++p;
		while(*p && isdigit(*p)) {
			b = (b*10) + (*p-'0');
			++p;
		}
	}
	return make_tuple(a, b, f);
}

GCodeProcessor::GCodeProcessor()
{
	line_no= -1;
}

GCodeProcessor::~GCodeProcessor() {}

// Parse the line containing 1 or more gcode words
bool GCodeProcessor::parse(const char *line, GCodes_t& gcodes)
{
	GCode gc;
	bool start= true;
	const char *p= line;
	const char *eos= line + strlen(line);
	int ln= 0;
	int cs= 0;
	int checksum;

	// deal with line numbers and checksums before we parse
    if(*p == 'N') {
    	char *pp;

    	// Get linenumber
        ln = strtol(p+1, &pp, 10);
        // get checksum
        char *csp= strchr(pp, '*');
        if(csp == nullptr) {
        	checksum= 0;
        }else{
        	checksum= strtol(csp+1, nullptr, 10);
        	eos= csp; // terminate line at checksum start
        }

 	    // if it is M110: Set Current Line Number
        if(strncmp(pp+1, "M110", 4) == 0) {
            line_no= ln;
            return true;
        }

        // Calculate checksum of string
		while(p != eos) {
            cs = cs ^ *p++;
        }
        cs &= 0x00ff;
        cs -= checksum;
        p = pp;

    } else {
        //Assume checks succeeded
        cs = 0x00;
        ln = line_no + 1;
    }

    // check the checksum
    int nextline = line_no + 1;
    if(cs == 0x00 && ln == nextline) {
        line_no = nextline;

    }else{
    	// checksum failed
    	return false;
    }

	while(p != eos) {
		if(isspace(*p)){
			++p;
			continue;
		}
		if(*p == ';') break; // skip rest of line for comment
		if(*p == '(') {
			// skip comments in ( ... )
			while(*p != '\0' && *p != ')') ++p;
			if(*p) ++p;
			continue;
		}

		char c= toupper(*p++);
		if((c == 'G' || c == 'M') && !start) {
			gcodes.push_back(gc);
			gc.clear();
			start= true;
		}

		// extract gcode word G01{.123}
		tuple<uint16_t, uint16_t, float> code= parseCode(p);
		if(start) {
			if(c == 'G' || c == 'M'){
				gc.setCommand(c, get<0>(code), get<1>(code));
				if(c == 'G' && get<0>(code) <= 3) {
					group1.clear();
					group1.setCommand(c, get<0>(code), get<1>(code));
				}

			}else{
				gc.setCommand('G', group1.getCode(), group1.getSubcode()); // modal group1, copies G code and subcode for this line
				gc.addArg(c, get<2>(code));
			}

			start= false;

		}else{
			gc.addArg(c, get<2>(code));
		}
	}
	gcodes.push_back(gc);
	return true;
}
