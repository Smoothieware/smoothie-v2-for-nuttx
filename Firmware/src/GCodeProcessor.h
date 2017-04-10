#pragma once

#include <vector>

#include "GCode.h"

class GCodeProcessor
{
public:
	GCodeProcessor();
	~GCodeProcessor();

	using GCodes_t = std::vector<GCode>;

	bool parse(const char *line, GCodes_t& gcodes);
	int getLineNumber() const { return line_no; }

private:
	// modal settings
	GCode group0, group1;
	int line_no;
};
