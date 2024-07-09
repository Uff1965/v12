﻿/*****************************************************************************\
'vi_timing' is a small library for approximate estimation of code execution
time in C and C++.

Copyright (C) 2024 V.Igraamar

This library was created as an experiment for educational purposes.
Do not expect much from it. If you spot a bug or can suggest any
improvement to the code, please email me at eMail:programmer.amateur@proton.me.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. 
If not, see <https://www.gnu.org/licenses/gpl-3.0.html#license-text>.
\*****************************************************************************/

//-V122_NOPTR
//-V1042

#ifndef VI_TIMING_TIMING_H
#	define VI_TIMING_TIMING_H 3.0
#	pragma once

#if defined(_WIN32)
#	include <Windows.h>
#elif defined(__linux__)
#	include <time.h> // for clock_gettime
#else
#	error "Unknown OS!"
#endif

#if defined(__x86_64__) || defined(__amd64__) // GNU on Intel
#   include <x86intrin.h>
#elif defined(_M_X64) || defined(_M_AMD64) // MS compiler for x64 or ARM64EC
#	include <intrin.h>
#	pragma intrinsic(__rdtscp, _mm_lfence)
#endif

#ifdef __cplusplus
#	include <atomic>
#	include <cstdint>
#	include <cstdio>
#	include <string>
#else
#	include <stdatomic.h>
#	include <stdint.h>
#	include <stdio.h>
#endif

#include "common.h"

// Define VI_TM_CALL and VI_TM_API vvvvvvvvvvvvvv
#if defined(_WIN32) // Windows x86 or x64
#	define VI_SYS_CALL __cdecl
#	ifdef _WIN64
#		define VI_TM_CALL
#	else
#		define VI_TM_CALL __fastcall
#	endif
#	ifdef vi_timing_EXPORTS
#		define VI_TM_API __declspec(dllexport)
#	else
#		define VI_TM_API __declspec(dllimport)
#	endif
#elif defined(__ANDROID__)
#	define CM_TM_DISABLE "Android not supported yet."
#	error "Android not supported yet."
#elif defined (__linux__)
#	define VI_SYS_CALL
#	define VI_TM_CALL
#	ifdef vi_timing_EXPORTS
#		define VI_TM_API __attribute__((visibility("default")))
#	else
#		define VI_TM_API
#	endif
#else
#	define CM_TM_DISABLE "Unknown platform!"
#	error "Unknown platform!"
#endif
// Define VI_TM_CALL and VI_TM_API ^^^^^^^^^^^^^^^^^^^^^^^

typedef VI_STD(uint64_t) vi_tmTicks_t;
typedef int (VI_SYS_CALL *vi_tmLogRAW_t)(const char* name, vi_tmTicks_t time, VI_STD(size_t) amount, VI_STD(size_t) calls_cnt, void* data);
typedef int (VI_SYS_CALL *vi_tmLogSTR_t)(const char* str, void* data); // Must be compatible with std::fputs!

#ifdef __cplusplus
	using vi_tmAtomicTicks_t = std::atomic<vi_tmTicks_t>;
#elif defined( __STDC_NO_ATOMICS__)
	// "<...> we left out support for some C11 optional features such as atomics <...>" [Microsoft]
	//	[https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#	error "Atomic objects and the atomic operation library are not supported."
#else
	typedef _Atomic(vi_tmTicks_t) vi_tmAtomicTicks_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Definition of vi_tmGetTicks() function for different platforms. vvvvvvvvvvvv
#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__amd64__) // MSC, GCC or CLANG on Intel
	static inline vi_tmTicks_t vi_tmGetTicks(void)
	{
//*
		VI_STD(uint32_t) _;
		const VI_STD(uint64_t) result = __rdtscp(&_);
		_mm_lfence();
		return result;
/*/
		return __rdtsc();
//*/
	}
#elif __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)
	static inline vi_tmTicks_t vi_tmGetTicks(void)
	{	VI_STD(uint64_t) result;
		__asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(result));
		return result;
	}
#elif defined(_WIN32) // Windows on other platforms.
	static inline vi_tmTicks_t vi_tmGetTicks(void)
	{	LARGE_INTEGER cnt;
		QueryPerformanceCounter(&cnt);
		return cnt.QuadPart;
	}
#elif defined(__linux__) // Linux on other platforms
	static inline vi_tmTicks_t vi_tmGetTicks(void)
	{	struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		return 1000000000ULL * ts.tv_sec + ts.tv_nsec;
	}
