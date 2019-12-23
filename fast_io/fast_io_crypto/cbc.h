#pragma once

namespace fast_io::crypto
{

template <input_stream T, typename Dec>
class basic_icbc
{
public:
	using native_interface_t = T;
	using char_type = typename native_interface_t::char_type;
	using cipher_type = Dec;

private:
	using unsigned_char_type = std::make_unsigned_t<char_type>;

public:
	using key_type = std::array<unsigned_char_type, cipher_type::key_size>;
	using block_type = std::array<unsigned_char_type, cipher_type::block_size>;

private:
	using block_iterator = typename block_type::iterator;
	block_type cipher_buf = {};
	block_iterator cipher_buf_pos = cipher_buf.begin();
	block_type plaintext_buf = {};
	block_iterator plaintext_buf_pos = plaintext_buf.end();
	key_type key;
	block_type iv;
	Dec dec;
	T ib;
	inline constexpr unsigned_char_type *mread(unsigned_char_type *pb, unsigned_char_type *pe)
	{
		auto pi(pb);

		std::size_t const input_length(pe - pi);

		if (plaintext_buf_pos != plaintext_buf.end())
		{
			std::size_t buffer_length(plaintext_buf.end() - plaintext_buf_pos);
			std::size_t min_length(input_length);
			if (buffer_length < min_length)
				min_length = buffer_length;
			pi = std::uninitialized_copy(plaintext_buf_pos, plaintext_buf_pos + min_length, pi);
			plaintext_buf_pos += min_length;
			if (plaintext_buf_pos != plaintext_buf.end())
				return pi;
		}

		for (; pi != pe;)
		{
			cipher_buf_pos = receive(ib,cipher_buf_pos, cipher_buf.end());
			if (cipher_buf_pos != cipher_buf.end())
				return pi;

			auto xored_text(dec(cipher_buf.data()));
			cipher_buf_pos = cipher_buf.begin();

			block_type plain(iv);
			for (std::size_t i(0); i != iv.size(); ++i)
				plain[i] ^= xored_text[i];

			iv = cipher_buf;

			std::size_t available_out_space(pe - pi);
			if (available_out_space < cipher_type::block_size)
			{
				pi = std::uninitialized_copy(plain.begin(), plain.begin() + available_out_space, pi);
				plaintext_buf_pos = plaintext_buf.begin() + available_out_space;
				std::uninitialized_copy(plain.begin() + available_out_space, plain.end(), plaintext_buf_pos);
				break;
			}
			else
			{
				pi = std::uninitialized_copy(plain.begin(), plain.end(), pi);
			}
		}
		return pi;
	}

public:
	template<typename IV,typename K,typename... Args>
	requires (std::constructible_from<key_type,K>&&std::constructible_from<block_type,IV>&&std::constructible_from<T,Args...>)
	inline constexpr basic_icbc(K&& init_key, IV&& init_iv, Args&& ...args) :  key(std::forward<K>(init_key)),iv(std::forward<IV>(init_iv)), dec(key.data()), ib(std::forward<Args>(args)...)
	{
	}
	template<std::contiguous_iterator Iter>
	inline constexpr Iter mmreceive(Iter begin, Iter end)
	{
		auto bgchadd(static_cast<unsigned_char_type *>(static_cast<void *>(std::to_address(begin))));
		return begin + (mread(bgchadd, static_cast<unsigned_char_type *>(static_cast<void *>(std::to_address(end)))) - bgchadd) / sizeof(*begin);
	}
	template<bool err=false>
	inline constexpr char_type mmget()
	{
		if (plaintext_buf_pos == plaintext_buf.end())
		{
			block_type tmp;
			auto next_ch(tmp.begin() + 1);
			auto ret(mread(tmp.data(), std::to_address(next_ch)));
			if (ret != next_ch)
			{
				plaintext_buf_pos = plaintext_buf.begin();
				if constexpr(err)
					return {0, true};
				else
					throw eof();
			}
			if constexpr(err)
				return std::pair<char_type, bool>{static_cast<char_type>(*tmp.begin()), false};
			else
				return static_cast<char_type>(*tmp.begin());
		}
		if constexpr(err)
			return std::pair<char_type, bool>{static_cast<char_type>(*plaintext_buf_pos++), false};
		else
			return static_cast<char_type>(*plaintext_buf_pos++);
			
	}
};

template <output_stream T, typename Enc>
class basic_ocbc
{
public:
	using native_interface_t = T;
	using char_type = typename native_interface_t::char_type;
	using cipher_type = Enc;

private:
	using unsigned_char_type = std::make_unsigned_t<char_type>;

public:
	using key_type = std::array<unsigned_char_type, cipher_type::key_size>;
	using block_type = std::array<unsigned_char_type, cipher_type::block_size>;
private:
	using block_iterator = typename block_type::iterator;
	block_type plaintext_buf = {};
	block_iterator plaintext_buf_pos = plaintext_buf.begin();
	key_type key;
	block_type iv;
	Enc enc;
	T ob;

	inline constexpr void write_remain()
	{
		if (plaintext_buf_pos != plaintext_buf.begin())
		{
			std::uninitialized_fill(plaintext_buf_pos, plaintext_buf.end(), 0);

			block_type xored_text(iv);
			for (std::size_t i(0); i != iv.size(); ++i)
				xored_text[i] ^= plaintext_buf[i];
			
			auto cipher(enc(xored_text.data()));
			send(ob,cipher.cbegin(), cipher.cend());
			iv = cipher;
			plaintext_buf_pos = plaintext_buf.begin();
		}
	}

public:
	template<typename K,typename IV,typename... Args>
	requires (std::constructible_from<key_type,K>&&std::constructible_from<block_type,IV>&&std::constructible_from<T,Args...>)
	basic_ocbc(K&& init_key, IV&& init_iv, Args&& ...args) : key(std::forward<K>(init_key)), iv(std::forward<IV>(init_iv)), enc(key.data()), ob(std::forward<Args>(args)...)
	{
	}

