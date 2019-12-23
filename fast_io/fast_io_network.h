#pragma once

//fast_io_network.h deals with sockets
#include"fast_io_hosted.h"
#include"fast_io_network/network.h"

namespace fast_io
{
using acceptor_buf = self_tie<basic_iobuf<acceptor>>;
using client_buf = self_tie<basic_iobuf<client>>;
}