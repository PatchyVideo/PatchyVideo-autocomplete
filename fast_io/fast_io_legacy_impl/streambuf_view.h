#pragma once

namespace fast_io
{
	
template<typename T>
class streambuf_view
{
	T* rdb;
public:
	using char_type = typename T::char_type;
	using traits_type = typename T::traits_type;
	streambuf_view(T* bufpointer):rdb(bufpointer){}
	inline auto rdbuf()
	{
		return rdb;
	}
};


template<typename T,std::contiguous_iterator Iter>
inline Iter reads(streambuf_view<T>& t,Iter begin,Iter end)
{
	using char_type = typename T::char_type;
	return begin+(t.rdbuf()->sgetn(static_cast<char_type*>(static_cast<void*>(std::to_address(begin))),(end-begin)*sizeof(*begin)/sizeof(char_type))*sizeof(char_type)/sizeof(*begin));
}

template<typename T>
inline typename T::char_type get(streambuf_view<T>& t)
{
	using traits_type = typename T::traits_type;
	auto ch(t.rdbuf()->sbumpc());
	if(ch==traits_type::eof())
		throw std::runtime_error("try to get() from EOF stream view");
	return traits_type::to_char_type(ch);
}

template<typename T>
inline std::pair<typename T::char_type,bool> try_get(streambuf_view<T>& t)
{
	using traits_type = typename T::traits_type;
	auto ch(t.rdbuf()->sbumpc());
	if(ch==traits_type::eof())
		return {0,true};
	return {traits_type::to_char_type(ch),false};
}

template<typename T>
inline void put(streambuf_view<T>& t,typename T::char_type ch)
{
	if(!t.rdbuf()->sputc(ch))
		throw std::runtime_error("put() failed for streambuf view");
}

template<typename T,std::contiguous_iterator Iter>
inline void writes(streambuf_view<T>& t,Iter begin,Iter end) 
{
	using char_type = typename T::char_type;
	if(!t.rdbuf()->sputn(static_cast<char_type const*>(static_cast<void const*>(std::to_address(begin))),(end-begin)*sizeof(*begin)/sizeof(char_type)))
		throw std::runtime_error("writes failed for stream view");
}

template<typename T>
inline void flush(streambuf_view<T>& t)
{
	t.rdbuf()->flush();
}

template<typename stream>
streambuf_view(stream*) -> streambuf_view<stream>;

}