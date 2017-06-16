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

int OutputStream::flush_prepend()
{
	int n= prepending.size();
	if(n > 0) {
		os->write(prepending.c_str(), n);
		prepending.clear();
		prepend_ok= false;
	}
	return n;
}

int OutputStream::printf(const char *format, ...)
{
	if(os == nullptr) return 0;

	char buffer[132+4];
	va_list args;
	va_start(args, format);

	size_t size = vsnprintf(buffer, sizeof(buffer), format, args);

	va_end(args);

	if (size >= sizeof(buffer)-4) {
		// TODO should we append \n if he format ends in \n ?
		memcpy(&buffer[sizeof(buffer)-4], "...", 3);
		buffer[sizeof(buffer)-1]= '\0';
		size= sizeof(buffer)-1;
	}

	if(prepend_ok) {
		prepending.append(buffer, size);
	}else{
		os->write((const char*)buffer, size);
	}

	return size;
}
