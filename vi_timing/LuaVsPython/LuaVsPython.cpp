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
	int sorted[sample_size];
	static_assert(std::size(raw) == std::size(sorted));
	int descending[sample_size];
	static_assert(std::size(raw) == std::size(descending));
	const auto _sample_init = []
	{	std::mt19937 gen{/*std::random_device{}()*/ }; // For ease of debugging, the sequence is constant.
		std::uniform_int_distribution distrib{ 1, 1'000 };

		std::ranges::generate(raw, [&] { return distrib(gen); });
		std::ranges::copy(raw, sorted);
		std::ranges::sort(sorted);
		std::ranges::reverse_copy(sorted, descending);
		return nullptr;
	}();

} // namespace

const int(&sample_raw)[sample_size] = raw;
const int(&sample_sorted)[sample_size] = sorted;
const int(&sample_descending)[sample_size] = descending;

std::vector<std::unique_ptr<const test_t>> test_t::items_;
bool test_t::registrar(std::unique_ptr<test_t> p)
{	items_.emplace_back(std::move(p));
	return true;
}

void test_interface_t::test() const
{
	InitializeEngine("1. Initialize");
	auto p_code = CompileScript("3. Compile");
	{
		std::string buff = ExportCode("4. Export P-code", p_code);
		p_code = nullptr;
		p_code = ImportCode("5. Import P-code", buff);
	}
	ExecutionScript("6. Execution", p_code);

	Work();

	CloseScript("8. Close");
	FinalizeEngine("9. Finalize");
}

void test_interface_t::Work() const
{	WorkGetString("7.1 Get string");

	WorkCallEmpty("7.2.1 Call empty");
	WorkCallEmpty("7.2.2 Call empty");
	WorkCallEmpty("7.2.3 Call empty");

	WorkCallSimple("7.3.1 Call simple");
	WorkCallSimple("7.3.2 Call simple");
	WorkCallSimple("7.3.3 Call simple");

	void *args = WorkBubbleSortArgs("7.4.1 bubble_sort (arg init)", false);
	WorkBubbleSortRun("7.4.2 bubble_sort", args, false);

	args = WorkBubbleSortArgs("7.5.1 bubble_sort (arg init)", true);
	WorkBubbleSortRun("7.5.2 bubble_sort", args, true);
}

int main()
{
//	VI_TM_FUNC;

	vi_tm::warming();

	for (const auto &t : test_t::items_)
	{	t->test(); // Подгружаем весь код из библиотек.
	}

	for (const auto &t : test_t::items_)
	{	std::cout << "Timing " << t->title() << "\n";
		VI_TM_CLEAR();
		t->test();
		VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
		endl(std::cout);
	}

	std::cout << "Hello World!\n";
}
