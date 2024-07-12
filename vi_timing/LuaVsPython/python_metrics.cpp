// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://docs.python.org/3/extending/index.html
// https://docs.python.org/3/extending/embedding.html

#include "header.h"
#include "lua_metrics.h"

#include <vi_timing/timing.h>

#include <Python.h>

#include <array>
#include <cassert>
#include <iostream>
#include <string>
#include <thread>

using namespace std::string_literals;

namespace
{
	static constexpr char sample[] = "global string";
	static constexpr int CNT = 1'000;
}

#define YIELD(s) \
	std::this_thread::yield(); \
	for(int n = 0; n < 5; ++n) \
	{	VI_TM(s); \
	} \
	VI_TM_CLEAR(s)

#define START(s) \
	YIELD(" " s); \
	{	VI_TM(" " s)

#define START_F(s) \
	YIELD("*" s); \
	for (auto &ptr : arr) \
	{	VI_TM("*" s)

#define END } \
	do{} while(0)

void python_test()
{	VI_TM_CLEAR();

	PyObject* mod = nullptr;
	std::array<PyObject*, CNT> arr;

	START("1 Initialize");
		Py_Initialize();
	END;

	START("2 dofile");
		mod = PyImport_ImportModule("sample");
		assert(mod);
	END;

	START_F("2 dofile");
		ptr = PyImport_ImportModule("sample");
		assert(ptr);
	END;

	START("3 Get string");
		auto p = PyObject_GetAttrString(mod, "str");
		assert(p);
		char* sz{};
		verify(PyArg_Parse(p, "s", &sz));
		assert(sz && 0 == strcmp(sz, sample));
		Py_DECREF(p);
	END;

	START_F("3 Get string");
		auto p = PyObject_GetAttrString(ptr, "str");
		assert(p);
		char* sz{};
		verify(PyArg_Parse(p, "s", &sz));
		assert(sz && 0 == strcmp(sz, sample));
		Py_DECREF(p);
	END;

	START("4 Call empty");
		auto func = PyObject_GetAttrString(mod, "empty_func");
		assert(func);
		auto ret = PyObject_CallNoArgs(func);
		assert(ret);
		Py_DECREF(ret);
		Py_DECREF(func);
	END;

	START_F("4 Call empty");
		auto func = PyObject_GetAttrString(ptr, "empty_func");
		assert(func);
		auto ret = PyObject_CallNoArgs(func);
		assert(ret);
		Py_DECREF(ret);
		Py_DECREF(func);
	END;

	START("5 Call strlen");
		auto func = PyObject_GetAttrString(mod, "strlen_func");
		assert(func);
		auto args = Py_BuildValue("(s)", "global string");
		assert(args);
		auto ret = PyObject_Call(func, args, nullptr);
		assert(ret);
		int len = 0;
		verify(PyArg_Parse(ret, "i", &len));
		assert(strlen(sample) == len);
		Py_DECREF(ret);
		Py_DECREF(args);
		Py_DECREF(func);
	END;

	START_F("5 Call strlen");
		auto func = PyObject_GetAttrString(ptr, "strlen_func");
		assert(func);
		auto args = Py_BuildValue("(s)", "global string");
		assert(args);
		auto ret = PyObject_Call(func, args, nullptr);
		assert(ret);
		int len = 0;
		verify(PyArg_Parse(ret, "i", &len));
		assert(strlen(sample) == len);
		Py_DECREF(ret);
		Py_DECREF(args);
		Py_DECREF(func);
	END;

	START("98 close");
		Py_DECREF(mod);
	END;

	START_F("98 close");
		Py_DECREF(ptr);
	END;

	START("99 Finalize");
		Py_Finalize();
	END;

	std::cout << "Python test1 result:\n";
	VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
}
