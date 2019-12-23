#pragma once

namespace fast_io
{
template<output_stream T>
struct hmac
{
	using char_type = char;
	using key_type = std::array<char_type,T::crypto_hash_block_size>;
	using native_handle_t = T;
	native_handle_t hash_stream;
	key_type key{};
	template<std::contiguous_iterator Iter>
	hmac(Iter cbegin,Iter cend)
	{
		if(hash_stream.crypto_hash_block_size<static_cast<std::size_t>(cend-cbegin))
		{
			send(hash_stream,cbegin,cend);
			flush(hash_stream);
			if constexpr(std::endian::native==std::endian::little)
				for(auto & e : hash_stream.digest)
					e=details::big_endian(e);
			std::copy(static_cast<char_type const*>(static_cast<void const*>(hash_stream.digest.data())),
				static_cast<char_type const*>(static_cast<void const*>(std::to_address(hash_stream.digest.end()))),
				key.data());
			clear(hash_stream);
		}
		else
		{
			std::copy(static_cast<char_type const*>(static_cast<void const*>(std::to_address(cbegin))),
				static_cast<char_type const*>(static_cast<void const*>(std::to_address(cend))),
				key.data());
		}
		auto inner(key);
		for(auto & e : inner)
			e^=0x36;
		send(hash_stream,inner.cbegin(),inner.cend());
	}
	hmac(std::string_view sv):hmac(sv.cbegin(),sv.cend()){}
};

template<output_stream T,std::contiguous_iterator Iter>
inline void send(hmac<T>& hm,Iter cbegin,Iter cend)
{
	send(hm.hash_stream,cbegin,cend);
}

template<buffer_output_stream T>
inline constexpr auto oreserve(hmac<T>& hm,std::size_t size)
{
	return oreserve(hm.hash_stream,size);
}

template<buffer_output_stream T>
inline constexpr void orelease(hmac<T>& hm,std::size_t size)
{
	orelease(hm.hash_stream,size);
}

template<output_stream T>
inline void flush(hmac<T>& hm)
{
	flush(hm.hash_stream);
	auto outer(hm.key);
	for(auto & e : outer)
		e^=0x5c;
	auto digest(hm.hash_stream.digest);
	if constexpr(std::endian::native==std::endian::little)
		for(auto & e : digest)
			e=details::big_endian(e);
	clear(hm.hash_stream);
	send(hm.hash_stream,outer.cbegin(),outer.cend());
	send(hm.hash_stream,digest.cbegin(),digest.cend());
	flush(hm.hash_stream);
}

template<io_stream T,std::contiguous_iterator Iter>
inline void receive(hmac<T>& hm,Iter begin,Iter end)
{
	receive(hm.hash_stream,begin,end);
}

template<buffer_output_stream output,output_stream T>
requires (printable<output,T>)
inline void print_define(output& out,hmac<T> const& hm)
{
	print(out,hm.hash_stream);
}


template<output_stream T>
inline decltype(auto) get_digest(hmac<T>& hm)
{
	return get_digest(hm.hash_stream);
}

}