#ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	define VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	pragma once

#	include <array>
#	include <cassert>
#	include <fstream>
#	include <memory>
#	include <sstream>
#	include <string>
#	include <thread>


#	ifdef NDEBUG
constexpr auto sample_size = 500U;
#	else
constexpr auto sample_size = 300U;
#	endif

// Sample data for testing the bubble_sort functions
extern const std::array<int, sample_size> &sample_raw;
extern const std::array<int, sample_size> &sample_ascending_sorted;
extern const std::array<int, sample_size> &sample_descending_sorted;

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
	mutable std::string p_code_;

	void test() const override;

	// Ёти функции будут вызваны из test() в пор€дке объ€влени€:
	virtual void InitializeEngine(const char* title) const = 0;
	virtual void* CompileScript(const char* title) const = 0;
	virtual std::string ExportCode(const char* title, void* code) const = 0;
	virtual void RestartEngine(const char* title) const = 0;
	virtual void* ImportCode(const char* title, const std::string& p_code) const = 0;
	virtual void ExecutionScript(const char* title, void* code) const = 0;
	virtual void FunctionRegister(const char* title) const = 0;

	virtual void WorkGetGlobalString(const char* title) const = 0;
	virtual void WorkCallEmptyFunction(const char* title) const = 0; // Ѕудет вызвана несколько раз
	virtual void* WorkBubbleSortPreparingArguments(const char* title, bool descending) const = 0; // и WorkBubbleSortRun будут вызваны дважды с разными аргументами.
	virtual void WorkBubbleSortRun(const char* title, void* args, bool descending) const = 0;

	virtual void FinalizeEngine(const char* title) const = 0;
};

inline void test_interface_t::test() const
{
	InitializeEngine("1. Initialize");

	p_code_.clear();
	{	auto code = CompileScript("2. Compile");
		p_code_ = ExportCode("3. Export P-code", code);
	}

	RestartEngine("4. Restart");

	auto code = ImportCode("5. Import P-code", p_code_);
	ExecutionScript("6. Execution", code);
	FunctionRegister("7 Register function");

	{	WorkGetGlobalString("8.1 Get string");

		WorkCallEmptyFunction("8.2.1 Call empty");
		WorkCallEmptyFunction("8.2.2 Call empty");
		WorkCallEmptyFunction("8.2.3 Call empty");
		WorkCallEmptyFunction("8.2.4 Call empty");
		WorkCallEmptyFunction("8.2.5 Call empty");

		void *args = WorkBubbleSortPreparingArguments("8.3.1 bubble_sort (arg init)", true);
		WorkBubbleSortRun("8.3.2 bubble_sort ascending", args, true);

		args = WorkBubbleSortPreparingArguments("8.4.1 bubble_sort (arg init)", false);
		WorkBubbleSortRun("8.4.2 bubble_sort descending", args, false);
	}

	FinalizeEngine("9. Finalize");
}

#endif // #ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
