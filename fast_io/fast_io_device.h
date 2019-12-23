#pragma once

//fast_io_devices.h defines commonly used io devices and their correlated mutex verisons.
#include"fast_io_hosted.h"


namespace fast_io
{
using pipe = io_wrapper<system_pipe>;

using isystem_file = input_file_wrapper<system_file>;
using osystem_file = output_file_wrapper<system_file>;
using iosystem_file = io_file_wrapper<system_file>;

using sync = basic_sync<basic_file_wrapper<system_file,fast_io::open::app|fast_io::open::binary>,basic_ostring<std::string>>;
using fsync = basic_fsync<basic_file_wrapper<system_file,fast_io::open::app|fast_io::open::binary>,basic_ostring<std::string>>;

using sync_mutex = basic_iomutex<sync>;
using fsync_mutex = basic_iomutex<fsync>;

using ibuf = basic_ibuf<isystem_file>;
using obuf = basic_obuf<osystem_file>;
using iobuf = basic_iobuf<iosystem_file>;

using ibuf_mutex = basic_iomutex<ibuf>;
using obuf_mutex = basic_iomutex<obuf>;
using iobuf_mutex = basic_iomutex<iobuf>;

using dynamic_buf = basic_iobuf<dynamic_stream>;


using u8pipe = io_wrapper<u8system_pipe>;

using u8isystem_file = input_file_wrapper<u8system_file>;
using u8osystem_file = output_file_wrapper<u8system_file>;
using u8iosystem_file = io_file_wrapper<u8system_file>;

using u8sync = basic_sync<basic_file_wrapper<system_file,fast_io::open::app|fast_io::open::binary>,basic_ostring<std::u8string>>;
using u8fsync = basic_fsync<basic_file_wrapper<system_file,fast_io::open::app|fast_io::open::binary>,basic_ostring<std::u8string>>;

using u8sync_mutex = basic_iomutex<u8sync>;
using u8fsync_mutex = basic_iomutex<u8fsync>;

using u8ibuf = basic_ibuf<u8isystem_file>;
using u8obuf = basic_obuf<u8osystem_file>;
using u8iobuf = basic_iobuf<u8iosystem_file>;

using u8ibuf_mutex = basic_iomutex<u8ibuf>;
using u8obuf_mutex = basic_iomutex<u8obuf>;
using u8iobuf_mutex = basic_iomutex<u8iobuf>;

using u8dynamic_buf = basic_iobuf<u8dynamic_stream>;

}