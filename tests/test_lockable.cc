#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <fileio.h>

#include <mutex>

using stdex::file;

TEST_CASE("lockable")
{
	file fh;
	std::unique_lock<file> lk(fh, std::defer_lock);
	bool got = lk.try_lock();

	REQUIRE(got);
}
