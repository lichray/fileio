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

		REQUIRE(ec.value() == EBADF);
		REQUIRE(ec.category() == std::system_category());
	}

	SECTION("attempt to print a character is also an error")
	{
		REQUIRE_SYSTEM_ERROR(fh.print(L'a'), EBADF);
	}
}

TEST_CASE("printing characters")
{
	std::string s;
	std::wstring s1 = L"Is the Order a Rabbit?";
	std::string st = "Is the Order a Rabbit?";

	SECTION("unbuffered")
	{
		file fh(test_writer{s}, opening::for_write);

		for (auto c : s1)
			fh.print(c);

		REQUIRE(s == st);
	}

	SECTION("fully buffered")
	{
		file fh(test_writer{s}, opening::for_write |
		    opening::buffered);

		for (auto c : s1)
			fh.print(c);

		REQUIRE(s.empty());

		fh.flush();
		REQUIRE(s == st);
	}

	SECTION("line buffered")
	{
		file fh(test_writer{s}, opening::for_write |
		    opening::line_buffered);

		for (auto c : s1)
			fh.print(c);

		REQUIRE(s.empty());

		fh.print(L'\n');
		REQUIRE(s == st + '\n');
	}
}
