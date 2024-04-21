// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://docs.python.org/3/extending/index.html

#include "header.h"
#include "lua_metrics.h"

#include <vi_timing/timing.h>

#include <Python.h>

#include <cassert>
#include <string>

#define TM(s) VI_TM(((s) + suffix).c_str())

namespace
{
	using namespace std::string_literals;

	inline bool verify(bool b) { assert(b); return b; }

#ifndef NDEBUG
	static constexpr char sample[] = "global string";
#endif

	void test(const std::string& suffix)
	{
		TM("Test");

		{	TM("Initialize");
			Py_Initialize();
		}

		PyObject* mod{};
		PyObject* dict{};
		{	TM("Loadfile");
			mod = PyImport_ImportModule("sample");
			assert(mod);
			dict = PyModule_GetDict(mod);
			assert(dict);
		}

		{	TM("Get string");
			auto obj = PyRun_String("str", Py_eval_input, dict, dict);
			assert(obj);
			char* sz{};
			verify(PyArg_Parse(obj, "s", &sz));
			assert(sz && 0 == strcmp(sz, sample));
			Py_DECREF(obj);
		}

		{	TM("Call empty");
			auto obj = PyRun_String("empty_func()", Py_file_input, dict, dict);
			assert(obj);
			Py_DECREF(obj);
		}

		{	TM("Call strlen");
			auto ret = PyRun_String("strlen_func(\"global string\")", Py_eval_input, dict, dict);
			assert(ret);
			int len = 0;
			verify(PyArg_Parse(ret, "i", &len));
			assert(strlen(sample) == len);
			Py_DECREF(ret);
		}

		{	TM("Close");
			Py_DECREF(mod);
			Py_Finalize();
		}
	}

	void test2(const std::string& suffix)
	{
		TM("Test");

		{
			TM("Initialize");
			Py_Initialize();
		}

		PyObject* mod = nullptr;
		{
			TM("Loadfile");
			mod = PyImport_ImportModule("sample");
			assert(mod);
		}

		{
			TM("Get string");
			auto p = PyObject_GetAttrString(mod, "str");
			assert(p);
			char* sz{};
			verify(PyArg_Parse(p, "s", &sz));
			assert(sz && 0 == strcmp(sz, sample));
			Py_DECREF(p);
		}

		{
			TM("Call empty");
			auto func = PyObject_GetAttrString(mod, "empty_func");
			assert(func);
			auto ret = PyObject_CallNoArgs(func);
			assert(ret);
			Py_DECREF(ret);
			Py_DECREF(func);
		}

		{
			TM("Call strlen");
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

		{
			TM("Close");
			Py_DECREF(mod);
			Py_Finalize();
		}
	}
} // namespace {

void python_test()
{
	//test(" (pyt)"s);
	//test(" bis (pyt)"s);

	test2(" (py2)"s);
	test2(" bis (py2)"s);
}
