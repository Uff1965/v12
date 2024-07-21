#ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	define VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
#	pragma once

#	include <memory>
#	include <string>
#	include <string_view>
#	include <vector>

#	ifdef NDEBUG
constexpr auto sample_size = 3'000U;
constexpr auto CNT = 1'000U;
#	else
constexpr auto sample_size = 300U;
constexpr auto CNT = 1'000U;
#	endif
extern const int(&sample_raw)[sample_size];
extern const int(&sample_sorted)[sample_size];
extern const int(&sample_descending)[sample_size];

class test_t
{
public:
	static std::vector<std::unique_ptr<const test_t>> items_;

	virtual ~test_t() = default;
	virtual void test() const = 0;
	virtual std::string title() const = 0;
	static bool registrar(std::unique_ptr<test_t> p);
};

class test_interface_t: public test_t
{
	void test() const override;

	virtual void InitializeEngine(const char* tm) const = 0;
	virtual void* CompileScript(const char* tm) const = 0;
	virtual std::string ExportCode(const char* tm, void* code) const = 0;
	virtual void* ImportCode(const char* tm, const std::string& code) const = 0;
	virtual void ExecutionScript(const char* tm, void* code) const = 0;
	virtual void Work() const;
	virtual void CloseScript(const char* tm) const = 0;
	virtual void FinalizeEngine(const char* tm) const = 0;

	virtual void WorkGetString(const char* tm) const = 0;
	virtual void WorkCallEmpty(const char* tm) const = 0;
	virtual void WorkCallSimple(const char* tm) const = 0;
	virtual void* WorkBubbleSortArgs(const char* tm, bool descending) const = 0;
	virtual void WorkBubbleSortRun(const char* tm, void* args, bool descending) const = 0;
};

#endif // #ifndef VI_TIMING_LUAVSPYTHON_LUAVSPYTHON_H_
