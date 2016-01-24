#include <fileio.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using stdex::file;
using stdex::whence;
using stdex::opening;

// a reader which often reads less bytes
struct test_reader
{
	int read(char* p, int sz)
	{
		auto z = randint(1, sz);
		auto n = s.copy(p, z, pos);
		pos += n;
		return n;
	}

	explicit test_reader(std::string& sr) : s(sr)
	{}

	std::string& s;
	size_t pos = 0;
};

// a reader which gives error on the second read
struct half_faulty_reader
{
	int read(char* p, int sz)
	{
		if (times)
			return -1;
		else
		{
			++times;
			auto n = (sz + 1) / 2;
			memset(p, '@', n);
			return n;
		}
	}

	int times = 0;
};

TEST_CASE("file is not opened for read")
{
	std::string s1 = "LoveLive!";
	char s[10];
	file fh(test_reader{s1}, opening::for_write);
	file::io_result r;

	SECTION("reading no data has no error, no effect")
	{
		REQUIRE_NOTHROW(r = fh.read(s, 0));
		REQUIRE(r);
		REQUIRE(r.count() == 0);
	}

	SECTION("attempt to read some data is an error")
	{
		std::error_code ec;
		r = fh.read(s, 1, ec);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == 0);
		REQUIRE(ec.value() == EBADF);
		REQUIRE(ec.category() == std::system_category());
	}

	SECTION("attempt to get a byte is also an error")
	{
		std::error_code ec;
		char c;
		r = fh.read(c, ec);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == 0);
		REQUIRE(ec.value() == EBADF);
		REQUIRE(ec.category() == std::system_category());
	}
}

TEST_CASE("fixed length")
{
	std::string s1 = "Bokura no Live Kimi to no Life";
	char s[40];
	file::io_result r;

	CHECK(s1.size() < 40);

	SECTION("all buffered")
	{
		file fh(test_reader{s1}, opening::for_read);

		r = fh.read(s, 1);

		REQUIRE(r);
		REQUIRE(r.count() == 1);
		REQUIRE(s[0] == s1[0]);

		r = fh.read(s + 1, 40);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == s1.size() - 1);
		REQUIRE(stdex::string_view(s, s1.size()) == s1);

		char c;
		r = fh.read(c);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == 0);
	}

	SECTION("small buffer, byte by byte")
	{
		file fh(test_reader{s1}, opening::for_read, 10);

		char c;
		std::string x;
		while ((r = fh.read(c)))
			x.push_back(c);

		REQUIRE(x == s1);
	}

	SECTION("small buffer, read all")
	{
		file fh(test_reader{s1}, opening::for_read, 10);

		r = fh.read(s, 100);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == s1.size());
		REQUIRE(stdex::string_view(s, r.count()) == s1);
	}
}

TEST_CASE("swapping")
{
	std::string s1 = "Sore wa Bokutachi no Kiseki";
	char s[40];

	file fh(test_reader{s1}, opening::for_read, 10);
	file::io_result r;

	r = fh.read(s[0]);

	REQUIRE(r);
	REQUIRE(r.count() == 1);

	SECTION("swap and continue")
	{
		file f2(test_reader{s1}, opening::for_read, 15);
		char s2[40];

		auto r = f2.read(s2, 20);
		auto p = s2 + r.count();

		swap(fh, f2);

		char c;
		while ((fh.read(c)))
			*p++ = c;

		r = f2.read(s + 1, 40);

		REQUIRE(stdex::string_view(s, r.count() + 1) == s1);
		REQUIRE(stdex::string_view(s2, s1.size()) == s1);
	}
}

TEST_CASE("error reporting")
{
	errno = 0;
	char s[40];

	SECTION("ranged read")
	{
		file fh(half_faulty_reader{}, opening::for_read, 20);
		file::io_result r;

		r = fh.read(s, 30);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == 10);
	}

	SECTION("byte-wise read")
	{
		file fh(half_faulty_reader{}, opening::for_read, 1);
		file::io_result r;

		r = fh.read(s[0]);

		REQUIRE(r);
		REQUIRE(r.count() == 1);

		r = fh.read(s[1]);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == 0);
	}
}
