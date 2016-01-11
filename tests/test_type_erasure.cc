#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <fileio.h>

#include <mutex>
#include <array>

using stdex::file;
using stdex::whence;
using stdex::opening;

TEST_CASE("is_readable and is_seekable")
{
	struct fake_reader
	{
		int read(char*, unsigned int)
		{
			return 0;
		}

		int seek(int x, whence)
		{
			return x;
		}
	};

	file fh{fake_reader(), opening::for_read | opening::for_write};
	std::array<char, 80> buf;
	auto r = fh.read(buf.data(), buf.size());

	REQUIRE(fh.readable());
	REQUIRE_FALSE(fh.writable());
	REQUIRE_FALSE(r);
	REQUIRE(r.count() == 0);

	auto x = fh.seek(3, whence::beginning);

	REQUIRE(x == 3);
}

TEST_CASE("is_writable and fileno")
{
	struct fake_writer
	{
		ptrdiff_t write(char const*, size_t)
		{
			return 0;
		}
	};

	file fh{fake_writer(), opening::for_read | opening::for_write};
	std::array<char, 80> const buf = {};
	auto r = fh.write(buf.data(), buf.size());

	REQUIRE(fh.writable());
	REQUIRE_FALSE(fh.readable());
	REQUIRE_FALSE(r);
	REQUIRE(r.count() == 0);

	REQUIRE(fh.fileno() == -1);
	REQUIRE_FALSE(fh.isatty());
}

TEST_CASE("error handling")
{
	struct faulty_stream
	{
		int read(char const*, int)
		{
			errno = EBUSY;
			return -1;
		}

		int write(char const*, int)
		{
			errno = EPERM;
			return -1;
		}

		int seek(int, whence)
		{
			errno = ENOTSUP;
			return -1;
		}

		int close() noexcept
		{
			errno = EAGAIN;
			return -1;
		}

		int resize(size_t)
		{
			errno = ERANGE;
			return -1;
		}
	};

#define REQUIRE_SYSTEM_ERROR(expr, eno)                                     \
	do                                                                  \
	{                                                                   \
		Catch::ResultBuilder __catchResult(                         \
		    "REQUIRE_SYSTEM_ERROR", CATCH_INTERNAL_LINEINFO, #expr, \
		    Catch::ResultDisposition::Normal);                      \
		if (__catchResult.allowThrows())                            \
			try                                                 \
			{                                                   \
				(void) expr;                                \
				__catchResult.captureResult(                \
				    Catch::ResultWas::DidntThrowException); \
			}                                                   \
			catch (std::system_error& ex)                       \
			{                                                   \
				__catchResult.captureResult(                \
				    Catch::ResultWas::Ok);                  \
				REQUIRE(ex.code().value() == eno);          \
			}                                                   \
			catch (...)                                         \
			{                                                   \
				__catchResult.useActiveException(           \
				    Catch::ResultDisposition::Normal);      \
			}                                                   \
		else                                                        \
			__catchResult.captureResult(Catch::ResultWas::Ok);  \
		INTERNAL_CATCH_REACT(__catchResult)                         \
	} while (Catch::alwaysFalse())

	file fh{faulty_stream(), opening::for_read | opening::for_write};
	std::array<char, 80> buf = {};

	REQUIRE(fh.readable());
	REQUIRE(fh.writable());

	REQUIRE_SYSTEM_ERROR(fh.read(buf.data(), buf.size()), EBUSY);
	REQUIRE_SYSTEM_ERROR(fh.write(buf.data(), buf.size()), EPERM);
	REQUIRE_SYSTEM_ERROR(fh.rewind(), ENOTSUP);
	REQUIRE_SYSTEM_ERROR(fh.resize(0), ERANGE);
	REQUIRE_SYSTEM_ERROR(fh.truncate(), ENOTSUP);
	REQUIRE_FALSE(fh.closed());
	REQUIRE_SYSTEM_ERROR(fh.close(), EAGAIN);
	REQUIRE(fh.closed());

	// truncate is tell + resize, stops at the first error
	std::error_code ec(EBUSY, std::system_category());
	fh.truncate(ec);

	REQUIRE(ec.value() == ENOTSUP);
}
