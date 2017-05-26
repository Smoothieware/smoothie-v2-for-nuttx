#include <iostream>
#include <sstream>
#include <set>
#include <tuple>

#include "prettyprint.hpp"

#include "../easyunit/test.h"

TEST(OutputStreamsTest,basic)
{
	std::cout << "Hello World!" << "\n";
	std::cout << "Hello World, " << 1.234F << " that was a number\n";
	std::set<int> s;
	s.insert(1);
	s.insert(2);
	s.insert(3);
	s.insert(4);
	
	std::cout << "set: " << s << "\n";

	auto t= std::make_tuple(1,2,3,4);
	std::cout << "tuple: " << t << "\n";
	
}


TEST(StringStreamsTest,basic)
{
	std::ostringstream oss;
	oss << "Hello World!";

	
	ASSERT_TRUE(oss.str() == "Hello World!");  
	
}

