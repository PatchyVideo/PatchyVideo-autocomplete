#pragma once
#include<vector>
#include<complex>

namespace fast_io
{
/*
namespace details
{

Precisions are not enough
template<int op>
void fft(std::vector<std::complex<long double>>& a)
{
	long double constexpr two_pi_op(2*3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825*op);
	if(a.size()<2)
		return;
	std::vector<std::complex<long double>> a1,a2;
	{
		std::size_t const half_n(a.size()>>1);
		a1.reserve(half_n);
		a2.reserve(half_n);
	}
	for(std::size_t i(0);i<a.size();i+=2)
	{
		a1.emplace_back(a[i]);
		a2.emplace_back(a[i+1]);
	}
	fft<op>(a1);
	fft<op>(a2);
	std::complex<long double> w(1,0);
	std::size_t const n(a.size());
	std::complex<long double> wn(cos(two_pi_op/n),sin(two_pi_op/n));
	for(std::size_t i(0),n2(a.size()>>1);i!=n2;++i)
	{
		a[i]=a1[i]+w*a2[i];
		a[i+n2]=a1[i]-w*a2[i];
		w*=wn;
	}
}
}
*/

class natural;

template<character_input_stream input>
inline void scan_define(input&,natural&);

class natural
{
	std::vector<std::uint64_t> cont;
public:
	natural()=default;
	explicit natural(std::uint64_t value):cont(1,value)
	{
		if(!value)
			cont.pop_back();
	}
	explicit natural(std::string_view sv)
	{
		istring_view is(sv);
		scan_define(is,*this);
	}
	auto& vec() {return cont;}
	auto& vec() const{return cont;}
	inline natural& operator+=(natural const& other)
	{
		if(cont.size()<other.cont.size())
			cont.resize(other.cont.size()+1);
		else
			cont.emplace_back(0);
		auto it(cont.begin());
		auto carry(false);
		for(auto const & e : other.cont)
		{
			auto const v(*it);
			carry=(*it+=e+carry)<v;
			++it;
		}
		for(;carry;++it)
			carry=(++*it)==0;
		if(!cont.back())
			cont.pop_back();
		return *this;
	}
	inline natural& operator+=(std::uint64_t u)
	{
		cont.emplace_back(0);
		auto it(cont.begin());
		auto v(*it);
		for(auto carry((*it+=u)<v);carry;carry=(++*++it)==0);
		if(!cont.back())
			cont.pop_back();
		return *this;
	}
	inline natural& operator++()
	{
		*this += 1;
		return *this;
	}
	inline natural operator++(int)
	{
		auto temp(*this);
		++*this;
		return temp;
	}
	inline natural& operator--()
	{
		*this -= 1;
		return *this;
	}
	inline natural operator--(int)
	{
		auto temp(*this);
		--*this;
		return temp;
	}
	inline natural& operator-=(std::uint64_t other)
	{
		auto it(cont.begin());
		auto v(*it);
		for(auto carry(v<=(*it-=other));carry;)
		{
			carry=(*++it)==0;
			--*it;
		};
		trim();
		return *this;
	}
	inline natural& operator-=(natural const& other)
	{
		auto it(cont.begin());
		auto carry(false);
		for(auto const & e : other.cont)
		{
			auto const v(*it);
			carry=v<=(*it-=e+carry);
			++it;
		}
		for(;carry;++it)
		{
			carry=*it==0;
			--*it;
		}
		for(;!cont.back();cont.pop_back());
		return *this;
	}
	inline natural& operator+=(natural&& other)
	{
		if(cont.capacity()<other.cont.capacity()&&other.cont.size()!=other.cont.capacity())
		{
			using std::swap;
			swap(cont,other.cont);
		}
		return *this+=other;
	}
/*
	inline natural& operator*=(natural const& other)
	{
		if(cont.empty()||other.cont.empty())
		{
			cont.clear();
			return *this;
		}
		auto data8(static_cast<std::uint8_t const*>(static_cast<void const*>(cont.data())));
		auto other_data8(static_cast<std::uint8_t const*>(static_cast<void const*>(other.cont.data())));
		std::size_t n(cont.size()*8);
		std::size_t m(other.cont.size()*8);
		std::size_t p(m+n-2);
		std::size_t q(1);
		for(;q<=p;q<<=1);
		std::vector<std::complex<long double>> a;
		a.reserve(q);
		for(std::size_t i(0);i!=n;++i)
			a.emplace_back(data8[i]);
		std::vector<std::complex<long double>> b;
		b.reserve(q);
		for(std::size_t j(0);j!=m;++j)
			b.emplace_back(other_data8[j]);
		a.resize(q);
		details::fft<1>(a);
		b.resize(q);
		details::fft<1>(b);
		std::vector<std::complex<long double>> c;
		c.reserve(q);
		for(std::size_t i(0);i!=q;++i)
			c.emplace_back(a[i]*b[i]);
		details::fft<-1>(c);
		std::vector<std::uint64_t> buffer(p+1);
		for(std::size_t i(0);i!=buffer.size();++i)
			buffer[i]=static_cast<std::uint64_t>(std::round(c[i].real()/q));
		for(std::size_t i(0);i!=buffer.size();++i)
		{
			buffer[i+1]+=buffer[i]>>8;
			buffer[i]&=255;
		}
		for(;!buffer.back();buffer.pop_back());
		cont.clear();
		for(auto i(buffer.cbegin()),e(buffer.cend());i<e;i+=8)
		{
			auto ej(i+8);
			if(e<ej)
				ej=e;
			std::uint64_t v(0);
			for(auto j(ej);j!=i;--j)
			{
				v<<=8;
				v+=j[-1];
			}
			cont.emplace_back(v);
		}
		return *this;
	}*/

