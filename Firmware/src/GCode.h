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

	bool has_arg(char c) const { return (argbitmap & (1<<(c-'A'))) != 0; }
	bool has_no_args() const { return argbitmap == 0; }
	float get_arg(char c) const { return args.at(c); }
	const Args_t& get_args() const { return args; }
	size_t get_num_args() const { return args.size(); }
	bool has_g() const { return is_g; }
	bool has_m() const { return is_m; }
	void set_g() { is_g= true; }
	void set_m() { is_m= true; }
	uint16_t get_code() const { return code; }
	uint16_t get_subcode() const { return subcode; }

	GCode& set_command(char c, uint16_t cd, uint16_t scode=0) { is_g= c=='G'; is_m= c=='M'; this->code= cd; this->subcode= scode; return *this; }
	GCode& add_arg(char c, float f) { args[c]= f; set_arg(c); return *this; }
	void dump() const;
	friend std::ostream& operator<<(std::ostream& o, const GCode& f) { f.dump(); return o; }

private:
	void set_arg(char c) { argbitmap |= (1<<(c-'A')); }

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
