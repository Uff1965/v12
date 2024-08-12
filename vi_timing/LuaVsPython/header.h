#ifndef VI_TIMING_LUAVSPYTHON_HEADER_H_
#   define VI_TIMING_LUAVSPYTHON_HEADER_H_
#	pragma once

#if defined(_MSC_VER) && defined(_DEBUG)
//  To enable all the debug heap functions, include the following statements in your C++ program, in the following order
//  https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library?view=msvc-170
#   define _CRTDBG_MAP_ALLOC //-V2573 //-V3547
#   include <stdlib.h>
#   include <crtdbg.h>
#else
#	include <stdlib.h>
#endif

#include <cassert>
#include <utility>

inline bool verify(bool b) { assert(b); return b; } //-V:verify:530 //-V3549

namespace vi_gsl
{
    template<typename F>
    class final_action
    {   F action_;
    public:
        explicit final_action(F&& f) noexcept : action_(std::forward<F>(f)) {}
        ~final_action() noexcept { action_(); }

        final_action(final_action const&) = delete;
        void operator=(final_action const&) = delete;
    };

    template< typename F >
    inline [[nodiscard]] auto finally(F&& action)
    {   return final_action<typename std::decay_t<F>>{std::forward<F>(action)};
    }
} // namespace vi_gsl

#endif // #ifndef VI_TIMING_LUAVSPYTHON_HEADER_H_
