#pragma once

namespace fast_io::crypto
{

template <input_stream T, typename Dec>
class basic_iecb
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
	T ib;
	Dec dec;

	unsigned_char_type *mread(unsigned_char_type *pb, unsigned_char_type *pe)
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
			cipher_buf_pos = reads(ib,cipher_buf_pos, cipher_buf.end());
			if (cipher_buf_pos != cipher_buf.end())
				return pi;

			auto plain(dec(cipher_buf.data()));
			cipher_buf_pos = cipher_buf.begin();

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

	template<typename T1, typename ...Args>
	requires std::constructible_from<key_type, T1>  && std::constructible_from<T,Args...>
	basic_iecb(T1 &&init_key, Args&& ...args) : key(std::forward<T1>(init_key)), ib(std::forward<Args>(args)...), dec(key.data()){}


	template<std::contiguous_iterator Iter>
	Iter reads(Iter begin, Iter end)
	{
		auto bgchadd(static_cast<unsigned_char_type *>(static_cast<void *>(std::to_address(begin))));
		return begin + (mread(bgchadd, static_cast<unsigned_char_type *>(static_cast<void *>(std::to_address(end)))) - bgchadd) / sizeof(*begin);
	}
	char_type get()
	{
		if (plaintext_buf_pos == plaintext_buf.begin())
		{
			block_type tmp;
			auto next_ch(tmp.begin() + 1);
			auto ret(mread(tmp.data(), std::to_address(next_ch)));
			if (ret != next_ch)
				throw eof();
			return static_cast<char_type>(*tmp.begin());
		}
		auto ch(*plaintext_buf_pos);
		if (plaintext_buf_pos == plaintext_buf.end())
			plaintext_buf_pos = plaintext_buf.begin();
		else
			++plaintext_buf_pos;
		return static_cast<char_type>(ch);
	}

	std::pair<char_type, bool> try_get()
	{
		if (plaintext_buf_pos == plaintext_buf.begin())
		{
			block_type tmp;
			auto next_ch(tmp.begin() + 1);
			auto ret(mread(tmp.data(), std::to_address(next_ch)));
			if (ret != next_ch)
				return {0, true};
			return {static_cast<char_type>(*tmp.begin()), false};
		}
		auto ch(*plaintext_buf_pos);
		if (plaintext_buf_pos == plaintext_buf.end())
			plaintext_buf_pos = plaintext_buf.begin();
		else
			++plaintext_buf_pos;
		return {static_cast<char_type>(ch), false};
	}
};

template <output_stream T, typename Enc>
class basic_oecb
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
	T ob;
	Enc enc;

	void write_remain()
	{
		if (plaintext_buf_pos != plaintext_buf.begin())
		{
			std::uninitialized_fill(plaintext_buf_pos, plaintext_buf.end(), 0);
			
			auto cipher(enc(plaintext_buf.data()));
			writes(ob,cipher.cbegin(), cipher.cend());
			plaintext_buf_pos = plaintext_buf.begin();
		}
	}

public:

	template<typename T1, typename ...Args>
	requires std::constructible_from<key_type, T1>  && std::constructible_from<T, Args...>
	basic_oecb(T1 &&init_key, Args&& ...args) : key(std::forward<T1>(init_key)), ob(std::forward<Args>(args)...), enc(key.data()){}
	
	void mmflush()
	{
		write_remain();
		flush(ob);
	}

	template<std::contiguous_iterator Iter>
	void mmwrites(Iter b, Iter e)
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

			auto cipher(enc(plaintext_buf.data()));
			writes(ob,cipher.cbegin(), cipher.cend());

			plaintext_buf_pos = plaintext_buf.begin();
		}

		for (; pi + cipher_type::block_size <= pe; pi += cipher_type::block_size)
		{
			auto cipher(enc(pi));
			writes(ob,cipher.cbegin(), cipher.cend());
		}
		plaintext_buf_pos = std::uninitialized_copy(pi, pe, plaintext_buf.begin());
	}

	void mmput(char_type ch) {
		if (plaintext_buf_pos == plaintext_buf.end())
		{
			auto cipher(enc(plaintext_buf.data()));
			writes(ob,cipher.cbegin(), cipher.cend());
			plaintext_buf_pos = plaintext_buf.begin();
		}
		*plaintext_buf_pos = static_cast<unsigned_char_type>(ch);
		++plaintext_buf_pos;
	}
	basic_oecb(basic_oecb const&)=delete;
	basic_oecb& operator=(basic_oecb const&)=delete;
	basic_oecb(basic_oecb&& oecb) noexcept:
		plaintext_buf(std::move(plaintext_buf)),
		plaintext_buf_pos(std::move(oecb.plaintext_buf_pos)),
		key(std::move(oecb.key)),
		ob(std::move(oecb.ob)),
		enc(std::move(oecb.enc))
	{
		oecb.plaintext_buf_pos=plaintext_buf.begin();
	}
	basic_oecb& operator=(basic_oecb&& oecb) noexcept
	{
		if(std::addressof(oecb)!=this)
		{
			try
			{
				mmflush();
			}
			catch(...){}
			plaintext_buf=std::move(oecb.plaintext_buf);
			plaintext_buf_pos=std::move(oecb.plaintext_buf_pos);
			key=std::move(oecb.key);
			ob=std::move(oecb.ob);
			enc=std::move(oecb.enc);
			oecb.plaintext_buf_pos=plaintext_buf.begin();
		}
	}
	~basic_oecb()
	try
	{
		write_remain();
	}
	catch (...)
	{
	}
	void swap(basic_oecb &other) noexcept
	{
		using std::swap;
		swap(plaintext_buf,other.plaintext_buf);
		swap(plaintext_buf_pos,other.plaintext_buf_pos);
		swap(key,other.key);
		swap(ob,other.ob);
		swap(enc,other.enc);
	}
};

template <output_stream T, typename Enc>
inline void swap(basic_oecb<T,Enc>& a,basic_oecb<T,Enc>& b) noexcept
{
	a.swap(b);
}


template <input_stream T, typename Enc,std::contiguous_iterator Iter>
inline constexpr auto reads(basic_iecb<T,Enc>& ecb,Iter begin,Iter end)
{
	return ecb.mmreads(begin,end);
}


template <input_stream T, typename Enc>
inline constexpr auto try_get(basic_iecb<T,Enc>& ecb)
{
	return ecb.mmtry_get();
}

template <input_stream T, typename Enc>
inline constexpr auto get(basic_iecb<T,Enc>& ecb)
{
	return ecb.mmget();
}

template <output_stream T, typename Enc,std::contiguous_iterator Iter>
inline constexpr void writes(basic_oecb<T,Enc>& ecb,Iter cbegin,Iter cend)
{
	ecb.mmwrites(cbegin,cend);
}

template <output_stream T, typename Enc>
inline constexpr void flush(basic_oecb<T,Enc>& ecb)
{
	ecb.mmflush();
}
template <output_stream T, typename Enc>
inline constexpr void put(basic_oecb<T,Enc>& ecb,typename basic_oecb<T,Enc>::char_type ch)
{
	ecb.mmput(ch);
}

} // namespace fast_io::crypto
