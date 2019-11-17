#pragma once

namespace fast_io::sock::details
{

template<typename Func,typename ...Args>
inline auto call_posix(Func&& func,Args&& ...args)
{
	auto ret(func(std::forward<Args>(args)...));
	if(ret==-1)
		throw std::system_error(errno,std::generic_category());
	return ret;
}

template<typename ...Args>
inline auto socket(Args&& ...args)
{
	return call_posix(::socket,std::forward<Args>(args)...);
}

template<typename T>
inline auto accept(int sck,T& sock_address,socklen_t& storage_size)
{
	return call_posix(::accept,sck,std::addressof(sock_address),std::addressof(storage_size));
}

template<typename T,std::unsigned_integral sock_type_size>
inline auto connect(int sck,T& sock_address,sock_type_size size)
{
	return call_posix(::connect,sck,std::addressof(sock_address),size);
}

template<typename ...Args>
inline auto send(Args&& ...args)
{
	return call_posix(::send,std::forward<Args>(args)...);
}
template<typename ...Args>
inline auto recv(Args&& ...args)
{
	return call_posix(::recv,std::forward<Args>(args)...);
}

template<typename ...Args>
inline auto closesocket(Args&& ...args)
{
	return call_posix(::close,std::forward<Args>(args)...);
}

template<typename T,std::unsigned_integral sock_type_size>
inline auto bind(int sck,T& sock_address,sock_type_size size)
{
	return call_posix(::bind,sck,std::addressof(sock_address),size);
}

template<typename ...Args>
inline auto bind(Args&& ...args)
{
	return call_posix(::bind,std::forward<Args>(args)...);
}

template<typename ...Args>
inline auto listen(Args&& ...args)
{
	return call_posix(::listen,std::forward<Args>(args)...);
}

inline auto inet_pton(family fm,std::string_view address,void* dst)
{
	return call_posix(::inet_pton,static_cast<int>(fm),address.data(),dst);
}

class gai_exception:public std::runtime_error
{
	int ec;
public:
	explicit gai_exception(int errorc):std::runtime_error(gai_strerror(ec)),ec(errorc){}	
	auto get() const
	{
		return ec;
	}
};

template<typename ...Args>
inline void getaddrinfo(Args&& ...args)
{
	int ec(::getaddrinfo(std::forward<Args>(args)...));
	if(ec)
		throw gai_exception(ec);
}

template<typename ...Args>
inline void freeaddrinfo(Args&& ...args)
{
	::freeaddrinfo(std::forward<Args>(args)...);
}


using address_family = sa_family_t;
using socket_type = int;
inline constexpr auto invalid_socket(-1);
}