#pragma once
// Since the library is written before C++20. Will use standard libraries concepts after C++ 20 being official published. PLEASE Do not use these concepts!!!

namespace fast_io
{

//Since Currently No C++ Compilers have implemented this important stuff. We can only emulate it with memcpy.
//And unfortuntely, this is not what it should be. It is impossible to correctly implement this without compiler magic.
template<typename To,typename From>
requires (sizeof(To)==sizeof(From) && std::is_trivially_copyable_v<To> && std::is_trivial_v<From>)
inline To bit_cast(From const& src) noexcept
{
	To dst;
	std::memcpy(std::addressof(dst), std::addressof(src), sizeof(To));
	return dst;
}


namespace details
{

template<std::unsigned_integral U>
inline constexpr U big_endian(U u)
{
	if constexpr(std::endian::little==std::endian::native)
	{
		auto pun(bit_cast<std::array<std::byte,sizeof(U)>>(u));
		std::reverse(pun.begin(),pun.end());
		return bit_cast<U>(pun);
	}
	else
		return u;
}
}

}