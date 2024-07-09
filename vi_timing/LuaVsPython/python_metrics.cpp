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

#define YIELD(s) /*std::this_thread::yield(); VI_TM_CLEAR(s)*/

void python_test()
{	VI_TM_CLEAR();

	PyObject* mod = nullptr;
	std::array<PyObject*, CNT> arr;

	YIELD("1 Initialize");
	{	VI_TM("1 Initialize");
		Py_Initialize();
	}

	YIELD("2 dofile");
	{	VI_TM("2 dofile");
		mod = PyImport_ImportModule("sample");
		assert(mod);
	}

	YIELD("*2 dofile");
	for(auto &ptr : arr)
	{	VI_TM("*2 dofile");
		ptr = PyImport_ImportModule("sample");
		assert(ptr);
	}

	YIELD("3 Get string");
	{	VI_TM("3 Get string");
		auto p = PyObject_GetAttrString(mod, "str");
		assert(p);
		char* sz{};
		verify(PyArg_Parse(p, "s", &sz));
		assert(sz && 0 == strcmp(sz, sample));
		Py_DECREF(p);
	}

	YIELD("*3 Get string");
	for (auto ptr : arr)
	{	VI_TM("*3 Get string");
		auto p = PyObject_GetAttrString(ptr, "str");
		assert(p);
		char* sz{};
		verify(PyArg_Parse(p, "s", &sz));
		assert(sz && 0 == strcmp(sz, sample));
		Py_DECREF(p);
	}

	YIELD("4 Call empty");
	{	VI_TM("4 Call empty");
		auto func = PyObject_GetAttrString(mod, "empty_func");
		assert(func);
		auto ret = PyObject_CallNoArgs(func);
		assert(ret);
		Py_DECREF(ret);
		Py_DECREF(func);
	}

	YIELD("*4 Call empty");
	for (auto ptr : arr)
	{	VI_TM("*4 Call empty");
		auto func = PyObject_GetAttrString(ptr, "empty_func");
		assert(func);
		auto ret = PyObject_CallNoArgs(func);
		assert(ret);
		Py_DECREF(ret);
		Py_DECREF(func);
	}

	YIELD("5 Call strlen");
	{	VI_TM("5 Call strlen");
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
	}

	YIELD("*5 Call strlen");
	for (auto ptr : arr)
	{	VI_TM("*5 Call strlen");
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
	}

	YIELD("98 close");
	{	VI_TM("98 close");
		Py_DECREF(mod);
	}

	YIELD("*98 close");
	for (auto ptr : arr)
	{	VI_TM("*98 close");
		Py_DECREF(ptr);
	}

	YIELD("99 Finalize");
	{	VI_TM("99 Finalize");
		Py_Finalize();
	}

	std::cout << "Python test1 result:\n";
	VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
}
