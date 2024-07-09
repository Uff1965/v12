// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include "lua_metrics.h"
#include "python_metrics.h"

#include <Windows.h>

#include <vi_timing/timing.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace
{
	namespace ch = std::chrono;
	using namespace std::literals;

	const auto _1 = (
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

	//VI_TM_INIT(vi_tmSortByName | vi_tmSortAscending | vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowDiscreteness);
	//VI_TM("Well, that's all!");
	const auto initializing_global_variables_can_also_take_time = []{std::this_thread::sleep_for(10ms); return 0;}();

	std::size_t l = 0;
	const char sample[] = "global string";

VI_OPTIMIZE_OFF
	void empty_func()
	{
	}

	std::size_t strlen_func(const char *s)
	{	return std::strlen(s);
	}
VI_OPTIMIZE_ON

	void tm_test()
	{
		constexpr auto CNT = 1'000;

		{
			constexpr char key_s[] =	"Call empty (CPP)";
			constexpr char key_sum[] =	"Call empty (CPP) SUM";
			constexpr char key[] =		"Call empty (CPP) *";

			std::this_thread::yield();
			empty_func();

			{
				vi_tm::clear(key_s);
				VI_TM(key_s);
				empty_func();
			}

			{
				vi_tm::clear(key);
				VI_TM(key_sum, CNT);
				for (int n = 0; n < CNT; ++n)
				{
					VI_TM(key);
					empty_func();
				}
			}
		}

		{
			constexpr char key_s[] =	"Call strlen (CPP)";
			constexpr char key_sum[] =	"Call strlen (CPP) SUM";
			constexpr char key[] =		"Call strlen (CPP) *";

			std::this_thread::yield();
			l = strlen_func(sample);

			{
				vi_tm::clear(key_s);
				VI_TM(key_s);
				l = strlen_func(sample);
			}

			{
				vi_tm::clear(key);
				VI_TM(key_sum, CNT);
				for (int n = 0; n < CNT; ++n)
				{
					VI_TM(key);
					l = strlen_func(sample);
				}
			}
		}

		{
			constexpr char key_s[] =	"Empty (CPP)";
			constexpr char key_sum[] =	"Empty (CPP) SUM";
			constexpr char key[] =		"Empty (CPP) *";

			std::this_thread::yield();

			{
				vi_tm::clear(key_s);
				VI_TM(key_s);
			}

			{
				vi_tm::clear(key);
				VI_TM(key_sum, CNT);
				for (int n = 0; n < CNT; ++n)
				{
					VI_TM(key);
				}
			}
		}
	}

VI_OPTIMIZE_OFF
	void tm_for()
	{
		constexpr auto CNT = 100;
		constexpr char key[] = "For";

		std::size_t diff = 0;
		std::size_t diff0 = 0;
		std::size_t diff1 = 0;
		std::size_t diff2 = 0;

		std::this_thread::yield();
		{
			VI_TM(key);
			vi_tm::clear(key);
		}

		{
			VI_TM("XXXX");

			auto s = vi_tmGetTicks();
			auto e = vi_tmGetTicks();
			diff = e - s;

			s = vi_tmGetTicks();
			{
				VI_TM(key);
			}
			e = vi_tmGetTicks();
			diff0 = e - s - diff;

			s = vi_tmGetTicks();
			for (int n = 0; n < CNT; ++n)
			{
				{ VI_TM(key); }
			}
			e = vi_tmGetTicks();
			diff1 = e - s - diff;

			s = vi_tmGetTicks();
			for (int n = 0; n < CNT; ++n)
			{
				{ VI_TM(key); }
				{ VI_TM(key); }
			}
			e = vi_tmGetTicks();
			diff2 = e - s - diff;
		}

		std::cout << "diff: " << diff << std::endl;
		std::cout << "diff0: " << diff0 << std::endl;
		std::cout << "diff1: " << diff1 << std::endl;
		std::cout << "diff2: " << diff2 << std::endl;
		std::cout << "Overmeasure: " << static_cast<double>(2 * diff1 - diff2) / CNT << std::endl;
		std::cout << "Load: " << static_cast<double>(diff2 - diff1) / CNT << std::endl;
		endl(std::cout);
	}
VI_OPTIMIZE_ON

} // namespace

int main()
{
//	VI_TM_FUNC;

	vi_tm::warming();

	//tm_for();
	//tm_test();
	lua_test();
	python_test();

    std::cout << "Hello World!\n";
}
