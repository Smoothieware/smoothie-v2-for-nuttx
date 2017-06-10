#include <iostream>
#include <sstream>
#include <set>
#include <tuple>

#include "prettyprint.hpp"

#include "../easyunit/test.h"


TEST(StringStreamsTest,basic)
{
	std::ostringstream oss;
	oss << "Hello World!";
	ASSERT_TRUE(oss.str() == "Hello World!");
}

TEST(OutputStreamsTest,basic)
{
	std::cout << "Hello World!" << "\n";
	std::cout << "Hello World, " << 1.234F << " that was a number\n";
	std::set<int> s;
	s.insert(1);
	s.insert(2);
	s.insert(3);
	s.insert(4);

	// test prettyprint (also tests stringstrem)
	{
		std::ostringstream oss;
		oss << s;
		ASSERT_TRUE(oss.str() == "{1, 2, 3, 4}");
	}

	auto t= std::make_tuple(1,2,3,4);
	{
		std::ostringstream oss;
		oss << t;
		printf("tuple: %s\n", oss.str().c_str());
		ASSERT_TRUE(oss.str() == "(1, 2, 3, 4)");
	}
}



