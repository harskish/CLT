#pragma once

/* Choose between cl.hpp and cl2.hpp */

#ifdef CLT_CL_LEGACY_HEADER
#include "CL/cl.hpp"
#else
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include "../external/CL/cl2.hpp"
#endif
