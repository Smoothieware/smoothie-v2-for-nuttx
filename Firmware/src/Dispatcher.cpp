#include "Dispatcher.h"
//#include "Kernel.h"
#include "GCode.h"
#include "GCodeProcessor.h"
#include "OutputStream.h"

#include <ctype.h>
#include <cmath>
#include <string.h>
#include <cstdarg>

using namespace std;

#define DEBUG_WARNING printf
//#define DEBUG_WARNING(...)

// NOTE this can be called recursively by commands handlers that need to issue their own commands
// it can also be called concurrently from different threads, so no changing class context, that is why it is const
bool Dispatcher::dispatch(GCode& gc) const
{
	if(gc.hasM() && gc.getCode() == 503) {
		// alias M503 to M500.3
		gc.setCommand('M', 500, 3);
	}

	auto& handler= gc.hasG() ? gcode_handlers : mcode_handlers;
	const auto& f= handler.equal_range(gc.getCode());
	bool ret= false;

	for (auto it=f.first; it!=f.second; ++it) {
		if(it->second(gc)) {
			ret= true;
		}else{
			DEBUG_WARNING("handler did not handle %c%d\n", gc.hasG() ? 'G':'M', gc.getCode());
		}
	}

	// special case is M500 - M503
	if(gc.hasM() && gc.getCode() >= 500 && gc.getCode() <= 503) {
		ret= handleConfigurationCommands(gc);
	}

	if(ret) {
		// get output stream
		OutputStream output_stream;

		bool send_ok= true;
		if(output_stream.isPrependOK()) {
			// output the result after the ok
			output_stream.setPrependOK(false);
			output_stream.printf("ok ");
			output_stream.printf("FLUSH"); // this flushes the internally stored string to the IO Device
			send_ok= false;
		}

		if(output_stream.isAppendNL()) {
			// append newline
			output_stream.printf("\r\n");
		}

		if(send_ok) {
			output_stream.printf("ok\r\n");
		}

		return true;
	}

	return false;
}

// convenience to dispatch a one off command
// Usage: dispatch('M', 123, [subcode,] 'X', 456, 'Y', 789, ..., 0); // must terminate with 0
// dispatch('M', 123, 0);
bool Dispatcher::dispatch(char cmd, uint16_t code, ...) const
{
	GCode gc;
    va_list args;
    va_start(args, code);
    char c= va_arg(args, int); // get first arg
    if(c > 0 && c < 'A') { // infer subcommand
		gc.setCommand(cmd, code, (uint16_t)c);
		c= va_arg(args, int); // get next arg
	}else{
		gc.setCommand(cmd, code);
	}

    while(c != 0) {
    	float v= (float)va_arg(args, double);
    	gc.addArg(c, v);
    	c= va_arg(args, int); // get next arg
    }

    va_end(args);
    return dispatch(gc);
}

Dispatcher::Handlers_t::iterator Dispatcher::addHandler(HANDLER_NAME gcode, uint16_t code, Handler_t fnc)
{
	Handlers_t::iterator ret;
	switch(gcode) {
		case GCODE_HANDLER: ret= gcode_handlers.insert( Handlers_t::value_type(code, fnc) ); break;
		case MCODE_HANDLER: ret= mcode_handlers.insert( Handlers_t::value_type(code, fnc) ); break;
	}
	return ret;
}

void Dispatcher::removeHandler(HANDLER_NAME gcode, Handlers_t::iterator i)
{
	switch(gcode) {
		case GCODE_HANDLER: gcode_handlers.erase(i); break;
		case MCODE_HANDLER: mcode_handlers.erase(i); break;
	}
}

// mainly used for testing
void Dispatcher::clearHandlers()
{
	gcode_handlers.clear();
	mcode_handlers.clear();
}

bool Dispatcher::handleConfigurationCommands(GCode& gc) const
{
	// if(gc.getCode() == 500) {
	// 	if(gc.getSubcode() == 3) {
	// 		if(loaded_configuration)
	// 			gc.getOS().printf("// Saved configuration is active\n");
	// 		// just print it
	// 		return true;

	// 	}else if(gc.getSubcode() == 0){
	// 		return writeConfiguration(gc.getOS());
	// 	}

	// }else if(gc.getCode() == 501) {
	// 	return loadConfiguration(gc.getOS());

	// }else if(gc.getCode() == 502) {
	// 	// delete the saved configuration
	// 	uint32_t zero= 0xFFFFFFFFUL;
	// 	if(THEKERNEL.nonVolatileWrite(&zero, 4, 0) == 4) {
	// 		gc.getOS().printf("// Saved configuration deleted - reset to restore defaults\n");
	// 	}else{
	// 		gc.getOS().printf("// Failed to delete saved configuration\n");
	// 	}
	// 	return true;
	// }

	return false;
}

bool Dispatcher::writeConfiguration(OutputStream& output_stream) const
{
	// write stream to non volatile memory
	// prepend magic number so we know there is a valid configuration saved
	// std::string str= "CONF";
	// str.append(output_stream.str());
	// str.append(4, 0); // terminate with 4 nuls
	// output_stream.clear(); // clear to save some memory
	// size_t n= str.size();
	// size_t r= THEKERNEL.nonVolatileWrite((void *)str.data(), n, 0);
	// if(r == n) {
	// 	output_stream.printf("// Configuration saved\n");
	// }else{
	// 	output_stream.printf("// Failed to save configuration, needed to write %d, wrote %d\n", n, r);
	// }
	return true;
}

bool Dispatcher::loadConfiguration() const
{
	OutputStream os;
	return loadConfiguration(os);
}

bool Dispatcher::loadConfiguration(OutputStream& output_stream) const
{
	// load configuration from non volatile memory
	// char buf[64];
	// size_t r= THEKERNEL.nonVolatileRead(buf, 4, 0);
	// if(r == 4) {
	// 	if(strncmp(buf, "CONF", 4) == 0) {
	// 		size_t off= 4;
	// 		// read everything until we get some nulls
	// 		std::string str;
	// 		do {
	// 			r= THEKERNEL.nonVolatileRead(buf, sizeof(buf), off);
	// 			if(r != sizeof(buf)) {
	// 				output_stream.printf("// Read failed\n");
	// 				return true;
	// 			}
	// 			str.append(buf, r);
	// 			off += r;
	// 		}while(str.find('\0') == string::npos);

	// 		// foreach line dispatch it
	// 		std::stringstream ss(str);
	// 		std::string line;
	// 		std::vector<string> lines;
	// 		GCodeProcessor& gp= THEKERNEL.getGCodeProcessor();
	// 	    while(std::getline(ss, line, '\n')){
 //  				if(line.find('\0') != string::npos) break; // hit the end
 //  				lines.push_back(line);
 //  				// Parse the Gcode
	// 			GCodeProcessor::GCodes_t gcodes;
	// 			gp.parse(line.c_str(), gcodes);
	// 			// dispatch it
	// 			for(auto& i : gcodes) {
	// 				if(i.getCode() >= 500 && i.getCode() <= 503) continue; // avoid recursion death
	// 				dispatch(i);
	// 			}
	// 		}
	// 		for(auto& s : lines) {
	// 			output_stream.printf("// Loaded %s\n", s.c_str());
	// 		}
	// 		loaded_configuration= true;

	// 	}else{
	// 		output_stream.printf("// No saved configuration\n");
	// 		loaded_configuration= false;
	// 	}

	// }else{
	// 	output_stream.printf("// Failed to read saved configuration\n");
	// }
	return true;
}
