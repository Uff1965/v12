// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "LuaVsPython.h"
#include "statistics.h"

#include <Windows.h>

#include <vi_timing/timing.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <random>
#include <thread>

using namespace std::literals;

namespace
{
	constexpr auto g_repeat = 20U;

	const auto _ =
	(
#if defined(_MSC_VER) && defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF), // To automatically call the _CrtDumpMemoryLeaks function when the program ends
		verify(-1 != _set_error_mode(_OUT_TO_MSGBOX)), // Error sink is a message box. To be able to ignore errors in debugging sessions.
#endif
#ifdef WIN32
		verify(::SetThreadAffinityMask(::GetCurrentThread(), 0b0000'0001)),
		verify(::SetPriorityClass(::GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS)),
		verify(::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL)),
		[]
		{	if (const auto hOut = ::GetStdHandle(STD_OUTPUT_HANDLE); verify(hOut != NULL && hOut != INVALID_HANDLE_VALUE))
			{	if (DWORD dwMode = 0; FALSE != ::GetConsoleMode(hOut, &dwMode))
				{	verify(FALSE != ::SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING));
				}
			}
		}(),
#endif
		nullptr
	);

	//VI_TM_INIT(vi_tmSortByName | vi_tmSortAscending | vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowResolution);
	//VI_TM("Well, that's all!");
	const auto initializing_global_variables_can_also_take_time = []{std::this_thread::sleep_for(1ms); return 0;}();

	int raw[sample_size];
	int ascending[sample_size];
	static_assert(std::size(raw) == std::size(ascending));
	int descending[sample_size];
	static_assert(std::size(raw) == std::size(descending));
	const auto _sample_init = []
	{	std::mt19937 gen{/*std::random_device{}()*/ }; // For ease of debugging, the sequence is stable.
		std::uniform_int_distribution distrib{ 1, 1'000 };

		std::ranges::generate(raw, [&] { return distrib(gen); });
		std::ranges::copy(raw, ascending);
		std::ranges::sort(ascending);
		std::ranges::reverse_copy(ascending, descending);
		return nullptr;
	}();

} // namespace

const int(&sample_raw)[sample_size] = raw;
const int(&sample_ascending_sorted)[sample_size] = ascending;
const int(&sample_descending_sorted)[sample_size] = descending;

auto& Items()
{	static std::vector<std::unique_ptr<const test_t>> items;
	return items;
}
bool test_t::registrar(std::unique_ptr<test_t> p)
{	Items().emplace_back(std::move(p));
	return true;
}

struct item_t
{	vi_tmTicks_t time_;
	std::size_t amount_;
	std::size_t calls_cnt_;
};

int main(int argc, char* argv[])
{
//	VI_TM_FUNC;

	vi_tm::warming();

	for (const auto &t : Items())
	{	t->test(); // Подгружаем весь код из библиотек.
	}

	std::map<std::string, std::map<std::string, std::vector<item_t>>> reports;

	unsigned long flags = vi_tmSortByName | vi_tmSortAscending | vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowResolution;

	for (unsigned n = 0; n < g_repeat; ++n)
	{
		std::cout << "Repeat: " << n + 1 << " of " << g_repeat << "\n";

		for (const auto &t : Items())
		{
			std::cout << "Engine: \'" << t->title() << "\'\n";

			auto &rep = reports[t->title()];
			VI_TM_CLEAR();
			t->test();
			VI_TM_REPORT(flags);
			endl(std::cout);
			flags = vi_tmSortByName | vi_tmSortAscending;

			auto fn = [](const char *name, vi_tmTicks_t time, std::size_t amount, std::size_t calls_cnt, void *data)
				{
					auto &report = *static_cast<std::map<std::string, std::vector<item_t>>*>(data);

					auto &items = report[name];
					items.reserve(g_repeat);
					items.emplace_back(time, amount, calls_cnt);
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
				std::transform(action.second.begin(), action.second.end(), data.begin(), [](const auto &item) { return item.time_; });

				std::cout << action.first << ":\t";
				const auto stat = misc::calc_stat(data);

				std::cout <<
					stat.average_ << "\t" <<
					stat.median_ << "\t" <<
					stat.min_ << "\t" <<
					stat.max_ << "\t" <<
					stat.deviation_ << "\n";
			}
		}
	}

	std::cout << "Hello World!\n";
}
