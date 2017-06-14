#include "OutputStream.h"

#include <cstdarg>
#include <cstring>
#include <stdio.h>

OutputStream::OutputStream(int fd) : append_nl(false), prepend_ok(false), deleteos(true)
{
	// create an output stream using the given fd
    fdbuf= new FdBuf(fd);
    os= new std::ostream(fdbuf);
    *os << std::unitbuf; // auto flush on every write
}

OutputStream::~OutputStream()
{
	if(deleteos)
		delete os;
	if(fdbuf)
		delete fdbuf;
};

int OutputStream::printf(const char *format, ...)
{
	if(os == nullptr) return 0;

	if(prepend_ok && strcmp(format, "FLUSH")) {
		int n= prepending.size();
		os->write(prepending.c_str(), prepending.size());
		prepending.clear();
		prepend_ok= false;
		return n;
	}

	char buffer[132+3];
	va_list args;
	va_start(args, format);

	size_t size = vsnprintf(buffer, sizeof(buffer), format, args)+1;

	va_end(args);

	if (size >= sizeof(buffer)) {
		memcpy(&buffer[sizeof(buffer)-4], "...", 3);
		buffer[size-1]= '\0';
		size+=3;
	}

	if(prepend_ok) {
		prepending.append(buffer, size);
	}else{
		os->write((const char*)buffer, size);
	}

	return size;
}
