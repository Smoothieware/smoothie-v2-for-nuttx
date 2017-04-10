#include "OutputStream.h"
#include "IODeviceSTDIO.h"

#include <cstdarg>
#include <cstring>
#include <stdio.h>

OutputStream::OutputStream() : append_nl(false), prepend_ok(false)
{
	io= new IODeviceSTDIO(); std_io= true;
};

OutputStream::~OutputStream()
{
	if(std_io) delete io;
};

OutputStream::OutputStream(const OutputStream &to_copy)
{
	io= to_copy.io;
	append_nl= to_copy.append_nl;
	prepend_ok= to_copy.prepend_ok;
	prepending= to_copy.prepending;
}

OutputStream &OutputStream::operator= (const OutputStream &to_copy)
{
	if( this != &to_copy ) {
		io= to_copy.io;
		append_nl= to_copy.append_nl;
		prepend_ok= to_copy.prepend_ok;
		prepending= to_copy.prepending;
	}
	return *this;
}

int OutputStream::printf(const char *format, ...)
{
	if(prepend_ok && strcmp(format, "FLUSH")) {
		int n= prepending.size();
		io->write(prepending.c_str(), append_nl);
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
		io->write((const char*)buffer, size);
	}

	return size;
}
