// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*****************************************************************************\
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

#include <timing.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cfloat>
#include <chrono>
#include <climits>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ch = std::chrono;
using namespace std::literals;

namespace
{
	constexpr auto operator""_ps(long double v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ps(unsigned long long v) noexcept { return ch::duration<double, std::pico>(v); };
	constexpr auto operator""_ks(long double v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_ks(unsigned long long v) noexcept { return ch::duration<double, std::kilo>(v); };
	constexpr auto operator""_Ms(long double v) noexcept { return ch::duration<double, std::mega>(v); };
	constexpr auto operator""_Ms(unsigned long long v) noexcept { return ch::duration<double, std::mega>(v); };

	[[nodiscard]] constexpr double round(double num, unsigned char prec, unsigned char dec = 1)
	{ // Rounding to 'dec' decimal place and no more than 'prec' significant symbols.
		assert(num >= 0 && prec > dec && prec <= 3 + dec);

		double result = num;
		if (num >= 0 && prec > dec && prec <= 3 + dec)
		{	auto power = static_cast<signed char>(std::floor(std::log10(num)));
			auto t = 1U + (3 + power % 3) % 3;
			if (prec > t)
			{	t += std::min(dec, static_cast<unsigned char>(prec - t));
			}
			else
			{	t = prec;
			}
			power -= t - 1;

			const auto factor = std::pow(10, -power);
			result = std::round(factor * (num * (1 + std::numeric_limits<decltype(num)>::epsilon()))) / factor;
		}

		return result;
	}

#ifndef NDEBUG
	const auto unit_test_round = []
	{
		struct
		{	int line_;
			double org_;
			double rnd_;
			unsigned char precision_ = 2;
			unsigned char dec_ = 1;
		}
		constexpr static samples[] =
		{
			{__LINE__, 0.0, 0.0, 1, 0},
			{__LINE__, 0.0, 0.0, 2, 1},
			{__LINE__, 0.0, 0.0, 4, 1},
			{__LINE__, 0.0, 0.0, 5, 2},

			{__LINE__, 1.23456789, 1.0, 1, 0},
			{__LINE__, 1.23456789, 1.2, 2, 1},
			{__LINE__, 1.23456789, 1.2, 4, 1},
			{__LINE__, 1.23456789, 1.23, 5, 2},

			{__LINE__, 123.456789, 100.0, 1, 0},
			{__LINE__, 123.456789, 120.0, 2, 1},
			{__LINE__, 123.456789, 123.5, 4, 1},
			{__LINE__, 123.456789, 123.46, 5, 2},

			{__LINE__, 12.3456789e3, 10.0e3, 1, 0},
			{__LINE__, 12.3456789e3, 12.0e3, 2, 1},
			{__LINE__, 12.3456789e3, 12.3e3, 4, 1},
			{__LINE__, 12.3456789e3, 12.35e3, 5, 2},

			{__LINE__, 0.123456789, 0.10, 1, 0},
			{__LINE__, 0.123456789, 0.12, 2, 1},
			{__LINE__, 0.123456789, 0.1235, 4, 1},
			{__LINE__, 0.123456789, 0.12346, 5, 2},

			{__LINE__, 0.00123456789, 0.001, 1, 0},
			{__LINE__, 0.00123456789, 0.0012, 2, 1},
			{__LINE__, 0.00123456789, 0.0012, 4, 1},
			{__LINE__, 0.00123456789, 0.00123, 5, 2},

			{__LINE__, 0.0123456789e-3, 0.010e-3, 1, 0},
			{__LINE__, 0.0123456789e-3, 0.012e-3, 2, 1},
			{__LINE__, 0.0123456789e-3, 0.0123e-3, 4, 1},
			{__LINE__, 0.0123456789e-3, 0.01235e-3, 5, 2},
		};

		for (auto& i : samples)
		{	const auto rnd = round(i.org_, i.precision_, i.dec_);
			assert(std::max(rnd, i.rnd_)* DBL_EPSILON >= std::abs(rnd - i.rnd_));
		}

		return 0;
	}();
#endif // #ifndef NDEBUG

	struct duration_t : ch::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T&& v) : ch::duration<double>{ std::forward<T>(v) } {}

		[[nodiscard]] friend std::string to_string(duration_t sec, unsigned char precision = 2, unsigned char dec = 1)
		{	sec = duration_t{ round(sec.count(), precision, dec) };

			struct { std::string_view suffix_; double factor_{}; } k;
			if (10_ps > sec) { k = { "ps"sv, 1.0 }; }
			else if (1ns > sec) { k = { "ps"sv, 1e12 }; }
			else if (1us > sec) { k = { "ns"sv, 1e9 }; }
			else if (1ms > sec) { k = { "us"sv, 1e6 }; }
			else if (1s > sec) { k = { "ms"sv, 1e3 }; }
			else if (1_ks > sec) { k = { "s "sv, 1e0 }; }
			else if (1_Ms > sec) { k = { "ks"sv, 1e-3 }; }
			else if (1'000_Ms > sec) { k = { "Ms"sv, 1e-6 }; }
			else { k = { "Gs"sv, 1e-9 }; }

			std::ostringstream ss;
			ss << std::fixed << std::setprecision(dec) << sec.count() * k.factor_ << k.suffix_;
			return ss.str();
		}

		[[nodiscard]] friend bool operator<(duration_t l, duration_t r)
		{	return l.count() < r.count() && to_string(l, 2, 1) != to_string(r, 2, 1);
		}
	};

#ifndef NDEBUG
	const auto unit_test_to_string = []
	{
		struct
		{	int line_;
			duration_t sec_;
			std::string_view res_;
			unsigned char precision_{ 2 };
			unsigned char dec_{ 1 };
		}
		static constexpr samples[] =
		{
			{__LINE__, 0s, "0.0ps"},
			{__LINE__, 0.01234567891s, "12.346ms", 6, 3},
			{__LINE__, 0.01234567891s, "12.35ms", 5, 2},
			{__LINE__, 0.1_ps, "0.0ps"},
			{__LINE__, 1_ps, "0.0ps"}, // The lower limit of accuracy is 10ps.
			{__LINE__, 10.01ms, "10.0ms"},
			{__LINE__, 10.1ms, "10.0ms"},
			{__LINE__, 10_ps, "10.0ps"},
			{__LINE__, 100_ps, "100.0ps"},
			{__LINE__, 100ms, "100.0ms"},
			{__LINE__, 100ns, "100.0ns"},
			{__LINE__, 100s, "100.0s "},
			{__LINE__, 100us, "100.0us"},
			{__LINE__, 10ms, "10.0ms"},
			{__LINE__, 10ns, "10.0ns"},
			{__LINE__, 10s, "10.0s "},
			{__LINE__, 10us, "10.0us"},
			{__LINE__, 12.34567891s, "12.346s ", 6, 3},
			{__LINE__, 12.34567891s, "12.35s ", 5, 2},
			{__LINE__, 123.456789_ks, "123.457ks", 6, 3},
			{__LINE__, 123.4ns, "100ns", 1, 0},
			{__LINE__, 123.4ns, "120.0ns", 2, 1},
			{__LINE__, 123.4ns, "120.0ns", 2},
			{__LINE__, 123.4ns, "123.00ns", 3, 2},
			{__LINE__, 123.4ns, "123.0ns", 3},
			{__LINE__, 123.4ns, "123.40ns", 4, 2},
			{__LINE__, 123.4ns, "123.4ns", 4},
			{__LINE__, 1234.56789_ks, "1.2Ms", 3, 1},
			{__LINE__, 1h, "3.6ks"},
			{__LINE__, 1min, "60.0s "},
			{__LINE__, 1ms, "1.0ms"},
			{__LINE__, 1ns, "1.0ns"},
			{__LINE__, 1s, "1.0s "},
			{__LINE__, 1us, "1.0us"},
			{__LINE__, 4.499999999999ns, "4.50ns", 3, 2},
			{__LINE__, 4.499999999999ns, "4.50ns", 4, 2},
			{__LINE__, 4.499999999999ns, "4.5ns", 2, 1},
			{__LINE__, 4.499999999999ns, "4.5ns", 2},
			{__LINE__, 4.499999999999ns, "4.5ns", 3},
			{__LINE__, 4.499999999999ns, "4.5ns", 4},
			{__LINE__, 4.499999999999ns, "4ns", 1, 0},
			{__LINE__, 4.999999999999_ps, "0.00ps", 3, 2},
			{__LINE__, 4.999999999999_ps, "0.00ps", 4, 2},
			{__LINE__, 4.999999999999_ps, "0.0ps", 2, 1},
			{__LINE__, 4.999999999999_ps, "0.0ps", 2},
			{__LINE__, 4.999999999999_ps, "0.0ps", 3},
			{__LINE__, 4.999999999999_ps, "0.0ps", 4},
			{__LINE__, 4.999999999999_ps, "0ps", 1, 0},
			{__LINE__, 4.999999999999ns, "5.00ns", 3, 2},
			{__LINE__, 4.999999999999ns, "5.00ns", 4, 2},
			{__LINE__, 4.999999999999ns, "5.0ns", 2, 1},
			{__LINE__, 4.999999999999ns, "5.0ns", 2},
			{__LINE__, 4.999999999999ns, "5.0ns", 3},
			{__LINE__, 4.999999999999ns, "5.0ns", 4},
			{__LINE__, 4.999999999999ns, "5ns", 1, 0},
			{__LINE__, 5.000000000000_ps, "0.00ps", 3, 2},
			{__LINE__, 5.000000000000_ps, "0.00ps", 4, 2},
			{__LINE__, 5.000000000000_ps, "0.0ps", 2},
			{__LINE__, 5.000000000000_ps, "0.0ps", 3},
			{__LINE__, 5.000000000000_ps, "0.0ps", 4},
			{__LINE__, 5.000000000000_ps, "0ps", 1, 0},
			{__LINE__, 5.000000000000ns, "5.00ns", 3, 2},
			{__LINE__, 5.000000000000ns, "5.00ns", 4, 2},
			{__LINE__, 5.000000000000ns, "5.0ns", 2, 1},
			{__LINE__, 5.000000000000ns, "5.0ns", 2},
			{__LINE__, 5.000000000000ns, "5.0ns", 3},
			{__LINE__, 5.000000000000ns, "5.0ns", 4},
			{__LINE__, 5.000000000000ns, "5ns", 1, 0},
			//**********************************
			{__LINE__, 0.0_ps, "0.0ps"},
			{__LINE__, 0.123456789us, "123.5ns", 4},
			{__LINE__, 1.23456789s, "1s ", 1, 0},
			{__LINE__, 1.23456789s, "1.2s ", 3},
			{__LINE__, 1.23456789s, "1.2s "},
			{__LINE__, 1.23456789us, "1.2us"},
			{__LINE__, 1004.4ns, "1.0us", 2},
			{__LINE__, 12.3456789s, "10s ", 1, 0},
			{__LINE__, 12.3456789s, "12.3s ", 3},
			{__LINE__, 12.3456789us, "12.3us", 3},
			{__LINE__, 12.3456s, "12.0s "},
			{__LINE__, 12.34999999ms, "10ms", 1, 0},
			{__LINE__, 12.34999999ms, "12.3ms", 3},
			{__LINE__, 12.34999999ms, "12.3ms", 4},
			{__LINE__, 12.4999999ms, "12.0ms"},
			{__LINE__, 12.4999999ms, "12.5ms", 3},
			{__LINE__, 12.5000000ms, "13.0ms"},
			{__LINE__, 123.456789ms, "123.0ms", 3},
			{__LINE__, 123.456789us, "120.0us"},
			{__LINE__, 123.4999999ms, "123.5ms", 4},
			{__LINE__, 1234.56789us, "1.2ms"},
			{__LINE__, 245.0_ps, "250.0ps"},
			{__LINE__, 49.999_ps, "50.0ps"},
			{__LINE__, 50.0_ps, "50.0ps"},
			{__LINE__, 9.49999_ps, "0.0ps"},
			{__LINE__, 9.9999_ps, "10.0ps"}, // The lower limit of accuracy is 10ps.
			{__LINE__, 9.999ns, "10.0ns"},
			{__LINE__, 99.49999_ps, "99.0ps"},
			{__LINE__, 99.4999ns, "99.0ns"},
			{__LINE__, 99.4ms, "99.0ms"},
			{__LINE__, 99.5_ps, "100.0ps"},
			{__LINE__, 99.5ms, "100.0ms"},
			{__LINE__, 99.5ns, "100.0ns"},
			{__LINE__, 99.5us, "100.0us"},
			{__LINE__, 99.999_ps, "100.0ps"},
			{__LINE__, 999.0_ps, "1.0ns"},
			{__LINE__, 999.45ns, "1us", 1, 0},
			{__LINE__, 999.45ns, "1.0us", 2},
			{__LINE__, 999.45ns, "999.0ns", 3},
			{__LINE__, 999.45ns, "999.5ns", 4},
			{__LINE__, 999.45ns, "999.45ns", 5, 2},
			{__LINE__, 999.55ns, "1.0us", 3},
			{__LINE__, 99ms, "99.0ms"},
		};

		for (auto& i : samples)
		{	const auto str = to_string(i.sec_, i.precision_, i.dec_);
			assert(i.res_ == str);
		}

		return 0;
	}();
#endif // #ifndef NDEBUG

	inline std::ostream& operator<<(std::ostream& os, const duration_t& d)
	{	return os << to_string(d);
	}

	inline auto now() noexcept(noexcept(ch::steady_clock::now()))
	{	return ch::steady_clock::now();
	}

	void warming(bool all, ch::milliseconds ms)
	{
		if (ch::microseconds::zero() == ms)
			return;

		std::atomic_bool done = false; // It must be defined before 'threads'!!!
		auto load = [&done] {while (!done) {/**/ }}; //-V776

		const auto hwcnt = std::thread::hardware_concurrency();
		std::vector<std::thread> threads((all && 1 < hwcnt) ? hwcnt - 1 : 0);
		for (auto& t : threads)
		{	t = std::thread{ load };
		}

		for (const auto stop = now() + ms; now() < stop;)
		{	/*The thread is fully loaded.*/
		}

		done = true;

		for (auto& t : threads)
		{	t.join();
		}
	}

	constexpr auto cache_warmup = 5U;

	duration_t seconds_per_tick()
	{
		auto get_pair = []
			{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
				for (auto n = 0U; n < cache_warmup; ++n) // Cache warm-up.
				{	(void)now();
					(void)vi_tmGetTicks();
				}

				// Are waiting for the start of a new time interval.
				auto next = now();
				for (const auto prev = next; prev >= next; )
				{	next = now();
				}

				return std::tuple{ vi_tmGetTicks(), next };
			};

		const auto [tick1, time1] = get_pair();
		// Load the thread at 100% for 256ms.
		for (auto stop = time1 + 256ms; now() < stop;)
		{/**/}
		const auto [tick2, time2] = get_pair();

		return duration_t(time2 - time1) / (tick2 - tick1);
	}

	duration_t duration()
	{
		static constexpr auto CNT = 100U;

		static auto equal = []
		{	// The order of calling the functions is deliberately broken. To push 'vi_tmGetTicks()' and 'vi_tmFinish()' further apart.
			auto p = vi_tmItem("", 1);
			const auto s = vi_tmGetTicks();
			const auto e = vi_tmGetTicks();
			(void)std::atomic_fetch_add_explicit(p, e - s, std::memory_order::memory_order_relaxed);
		};

		auto start = []
		{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
			for (auto n = 0U; n < cache_warmup; ++n) // Cache warm-up.
			{	equal(); // Preload function and create a service item with empty name ""!
				(void)now();
			}
			// Are waiting for the start of a new time interval.
			auto next = now();
			for (const auto prev = next; next <= prev; next = now())
			{/**/}
			return next;
		};

		// The first measurement is made to determine the pure time of the function call.
		auto s = start();
		for (unsigned int cnt = 0; cnt < CNT; ++cnt)
		{	equal();
		}
		auto e = now();
		const auto pure = e - s;

		// The second measurement is made to determine the dirty time of the function call.
		static constexpr auto CNT_EXT = 20U;
		s = start();
		for (size_t cnt = 0; cnt < CNT; ++cnt)
		{	equal();

			// And CNT_EXT more calls.
			equal(); equal(); equal(); equal(); equal();
			equal(); equal(); equal(); equal(); equal();
			equal(); equal(); equal(); equal(); equal();
			equal(); equal(); equal(); equal(); equal();
		}
		e = now();
		const auto dirty = e - s;

		assert(dirty > pure);
		const auto diff = dirty - pure;
		return duration_t{ std::max(decltype(diff){0}, diff) / static_cast<double>(CNT * CNT_EXT) };
	}

	double overmeasure()
	{
		constexpr auto CNT = 1'000U;
		constexpr auto CNT_EXT = 4U;

		auto start = []
			{	std::this_thread::yield(); // To minimize the likelihood of interrupting the flow between measurements.
				vi_tmTicks_t result;
				// Cache warm-up.
				for (auto n = 0U; n < cache_warmup; ++n)
				{	result = vi_tmGetTicks();
				}
				// Waiting for the time interval to begin.
				for (const auto prev = result; prev >= result; result = vi_tmGetTicks())
				{/**/}
				return result;
			};

		volatile vi_tmTicks_t e;
		vi_tmTicks_t s = start();
		for (auto cnt = 0; cnt < CNT; ++cnt)
		{	e = vi_tmGetTicks();
		}
		const auto pure = e - s;

		s = start();
		for (auto cnt = 0; cnt < CNT; ++cnt)
		{	e = vi_tmGetTicks(); //-V761

			// CNT_EXT calls
			e = vi_tmGetTicks();
			e = vi_tmGetTicks();
			e = vi_tmGetTicks();
			e = vi_tmGetTicks();
		}
		const auto dirty = e - s;

		assert(dirty > pure);
		return (dirty <= pure) ? 0.0 : static_cast<double>(dirty - pure) / (CNT_EXT * CNT);
	}

	double discreteness()
	{	auto CNT = 64U;
		vi_tmTicks_t last, first;
		std::this_thread::yield(); // Reduce the likelihood of interrupting measurements by switching threads.
		for (auto n = 0U; n < cache_warmup; ++n) // Cache warm-up.
		{	last = first = vi_tmGetTicks(); // Preloading a function into cache
		}

		for (auto cnt = CNT; cnt; )
		{	if (const auto c = vi_tmGetTicks(); c != last)
			{	last = c;
				--cnt;
			}
		}

		return static_cast<double>(last - first) / static_cast<double>(CNT);
	}

	constexpr std::string_view title_name_{ "Name" };
	constexpr std::string_view title_average_{ "Avg." };
	constexpr std::string_view title_total_{ "Tot." };
	constexpr std::string_view title_amount_{ "Amt." };
	constexpr std::string_view Ascending { "(^)" };
	constexpr std::string_view Descending{ "(v)" };

	struct traits_t
	{
		struct itm_t
		{
			std::string_view on_name_; // Name
			vi_tmTicks_t on_total_{}; // Total ticks duration
			std::size_t on_amount_{}; // Number of measured units
			std::size_t on_calls_cnt_{}; // To account for overheads

			duration_t total_time_{.0}; // seconds
			duration_t average_{.0}; // seconds
			std::string total_txt_{ "<too few>" };
			std::string average_txt_{ "<too few>" };
			itm_t(const char* n, vi_tmTicks_t t, std::size_t a, std::size_t c) noexcept
				: on_name_{ n }, on_total_{ t }, on_amount_{ a }, on_calls_cnt_{ c }
			{/**/
			}
		};

		std::vector<itm_t> meterages_;

		std::uint32_t flags_;
		const duration_t tick_duration_ = seconds_per_tick();
		const double overmeasure_ = overmeasure(); // ticks
		const double discreteness_ = discreteness(); // ticks
		std::size_t max_amount_{};
		std::size_t max_len_name_{ title_name_.length()};
		std::size_t max_len_total_{ title_total_.length() };
		std::size_t max_len_average_{ title_average_.length() };
		std::size_t max_len_amount_{ title_amount_.length() };
		
		traits_t(std::uint32_t flags) : flags_{ flags }
		{
			const auto ad = Descending.length();
			assert(ad == Ascending.length());

			switch (flags_ & static_cast<uint32_t>(vi_tmSortMask))
			{
			case vi_tmSortByAmount:
				max_len_amount_ += ad;
				break;
			case vi_tmSortByName:
				max_len_name_ += ad;
				break;
			case vi_tmSortByTime:
				max_len_total_ += ad;
				break;
			case vi_tmSortBySpeed:
			default:
				max_len_average_ += ad;
				break;
			}
		}
	};

	int collector_meterages(const char* name, vi_tmTicks_t total, std::size_t amount, std::size_t calls_cnt, void* _traits)
	{
		assert(_traits);
		auto& traits = *static_cast<traits_t*>(_traits);
		assert(calls_cnt && amount >= calls_cnt);

		auto& itm = traits.meterages_.emplace_back(name, total, amount, calls_cnt);

		if
		(	const auto total_over_ticks = traits.overmeasure_ * itm.on_calls_cnt_;
			itm.on_total_ > total_over_ticks + traits.discreteness_ * (1.0 + 0.01 * itm.on_calls_cnt_)
		)
		{	itm.total_time_ = traits.tick_duration_ * (itm.on_total_ - total_over_ticks);
			itm.average_ = itm.total_time_ / itm.on_amount_;
			itm.total_txt_ = to_string(itm.total_time_);
			itm.average_txt_ = to_string(itm.average_);
		}

		traits.max_len_total_ = std::max(traits.max_len_total_, itm.total_txt_.length());
		traits.max_len_average_ = std::max(traits.max_len_average_, itm.average_txt_.length());
		traits.max_len_name_ = std::max(traits.max_len_name_, itm.on_name_.length());

		if (itm.on_amount_ > traits.max_amount_)
		{	traits.max_amount_ = itm.on_amount_;
			auto max_len_amount = static_cast<std::size_t>(std::floor(std::log10(itm.on_amount_)));
			max_len_amount += max_len_amount / 3; // for thousand separators
			max_len_amount += 1;
			traits.max_len_amount_ = std::max(traits.max_len_amount_, max_len_amount);
		}

		return 1; // Continue enumerate.
	}

	template<vi_tmReportFlags E> auto make_tuple(const traits_t::itm_t& v);
	template<vi_tmReportFlags E> bool less(const traits_t::itm_t& l, const traits_t::itm_t& r)
	{	return make_tuple<E>(r) < make_tuple<E>(l);
	}

	template<> auto make_tuple<vi_tmSortByName>(const traits_t::itm_t& v)
	{	return std::tuple{ v.on_name_ };
	}
	template<> auto make_tuple<vi_tmSortBySpeed>(const traits_t::itm_t& v)
	{	return std::tuple{ v.average_, v.total_time_, v.on_amount_, v.on_name_ };
	}
	template<> auto make_tuple<vi_tmSortByTime>(const traits_t::itm_t& v)
	{	return std::tuple{ v.total_time_, v.average_, v.on_amount_, v.on_name_ };
	}
	template<> auto make_tuple<vi_tmSortByAmount>(const traits_t::itm_t& v)
	{	return std::tuple{ v.on_amount_, v.average_, v.total_time_, v.on_name_ };
	}

	struct meterage_comparator_t
	{
		uint32_t flags_{};

		explicit meterage_comparator_t(uint32_t flags) noexcept : flags_{ flags } {}

		bool operator ()(const traits_t::itm_t& l, const traits_t::itm_t& r) const
		{
			auto pr = &less<vi_tmSortBySpeed>;
			switch (flags_ & static_cast<uint32_t>(vi_tmSortMask))
			{
			case static_cast<uint32_t>(vi_tmSortByName):
				pr = less<vi_tmSortByName>;
				break;
			case static_cast<uint32_t>(vi_tmSortByAmount):
				pr = less<vi_tmSortByAmount>;
				break;
			case static_cast<uint32_t>(vi_tmSortByTime):
				pr = less<vi_tmSortByTime>;
				break;
			case static_cast<uint32_t>(vi_tmSortBySpeed):
				break;
			default:
				assert(false);
				break;
			}

			const bool desc = !(static_cast<uint32_t>(vi_tmSortAscending) & flags_);
			return desc ? pr(l, r) : pr(r, l);
		}
	};

	struct meterage_format_t
	{
		const traits_t& traits_;
		const vi_tmLogSTR_t fn_;
		void* const data_;
		std::size_t number_len_{ 0 };
		mutable std::size_t n_{ 0 };

		struct pg_t
		{	char fill_{};
			char left_{};
			char middle_{};
			char right_{};
		};
		static constexpr pg_t pseudographics_[4] =
		{	{'\x20', '\xBA', '\xB3', '\xBA'}, // Normal: fill, left, middle, right
			{'\xCD', '\xC9', '\xD1', '\xBB'}, // Top
			{'\xC4', '\xC7', '\xC5', '\xB6'}, // Middle 
			{'\xCD', '\xC8', '\xCF', '\xBC'}, // Bottom 
		};
		static constexpr pg_t ascetic_[4] =
		{	{'\x20'},
		};
		const pg_t *pg_ = pseudographics_; // ascetic_;

		struct strings_t
		{	std::string number_;
			std::string name_;
			std::string average_;
			std::string total_;
			std::string amount_;
		};

		meterage_format_t(traits_t& traits, vi_tmLogSTR_t fn, void* data);
		[[nodiscard]] std::size_t buffsize() const;
		int print(const strings_t& strings, const pg_t &pg, char fill_name = 0) const;
		int header() const;
		int footer() const;
		int operator ()(int init, const traits_t::itm_t& i) const;
	};

} // namespace {

meterage_format_t::meterage_format_t(traits_t& traits, vi_tmLogSTR_t fn, void* data)
	:traits_{ traits }, fn_{ fn }, data_{ data }
{
	if (auto size = traits_.meterages_.size(); size >= 1)
	{	number_len_ = 1U + static_cast<std::size_t>(std::floor(std::log10(size)));
	}
}

std::size_t meterage_format_t::buffsize() const
{	auto &pg = pg_[0];
	return
		(pg.left_? 2: 0) + number_len_ +
		(pg.middle_? 3: 1) + traits_.max_len_name_ +
		(pg.middle_? 3: 1) + traits_.max_len_average_ +
		(pg.middle_? 3: 1) + traits_.max_len_total_ +
		(pg.middle_? 3: 1) + traits_.max_len_amount_ +
		(pg.right_? 2: 0) + 1;
}

int meterage_format_t::print(const strings_t& strings, const pg_t &pg, char fill_name) const
{	assert(pg.fill_);
	std::ostringstream str;
	str.fill(pg.fill_);

	pg.left_ && str << pg.left_ << pg.fill_;
	str << std::setw(number_len_) << strings.number_ << pg.fill_;
	pg.middle_ && str << pg.middle_ << pg.fill_;
	fill_name && str << std::setfill(fill_name);
	str << std::setw(traits_.max_len_name_ + 1) << std::left << strings.name_;
	fill_name && str << std::setfill(pg.fill_);
	pg.middle_ && str << pg.middle_ << pg.fill_;
	str << std::setw(traits_.max_len_average_) << std::right << strings.average_ << pg.fill_;
	pg.middle_ && str << pg.middle_ << pg.fill_;
	str << std::setw(traits_.max_len_total_) << strings.total_ << pg.fill_;
	pg.middle_ && str << pg.middle_ << pg.fill_;
	str << std::setw(traits_.max_len_amount_) << strings.amount_;
	pg.right_ && str << pg.fill_ << pg.right_;
	str << "\n";

	auto buff = str.str();
	assert(buffsize() == buff.size());
	return fn_(buff.c_str(), data_);
}

int meterage_format_t::header() const
{
	static const strings_t empty{};
	const auto order = (traits_.flags_ & static_cast<uint32_t>(vi_tmSortAscending)) ? Ascending : Descending;
	strings_t strings
	{	"#",
		std::string{title_name_},
		std::string{title_average_},
		std::string{title_total_},
		std::string{title_amount_}
	};

	switch (auto s = traits_.flags_ & static_cast<uint32_t>(vi_tmSortMask))
	{
		case vi_tmSortBySpeed:
			strings.average_ += order;
			break;
		case vi_tmSortByAmount:
			strings.amount_ += order;
			break;
		case vi_tmSortByName:
			strings.name_ += order;
			break;
		case vi_tmSortByTime:
			strings.total_ += order;
			break;
		default:
			assert(false);
			break;
	}

	auto result = 0;

	if(auto& pg = pg_[1]; pg.fill_)
		result += print(empty, pg);

	if (auto& pg = pg_[0]; pg.fill_)
		result += print(strings, pg);

	if (auto& pg = pg_[2]; pg.fill_)
		result += print(empty, pg);

	return result;
}

int meterage_format_t::footer() const
{
	int result = 0;
	if (auto& pg = pg_[3]; pg.fill_)
		result = print({}, pg);
	return result;
}

int meterage_format_t::operator ()(int init, const traits_t::itm_t& i) const
{
	std::ostringstream str;
	{	struct thousands_sep_facet_t final : std::numpunct<char>
		{	char do_thousands_sep() const override { return '\''; }
			std::string do_grouping() const override { return "\x3"; }
		};

		str.imbue(std::locale(str.getloc(), new thousands_sep_facet_t)); //-V2511
		str << i.on_amount_;
	}

	++n_;

	constexpr auto rift = 3;
	const char fill_name = (traits_.meterages_.size() <= rift || n_ % rift) ? 0 : '.';

	strings_t strings
	{	std::to_string(n_),
		std::string{i.on_name_},
		i.average_txt_,
		i.total_txt_,
		str.str()
	};

	return init + print(strings, pg_[0], fill_name);
}

VI_TM_API int VI_TM_CALL vi_tmReport(std::uint32_t flags, vi_tmLogSTR_t fn, void* data)
{
	warming(false, 512ms); //-V601

	traits_t traits{ flags };
	vi_tmResults(collector_meterages, &traits);

	std::sort(traits.meterages_.begin(), traits.meterages_.end(), meterage_comparator_t{ flags });

	std::ostringstream str;
	if (flags & static_cast<uint32_t>(vi_tmShowUnit))
	{	str << "One tick corresponds: " << traits.tick_duration_ << ";\n";
	}
	if (flags & static_cast<uint32_t>(vi_tmShowDiscreteness))
	{	str << "Discreteness: " << traits.tick_duration_ * traits.discreteness_ << ";\n";
	}
	if (flags & static_cast<uint32_t>(vi_tmShowOverhead))
	{	str << "Overmeasure: " << traits.tick_duration_ * traits.overmeasure_ << ";\n";
	}
	if (flags & static_cast<uint32_t>(vi_tmShowDuration))
	{	str << "Measurement duration: " << duration() << ";\n";
	}
	int result = fn(str.str().c_str(), data);

	meterage_format_t mf{ traits, fn, data };
	result += mf.header();
	result += std::accumulate(traits.meterages_.begin(), traits.meterages_.end(), result, mf);
	result += mf.footer();
	return result;
}
