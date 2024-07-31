#ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	define VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	pragma once

#	include <fstream>
#	include <memory>
#	include <sstream>
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

inline std::string read_file(const char *filename)
{	std::ifstream file(filename);
	assert(file);
	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
};

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
	virtual void FunctionRegister(const char* title) const = 0;
	// «десь будут вызваны функции Work*()
	virtual void FinalizeEngine(const char* title) const = 0;

	// Ёти функции будут вызваны после ExecutionScript() и перед CloseScript() в пор€дке объ€влени€ (некотороые по несколько раз):
	virtual void WorkGetGlobalString(const char* title) const = 0;
	virtual void WorkCallEmptyFunction(const char* title) const = 0;
	virtual void* WorkBubbleSortPreparingArguments(const char* title, bool descending) const = 0;
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
	FunctionRegister("6 Register function");

	{	WorkGetGlobalString("7.1 Get string");

		WorkCallEmptyFunction("7.2.1 Call empty");
		WorkCallEmptyFunction("7.2.2 Call empty");
		WorkCallEmptyFunction("7.2.3 Call empty");
		WorkCallEmptyFunction("7.2.4 Call empty");
		WorkCallEmptyFunction("7.2.5 Call empty");

		void *args = WorkBubbleSortPreparingArguments("7.3.1 bubble_sort (arg init)", true);
		WorkBubbleSortRun("7.3.2 bubble_sort ascending", args, true);

		args = WorkBubbleSortPreparingArguments("7.4.1 bubble_sort (arg init)", false);
		WorkBubbleSortRun("7.4.2 bubble_sort descending", args, false);
	}

	FinalizeEngine("8. Finalize");
}

#endif // #ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
