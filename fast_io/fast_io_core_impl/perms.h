#pragma once

namespace fast_io
{
//https://github.com/gcc-mirror/gcc/blob/master/libstdc++-v3/include/bits/fs_fwd.h
enum class perms : unsigned
{
none		=  0,
owner_read	=  0400,
owner_write	=  0200,
owner_exec	=  0100,
owner_all	=  0700,
group_read	=   040,
group_write	=   020,
group_exec	=   010,
group_all	=   070,
others_read	=    04,
others_write	=    02,
others_exec	=    01,
others_all	=    07,
all		=  0777,
set_uid		= 04000,
set_gid		= 02000,
sticky_bit	= 01000,
mask		= 07777,
unknown		= 0xFFFF,
add_perms	= 0x10000,
remove_perms	= 0x20000,
symlink_nofollow= 0x40000
};

constexpr perms operator&(perms x, perms y) noexcept
{
using utype = typename std::underlying_type<perms>::type;
return static_cast<perms>(static_cast<utype>(x) & static_cast<utype>(y));
}

constexpr perms operator|(perms x, perms y) noexcept
{
using utype = typename std::underlying_type<perms>::type;
return static_cast<perms>(static_cast<utype>(x) | static_cast<utype>(y));
}

constexpr perms operator^(perms x, perms y) noexcept
{
using utype = typename std::underlying_type<perms>::type;
return static_cast<perms>(static_cast<utype>(x) ^ static_cast<utype>(y));
}

constexpr perms operator~(perms x) noexcept
{
using utype = typename std::underlying_type<perms>::type;
return static_cast<perms>(~static_cast<utype>(x));
}

inline perms& operator&=(perms& x, perms y) noexcept{return x=x&y;}

inline perms& operator|=(perms& x, perms y) noexcept{return x=x|y;}

inline perms& operator^=(perms& x, perms y) noexcept{return x=x^y;}

template<perms pm>
struct perms_interface_t
{
inline constexpr static fast_io::open::mode permission = {pm};
explicit constexpr perms_interface_t()=default;
};

template<perms pm>
inline constexpr perms_interface_t<pm> perms_interface{};

namespace details::perm
{
template<char fillch,character_output_stream output>
inline constexpr void print_perm_per_check(output& out,perms p,perms checked)
{
	if((p&checked)==perms::none)
		put(out,0x2d);
	else
		put(out,fillch);
}
}


template<character_output_stream output>
inline constexpr void print_define(output& out,perms p)
{
	details::perm::print_perm_per_check<0x72>(out,p,perms::owner_read);
	details::perm::print_perm_per_check<0x77>(out,p,perms::owner_write);
	details::perm::print_perm_per_check<0x78>(out,p,perms::owner_exec);
	details::perm::print_perm_per_check<0x72>(out,p,perms::group_read);
	details::perm::print_perm_per_check<0x77>(out,p,perms::group_write);
	details::perm::print_perm_per_check<0x78>(out,p,perms::group_exec);
	details::perm::print_perm_per_check<0x72>(out,p,perms::others_read);
	details::perm::print_perm_per_check<0x77>(out,p,perms::others_write);
	details::perm::print_perm_per_check<0x78>(out,p,perms::others_exec);
}

}