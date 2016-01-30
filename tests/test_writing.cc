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

// a writer which gives error on the second write
struct half_faulty_writer
{
	int write(char const* p, int sz)
	{
		if (times)
			return -1;
		else
		{
			++times;
			return (sz + 1) / 2;
		}
	}

	int times = 0;
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

	SECTION("attempt to put a byte is also an error")
	{
		std::error_code ec;
		r = fh.write('x', ec);

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

	r = fh.write('!');

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
	CHECK(s1.size() > 12);
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
		REQUIRE(s == (s1 + s2).substr(0, 22));

		fh.close();
		REQUIRE(s == s1 + s2);
	}

	SECTION("opened with a desired length")
	{
		// has the same effect as long as the underlying
		// writer is not a TTY
		file fh(test_writer{s}, opening::for_write, 12);
		file::io_result r;

		for (auto c : s1)
			fh.write(c);

		REQUIRE(s == s1.substr(0, 12));

		fh.flush();
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

		r = fh.write(s2.data(), s2.size());

		REQUIRE(r);
		REQUIRE(r.count() == s2.size());
		REQUIRE(s == (s1 + s2).substr(0, 40));

		/* so far same as fully buffered */

		r = fh.write("\n", 1);

		REQUIRE(r);
		REQUIRE(r.count() == 1);
		REQUIRE(s == s1 + s2 + "\n");

		s.clear();

		SECTION("write across NL, then put")
		{
			fh.write(s3.data(), s3.size());
			r = fh.write(s4.data(), s4.size());

			REQUIRE(r);
			REQUIRE(r.count() == s4.size());
			REQUIRE(s == s3 + s4.substr(0, s4.find('\n') + 1));

			for (auto c : s5)
				fh.write(c);

			REQUIRE(s == s3 + s4 + s5);
		}

		SECTION("put across NL, then write")
		{
			for (auto c : s3)
				fh.write(c);
			for (auto c : s4)
				fh.write(c);

			REQUIRE(s == s3 + s4.substr(0, s4.find('\n') + 1));

			r = fh.write(s5.data(), s5.size());

			REQUIRE(r);
			REQUIRE(r.count() == s5.size());
			REQUIRE(s == s3 + s4 + s5);
		}

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

TEST_CASE("moving and swapping")
{
	std::string s;
	std::string s1{"", 1};

	file fh(test_writer{s}, opening::for_write | opening::fully_buffered);
	file::io_result r;

	r = fh.write("", 1);

	REQUIRE(r);
	REQUIRE(r.count() == 1);
	REQUIRE(s.empty());

	SECTION("move assignment flushes, swapping does not")
	{
		file f2(test_writer{s},
		    opening::for_write | opening::fully_buffered);

		f2.write("\n", 2);
		REQUIRE(s.empty());

		swap(fh, f2);
		REQUIRE(s.empty());

		f2 = {};
		REQUIRE(s == s1);
		REQUIRE(f2.closed());

		f2 = std::move(fh);
		REQUIRE(s == s1);
	}

	SECTION("closing making the file not writable")
	{
		REQUIRE(s.empty());
		REQUIRE(fh.writable());

		// calling twice has no effect
		fh.close();
		fh.close();

		REQUIRE(s == s1);
		REQUIRE_FALSE(fh.writable());
	}
}

TEST_CASE("error reporting")
{
	errno = 0;
	std::string s1 = "Wonderful Rush";

	SECTION("unbuffered ranged write")
	{
		file fh(half_faulty_writer{}, opening::for_write);
		file::io_result r;

		r = fh.write(s1.data(), s1.size());

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == s1.size() / 2);
	}

	SECTION("unbuffered byte-wise write")
	{
		file fh(half_faulty_writer{}, opening::for_write);
		file::io_result r;

		r = fh.write(s1[0]);

		REQUIRE(r);
		REQUIRE(r.count() == 1);

		r = fh.write(s1[1]);

		REQUIRE_FALSE(r);
		REQUIRE(r.count() == 0);
	}
}
