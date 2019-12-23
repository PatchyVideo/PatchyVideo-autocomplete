#pragma once

#include"fast_io_core.h"
#include"fast_io_hash/sha1.h"
#include"fast_io_hash/hmac.h"

namespace fast_io
{
using hmac_sha1=hmac<sha1>;
}