	inline explicit operator bool() const
	{
		return !cont.empty();
	}
	inline natural& operator*=(natural&& other)
	{
		if(cont.capacity()<other.cont.capacity()&&cont.size()+other.cont.size()<other.cont.capacity())
		{
			using std::swap;
			swap(cont,other.cont);
		}
		return *this*=other;
	}

	inline natural& operator*=(natural const& other)
	{
		if(cont.empty()||other.cont.empty())
		{
			cont.clear();
			return *this;
		}
		std::size_t const this_size(cont.size()<<1),other_size(other.cont.size()<<1);
		std::vector<std::uint64_t> temp(this_size+other_size+1);
		auto p32(static_cast<std::uint32_t const*>(static_cast<void const*>(cont.data()))),otherp32(static_cast<std::uint32_t const*>(static_cast<void const*>(other.cont.data())));
		for(std::size_t i(0);i!=this_size;++i)
		{
			std::uint64_t const pi64(p32[i]);
			auto ci(temp.data()+i);
			for(std::size_t j(0);j!=other_size;++j)
			{
				std::uint64_t const v(pi64*otherp32[j]);
				ci[j]+=v&UINT32_MAX;
				ci[j+1]+=(ci[j]>>32)+(v>>32);
				ci[j]&=UINT32_MAX;
			}
		}
		for(auto i(temp.begin());i!=temp.end();++i)
		{
			i[1]+=*i>>32;
			*i&=UINT32_MAX;
		}
		cont.clear();
		auto i(temp.cbegin());
		for(;i+2<temp.cend();i+=2)
			cont.emplace_back(*i+(i[1]<<32));
		if(i!=temp.cend())
			cont.emplace_back(temp.back());
		for(;!cont.back();cont.pop_back());
		return *this;
	}

