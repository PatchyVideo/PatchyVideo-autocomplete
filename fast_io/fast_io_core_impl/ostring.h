#pragma once


namespace fast_io
{

template< typename T>
class basic_ostring
{
	T s;
public:
	using value_type = T;
	using char_type = typename T::value_type;
	template<typename ...Args>
	requires std::constructible_from<T,Args...>
	constexpr basic_ostring(Args&& ...args):s(std::forward<Args>(args)...){}
	constexpr auto& str()
	{
		return s;
	}
	constexpr void clear(){s.clear();}
	constexpr auto empty() const {return s.empty();}
};

template<typename T>
[[nodiscard]] inline constexpr std::size_t osize(basic_ostring<T>& ob)
{
	return ob.str().size();
}

template<typename T>
[[nodiscard]] inline constexpr auto oreserve(basic_ostring<T>& ob,std::size_t size)
{
	ob.str().append(size,0);
	return ob.str().end();
}

template<typename T>
inline constexpr void orelease(basic_ostring<T>& ob,std::size_t size)
{
	ob.str().erase(ob.str().cend()-size,ob.str().cend());
}

template<typename T,std::contiguous_iterator Iter>
inline constexpr void writes(basic_ostring<T>& ostr,Iter cbegin,Iter cend)
{
	using char_type = typename T::value_type;
	ostr.str().append(static_cast<char_type const*>(static_cast<void const*>(std::to_address(cbegin))),static_cast<char_type const*>(static_cast<void const*>(std::to_address(cend))));
}
template<typename T>
inline constexpr void put(basic_ostring<T>& ostr,typename T::value_type ch)
{
	ostr.str().push_back(ch);
}

template<typename T>
inline constexpr void flush(basic_ostring<T>&){}

template<typename T>
inline constexpr void fill_nc(basic_ostring<T>& os,std::size_t count,typename T::value_type const& ch)
{
	os.str().append(count,ch);
}


}