	inline constexpr void mmflush()
	{
		write_remain();
		flush(ob);
	}

	template<std::contiguous_iterator Iter>
	inline constexpr void mmsend(Iter b, Iter e)
	{
		auto pb(static_cast<unsigned_char_type const *>(static_cast<void const *>(std::addressof(*b))));
		auto pi(pb), pe(pb + (e - b) * sizeof(*b) / sizeof(unsigned_char_type));
		std::size_t const input_length(pe - pi);

		if (plaintext_buf_pos != plaintext_buf.begin())
		{
			std::size_t min_length(plaintext_buf.end() - plaintext_buf_pos);
			if (input_length < min_length)
				min_length = input_length;
			plaintext_buf_pos = std::uninitialized_copy(pi, pi + min_length, plaintext_buf_pos);
			pi += min_length;

			if (plaintext_buf_pos != plaintext_buf.end())
				return;

			block_type xored_text(iv);
			for (std::size_t i = 0; i != iv.size(); ++i)
				xored_text[i] ^= plaintext_buf[i];
			
			auto cipher(enc(xored_text.data()));
			send(ob,cipher.cbegin(), cipher.cend());
			iv = cipher;

			plaintext_buf_pos = plaintext_buf.begin();
		}

		for (; pi + cipher_type::block_size <= pe; pi += cipher_type::block_size)
		{
			block_type xored_text(iv);
			for (std::size_t i(0); i != iv.size(); ++i)
				xored_text[i] ^= pi[i];
			auto cipher(enc(xored_text.data()));
			send(ob,cipher.cbegin(), cipher.cend());
			iv = cipher;
		}
		plaintext_buf_pos = std::uninitialized_copy(pi, pe, plaintext_buf.begin());
	}

	inline constexpr void mmput(char_type ch) {
		if (plaintext_buf_pos == plaintext_buf.end())
		{
			block_type xored_text(iv);
			for (std::size_t i = 0; i != iv.size(); ++i)
				xored_text[i] ^= plaintext_buf[i];
			
			auto cipher(enc(xored_text.data()));
			send(ob,cipher.cbegin(), cipher.cend());
			iv = cipher;
			plaintext_buf_pos = plaintext_buf.begin();
		}
		*plaintext_buf_pos = static_cast<unsigned_char_type>(ch);
		++plaintext_buf_pos;
	}
	basic_ocbc(basic_ocbc const&)=delete;
	basic_ocbc& operator=(basic_ocbc const&)=delete;
	basic_ocbc(basic_ocbc&& ocbc) noexcept:
		plaintext_buf(std::move(plaintext_buf)),
		plaintext_buf_pos(std::move(ocbc.plaintext_buf_pos)),
		key(std::move(ocbc.key)),
		iv(std::move(ocbc.iv)),
		ob(std::move(ocbc.ob)),
		enc(std::move(ocbc.enc))
	{
		ocbc.plaintext_buf_pos = plaintext_buf.begin();
	}
	basic_ocbc& operator=(basic_ocbc&& ocbc) noexcept
	{
		if(std::addressof(ocbc)!=this)
		{
			try
			{
				mmflush();
			}
			catch(...){}
			plaintext_buf=std::move(ocbc.plaintext_buf);
			plaintext_buf_pos=std::move(ocbc.plaintext_buf_pos);
			key=std::move(ocbc.key);
			iv=std::move(ocbc.iv);
			ob=std::move(ocbc.ob);
			enc=std::move(ocbc.enc);
			ocbc.plaintext_buf_pos=plaintext_buf.begin();
		}
	}
	~basic_ocbc()
	{
		try
		{
			write_remain();
		}
		catch (...)
		{
		}
	}
	void swap(basic_ocbc &other) noexcept
	{
		using std::swap;
		swap(plaintext_buf,other.plaintext_buf);
		swap(plaintext_buf_pos,other.plaintext_buf_pos);
		swap(key,other.key);
		swap(key,other.iv);
		swap(ob,other.ob);
		swap(enc,other.enc);
	}
};

template <output_stream T, typename Enc>
inline void swap(basic_ocbc<T,Enc>& a,basic_ocbc<T,Enc>& b) noexcept
{
	a.swap(b);
}

template <input_stream T, typename Enc,std::contiguous_iterator Iter>
inline constexpr auto receive(basic_icbc<T,Enc>& cbc,Iter begin,Iter end)
{
	return cbc.mmreceive(begin,end);
}

template <bool err=false,input_stream T, typename Enc>
inline constexpr auto get(basic_icbc<T,Enc>& cbc)
{
	if constexpr(err)
		return cbc.mmtry_get();
	else
		return cbc.mmget();
}

template <output_stream T, typename Enc,std::contiguous_iterator Iter>
inline constexpr void send(basic_ocbc<T,Enc>& cbc,Iter cbegin,Iter cend)
{
	cbc.mmsend(cbegin,cend);
}

template <output_stream T, typename Enc>
inline constexpr void flush(basic_ocbc<T,Enc>& cbc)
{
	cbc.mmflush();
}
template <output_stream T, typename Enc>
inline constexpr void put(basic_ocbc<T,Enc>& cbc,typename basic_ocbc<T,Enc>::char_type ch)
{
	cbc.mmput(ch);
}

} // namespace fast_io::crypto
