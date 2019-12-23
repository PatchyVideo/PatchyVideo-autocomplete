#pragma once
#include<variant>

namespace fast_io
{

struct ipv4
{
	std::array<std::byte, 4> storage{};
};


template<character_input_stream input>
inline constexpr void scan_define(input& in,ipv4& v4)
{
	for(auto& e: v4.storage)
		scan(in,e);
}

inline constexpr std::size_t native_socket_address_size(ipv4 const&)
{
	return sizeof(sockaddr_in);
}

inline constexpr auto family(ipv4 const&)
{
	return sock::family::ipv4;
}


struct socket_address_storage
{
	sockaddr sock;
	std::array<std::byte,sizeof(sockaddr_storage)<sizeof(sockaddr)?0x0:sizeof(sockaddr_storage)-sizeof(sockaddr)> storage;
};


//use memcpy is THE only way to do correct ip address punning
template<std::integral U>
inline constexpr auto to_socket_address_storage(ipv4 const& add,U port)
{
	sockaddr_in v4st{};
	v4st.sin_family=static_cast<sock::details::address_family>(fast_io::sock::family::ipv4);
	v4st.sin_port=details::big_endian(static_cast<std::uint16_t>(port));
	std::memcpy(std::addressof(v4st.sin_addr),add.storage.data(),sizeof(add.storage));
	socket_address_storage stor{};
	std::memcpy(std::addressof(stor),std::addressof(v4st),sizeof(sockaddr_in));
	return stor;
}

template<character_output_stream output>
inline constexpr void print_define(output& os, ipv4 const &v)
{
	print(os, v.storage.front());
	put(os, 0x2E);
	print(os, v.storage[1]);
	put(os, 0x2E);
	print(os, v.storage[2]);
	put(os, 0x2E);
	print(os, v.storage.back());
}

struct ipv6
{
	std::array<std::byte, 16> storage{};
};
inline constexpr std::size_t native_socket_address_size(ipv6 const&)
{
	return sizeof(sockaddr_in6);
}

inline constexpr auto family(ipv6 const&)
{
	return sock::family::ipv6;
}

template<character_input_stream input>
inline constexpr void scan_define(input& in,ipv6& v6)
{
	constexpr auto npos(static_cast<std::size_t>(-1));
	std::basic_string<typename input::char_type> str;
	scan(in,str);
	if(str.size()<2)
		throw std::runtime_error(reinterpret_cast<char const*>(u8"ipv6 address too short"));
	else if(39<str.size())
		throw std::runtime_error(reinterpret_cast<char const*>(u8"ipv6 address too long"));
	std::size_t colons(0),position(npos);
	if(str.front()!=0x3a)
		++colons;
	if(str.back()!=0x3a)
		++colons;
	for(std::size_t i(0);i!=str.size();++i)
		if(str[i]==0x3a)
		{
			++colons;
			if(i+1!=str.size()&&str[i+1]==0x3a)
			{
				position=i;
				++i;
			}
		}
	if(7<colons)
		throw std::runtime_error(reinterpret_cast<char const*>(u8"too many : for ipv6 address"));
	if(position!=npos)
	{
		std::u8string tempstr(1,0x20);
		for(std::size_t i(9-colons);i--;)
			tempstr.append(u8"0 ",2);
		str.insert(position,tempstr);
	}
	fast_io::basic_istring_view<std::basic_string_view<typename input::char_type>> istrbuf(str);
	std::uint16_t temp{};
	for(auto i(v6.storage.begin()),e(v6.storage.end());i!=e;++i)
	{
		fast_io::scan(istrbuf,fast_io::hex(temp));
		*i=static_cast<std::byte>(temp>>8);
		*++i=static_cast<std::byte>(temp&255);
	}
}

template<std::integral U>
inline constexpr auto to_socket_address_storage(ipv6 const& add,U port)
{
	sockaddr_in6 v6st{};
	v6st.sin6_family=static_cast<sock::details::address_family>(fast_io::sock::family::ipv6);
	v6st.sin6_port=details::big_endian(static_cast<std::uint16_t>(port));
	std::memcpy(std::addressof(v6st.sin6_addr),add.storage.data(),sizeof(add.storage));
	socket_address_storage stor{};
	std::memcpy(std::addressof(stor),std::addressof(v6st),sizeof(sockaddr_in6));
	return stor;
}

template<character_output_stream output,std::size_t base,bool uppercase,typename T>
requires std::same_as<ipv6,std::remove_cvref_t<T>>
inline constexpr void print_define(output& os,manip::base_t<base,uppercase,T> e)
{
	std::array<std::uint16_t,8> storage{};
	for(std::size_t i(0);i!=storage.size();++i)
		storage[i]=(std::to_integer<std::uint16_t>(e.reference.storage[i<<1])<<8)|std::to_integer<std::uint16_t>(e.reference.storage[(i<<1)+1]);
	constexpr auto npos(static_cast<std::size_t>(-1));
	std::size_t last_zero_range(npos);
	std::size_t maximum_zero_size(0),maximum_zero_start(npos);
	for(std::size_t i(0),sz(storage.size());i!=sz;++i)
	{
		auto& e(storage[i]);
		if(e)
			last_zero_range=npos;
		else
		{
			if(maximum_zero_start==npos)
			{
				maximum_zero_size=1;
				maximum_zero_start=last_zero_range=i;
			}
			else if(last_zero_range==npos)
				last_zero_range=i;
			else
			{
				if(maximum_zero_size<i+1-last_zero_range)
				{
					maximum_zero_size=i+1-last_zero_range;
					maximum_zero_start=last_zero_range;
				}
			}
		}
	}
	if(maximum_zero_size)
	{
		if(maximum_zero_start)
			print(os,fast_io::base<base,uppercase>(storage.front()));
		for(std::size_t i(1);i<maximum_zero_start;++i)
			print(os,fast_io::char_view(0x3a),fast_io::base<base,uppercase>(storage[i]));
		print(os,u8"::");
		std::size_t const maximum_zero_end(maximum_zero_start+maximum_zero_size);
		if(maximum_zero_end==storage.size())
			return;
		print(os,fast_io::base<base,uppercase>(storage[maximum_zero_end]));
		for(std::size_t i(maximum_zero_end+1);i<storage.size();++i)
			print(os,fast_io::char_view(0x3a),fast_io::base<base,uppercase>(storage[i]));
	}
	else
	{
		print(os, fast_io::hex(storage.front()));
		for (auto i(storage.cbegin() + 1); i != storage.cend(); ++i)
			print(os,fast_io::char_view(0x3a),fast_io::base<base,uppercase>(*i));
	}
}

template<character_output_stream output,typename T>
requires std::same_as<ipv6,std::remove_cvref_t<T>>
inline constexpr void print_define(output& out,T const& v)
{
	print(out,fast_io::hex(v));
}

class address
{
public:
	using variant_type = std::variant<ipv4, ipv6>;
private:
	variant_type var;
public:
	template<typename... Args>
	requires std::constructible_from<variant_type, Args...>
	explicit constexpr address(Args &&... args) :var(std::forward<Args>(args)...){}

	constexpr auto& variant() { return var; }
	constexpr auto& variant() const { return var; }
};

template<std::integral U>
inline constexpr auto to_socket_address_storage(address const& v,U port)
{
	return std::visit([port](auto const &arg) {
		return to_socket_address_storage(arg,port);
	}, v.variant());
}

inline constexpr std::size_t native_socket_address_size(address const& v)
{
	return std::visit([](auto const &arg) {
		return native_socket_address_size(arg);
	}, v.variant());
}


inline constexpr auto family(address const& v)
{
	return std::visit([](auto const &arg) {
		return family(arg);
	}, v.variant());
}

template<character_output_stream output>
inline constexpr void print_define(output &os, address const &v)
{
	std::visit([&os](auto const &arg) {
		print_define(os,arg);
	}, v.variant());
}

}