	inline natural& operator*=(std::uint32_t other1)
	{
		if(cont.empty()||!other1)
		{
			cont.clear();
			return *this;
		}
		std::uint64_t const other(other1);
		std::size_t const this_size(cont.size()<<1);
		auto p32(static_cast<std::uint32_t*>(static_cast<void*>(cont.data())));
		std::uint64_t carry(0);
		for(std::size_t i(0);i!=this_size;++i)
		{
			std::uint64_t const temp(p32[i]*other);
			std::uint64_t const carry_temp(temp>>32);
			std::uint64_t const temp1((temp&std::numeric_limits<std::uint32_t>::max())+carry);
			p32[i]=temp1&std::numeric_limits<std::uint32_t>::max();
			carry=carry_temp+(temp1>>32);
		}
		if(carry)
			cont.emplace_back(carry);
		return *this;
	}
	inline bool operator==(natural const& e) const
	{
		return cont==e.cont;
	}
	inline bool operator!=(natural const& e) const
	{
		return cont!=e.cont;
	}
	inline bool operator<(natural const& e) const
	{
		if(cont.size()==e.cont.size())
			return std::lexicographical_compare(cont.crbegin(),cont.crend(),e.cont.crbegin(),e.cont.crend());
		return cont.size()<e.cont.size();
	}
	inline bool operator>(natural const& e) const
	{
		return e.operator<(*this);
	}
	inline bool operator<=(natural const& e) const
	{
		return !e.operator<(*this);
	}
	inline bool operator>=(natural const& e) const
	{
		return !operator<(e);
	}
	inline bool operator==(std::uint64_t e) const
	{
		switch(cont.size())
		{
		case 0:
			return !e;
		case 1:
			return cont.front()==e;
		default:
			return false;
		}
	}
	inline bool operator<(std::uint64_t e) const
	{
		switch(cont.size())
		{
		case 0:
			return e;
		case 1:
			return cont.front()<e;
		default:
			return false;
		}
	}
	inline natural& operator<<=(std::size_t offset)
	{
		std::size_t const quo(offset/64),md(offset%64),mdp(64-md);
		cont.emplace_back(0);
		for(std::size_t i(cont.size()-1);i--;)
		{
			cont[i+1]|=cont[i]>>mdp;
			cont[i]<<=md;
		}
		cont.insert(cont.cbegin(),quo,0);
		for(;!cont.empty()&&!cont.back();cont.pop_back());
		return *this;		
	}
	inline natural& operator>>=(std::size_t offset)
	{
		std::size_t const quo(offset/64),md(offset%64),mdp(64-md);
		if(cont.size()<=quo)
		{
			cont.clear();
			return *this;
		}
		cont.erase(cont.cend()-quo,cont.cend());
		cont[0]>>=md;
		for(std::size_t i(1);i!=cont.size();++i)
		{
			cont[i-1]|=cont[i]<<mdp;
			cont[i]>>=md;
		}
		for(;!cont.empty()&&!cont.back();cont.pop_back());
		return *this;
	}
	inline natural& operator^=(std::uint64_t u)
	{
		if(cont.empty())
		{
			cont.emplace_back(u);
			return *this;
		}
		cont.front()^=u;
		return *this;
	}
	inline natural& operator&=(std::uint64_t u)
	{
		if(cont.empty())
			return *this;
		
		auto const ret(cont.front()&u);
		cont.clear();
		if(ret)
			cont.emplace_back(ret);
		return *this;
	}
	inline natural& operator|=(std::uint64_t u)
	{
		if(cont.empty())
		{
			cont.emplace_back(u);
			return *this;
		}
		cont.front()|=u;
		return *this;
	}

	inline std::size_t size() const
	{
		std::size_t ret((cont.size() - 1) * 64);
		auto last(cont.back());
		while (last) {
			last >>= 1;
			++ret;
		}
		return ret;
	}

	bool operator[](std::size_t b) const
	{
		return (cont[b / 64] >> (b % 64)) & 1;
	}

	inline natural& operator/=(std::uint32_t v)
	{
		if(cont.empty())
			return *this;
		std::size_t const this_size(cont.size()<<1);
		auto p32(static_cast<std::uint32_t*>(static_cast<void*>(cont.data())));
		std::uint64_t temp(0);
		for(std::size_t i(this_size);i--;)
		{
			auto const tot(p32[i]+(temp<<32));
			p32[i]=static_cast<std::uint32_t>(tot/v);
			temp=tot%v;
		}
		for(;!cont.empty()&&!cont.back();cont.pop_back());
		return *this;
	}

	inline natural& operator/=(natural const& rhs)
	{
		fast_io::natural quotient, remainder;
		for (std::size_t i(size()); i--;)
		{
			quotient <<= 1;
			remainder <<= 1;

			if ((*this)[i]) {
				++remainder;
			}

			if (remainder >= rhs) {
				remainder -= rhs;
				++quotient;
			}
		}

		return *this = std::move(quotient);
	}

	inline natural& operator%=(natural const& rhs)
	{
		fast_io::natural quotient, remainder;
		for (std::size_t i(size()); i--;)
		{
			quotient <<= 1;
			remainder <<= 1;

			if ((*this)[i]) {
				++remainder;
			}

			if (remainder >= rhs) {
				remainder -= rhs;
				++quotient;
			}
		}

		remainder.trim();
		return *this = std::move(remainder);
	}

	inline void trim()
	{
		for(;!cont.empty()&&!cont.back();cont.pop_back());
	}

