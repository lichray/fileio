#include <fileio.h>
#include <fileio/functional.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <array>

using stdex::file;
using stdex::whence;
using stdex::opening;

TEST_CASE("is_readable and is_seekable")
{
	struct fake_reader
	{
		int read(char*, unsigned int x)
		{
			return x;
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
	REQUIRE(r);
	REQUIRE(r.count() == buf.size());

	auto x = fh.seek(3, whence::beginning);

	REQUIRE(x == 3);
}

TEST_CASE("is_writable and fileno")
{
	struct fake_writer
	{
		ptrdiff_t write(char const*, size_t x)
		{
			return x;
		}
	};

	file fh{fake_writer(), opening::for_read | opening::for_write};
	std::array<char, 80> const buf = {};
	auto r = fh.write(buf.data(), buf.size());

	REQUIRE(fh.writable());
	REQUIRE_FALSE(fh.readable());
	REQUIRE(r);
	REQUIRE(r.count() == buf.size());

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

	file fh{faulty_stream(), opening::for_read | opening::for_write};
	std::array<char, 80> buf = {};

	REQUIRE(fh.readable());
	REQUIRE(fh.writable());

	REQUIRE_SYSTEM_ERROR(fh.read(buf.data(), buf.size()),
	    std::errc::device_or_resource_busy);
	REQUIRE_SYSTEM_ERROR(fh.write(buf.data(), buf.size()),
	    std::errc::operation_not_permitted);
	REQUIRE_SYSTEM_ERROR(fh.rewind(), std::errc::not_supported);
	REQUIRE_SYSTEM_ERROR(fh.resize(0), std::errc::result_out_of_range);
	REQUIRE_SYSTEM_ERROR(fh.truncate(), std::errc::not_supported);
	REQUIRE_FALSE(fh.closed());
	REQUIRE_SYSTEM_ERROR(fh.close(),
	    std::errc::resource_unavailable_try_again);
	REQUIRE(fh.closed());

	// truncate is tell + resize, stops at the first error
	auto ec = make_error_code(std::errc::device_or_resource_busy);
	fh.truncate(ec);

	REQUIRE(ec == std::errc::not_supported);
}

using stdex::signature;

void do_f1(signature<void(char const*)> f)
{
	auto f2 = f;
	f = std::move(f2);
	f("");
}

void do_f1(signature<void(int*)> f)
{
	f(nullptr);
}

enum be_called
{
	no_qs, const_qs, rvref_qs,
};

struct F2
{
	int operator()() &
	{
		return no_qs;
	}

	int operator()() const &
	{
		return const_qs;
	}

	int operator()() &&
	{
		return rvref_qs;
	}

	int operator()() const &&
	{
		return const_qs | rvref_qs;
	}
};

auto do_f2(signature<int()> f)
{
	return f();
}

struct F3
{
	F3 next()
	{
		return { char(c + 1) };
	}

	char c;
};

auto do_f3(signature<F3(F3)> f, F3 obj)
{
	return f(obj);
};

auto do_f3(signature<char(F3)> f, F3 obj)
{
	return f(obj);
};

TEST_CASE("signature")
{
	// overloading and void-discarding
	do_f1([](char const*) { return 3; });

	// forwarding
	F2 f2;
	F2 const f2c;

	REQUIRE(do_f2(f2) == no_qs);
	REQUIRE(do_f2(f2c) == const_qs);
	REQUIRE(do_f2(F2()) == rvref_qs);
	REQUIRE(do_f2((F2 const)f2) == (const_qs | rvref_qs));

	// more overloading plus INVOKE support
	F3 f3 = { 'a' };

	REQUIRE(do_f3(&F3::c, f3) == 'a');
	REQUIRE(do_f3(&F3::next, f3).c == 'b');
}
