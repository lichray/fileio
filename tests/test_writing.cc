#include <fileio.h>

#include "test_data.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

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
	file::io_result r;

	SECTION("writing no data has no error, no effect")
	{
		REQUIRE_NOTHROW(r = fh.write("x", 0));
		REQUIRE(r);
		REQUIRE(r.count() == 0);
	}

	SECTION("attempt to write some data is an error")
	{
		std::error_code ec;
		r = fh.write("x", 1, ec);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == 0);
		REQUIRE(ec.value() == EBADF);
		REQUIRE(ec.category() == std::system_category());
	}
}

TEST_CASE("file is unbuffered")
{
	errno = 0;

	std::string s;
	std::string s1 = "Ginger ale";
	file fh(test_writer{s}, opening::for_write | opening::append_mode);
	file::io_result r;

	r = fh.write(s1.data(), s1.size());

	REQUIRE(r);
	REQUIRE(r.count() == s1.size());
	REQUIRE(s == s1);
	REQUIRE(errno == 0);

	r = fh.write("!", 1);

	REQUIRE(r);
	REQUIRE(r.count() == 1);
	REQUIRE(s == s1 + '!');

	r = fh.write("x", 0);

	REQUIRE(r);
	REQUIRE(r.count() == 0);
	REQUIRE(s == s1 + '!');
}

TEST_CASE("file is fully buffered")
{
	std::string s;
	std::string s1 = "A long time ago\n";
	std::string s2 = "in a galaxy far far away";

	auto tt = s1.size() + s2.size();
	CHECK(s1.size() < 21);
	CHECK(tt > 21);
	CHECK(tt < 42);

	SECTION("opened as fully_buffered with a desired length")
	{
		file fh(test_writer{s},
		    opening::for_write | opening::fully_buffered, 21);
		file::io_result r;

		r = fh.write(s1.data(), s1.size());

		REQUIRE(r);
		REQUIRE(r.count() == s1.size());
		REQUIRE(s.empty());

		r = fh.write(s2.data(), s2.size());

		REQUIRE(r);
		REQUIRE(r.count() == s2.size());
		REQUIRE(s == (s1 + s2).substr(0, 21));

		fh.close();
		REQUIRE(s == s1 + s2);
	}

	SECTION("opened with a desired length")
	{
		// has the same effect as long as the underlying
		// writer is not a TTY
		file fh(test_writer{s}, opening::for_write, 21);
		file::io_result r;

		fh.write(s1.data(), s1.size());
		REQUIRE(s.empty());

		fh.close();
		REQUIRE(s == s1);
	}
}

TEST_CASE("file is line buffered")
{
	std::string s;
	std::string s1 = "I am the bone of my sword";
	std::string s2 = "Steel is my body and fire is my blood";
	std::string s3 = "I have created over a thousand blades";
	std::string s4 = "Unknown to Death,\nNor known to Life";
	std::string s5 = "Have withstood pain to create many weapons\n";
	std::string s6 = "Yet, those hands will never hold anything\n";
	std::string s7 = "So as I pray, unlimited blade works.";

	CHECK(s2.size() < 40);
	CHECK(s5.size() > 40);
	CHECK(s6.size() > 40);
	CHECK(s7.size() < 40);

	{
		file fh(test_writer{s},
		    opening::for_write | opening::line_buffered, 40);
		file::io_result r;

		r = fh.write(s1.data(), s1.size());

		REQUIRE(r);
		REQUIRE(r.count() == s1.size());
		REQUIRE(s.empty());

		fh.write(s2.data(), s2.size());
		r = fh.write("\n", 1);

		REQUIRE(r);
		REQUIRE(r.count() == 1);
		REQUIRE(s == s1 + s2 + "\n");

		s.clear();
		fh.write(s3.data(), s3.size());
		r = fh.write(s4.data(), s4.size());

		REQUIRE(r);
		REQUIRE(r.count() == s4.size());
		REQUIRE(s == s3 + s4.substr(0, s4.find('\n') + 1));

		s.clear();
		r = fh.write(s5.data(), s5.size());

		REQUIRE(r);
		REQUIRE(r.count() == s5.size());
		REQUIRE(s == s4.substr(s4.find('\n') + 1) + s5);

		s.clear();
		r = fh.write(s6.data(), s6.size());

		REQUIRE(r);
		REQUIRE(r.count() == s6.size());
		REQUIRE(s == s6);

		s.clear();
		fh.write(s7.data(), s7.size());

		REQUIRE(s.empty());
	}

	// flushed upon destruction
	REQUIRE(s == s7);
}
