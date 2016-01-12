#include <random>
#include <algorithm>
#include <iterator>
#include <string>

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
auto random_text(size_t len, std::string const& from =
    "-0123456789abcdefghijklmnopqrstuvwxyz")
{
	std::string to;
	to.resize(len);

	std::generate(begin(to), end(to), [&]
	    {
		return from[randint<size_t>(0, from.size() - 1)];
	    });

	return to;
}
