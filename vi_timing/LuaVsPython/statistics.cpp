// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "statistics.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <vector>

namespace
{
	template<typename I>
	constexpr double average(I begin, I end)
	{	return std::accumulate(begin, end, 0.0) / static_cast<double>(std::distance(begin, end));
	}

	template<typename I>
	constexpr double median(I begin, I end)
    {   std::vector<double> buff(begin, end);
        const auto mid = buff.size() / 2;
        std::nth_element(buff.begin(), buff.begin() + mid, buff.end());
        return (buff.size() % 2)? buff[mid]: (*std::max_element(buff.begin(), buff.begin() + mid) + buff[mid]) / 2.0;
    }

	template<typename I>
	constexpr double standard_deviation(I begin, I end)
	{
		auto result = std::accumulate(begin, end, 0.0, [](auto i, auto v) {return i + std::pow(v, 2.0); });
		result *= static_cast<double>(std::distance(begin, end)) / std::pow(std::accumulate(begin, end, 0.0), 2.0);
		result = (result > 1.0) ? std::sqrt(result - 1.0) : 0.0;
		return result;
	}

#ifndef NDEBUG
	const auto test_stat = []
		{	constexpr auto epsilon = 1e-13;
			// Math online calculators: https://www.calculator.net/math-calculator.html

			static constexpr double samples1[] = {5, 2, 4, 4, 4, 5, 5};
			constexpr auto mean1 = 4.1428571428571;
			constexpr auto sd1 = 0.98974331861079 / mean1;
			constexpr auto median1 = 4.0;
			assert(std::abs(average(std::begin(samples1), std::end(samples1)) - mean1) < epsilon);
			assert(std::abs(standard_deviation(std::begin(samples1), std::end(samples1)) - sd1) < epsilon);
			assert(std::abs(median(std::begin(samples1), std::end(samples1)) - median1) < epsilon);

			static constexpr double samples2[] = {5, 2, 4, 7, 4, 4, 5, 5, 9, 3};
			constexpr auto mean2 = 4.8;
			constexpr auto sd2 = 1.8867962264113 / mean2;
			constexpr auto median2 = 4.5;
			assert(std::abs(average(std::begin(samples2), std::end(samples2)) - mean2) < epsilon);
			assert(std::abs(standard_deviation(std::begin(samples2), std::end(samples2)) - sd2) < epsilon);
			assert(std::abs(median(std::begin(samples2), std::end(samples2)) - median2) < epsilon);

			static constexpr double samples3[] =
			{	14.0, 8.9, 1.7, 1.4, 1.7,
				1.4, 8.3, 1.2, 9.1, 11.0,
				6.7, 11.0, 3.3, 12.0, 1.7,
				11.0, 1.4, 1.4, 11.0, 1.2
			};
			constexpr auto mean3 = 5.97;
			constexpr auto sd3 = 4.5676142569179 / mean3;
			constexpr auto median3 = 5.0;
			assert(std::abs(average(std::begin(samples3), std::end(samples3)) - mean3) < epsilon);
			assert(std::abs(standard_deviation(std::begin(samples3), std::end(samples3)) - sd3) < epsilon);
			assert(std::abs(median(std::begin(samples3), std::end(samples3)) - median3) < epsilon);

			return 0;
		}();
#endif
} // namespace {

misc::statistics_t misc::calc_stat(const std::vector<double> &data)
{	statistics_t result;
	result.average_ = average(data.begin(), data.end());
	result.median_ = median(data.begin(), data.end());
	result.min_ = *std::min_element(data.begin(), data.end());
	result.max_ = *std::max_element(data.begin(), data.end());
	result.deviation_ = standard_deviation(data.begin(), data.end());
	return result;
}
