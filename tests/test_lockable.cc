#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <fileio.h>

#include <mutex>

TEST_CASE("lockable")
{
	stdex::file fh;
	std::unique_lock<stdex::file> lk(fh, std::defer_lock);
	bool got = lk.try_lock();

	REQUIRE(got);
}