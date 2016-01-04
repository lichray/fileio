#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <fileio.h>

TEST_CASE("standard streams properties")
{
	REQUIRE_FALSE(stdex::_in.closed());
	REQUIRE(stdex::_in.readable());
	REQUIRE_FALSE(stdex::_in.writable());

	REQUIRE_FALSE(stdex::_out.closed());
	REQUIRE_FALSE(stdex::_out.readable());
	REQUIRE(stdex::_out.writable());

	REQUIRE_FALSE(stdex::_err.closed());
	REQUIRE_FALSE(stdex::_err.readable());
	REQUIRE(stdex::_out.writable());
}
