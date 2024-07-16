// vi_timing_proxy.h - Proxy header for the vi_timing library

#ifndef _VI_TIMING_PROXY_H_
#	define _VI_TIMING_PROXY_H_ "3.0.0.1"
#	pragma once

// Check if the vi/timing.h header is available and include it if possible
#	if !defined(VI_TM_DISABLE) && __has_include(<vi/timing.h>)
#		include <vi/timing.h>
#	else
// Define fallback macros if vi/timing.h is not available or VI_TM_DISABLE is defined

// Auxiliary macro
#		ifdef __cplusplus
#			define VI_MAYBE_UNUSED [[maybe_unused]]
#		else
#			define VI_MAYBE_UNUSED
#		endif

// Auxiliary macros for combining tokens and generating a unique identifier.
#		define VI_STR_AUX( a, b ) a##b
#		define VI_STR( a, b ) VI_STR_AUX( a, b )
#		define VI_MAKE_UNIC_ID( prefix ) VI_STR( prefix, __LINE__ )

// Fallback macros for timing functions
#		define VI_TM_INIT(...) VI_MAYBE_UNUSED int VI_MAKE_UNIC_ID(_vi_tm_dummy_) = (__VA_ARGS__, 0)
#		define VI_TM(...) VI_MAYBE_UNUSED int VI_MAKE_UNIC_ID(_vi_tm_dummy_) = (__VA_ARGS__, 0)
#		define VI_TM_REPORT(...) (void)(__VA_ARGS__)
#		define VI_TM_CLEAR(s) ((void)(s))
#		define VI_TM_FUNC (void)0
#	endif

#endif // #ifndef _VI_TIMING_PROXY_H_
