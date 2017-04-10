#pragma once

#include <string>

/**
	Handles an output stream from gcode/mcode handlers
	can be told to append a NL at end, and also to prepend or postpend the ok
*/
class IODevice;

class OutputStream
{
public:
	OutputStream();
	OutputStream(IODevice *iodev) : io(iodev), std_io(false), append_nl(false), prepend_ok(false) {};
	virtual ~OutputStream();
	OutputStream(const OutputStream &to_copy);
	OutputStream& operator= (const OutputStream &to_copy);

	void clear() { append_nl= false; prepend_ok= false; prepending.clear(); }
	int printf(const char *format, ...);
	void setAppendNL() { append_nl= true; }
	void setPrependOK(bool flg= true) { prepend_ok= flg; }
	bool isAppendNL() const { return append_nl; }
	bool isPrependOK() const { return prepend_ok; }

private:
	IODevice* io;
	std::string prepending;
	struct {
		bool std_io:1;
		bool append_nl:1;
		bool prepend_ok:1;
	};
};
