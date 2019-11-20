#pragma once
#include<mutex>

namespace fast_io
{

template<stream T>
class basic_iomutex
{
	std::unique_ptr<std::mutex> mtx;
	T handler;
public:
	using native_handle_type = T;
	using lock_guard_type = std::lock_guard<std::mutex>;
	using char_type = typename native_handle_type::char_type;
	template<typename ...Args>
	requires std::constructible_from<T,Args...>
	basic_iomutex(Args&& ...args):mtx(std::make_unique<std::mutex>()),handler(std::forward<Args>(args)...){}
	native_handle_type& native_handle()
	{
		return handler;
	}
	std::mutex& mutex()
	{
		return *mtx;
	}
	void swap(basic_iomutex& o) noexcept
	{
		using std::swap;
		swap(mtx,o.mtx);
		swap(handler,o.handler);
	}
};

template<stream T>
inline auto& mutex(basic_iomutex<T>& t)
{
	return t.mutex();
}

template<output_stream T,std::contiguous_iterator Iter>
inline auto writes(basic_iomutex<T>& t,Iter b,Iter e)
{
	std::lock_guard lg(t.mutex());
	return writes(t.native_handle(),b,e);
}

template<output_stream T>
inline void flush(basic_iomutex<T>& t)
{
	std::lock_guard lg(t.mutex());
	flush(t.native_handle());
}
template<input_stream T,std::contiguous_iterator Iter>
inline Iter reads(basic_iomutex<T>& t,Iter begin,Iter end)
{
	std::lock_guard lg(t.mutex());
	return reads(t.native_handle(),begin,end);
}


template<random_access_stream T,typename... Args>
inline auto seek(basic_iomutex<T>& t,Args&& ...args)
{
	std::lock_guard lg(t.mutex());
	return seek(t.native_handle(),std::forward<Args>(args)...);
}

template<character_output_stream output>
inline void fill_nc(basic_iomutex<output>& out,std::size_t count,typename output::char_type const& ch)
{
	std::lock_guard lg{out.mutex()};
	fill_nc(out.native_handle(),count,ch);
}

template<stream T>
inline void swap(basic_iomutex<T>& a,basic_iomutex<T>& b) noexcept
{
	a.swap(b);
}

}