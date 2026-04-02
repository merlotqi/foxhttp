#pragma once

/// \file config.hpp
/// Compile-time feature configuration for FoxHttp.
///
/// FOXHTTP_HAS_COROUTINES is automatically set based on the C++ standard:
///   - C++20 or later: coroutines enabled (value = 1)
///   - C++17: coroutines disabled (value = 0)

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#  define FOXHTTP_HAS_COROUTINES 1
#else
#  define FOXHTTP_HAS_COROUTINES 0
#endif