#else
#	error "You need to define function(s) for your OS and CPU"
#endif
// Definition of vi_tmGetTicks() function for different platforms. ^^^^^^^^^^^^

	// Main functions vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	VI_TM_API void VI_TM_CALL vi_tmInit(VI_STD(size_t) n);
	VI_TM_API vi_tmAtomicTicks_t* VI_TM_CALL vi_tmItem(const char* name, VI_STD(size_t) amount VI_DEFARG(1));
	VI_TM_API int VI_TM_CALL vi_tmResults(vi_tmLogRAW_t fn, void* data);
	VI_TM_API void VI_TM_CALL vi_tmClear(const char* name VI_DEFARG((const char*)0));
	// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// Supporting functions. vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	enum vi_tmReportFlags {
		vi_tmSortByTime = 0x00,
		vi_tmSortByName = 0x01,
		vi_tmSortBySpeed = 0x02,
		vi_tmSortByAmount = 0x03,
		vi_tmSortMask = 0x07, // 0b0000'0111

		vi_tmSortDescending = 0x00,
		vi_tmSortAscending = 0x08, // 0b0000'1000

		vi_tmShowOverhead = 0x0010,
		vi_tmShowUnit = 0x0020,
		vi_tmShowDuration = 0x0040,
		vi_tmShowDiscreteness = 0x0080,
		vi_tmShowMask = 0xF0, // 0b1111'0000
	};

	VI_TM_API int VI_TM_CALL vi_tmReport(VI_STD(uint32_t) flags, vi_tmLogSTR_t fn, void* data);
	struct vi_tmItem_t
	{	vi_tmAtomicTicks_t* item_;
		vi_tmTicks_t start_; // Order matters!!! 'start_' must be initialized last!
	};
	static inline struct vi_tmItem_t vi_tmStart(const char* name, VI_STD(size_t) amount VI_DEFARG(1)) VI_NOEXCEPT
	{	struct vi_tmItem_t result;
		result.item_ = vi_tmItem(name, amount);
		result.start_ = vi_tmGetTicks();
		return result;
	}
	static inline void vi_tmFinish(const struct vi_tmItem_t *itm) VI_NOEXCEPT
	{	const vi_tmTicks_t end = vi_tmGetTicks();
		(void)VI_STD(atomic_fetch_add_explicit)(itm->item_, end - itm->start_, VI_MEMORY_ORDER(memory_order_relaxed));
	}

	VI_TM_API void VI_TM_CALL vi_tmWarming(int all, unsigned int ms);

	// Supporting functions. ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
} // extern "C" {

namespace vi_tm
{
	struct timer_t: vi_tmItem_t
	{	timer_t(const timer_t&) noexcept = delete;
		timer_t& operator=(const timer_t&) noexcept = delete;
		timer_t(const char* name, std::size_t amount = 1) noexcept : vi_tmItem_t{ vi_tmStart(name, amount) } {}
		~timer_t() noexcept { vi_tmFinish(this); }
	};

	class init_t
	{	std::string title_;
		vi_tmLogSTR_t cb_;
		void* data_;
		std::uint32_t flags_;
	public:
		init_t
		(	std::uint32_t flags = vi_tmSortByTime,
			std::string title = "Timing report:",
			vi_tmLogSTR_t fn = reinterpret_cast<vi_tmLogSTR_t>(&std::fputs),
			void* data = stdout,
			std::size_t reserve = 64
		)
		: title_{ std::move(title) }, cb_{ fn }, data_{ data }, flags_{ flags }
		{	vi_tmInit(reserve);
		}

		~init_t()
		{	if (!title_.empty())
			{	(void)cb_((title_ + '\n').c_str(), data_);
			}

			(void)vi_tmReport(flags_, cb_, data_);
		}
	};

	inline void clear(const char* name)
	{	vi_tmClear(name);
	}

	inline int report
	(	std::uint32_t flags = vi_tmSortByTime,
		vi_tmLogSTR_t fn = reinterpret_cast<vi_tmLogSTR_t>(&std::fputs),
		void* data = stdout
	)
	{	return vi_tmReport(flags, fn, data);
	}

	inline void warming(bool all = false, unsigned int ms = 500) { vi_tmWarming((all? 1: 0), ms); }

} // namespace vi_tm {

#	if defined(VI_TM_DISABLE)
#		define VI_TM_INIT(...) VI_MAYBE_UNUSED int VI_MAKE_UNIC_ID(_vi_tm_dummy_) = (__VA_ARGS__, 0)
#		define VI_TM(...) VI_MAYBE_UNUSED int VI_MAKE_UNIC_ID(_vi_tm_dummy_) = (__VA_ARGS__, 0)
#		define VI_TM_REPORT(...) ((void)(__VA_ARGS__, 0))
#		define VI_TM_CLEAR(s) ((void)(s))
#	else
#		define VI_TM_INIT(...) vi_tm::init_t VI_MAKE_UNIC_ID(_vi_tm_init_)(__VA_ARGS__)
#		define VI_TM(...) vi_tm::timer_t VI_MAKE_UNIC_ID(_vi_tm_variable_) (__VA_ARGS__)
#		define VI_TM_REPORT(...) vi_tm::report(__VA_ARGS__)
#		define VI_TM_CLEAR(...) vi_tmClear(__VA_ARGS__)
#	endif

#	define VI_TM_FUNC VI_TM( VI_FUNCNAME )
#endif // #if !defined(__cplusplus) ^^^

#endif // #ifndef VI_TIMING_TIMING_H
