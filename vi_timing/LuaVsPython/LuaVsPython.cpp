// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "LuaVsPython.h"
#include "statistics.h"

#include <Windows.h>

#include <vi_timing/timing.h>

#include <array>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <thread>

using namespace std::literals;

namespace
{
	const auto _ =
	(
#if defined(_MSC_VER) && defined(_DEBUG)
//		_CrtSetBreakAlloc(311), // To set a breakpoint at the specified memory allocation number
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF), // To automatically call the _CrtDumpMemoryLeaks function when the program ends
		verify(-1 != _set_error_mode(_OUT_TO_MSGBOX)), // Error sink is a message box. To be able to ignore errors in debugging sessions.
#endif
#ifdef WIN32
//		verify(::SetThreadAffinityMask(::GetCurrentThread(), 0b0000'0001)), // To bind the thread to the first processor
		verify(::SetPriorityClass(::GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS)), // To increase the priority of the process
		verify(::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL)), // To increase the priority of the thread
#endif
		nullptr
	);

	//VI_TM_INIT(vi_tmSortByName | vi_tmSortAscending | vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowResolution);
	//VI_TM("Well, that's all!");
	const auto initializing_global_variables_can_also_take_time = []{std::this_thread::sleep_for(1ms); return 0;}();

	// Sample data for testing the bubble_sort functions
	std::array<int, sample_size> raw_data;
	std::array<int, sample_size> ascending_data;
	std::array<int, sample_size> descending_data;
	const auto _sample_init = []
	{	std::mt19937 gen{/*std::random_device{}()*/ }; // For ease of debugging, the sequence is stable.
		std::uniform_int_distribution distrib{ 1, 1'000 };

		std::ranges::generate(raw_data, [&] { return distrib(gen); });
		std::ranges::copy(raw_data, ascending_data.begin());
		std::ranges::sort(ascending_data);
		std::ranges::reverse_copy(ascending_data, descending_data.begin());
		return nullptr;
	}();

#ifdef NDEBUG
	constexpr auto g_repeat = 20U;
#else
	constexpr auto g_repeat = 1U;
#endif

} // namespace

const std::array<int, sample_size> &sample_raw = raw_data;
const std::array<int, sample_size> &sample_ascending_sorted = ascending_data;
const std::array<int, sample_size> &sample_descending_sorted = descending_data;

auto& TestItems()
{	static std::vector<std::unique_ptr<const test_t>> items;
	return items;
}
bool test_t::registrar(std::unique_ptr<test_t> p)
{	TestItems().emplace_back(std::move(p));
	return true;
}

struct item_t
{	vi_tmTicks_t time_;
	std::size_t amount_;
	std::size_t calls_cnt_;
};

int main(int argc, char* argv[])
{
	struct thousands_sep_facet_t final : std::numpunct<char>
	{	char do_decimal_point() const override { return ','; }
	};
	std::cout.imbue(std::locale(std::locale(), new thousands_sep_facet_t));

	vi_tm::warming();

	for (const auto &t : TestItems())
	{	t->test(); // Подгружаем весь код из библиотек.
	}

	std::map<std::string, std::map<std::string, std::vector<item_t>>> reports;

	unsigned long flags = vi_tmSortByName | vi_tmSortAscending | vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowResolution;

	for (unsigned n = 0; n < g_repeat; ++n)
	{
		std::cout << "Repeat: " << n + 1 << " of " << g_repeat << "\n";

		for (const auto &t : TestItems())
		{	std::cout << "Engine: \'" << t->title() << "\'\n";

			VI_TM_CLEAR();
			t->test();
			VI_TM_REPORT(flags);
			endl(std::cout);
			flags = vi_tmSortByName | vi_tmSortAscending;

			auto &rep = reports[t->title()];
			auto fn = [](const char *name, vi_tmTicks_t time, std::size_t amount, std::size_t calls_cnt, void *data)
				{
					if (amount)
					{	auto &report = *static_cast<std::map<std::string, std::vector<item_t>> *>(data);
						auto &items = report[name];
						items.reserve(g_repeat);
						items.emplace_back(time, amount, calls_cnt);
					}
					return -1;
				};
			vi_tmResults(fn, &rep);
		}
	}

	{
		std::map<std::string, std::map<std::string, misc::statistics_t>> result;
		for (auto &&engine : reports)
		{
			std::cout << "Engine: \'" << engine.first << "\'\n";
			for (auto &&action : engine.second)
			{
				std::vector<double> data(action.second.size());
				auto foo = [](const item_t &item) { return static_cast<double>(item.time_); };
				std::transform(action.second.begin(), action.second.end(), data.begin(), foo);

				std::cout << action.first << ";";
				const auto stat = misc::calc_stat(data);

				std::cout <<
					stat.average_ << ';' <<
					stat.median_ << ';' <<
					stat.min_ << ';' <<
					stat.max_ << ';' <<
					stat.deviation_ << "\n";
			}
		}
	}

	endl(std::cout);
	std::cout << "Hello World!\n";
}
