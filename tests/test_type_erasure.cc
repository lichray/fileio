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
		ssize_t write(char const*, size_t)
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

		int seek(int, stdex::file::seekdir)
		{
			errno = ENOTSUP;
			return -1;
		}

		int close() noexcept
		{
			errno = EAGAIN;
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

	stdex::file fh{faulty_stream()};
	std::array<char, 80> buf = {};

	REQUIRE_SYSTEM_ERROR(fh.read(buf.data(), buf.size()), EBUSY);
	REQUIRE_SYSTEM_ERROR(fh.write(buf.data(), buf.size()), EPERM);
	REQUIRE_SYSTEM_ERROR(fh.rewind(), ENOTSUP);
	REQUIRE_SYSTEM_ERROR(fh.close(), EAGAIN);
}
