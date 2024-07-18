#ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	define VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	pragma once

#	ifdef NDEBUG
constexpr auto sample_size = 10'000U;
constexpr auto CNT = 1'000U;
#	else
constexpr auto sample_size = 1'000U;
constexpr auto CNT = 1'000U;
#	endif
extern const int(&sample_raw)[sample_size];
extern const int(&sample_sorted)[sample_size];

#endif // #ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
