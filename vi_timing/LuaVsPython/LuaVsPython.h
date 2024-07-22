#ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	define VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	pragma once

#	include <memory>
#	include <string>
#	include <thread>

#	ifdef NDEBUG
constexpr auto sample_size = 3'000U;
#	else
constexpr auto sample_size = 600U;
#	endif

extern const int(&sample_raw)[sample_size];
extern const int(&sample_ascending_sorted)[sample_size];
extern const int(&sample_descending_sorted)[sample_size];

class test_t
{
public:
	virtual ~test_t() = default;
	virtual std::string title() const = 0;
	virtual void test() const = 0;

	static bool registrar(std::unique_ptr<test_t> p);
};

struct test_interface_t: test_t
{
	void test() const override;

	// Ёти функции будут вызваны из test() в пор€дке объ€влени€:
	virtual void InitializeEngine(const char* title) const = 0;
	virtual void* CompileScript(const char* title) const = 0;
	virtual std::string ExportCode(const char* title, void* code) const = 0;
	virtual void* ImportCode(const char* title, const std::string& p_code) const = 0;
	virtual void ExecutionScript(const char* title, void* code) const = 0;
	// «десь будут вызваны функции Work*()
	virtual void CloseScript(const char* title) const = 0;
	virtual void FinalizeEngine(const char* title) const = 0;

	// Ёти функции будут вызваны после ExecutionScript() и перед CloseScript() в пор€дке объ€влени€ (некотороые по несколько раз):
	virtual void WorkGetString(const char* title) const = 0;
	virtual void WorkCallEmpty(const char* title) const = 0;
	virtual void WorkCallSimple(const char* title) const = 0;
	virtual void* WorkBubbleSortArgs(const char* title, bool descending) const = 0;
	virtual void WorkBubbleSortRun(const char* title, void* args, bool descending) const = 0;
};

inline void test_interface_t::test() const
{
	InitializeEngine("1. Initialize");
	auto code = CompileScript("2. Compile");
	{
		std::string p_code = ExportCode("3. Export P-code", code);
		code = nullptr;
		code = ImportCode("4. Import P-code", p_code);
	}
	ExecutionScript("5. Execution", code);

	{	WorkGetString("6.1 Get string");

		WorkCallEmpty("6.2.1 Call empty");
		WorkCallEmpty("6.2.2 Call empty");
		WorkCallEmpty("6.2.3 Call empty");
		WorkCallEmpty("6.2.4 Call empty");
		WorkCallEmpty("6.2.5 Call empty");

		WorkCallSimple("6.3.1 Call simple");
		WorkCallSimple("6.3.2 Call simple");
		WorkCallSimple("6.3.3 Call simple");
		WorkCallSimple("6.3.4 Call simple");
		WorkCallSimple("6.3.5 Call simple");

		void *args = WorkBubbleSortArgs("6.4.1 bubble_sort (arg init)", false);
		WorkBubbleSortRun("6.4.2 bubble_sort", args, false);

		args = WorkBubbleSortArgs("6.5.1 bubble_sort (arg init)", true);
		WorkBubbleSortRun("6.5.2 bubble_sort", args, true);
	}

	CloseScript("7. Close");
	FinalizeEngine("8. Finalize");
}

#	define START(s) \
	std::this_thread::yield(); \
	for (auto n = 5; n--;) { VI_TM(s); } \
	VI_TM_CLEAR(s); \
	do { VI_TM(s)

//#	define START(s) \
//	do { VI_TM(s)

#	define FINISH \
	} while(0)

#endif // #ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
