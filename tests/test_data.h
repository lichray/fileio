#pragma once

#include <random>
#include <algorithm>
#include <iterator>
#include <string>
#include <ratio>

#if defined(_MSC_VER)
#pragma intrinsic(__rdtsc)
#endif

inline
uint_fast32_t __get_seed()
{
	uint_fast32_t lo;
#if defined(_MSC_VER)
	lo = unsigned(__rdtsc());
#elif defined(__i386__) || defined(__x86_64__)
	__asm__ __volatile__ ("rdtsc" : "=a"(lo));
#else
	lo = std::random_device{}();
#endif
	return lo;
}

inline
auto& __global_rng()
{
	thread_local std::mt19937 e{__get_seed()};
	return e;
}

template <typename IntType>
inline
IntType randint(IntType a, IntType b)
{
	static_assert(std::is_integral<IntType>(), "must be an integral type");
	static_assert(sizeof(IntType) > 1, "must not like a character type");

	using D = std::uniform_int_distribution<IntType>;

	// as long as the distribution carries no state
	return D{a, b}(__global_rng());
}

inline
void reseed()
{
	// as long as uniform_int_distribution carries no state
	__global_rng().seed(__get_seed());
}

inline
void reseed(uint_fast32_t value)
{
	// as long as uniform_int_distribution carries no state
	__global_rng().seed(value);
}

template <typename CharT>
inline
auto random_text(size_t len, CharT const* from =
    "-0123456789abcdefghijklmnopqrstuvwxyz")
{
	std::basic_string<CharT> to;
	to.resize(len);
	auto ub = std::char_traits<CharT>::length(from) - 1;

	std::generate(begin(to), end(to), [&]
	    {
		return from[randint<size_t>(0, ub)];
	    });

	return to;
}

template <typename Rng, typename T, typename Ratio = std::ratio<1, 2>>
inline
void maybe_replace(Rng& rng, T const& v, Ratio ro = {})
{
	using std::begin;
	using std::end;

	auto do_replace = [&]
	{
		auto it = std::next(begin(rng),
		    randint<size_t>(0, rng.size() - 1));
		*it = v;
	};

	for (auto i = 0; i < ro.num / ro.den; ++i)
		do_replace();
	if (randint<intmax_t>(1, ro.den) <= ro.num % ro.den)
		do_replace();
}
