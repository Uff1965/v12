// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "LuaVsPython.h"

#include <Windows.h>

#include <vi_timing/timing.h>

#include <algorithm>
#include <iostream>
#include <random>
#include <thread>

using namespace std::literals;

namespace
{
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

int main()
{
//	VI_TM_FUNC;

	vi_tm::warming();

	for (const auto &t : Items())
	{	t->test(); // Подгружаем весь код из библиотек.
	}

	unsigned long flags = vi_tmSortByName | vi_tmSortAscending | vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowResolution;
	for (const auto &t : Items())
	{	std::cout << "Timing: \'" << t->title() << "\'\n";
		VI_TM_CLEAR();
		t->test();
		VI_TM_REPORT(flags);
		endl(std::cout);

		flags = vi_tmSortByName | vi_tmSortAscending;
	}

	std::cout << "Hello World!\n";
}
