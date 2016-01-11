#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <fileio.h>

TEST_CASE("standard streams properties")
{
	REQUIRE_FALSE(stdex::_in.closed());
	REQUIRE(stdex::_in.readable());
	REQUIRE_FALSE(stdex::_in.writable());
	REQUIRE(stdex::_in.fileno() == 0);

	REQUIRE_FALSE(stdex::_out.closed());
	REQUIRE_FALSE(stdex::_out.readable());
	REQUIRE(stdex::_out.writable());
	REQUIRE(stdex::_out.fileno() == 1);

	REQUIRE_FALSE(stdex::_err.closed());
	REQUIRE_FALSE(stdex::_err.readable());
	REQUIRE(stdex::_err.writable());
	REQUIRE(stdex::_err.fileno() == 2);
}
