#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <fileio.h>

#include <mutex>
#include <array>

TEST_CASE("is_readable and is_seekable")
{
	struct fake_reader
	{
		int read(char*, unsigned int)
		{
			return 0;
		}

		int seek(int x, stdex::file::seekdir)
		{
			return x;
		}
	};

	stdex::file fh{fake_reader()};
	std::array<char, 80> buf;
	auto r = fh.read(buf.data(), buf.size());

	REQUIRE_FALSE(r);
	REQUIRE(r.count() == 0);

	auto x = fh.seek(3, stdex::file::beginning);

	REQUIRE(x == 3);
}

TEST_CASE("is_writable")
{
	struct fake_writer
	{
		int write(char const*, unsigned int)
		{
			return 0;
		}
	};

	stdex::file fh{fake_writer()};
	std::array<char, 80> const buf = {};
	auto r = fh.write(buf.data(), buf.size());

	REQUIRE_FALSE(r);
	REQUIRE(r.count() == 0);
}
