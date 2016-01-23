#include <fileio.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

using stdex::file;
using stdex::open_file;
using stdex::whence;
using stdex::opening;

TEST_CASE("open local files")
{
	auto fn = random_filename("fileio_t_");

	REQUIRE_SYSTEM_ERROR(open_file(fn, "r"), ENOENT);

	// create a file with some content
	{
		auto f = open_file(fn, "w");

		REQUIRE_FALSE(f.readable());
		REQUIRE(f.writable());

		f.print("sister's voice");
	}

	// read from beginning and write to the end
	{
		auto f = open_file(fn, "a+");

		REQUIRE(f.readable());
		REQUIRE(f.writable());

		// XXX test reading
		f.print(" ima todokeru yo");
	}

	// now can read the whole sentence
	{
		auto f = open_file(fn, "r");

		REQUIRE(f.readable());
		REQUIRE_FALSE(f.writable());

		// XXX test reading
	}

	::remove(fn.data());
	REQUIRE_SYSTEM_ERROR(open_file(fn, "r+"), ENOENT);

	// "a" can also create file
	{
		auto f = open_file(fn, "a");

		REQUIRE_FALSE(f.readable());
		REQUIRE(f.writable());

		f.rewind();
		f.print("sister's noise");
	}

	// read and write
	{
		auto f = open_file(fn, "r+");

		REQUIRE(f.readable());
		REQUIRE(f.writable());

		// XXX test reading
		f.print(" hibiki hajimeru");
	}

	// truncate and write
	{
		auto f = open_file(fn, "w+");

		REQUIRE(f.readable());
		REQUIRE(f.writable());

		// XXX test truncation
		f.print('\n');
	}

	// exclusive creation
	{
		REQUIRE_SYSTEM_ERROR(open_file(fn, "x"), EEXIST);

		::remove(fn.data());
		auto f = open_file(fn, "x");

		REQUIRE_FALSE(f.readable());
		REQUIRE(f.writable());

		f.print("sister's noise");
	}

	// write and read from itself
	{
		REQUIRE_SYSTEM_ERROR(open_file(fn, "x+"), EEXIST);

		::remove(fn.data());
		auto f = open_file(fn, "x+");

		REQUIRE(f.readable());
		REQUIRE(f.writable());

		f.print("fripSide");
		f.rewind();
		// XXX test reading
	}

	::remove(fn.data());
}

TEST_CASE("invalid mode strings")
{
	auto fn = random_filename("fileio_t_");

	for (auto md : {
	    "",
	    " r",
	    "rw",
	    "wx",
	    "r+b",
	    "rb ",
	    "rt",
	    "r+," })
		REQUIRE_SYSTEM_ERROR(open_file(fn, md), EINVAL);
}

#if defined(_WIN32)
TEST_CASE("Windows-only features")
{
	auto fn = random_filename(L"fileio_t_");

	SECTION("wide-oriented I/O")
	{
		auto f = open_file(fn, "w, ccs=utf-8");

		REQUIRE_FALSE(f.readable());
		REQUIRE(f.writable());

		// write in with BOM
		std::wstring s = L"今届け";
		f.print(s);

		// files are not shared
		REQUIRE_SYSTEM_ERROR(open_file(fn, "r,ccs=UNICODE"), EACCES);

		f.close();
		// now we can read it back with any Unicode mode
		f = open_file(fn, "a+, ccs=utf-16le");

		REQUIRE(f.readable());
		REQUIRE(f.writable());

		// XXX test reading
	}

	_wremove(fn.data());
	REQUIRE_SYSTEM_ERROR(open_file(fn, "r+,  ccs=ANSI"), ENOENT);

	SECTION("invalid modes")
	{
		for (auto md : {
		    "w ,ccs=utf-8",
		    "rb, ccs=utf-8",
		    "w, ccs =utf-8",
		    "w, ccs=utf-8 ",
		    "w, ccs=what",
		    "w, ansi" })
			REQUIRE_SYSTEM_ERROR(open_file(fn, md), EINVAL);
	}
}
#endif
