#ifndef VI_TIMING_LUAVSPYTHON__STATISTICS_H__
#	define VI_TIMING_LUAVSPYTHON__STATISTICS_H__
#	pragma once

#include <vector>

namespace misc
{
	struct statistics_t
	{
		double average_;
		double median_;
		double min_;
		double max_;
		double deviation_;
	};

	statistics_t calc_stat(const std::vector<double> &data);
}

#endif // #ifndef VI_TIMING_LUAVSPYTHON__STATISTICS_H__
