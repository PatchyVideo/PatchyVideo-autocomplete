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
requires (sizeof(typename T::value_type)==1||
	std::same_as<typename T::value_type,typename std::iterator_traits<Iter>::value_type>)
inline void send(basic_ostring<T>& ostr,Iter cbegin,Iter cend)
{
	using char_type = typename T::value_type;
//http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1072r2.html
//strict aliasing rule
	if constexpr(std::same_as<char_type,std::remove_cvref_t<decltype(*cbegin)>>)
		ostr.str().append(std::to_address(cbegin),std::to_address(cend));
	else
	{
		std::size_t const size(ostr.str().size());
		std::size_t const bytes(sizeof(*cbegin)*(cend-cbegin));
		ostr.str().append(bytes,0);
		memcpy(ostr.str().data()+size,std::to_address(cbegin),bytes);
	}
}
template<typename T>
inline void put(basic_ostring<T>& ostr,typename T::value_type ch)
{
	ostr.str().push_back(ch);
}

template<typename T>
inline void flush(basic_ostring<T>&){}

template<typename T>
inline void fill_nc(basic_ostring<T>& os,std::size_t count,typename T::value_type const& ch)
{
	os.str().append(count,ch);
}


}
