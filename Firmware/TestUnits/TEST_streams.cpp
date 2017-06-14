#include <iostream>
#include <sstream>
#include <set>
#include <tuple>

#include "OutputStream.h"

#include "prettyprint.hpp"

#include "../easyunit/test.h"


TEST(StringStreamsTest,basic)
{
	std::ostringstream oss;
	oss << "Hello World!";
	ASSERT_TRUE(oss.str() == "Hello World!");

	std::ostringstream oss2;
	oss2.write("hello", 5);
	ASSERT_TRUE(oss2.str() == "hello");
}

TEST(ostreamsTest,basic)
{
	std::cout << "Hello World!" << "\n";
	std::cout << "Hello World, " << 1.234F << " that was a number\n";
	std::set<int> s;
	s.insert(1);
	s.insert(2);
	s.insert(3);
	s.insert(4);

	// test prettyprint (also tests stringstream)
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

TEST(OutputStreamTest,null)
{
	OutputStream os;
	os.printf("hello");
}

TEST(OutputStreamTest,sstream)
{
	std::ostringstream oss;
	OutputStream os(&oss);
	os.printf("hello world");
	printf("oss = %s\n", oss.str().c_str());
	std::cout << oss.str() << "\n";
	ASSERT_EQUALS_V(0, strcmp(oss.str().c_str(), "hello world"));
}

TEST(OutputStreamTest,fdstream)
{
	OutputStream os(1); // stdout
	os.printf("hello world on fd stdout OutputStream\n");

	// also test cout
	OutputStream os2(std::cout);
	os2.printf("hello world from cout OutputStream\n");
}


