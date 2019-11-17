#pragma once

namespace fast_io
{

template<stream T,std::size_t interface_mode>
class basic_file_wrapper:public T
{
public:
	using char_type = typename T::char_type;
	using native_handle_type = typename T::native_handle_type;
	template<typename ...Args>
	requires std::constructible_from<T,Args...>
	constexpr basic_file_wrapper(native_interface_t,Args&& ...args):T(std::forward<Args>(args)...){}
	template<std::size_t om>
	constexpr basic_file_wrapper(std::string_view file,open::interface_t<om>):T(file,open::interface<fast_io::open::mode(om|interface_mode)>){}
	constexpr basic_file_wrapper(std::string_view file):T(file,open::interface<fast_io::open::mode(interface_mode)>){}
	constexpr basic_file_wrapper(std::string_view file,open::mode const& m):T(file,m|fast_io::open::mode(interface_mode)){}
	constexpr basic_file_wrapper(std::string_view file,std::string_view mode):basic_file_wrapper(file,fast_io::open::c_style(mode)){}
};

template<input_stream T>
using input_file_wrapper = basic_file_wrapper<T,fast_io::open::in|fast_io::open::binary>;
template<output_stream T>
using output_file_wrapper = basic_file_wrapper<T,fast_io::open::out|fast_io::open::binary>;
template<io_stream T>
using io_file_wrapper = basic_file_wrapper<T,fast_io::open::in|fast_io::open::out|fast_io::open::binary>;



template<stream T,std::size_t interface_mode>
class basic_wrapper:public T
{
public:
	using char_type = typename T::char_type;
	using native_handle_type = typename T::native_handle_type;
	template<std::size_t om>
	constexpr basic_wrapper(open::interface_t<om>):T(open::interface<fast_io::open::mode(om|interface_mode)>){}
	constexpr basic_wrapper():T(open::interface<fast_io::open::mode(interface_mode)>){}
	constexpr basic_wrapper(open::mode const& m):T(m|fast_io::open::mode(interface_mode)){}
	constexpr basic_wrapper(std::string_view mode):basic_wrapper(fast_io::open::c_style(mode)){}
};

template<input_stream T>
using input_wrapper = basic_wrapper<T,fast_io::open::in|fast_io::open::binary>;
template<output_stream T>
using output_wrapper = basic_wrapper<T,fast_io::open::out|fast_io::open::binary>;
template<io_stream T>
using io_wrapper = basic_wrapper<T,fast_io::open::in|fast_io::open::out|fast_io::open::binary>;

}
