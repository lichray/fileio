#include <fileio.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using stdex::file;
using stdex::whence;
using stdex::opening;

struct test_writer
{
	int write(char const* p, int sz)
	{
		s.append(p, sz);
		return sz;
	}

	std::string& s;
};

TEST_CASE("file is not opened for write")
{
	std::string s;
	file fh(test_writer{s}, opening::for_read);

	SECTION("writing no data has no error, no effect")
	{
		REQUIRE_NOTHROW(fh.print(""));
	}

	SECTION("attempt to print something is an error")
	{
		std::error_code ec;
		fh.print("x", ec);

		REQUIRE(ec == std::errc::bad_file_descriptor);
	}

	SECTION("attempt to print a character is also an error")
	{
		std::error_code ec;
		fh.print('x', ec);

		REQUIRE(ec == std::errc::bad_file_descriptor);

		REQUIRE_SYSTEM_ERROR(fh.print('a'),
		    std::errc::bad_file_descriptor);
	}
}

TEST_CASE("interaction with buffering")
{
	std::string s;
	std::string s1 = "hello, world\n";

	SECTION("unbuffered")
	{
		file fh(test_writer{s}, opening::for_write);

		fh.print(s1);
		REQUIRE(s == s1);

		fh.print('!');
		REQUIRE(s == s1 + '!');
	}

	SECTION("fully buffered")
	{
		file fh(test_writer{s}, opening::for_write |
		    opening::buffered);

		fh.print(s1);
		REQUIRE(s.empty());

		fh.print('!');
		REQUIRE(s.empty());

		fh.flush();
		REQUIRE(s == s1 + '!');
	}

	SECTION("line buffered")
	{
		file fh(test_writer{s}, opening::for_write |
		    opening::line_buffered);

		fh.print(s1);
		REQUIRE(s == s1);

		fh.print('!');
		REQUIRE(s == s1);

		fh.flush();
		REQUIRE(s == s1 + '!');
	}
}
