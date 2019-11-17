#pragma once

namespace fast_io
{

class socket
{
	sock::details::socket_type handle=sock::details::invalid_socket;
	void close_impl()
	try
	{
		if(handle!=sock::details::invalid_socket)
			sock::details::closesocket(handle);
	}
	catch(...)
	{}
protected:
	auto& protected_native_handle() {return handle;}
public:
	socket()=default;
	socket(sock::details::socket_type v):handle(v){}
	template<typename ...Args>
	socket(native_interface_t,Args&& ...args):handle(sock::details::socket(std::forward<Args>(args)...)){}
	socket(sock::family family,sock::type const &type,sock::protocal const &protocal = sock::protocal::none):
		handle(sock::details::socket(static_cast<sock::details::address_family>(family),static_cast<int>(type),static_cast<int>(protocal))){}
	auto native_handle() {return handle;}
	socket(socket const&) = delete;
	socket& operator=(socket const&) = delete;
	socket(socket && soc) noexcept:handle(soc.handle)
	{
		soc.handle = sock::details::invalid_socket;
	}
	socket& operator=(socket && soc) noexcept
	{
		if(soc.handle!=handle)
		{
			close_impl();
			handle = soc.handle;
			soc.handle = sock::details::invalid_socket;
		}
		return *this;
	}
	~socket()
	{
		close_impl();
	}
};


template<std::contiguous_iterator Iter>
inline Iter reads(socket& soc,Iter begin,Iter end)
{
	return begin+((sock::details::recv(soc.native_handle(),std::to_address(begin),static_cast<int>((end-begin)*sizeof(*begin)),0))/sizeof(*begin));
}
template<std::contiguous_iterator Iter>
inline Iter writes(socket& soc,Iter begin,Iter end)
{
	return begin+(sock::details::send(soc.native_handle(),std::to_address(begin),static_cast<int>((end-begin)*sizeof(*begin)),0)/sizeof(*begin));
}

inline constexpr void flush(socket&)
{
}

#if defined(__linux__) || defined(__WINNT__) || defined(_MSC_VER)
inline auto zero_copy_out_handle(socket& soc)
{
	return soc.native_handle();
}
#endif

struct address_info
{
	socket_address_storage storage={};
	socklen_t storage_size=sizeof(socket_address_storage);
};

class client:public socket
{
	address_info cinfo;
public:
	using char_type = char;
	template<typename T,std::integral U,typename ...Args>
	client(T const& add,U u,Args&& ...args):socket(family(add),std::forward<Args>(args)...),cinfo{to_socket_address_storage(add,u),sizeof(socket_address_storage)}
	{
		sock::details::connect(native_handle(),cinfo.storage,native_socket_address_size(add));
	}
	constexpr auto& info()
	{
		return cinfo;
	}
	constexpr auto& info() const
	{
		return cinfo;
	}
};

class server
{
	socket soc;
public:
	template<typename addrType,std::integral U,typename ...Args>
	requires (!std::integral<addrType>)
	server(addrType const& add,U u,Args&& ...args):soc(family(add),std::forward<Args>(args)...)
	{
		auto stg(to_socket_address_storage(add,u));
		sock::details::bind(soc.native_handle(),stg,native_socket_address_size(add));
		sock::details::listen(soc.native_handle(),10);
	}
	template<std::integral U,typename ...Args>
	server(U u,Args&& ...args):server(ipv4{},u,std::forward<Args>(args)...)
	{
	}
	constexpr auto& handle()
	{
		return soc;
	}
};

class acceptor:public socket
{
	address_info cinfo;
public:
	using native_handle_type = sock::details::socket_type;
	using char_type = char;
	acceptor(server& listener_socket)
	{
		protected_native_handle()=sock::details::accept(listener_socket.handle().native_handle(),cinfo.storage,cinfo.storage_size);
	}
	constexpr auto& get()
	{
		return cinfo;
	}
	constexpr auto& get() const
	{
		return cinfo;
	}
};

}