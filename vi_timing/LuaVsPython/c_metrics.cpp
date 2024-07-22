// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"
#include "LuaVsPython.h"

#include <vi_timing/timing.h>

#include <cassert>
#include <vector>

namespace
{
	constexpr char sample[] = "global string";
}

namespace ccc
{
	// c_ascending - C-функция которая будет зарегистрирована в Python под именем "c_ascending" и будет использована внутри скрипта
	static bool c_ascending(int l, int r)
	{	return l < r;
	}

	// c_descending - C-функция которая будет передана из C-кода в Python-скрипт в качестве аргумента функции bubble_sort
	static bool c_descending(int l, int r)
	{	return l > r;
	}

	const char global_string[] = "global string";

	void empty_func()
	{	// This function does nothing
	}

	int simple_func(int n)
	{	return n;
	}

	std::vector<int> bubble_sort(const std::vector<int>& a, bool (*cmp)(int, int))
	{
		std::vector<int> result{ a };
		if (!cmp)
		{	cmp = c_ascending;
		}

		bool swapped;
		do
		{
			swapped = false;
			for (unsigned i = 1; i < a.size(); ++i)
			{
				if (cmp(result[i], result[i - 1]))
				{	result[i - 1] = std::exchange(result[i], result[i - 1]);
					swapped = true;
				}
			}
		} while (swapped);

		return result;
	}
}

struct test_c_t final: test_interface_t
{
	std::string title() const override { return "C"; };

	void InitializeEngine(const char* tm) const override;
	void* CompileScript(const char* tm) const override;
	std::string ExportCode(const char* tm, void*) const override;
	void* ImportCode(const char* tm, const std::string& p_code) const override;
	void ExecutionScript(const char* tm, void*) const override;
	void CloseScript(const char* tm) const override;
	void FinalizeEngine(const char* tm) const override;

	void WorkGetString(const char* tm) const override;
	void WorkCallEmpty(const char* tm) const override;
	void WorkCallSimple(const char* tm) const override;
	void* WorkBubbleSortArgs(const char* tm, bool descending) const override;
	void WorkBubbleSortRun(const char* tm, void* py_args, bool descending) const override;

	inline static const auto _ = registrar(std::make_unique<test_c_t>());
};

void test_c_t::InitializeEngine(const char* tm) const
{	START(tm);
	FINISH;
}

void* test_c_t::CompileScript(const char* tm) const
{	START(tm);
	FINISH;
	return nullptr;
}

std::string test_c_t::ExportCode(const char* tm, void*) const
{	START(tm);
	FINISH;
	return {};
}

void* test_c_t::ImportCode(const char* tm, const std::string& p_code) const
{	START(tm);
	FINISH;
	return nullptr;
}

void test_c_t::ExecutionScript(const char* tm, void*) const
{	START(tm);
	FINISH;
}

void test_c_t::CloseScript(const char* tm) const
{	START(tm);
	FINISH;
}

void test_c_t::FinalizeEngine(const char* tm) const
{	START(tm);
	FINISH;
}

void test_c_t::WorkGetString(const char* tm) const
{	START(tm);
		const char *s = ccc::global_string;
		assert(0 == std::strcmp(sample, s));
	FINISH;
}

void test_c_t::WorkCallEmpty(const char* tm) const
{	START(tm);
		auto func = ccc::empty_func;
		assert(func);
		func();
	FINISH;
}

void test_c_t::WorkCallSimple(const char* tm) const
{	START(tm);
		const auto v = ccc::simple_func(777);
		assert(777 == v);
	FINISH;
}

void* test_c_t::WorkBubbleSortArgs(const char* tm, bool ) const
{	std::vector<int> *result = nullptr;
	START(tm);
		verify(result = new std::vector<int>(std::begin(sample_raw), std::end(sample_raw)));
	FINISH;
	return result;
}
	
void test_c_t::WorkBubbleSortRun(const char* tm, void* args, bool descending) const
{	std::vector<int> result;
	START(tm);
		auto func = ccc::bubble_sort;
		assert(func);
		auto v = static_cast<const std::vector<int>*>(args);
		assert(v);
		result = func(*v, descending? ccc::c_descending: nullptr);
		delete v;
	FINISH;

	auto &sample = descending? sample_descending_sorted: sample_ascending_sorted;
	for (unsigned i = 0; i < std::size(sample_ascending_sorted); ++i)
	{	[[maybe_unused]] const auto v = result[i];
		assert(v == sample[i]);
	}
}
