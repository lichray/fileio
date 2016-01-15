#include <fileio.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("standard streams properties")
{
	REQUIRE_FALSE(stdex::in.closed());
	REQUIRE(stdex::in.readable());
	REQUIRE_FALSE(stdex::in.writable());
	REQUIRE(stdex::in.fileno() == 0);

	REQUIRE_FALSE(stdex::out.closed());
	REQUIRE_FALSE(stdex::out.readable());
	REQUIRE(stdex::out.writable());
	REQUIRE(stdex::out.fileno() == 1);

	REQUIRE_FALSE(stdex::err.closed());
	REQUIRE_FALSE(stdex::err.readable());
	REQUIRE(stdex::err.writable());
	REQUIRE(stdex::err.fileno() == 2);
}
