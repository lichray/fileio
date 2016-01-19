#include <fileio.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

using stdex::file;
using stdex::whence;
using stdex::opening;

// a writer which often writes less bytes
struct test_writer
{
	int write(char const* p, int sz)
	{
		auto z = randint(0, sz);
		s.append(p, z);
		return z;
	}

	std::string& s;
};

TEST_CASE("file is not opened for write")
{
	std::string s;
	file fh(test_writer{s}, opening::for_read);

	SECTION("writing no data has no error, no effect")
	{
		REQUIRE_NOTHROW(fh.print(L""));
	}

	SECTION("attempt to print something is an error")
	{
		std::error_code ec;
		fh.print(L"x", ec);

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

TEST_CASE("printing strings")
{
	std::string s;

	SECTION("unbuffered")
	{
		file fh(test_writer{s}, opening::for_write);

		for (int len : { 10, 4096, 6000 })
		{
			auto s1 = random_text(len, L"0123456789");
			std::string st(begin(s1), end(s1));

			fh.print(s1);

			REQUIRE(s == st);
			s.clear();
		}
	}

	SECTION("fully buffered")
	{
		file fh(test_writer{s}, opening::for_write, 157);

		for (int len : { 0, 1, 100, 157, 158, 159, 1000 })
		{
			auto s1 = random_text(len, L"0123456789abcdef");
			std::string st(begin(s1), end(s1));

			fh.print(s1);
			fh.flush();

			REQUIRE(s == st);
			s.clear();
		}

		std::string st;
		for (int len : { 1, 100, 157, 158, 159, 1000 })
		{
			auto s1 = random_text(len, L"0123456789abcdef");
			st.append(begin(s1), end(s1));

			if (len % 2)
				for (auto c : s1)
					fh.put(c);
			else
				fh.print(s1);
		}

		fh.flush();
		REQUIRE(s == st);
	}

	SECTION("line buffered")
	{
		file fh(test_writer{s}, opening::for_write |
		    opening::line_buffered, 180);

		// without line feed
		{
			auto s1 = random_text(300, L"0123456789abcdef");

			fh.print(s1);
			fh.flush();

			REQUIRE(s == std::string(begin(s1), end(s1)));
			s.clear();
		}

		std::string st;

		// make the all inputs smaller than the buffer
		for (int len : { 1, 100, 157, 159, 158 })
		{
			auto s1 = random_text(len, L"0123456789abcdef\n");
			st.append(begin(s1), end(s1));

			if (len % 2)
				for (auto c : s1)
					fh.put(c);
			else
				fh.print(s1);

			auto nlpos = s.rfind('\n');
			if (nlpos != s.npos)
				REQUIRE(nlpos == s.size() - 1);
		}

		stdex::string_view sv(st.data(), st.rfind('\n') + 1);
		REQUIRE(s == sv);
	}
}
