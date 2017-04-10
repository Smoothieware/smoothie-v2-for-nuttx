#pragma once

#include <map>
#include <stdint.h>

class OutputStream;

class GCode
{
public:
	GCode();
	~GCode(){};

	void clear();

	using Args_t = std::map<char,float>;

	bool hasArg(char c) const { return (argbitmap & (1<<(c-'A'))) != 0; }
	bool hasNoArgs() const { return argbitmap == 0; }
	float getArg(char c) const { return args.at(c); }
	const Args_t& getArgs() const { return args; }

	bool hasG() const { return is_g; }
	bool hasM() const { return is_m; }
	void setG() { is_g= true; }
	void setM() { is_m= true; }
	uint16_t getCode() const { return code; }
	uint16_t getSubcode() const { return subcode; }

	GCode& setCommand(char c, uint16_t cd, uint16_t scode=0) { is_g= c=='G'; is_m= c=='M'; this->code= cd; this->subcode= scode; return *this; }
	GCode& addArg(char c, float f) { args[c]= f; setArg(c); return *this; }
	void dump() const;
	//friend std::ostream& operator<<(std::ostream& o, const GCode& f) { f.dump(o); return o; }

private:
	void setArg(char c) { argbitmap |= (1<<(c-'A')); }

	// one bit per argument letter, for quick lookup to see if a specific argument is specified
	uint64_t argbitmap;

	// map of actual argument/value pairs
	Args_t args;
	uint16_t code, subcode;

	struct {
		bool is_g:1;
		bool is_m:1;
		bool is_t:1;
		bool is_modal:1;
		bool is_immediate:1;
	};

};
