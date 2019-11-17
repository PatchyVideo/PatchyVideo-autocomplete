#pragma once
// Since the library is written before C++20. Will use standard libraries concepts after C++ 20 being official published. PLEASE Do not use these concepts!!!

#ifdef _MSC_VER
namespace std
{
template<typename T>
concept floating_point = std::is_floating_point_v<T>;
}
#endif


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

template<typename T>
concept Trivial_copyable=std::is_trivially_copyable_v<T>;

namespace details
{

template<typename Cont>
concept Container_impl = requires(Cont c)
{
	{c.begin};
	{c.end};
};

template<typename Cont>
concept Dynamic_size_container_impl = requires(Cont c)
{
	{c.insert};
	{c.size};
};

template<typename Cont>
concept Contiguous_container_impl = requires(Cont c)
{
	{c.data};
};

}

template<typename Cont>
concept Container = details::Container_impl<Cont>;

template<typename Cont>
concept Dynamic_size_container = Container<Cont> && details::Dynamic_size_container_impl<Cont>;

template<typename Cont>
concept Contiguous_container = Container<Cont> && details::Contiguous_container_impl<Cont>;

template<typename Cont>
concept Trivial_copyable_container = Container<Cont> && Trivial_copyable<typename Cont::value_type>;

template<typename Cont>
concept Contiguous_trivial_container = Contiguous_container<Cont> && Trivial_copyable_container<Cont>;

template<typename Cont>
concept Contiguous_dynamic_size_container = Dynamic_size_container<Cont>&&Contiguous_container<Cont>;

template<typename Cont>
concept Contiguous_trivial_dynamic_size_container = Contiguous_trivial_container<Cont>&&Contiguous_dynamic_size_container<Cont>;

template<typename Cont>
concept Contiguous_fixed_size_container = Contiguous_container<Cont>&&!Dynamic_size_container<Cont>;

template<typename Cont>
concept Contiguous_fixed_size_trivial_copyable_container = Contiguous_fixed_size_container<Cont>&& Trivial_copyable_container<Cont>;

template<typename Cont>
concept Contiguous_fixed_size_none_trivial_copyable_container =Contiguous_fixed_size_container<Cont>&&!Contiguous_fixed_size_trivial_copyable_container<Cont>;

}