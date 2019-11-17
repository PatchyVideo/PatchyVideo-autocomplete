#pragma once

namespace fast_io
{
	
namespace stream_view_details
{
template<typename T>
concept istream_concept_impl = requires(T& in)
{
	{in.read};
	{in.get};
	{in.gcount};
	{in.seekg};
	{in.tellg};
	{in.operator>>};
};

template<typename T>
concept ostream_concept_impl = requires(T& out)
{
	{out.write};
	{out.put};
	{out.seekp};
	{out.tellp};
	{out.operator<<};
};
}
	
template<typename T>
class stream_view
{
	T strm;
public:
	using char_type = typename T::char_type;
	using traits_type = typename T::traits_type;
	template<typename ...Args>
	requires std::constructible_from<T,Args...>
	stream_view(Args&& ...args):strm(std::forward<Args>(args)...){}
	auto& native_handle() { return strm;}
};

template<fast_io::stream_view_details::istream_concept_impl T,std::contiguous_iterator Iter>
inline Iter reads(stream_view<T>& t,Iter begin,Iter end)
{
	using char_type = typename T::char_type;
	return begin+(t.native_handle().sgetn(static_cast<char_type*>(static_cast<void*>(std::to_address(begin))),(end-begin)*sizeof(*begin)/sizeof(char_type))*sizeof(char_type)/sizeof(*begin));
}

template<fast_io::stream_view_details::istream_concept_impl T>
inline typename T::char_type get(stream_view<T>& t)
{
	using traits_type = typename T::traits_type;
	auto ch(t.native_handle().get());
	if(ch==traits_type::eof())
		throw std::runtime_error("try to get() from EOF stream view");
	return traits_type::to_char_type(ch);
}

template<fast_io::stream_view_details::istream_concept_impl T>
inline std::pair<typename T::char_type,bool> try_get(stream_view<T>& t)
{
	using traits_type = typename T::traits_type;
	auto ch(t.native_handle().get());
	if(ch==traits_type::eof())
		return {0,true};
	return {traits_type::to_char_type(ch),false};
}

template<fast_io::stream_view_details::ostream_concept_impl T>
inline void put(stream_view<T>& t,typename T::char_type ch)
{
	if(!t.native_handle().put(ch))
		throw std::runtime_error("put() failed for streambuf view");
}

template<fast_io::stream_view_details::ostream_concept_impl T,std::contiguous_iterator Iter>
inline void writes(stream_view<T>& t,Iter begin,Iter end) 
{
	using char_type = typename T::char_type;
	if(!t.native_handle().write(static_cast<char_type const*>(static_cast<void const*>(std::to_address(begin))),(end-begin)*sizeof(*begin)/sizeof(char_type)))
		throw std::runtime_error("writes failed for stream view");
}

template<fast_io::stream_view_details::ostream_concept_impl T>
inline void flush(stream_view<T>& t)
{
	t.native_handle().flush();
}

template<typename stream>
requires (fast_io::stream_view_details::istream_concept_impl<stream>||fast_io::stream_view_details::ostream_concept_impl<stream>)
stream_view(stream&&) noexcept -> stream_view<stream>;

}