	inline bool front() const
	{
		return cont.front() & 1;
	}
};



inline std::pair<natural, natural> div_mod(fast_io::natural const& lhs, fast_io::natural const& rhs)
{
	std::pair<fast_io::natural, fast_io::natural> value;
	fast_io::natural& quotient = value.first;
	fast_io::natural& remainder = value.second;
	for (std::size_t i(lhs.size()); i--;)
	{
		quotient <<= 1;
		remainder <<= 1;

		if (lhs[i]) {
			++remainder;
		}

		if (remainder >= rhs) {
			remainder -= rhs;
			++quotient;
		}
	}

	remainder.trim();
	return value;
}


template<typename T>
requires std::constructible_from<natural,T>
inline natural operator+(natural a,T&& b)
{
	return a+=std::forward<T>(b);
}

template<typename T>
requires std::constructible_from<natural,T>
inline natural operator-(natural a,T&& b)
{
	return a-=std::forward<T>(b);
}

inline std::uint64_t operator-(std::uint64_t a,natural b)
{
	if(b.vec().empty())
		return a;
	return a-b.vec().front();
}

template<typename T>
requires std::constructible_from<natural,T>
inline natural operator*(natural a,T&& b)
{
	return a*=std::forward<T>(b);
}

template<typename T>
requires std::constructible_from<natural,T>
inline natural operator/(natural a,T&& b)
{
	return a/=std::forward<T>(b);
}

inline std::uint32_t operator/(std::uint32_t a,natural b)
{
	return static_cast<std::uint32_t>(a/b.vec().front());
}

template<typename T>
requires std::constructible_from<natural,T>
inline natural operator%(natural a,T&& b)
{
	return a%=std::forward<T>(b);
}

inline std::uint32_t operator%(std::uint32_t a,natural b)
{
	return a%b.vec().front();
}

template<typename T>
requires std::constructible_from<natural,T>
inline natural operator&(natural a,T&& b)
{
	return a&=std::forward<T>(b);
}

template<typename T>
requires std::constructible_from<natural,T>
inline natural operator|(natural a,T&& b)
{
	return a|=std::forward<T>(b);
}

template<typename T>
requires std::constructible_from<natural,T>
inline natural operator^(natural a,T&& b)
{
	return a^=std::forward<T>(b);
}

inline bool operator==(std::uint64_t b,natural const& a)
{
	return a==b;
}
inline bool operator!=(natural const& a,std::uint64_t b)
{
	return !(a==b);
}
inline bool operator!=(std::uint64_t b,natural const& a)
{
	return a!=b;
}

inline bool operator<(std::uint64_t e,natural const& a)
{
	switch(a.vec().size())
	{
	case 0:
		return false;
	case 1:
		return e<a.vec().front();
	default:
		return true;
	}
}

inline bool operator>(natural const& a,std::uint64_t b)
{
	return b<a;
}

inline bool operator>(std::uint64_t a,natural const& b)
{
	return b<a;
}


inline bool operator<=(std::uint64_t a,natural const& b)
{
	return !(b<a);
}

inline bool operator<=(natural const& a,std::uint64_t b)
{
	return !(b<a);
}

inline bool operator>=(std::uint64_t a,natural const& b)
{
	return !(a<b);
}

inline bool operator>=(natural const& a,std::uint64_t b)
{
	return !(a<b);
}

inline natural operator<<(natural n,std::size_t offset)
{
	return n<<=offset;		
}

inline natural operator>>(natural n,std::size_t offset)
{
	return n>>=offset;		
}

inline std::uint32_t in_place_div_mod(fast_io::natural& n,std::uint32_t value)
{
	auto &cont(n.vec());
	if(cont.empty())
		return 0;
	std::size_t const this_size(cont.size()<<1);
	auto p32(static_cast<std::uint32_t*>(static_cast<void*>(cont.data())));
	std::uint64_t quo(0);
	for(std::size_t i(this_size);i--;)
	{
		auto const tot(p32[i]+(quo<<32));
		p32[i] = static_cast<std::uint32_t>(tot/value);
		quo=tot%value;
	}
	for(;!cont.empty()&&!cont.back();cont.pop_back());
	return static_cast<std::uint32_t>(quo);
}

inline natural pow_mod(fast_io::natural lhs, fast_io::natural rhs, fast_io::natural const& mod)
{
	natural ans(1);
	lhs %= mod;
	while (rhs)
	{
		if(rhs.front())
		{
			ans *= lhs;
			ans = ans % mod;
		}
		rhs >>= 1;
		lhs = (lhs * lhs) % mod;
	} 
	return ans;
}

inline std::pair<fast_io::natural,std::uint32_t> div_mod(fast_io::natural n,std::uint32_t value)
{
	auto temp(in_place_div_mod(n,value));
	return {std::move(n),static_cast<std::uint32_t>(temp)};
}

/*
namespace details
{
template<bool dir = true>
int fft(std::deque<double>& data, bool dir) const
{
	 //Verify size is a power of two
	 std::size_t n = data.size()/2;
	 if ((n == 0) || (n & (n-1))) return 1;

	 //rearrange data for signal flow chart
	 std::size_t bitr_j = 1;
	 for (std::size_t i = 3; i < 2*n-1; i += 2)
	 {
		  std::size_t msz = n;
		  while (bitr_j >= msz)
		  {
			   bitr_j -= msz;
			   msz >>= 1;
		  }
		  bitr_j += msz;

		  if (bitr_j > i)
		  {
			   double swap = data[bitr_j-1];
			   data[bitr_j-1] = data[i-1];
			   data[i-1] = swap;
			   swap = data[bitr_j];
			   data[bitr_j] = data[i];
			   data[i] = swap;
		  }
	 }

	 //Perform "butterfly" calculations
	 std::size_t lmax = 2;
	 while (lmax <= n)
	 {
		  double wr = 1;
		  double wi = 0;

		  double theta = (2*M_PI)/double(lmax*(dir?1.0:-1.0));
		  double wpr = cos(theta);
		  double wpi = sin(theta);

		  int pstep = 2*lmax;
		  for (std::size_t l = 1; l < lmax; l += 2)
		  {
			   for (std::size_t p = l; p < 2*n; p += pstep)
			   {
					std::size_t q = p + lmax;
					double tempr = wr*data[q-1] - wi*data[q];
					double tempi = wr*data[q] + wi*data[q-1];
					data[q-1] = data[p-1] - tempr;
					data[q] = data[p] - tempi;
					data[p-1] = data[p-1] + tempr;
					data[p] = data[p] + tempi;
			   }

			   //Find the next power of W
			   double wtemp = wr;
			   wr = wr*wpr - wi*wpi;
			   wi = wi*wpr + wtemp*wpi;
		  }

		  lmax = pstep;
	 }

	 //All is good
	 return 0;
}
}

natural mul_fft(const natural& lhs, const natural& rhs) const {
	//Convert each integer to input wanted by fft()
	size_t size = 1;
	while (size < lhs.vec().size()*2){
		size <<= 1;
	}
	while (size < rhs.vec().size()*2){
		size <<= 1;
	}

	std::deque<double> lhs_fft;
	lhs_fft.resize(size*2, 0);
	for (size_t i = 0; i < lhs.vec().size(); i++){
		lhs_fft[i*2] = double(lhs.vec()[lhs.vec().size()-1-i]);
	}

	std::deque<double> rhs_fft;
	rhs_fft.resize(size*2, 0);
	for (size_t i = 0; i < rhs.vec().size(); i++){
		rhs_fft[i*2] = double(rhs.vec()[rhs.vec().size()-1-i]);
	}

	//Compute the FFT of each
	details::fft(lhs_fft);
	details::fft(rhs_fft);

	//Perform pointwise multiplication (numbers are complex)
	std::deque<double> out_fft(2*size);
	for (size_t i = 0; i < 2*size; i+=2){
		out_fft[i] = lhs_fft[i]*rhs_fft[i] - lhs_fft[i+1]*rhs_fft[i+1];
		out_fft[i+1] = lhs_fft[i]*rhs_fft[i+1] + lhs_fft[i+1]*rhs_fft[i];
	}

	//Compute the inverse FFT of this number
	//remember to properly scale afterwards!
	details::fft<false>(out_fft);
	for (size_t i = 0; i < 2*size; i++){
		out_fft[i] /= size;
	}

	//Convert back to integer, carrying along the way
	double carry = 0;
	natural out;
	for (size_t i = 0; i < 2*size; i+=2){
		double current = out_fft[i]+carry;
		if (current > double(natural::NEG1)){
			carry = current / (double(natural::NEG1)+1);
			carry = double(floor(carry+0.0001));
			current = current - (carry*(natural::NEG1+1));

		}
		else {
			carry = 0;
		}
		out._value.push_front(INTEGER_DIGIT_T(current+0.0001));
	}

	//Finish up
	return out;
}
*/


namespace details
{

template<std::uint8_t base,bool uppercase,character_output_stream output>
inline void output_base_natural_number(output& out,natural a)
{
//number: 0:48 9:57
//upper: 65 :A 70: F
//lower: 97 :a 102 :f
	if(!a)
	{
		put(out,0x30);
		return;
	}
	std::vector<typename output::char_type> v(a.vec().size()*512/base+3);
	auto iter(v.data()+v.size());
	while(a)
	{
		auto rem=in_place_div_mod(a,base);
		if constexpr(10 < base)
		{
			if(rem<10)
				*--iter = static_cast<typename output::char_type>(rem+48);
			else
			{
				if constexpr (uppercase)
					*--iter = static_cast<typename output::char_type>(rem+55);	
				else
					*--iter = static_cast<typename output::char_type>(rem+87);
			}
		}
		else
			*--iter = static_cast<typename output::char_type>(rem+48);
	}
	send(out,iter,v.data()+v.size());
}


template<std::uint8_t base,character_input_stream input>
inline constexpr void input_base_number_phase2_natural(input& in,natural& a)
{
	using unsigned_char_type = std::make_unsigned_t<decltype(get(in))>;
	unsigned_char_type constexpr baseed(std::min(static_cast<unsigned_char_type>(base),static_cast<unsigned_char_type>(10)));
	while(true)
	{
		unsigned_char_type ch(get<true>(in).first);
		if((ch-=48)<baseed)
		{
			a*=base;
			a+=ch;
		}
		else if constexpr (10 < base)
		{
			unsigned_char_type constexpr bm10(base-10);
			if((ch-=17)<bm10||(ch-=32)<bm10)
			{
				a*=base;
				a+=(ch+10);
			}
			else
				return;
		}
		else
			return;
	}
}

template<std::uint8_t base,character_input_stream input>
inline constexpr void input_base_natural_number(input& in,natural& a)
{
	using unsigned_char_type = std::make_unsigned_t<decltype(get(in))>;
	unsigned_char_type constexpr baseed(std::min(static_cast<unsigned_char_type>(base),static_cast<unsigned_char_type>(10)));
	while(true)
	{
		unsigned_char_type ch(get(in));
		if((ch-=48)<baseed)
		{
			a=static_cast<natural>(ch);
			break;
		}
		else if constexpr (10 < base)
		{
			unsigned_char_type constexpr bm10(base-10);
			if((ch-=17)<bm10||(ch-=32)<bm10)
			{
				a=static_cast<natural>(ch+10);
				break;
			}
		}
	}
	input_base_number_phase2_natural<base>(in,a);
}
}

template<character_output_stream output>
inline constexpr void print_define(output& out,natural const& a)
{
	details::output_base_natural_number<10,false>(out,a);
}

template<std::size_t base,bool uppercase,character_output_stream output>
inline constexpr void print_define(output& out,manip::base_t<base,uppercase,natural const> v)
{
	details::output_base_natural_number<base,uppercase>(out,v.reference);
}
template<std::size_t base,bool uppercase,character_output_stream output>
inline constexpr void print_define(output& out,manip::base_t<base,uppercase,natural> v)
{
	details::output_base_natural_number<base,uppercase>(out,v.reference);
}


template<std::size_t base,bool uppercase,character_input_stream input>
inline constexpr void scan_define(input& in,manip::base_t<base,uppercase,natural> v)
{
	details::input_base_natural_number<base>(in,v.reference);
}

template<character_input_stream input>
inline void scan_define(input& in,natural& a)
{
	details::input_base_natural_number<10>(in,a);
}

template<character_output_stream output>
inline void write(output& out,natural const& n)
{
	write(out,n.vec());
}

template<character_input_stream input>
inline void read_define(input& in,natural& n)
{
	read_define(in,n.vec());
}

namespace literals
{
inline natural operator "" _n(char const* cstr, size_t n)
{
	return natural(std::string_view{cstr, n});
}
inline natural operator "" _nb(char const* cstr, size_t n)
{
	istring_view view{cstr, n};
	natural nt;
	scan_define(view,bin(nt));
	return nt;
}
inline natural operator "" _no(char const* cstr, size_t n)
{
	istring_view view{cstr, n};
	natural nt;
	scan_define(view,oct(nt));
	return nt;
}
inline natural operator "" _nh(char const* cstr, size_t n)
{
	istring_view view{cstr, n};
	natural nt;
	scan_define(view,hex(nt));
	return nt;
}
}

}