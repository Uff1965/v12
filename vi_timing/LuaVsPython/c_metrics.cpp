// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"
#include "c_metrics.h"
#include "LuaVsPython.h"

#include <vi_timing/timing.h>

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

#define START(s) \
	std::this_thread::yield(); \
	for (auto n = 5; n--;) \
	{	VI_TM(s); \
	} \
	VI_TM_CLEAR(s); \
	do { VI_TM(s)

#define FINISH \
	} while(0)

using namespace std::string_literals;

namespace
{
	constexpr char sample[] = "global string";
}

namespace ccc
{
	const char global_string[] = "global string";

	// Define an empty function
	void empty_func()
	{	// This function does nothing
	}

	// Define a function that returns the length of a string
	std::size_t strlen_func(const char* sz)
	{	// Return the length of the string
		return sz ? strlen(sz) : 0ULL;
	}

	void bubble_sort(std::vector<int> &a)
	{
		bool swapped;
		do
		{
			swapped = false;
			for (unsigned i = 1; i < a.size(); ++i)
			{
				if (a[i - 1] > a[i])
				{
					a[i - 1] = std::exchange(a[i], a[i - 1]);
					swapped = true;
				}
			}
		} while (swapped);
	}
} // namespace ccc

void c_test()
{	VI_TM_CLEAR(nullptr);

	START(" *** C ***   ");

		START("1 Initialize");
		FINISH;

		START("2 dofile");
		FINISH;

		START("3 Get string");
			const char *sz = ccc::global_string;
			std::size_t len = sz? strlen(sz): 0ULL;
			assert(sz && len == strlen(sample) && 0 == strcmp(sz, sample));
		FINISH;

		START("4 Call empty");
			auto func = ccc::empty_func;
			assert(func);
			func();
		FINISH;

		START("5 Call strlen");
			auto func = ccc::strlen_func;
			assert(func);
			const char *args = ccc::global_string;
			assert(args);
			auto len = func(args);
			assert(strlen(sample) == len);
		FINISH;

		{
			std::vector<int> args;

			START("6 Call bubble_sort (arg init)");
				// Создаем аргументы для вызова функции
				args.assign(std::begin(sample_raw), std::end(sample_raw));
			FINISH;

			START("7 Call bubble_sort (call)");
				auto func = ccc::bubble_sort;
				assert(func);
				// Вызываем функцию и получаем результат
				func(args);
			FINISH;

			for (unsigned i = 0; i < std::size(sample_sorted); ++i )
			{	[[maybe_unused]] const auto v = args[i];
				assert(v == sample_sorted[i]);
			}
		}

		START("98 close");
		FINISH;

		START("99 Finalize");
		FINISH;
	FINISH;

	std::cout << "C test result:\n";
	static constexpr auto flags =
		vi_tmSortByName |
		vi_tmSortAscending |
		vi_tmShowOverhead |
		vi_tmShowDuration |
		vi_tmShowUnit |
		vi_tmShowResolution;
	VI_TM_REPORT(flags);
}
