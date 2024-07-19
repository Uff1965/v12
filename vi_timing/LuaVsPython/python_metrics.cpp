// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://docs.python.org/3/extending/index.html
// https://docs.python.org/3/extending/embedding.html
// https://docs.python.org/3/c-api/index.html

#include "header.h"
#include "python_metrics.h"

#include <vi_timing/timing.h>
#include "LuaVsPython.h"
#include <Python.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>

#define START(s) \
	std::this_thread::yield(); \
	for (auto n = 5; n--;) \
	{	VI_TM(s); \
	} \
	VI_TM_CLEAR(s); \
	do { VI_TM(s)

#define FINISH \
	} while(0)

using namespace std::string_literals;

extern "C" PyObject* c_ascending(PyObject*, PyObject* args)
{	int l, r;
	const auto ret = PyArg_ParseTuple(args, "ii", &l, &r); // Парсим аргументы
	return verify(0 != ret) ? PyBool_FromLong(l < r) : nullptr;
}

extern "C" PyObject* c_descending(PyObject*, PyObject* args)
{	int l, r;
	const auto ret = PyArg_ParseTuple(args, "ii", &l, &r);
	return  verify(0 != ret) ? PyBool_FromLong(r < l) : nullptr;
}

namespace
{
	constexpr char sample[] = "global string";
}

void python_test()
{	VI_TM_CLEAR();

	START("  *** ALL ***");

		std::string text;
		START(" 0 Load text");
			std::ifstream file("sample.py");
			assert(file);
			std::ostringstream ss;
			ss << file.rdbuf();
			text = ss.str();
		FINISH;

		PyObject* module = nullptr;
		PyObject* dict = nullptr;

		START(" 1 Initialize");
			Py_Initialize();
		FINISH;

		{	PyObject *code = nullptr;

			START(" 2.1 dofile (load+compile)");
				verify(code = Py_CompileString(text.c_str(), "sample.py", Py_file_input));
				verify(dict = PyDict_New());

				static PyMethodDef func_def =
				{	nullptr, // Имя функции
					c_ascending, // Указатель на функцию
					METH_VARARGS, // Тип аргументов
					nullptr // Документация
				};
				auto func = PyCFunction_NewEx(&func_def, NULL, NULL);
				assert(func);
				verify(0 == PyDict_SetItemString(dict, "c_ascending", func));

			FINISH;
			START(" 2.2 dofile (call)");
				auto module = PyEval_EvalCode(code, dict, nullptr);
				assert(module);
				Py_DECREF(module);
			FINISH;
			Py_DECREF(code);
		}
		START(" 3 Get string");
			auto p = PyDict_GetItemString(dict, "global_string");
			assert(p);
			char* sz{};
			verify(PyArg_Parse(p, "s", &sz));
			assert(sz && 0 == strcmp(sz, sample));
		FINISH;

		START(" 4.1 Call empty");
			auto func = PyDict_GetItemString(dict, "empty_func");
			assert(func);
			auto ret = PyObject_CallNoArgs(func);
			assert(ret);
			Py_DECREF(ret);
		FINISH;

		START(" 4.11 Call empty");
			auto func = PyDict_GetItemString(dict, "empty_func");
			assert(func);
			auto ret = PyObject_CallNoArgs(func);
			assert(ret);
			Py_DECREF(ret);
		FINISH;

		START(" 4.12 Call empty");
			auto func = PyDict_GetItemString(dict, "empty_func");
			assert(func);
			auto ret = PyObject_CallNoArgs(func);
			assert(ret);
			Py_DECREF(ret);
		FINISH;

		for (unsigned n = 0; n < CNT; ++n)
		{	VI_TM(" 4.2 Call empty (many times)");
			auto func = PyDict_GetItemString(dict, "empty_func");
			assert(func);
			auto ret = PyObject_CallNoArgs(func);
			assert(ret);
			Py_DECREF(ret);
		}

		{
			PyObject* result = nullptr;
			PyObject* args = nullptr;

			START(" 5.1 Call bubble_sort (arg init)");
				verify(args = PyTuple_New(2));
				{	auto tuple = PyTuple_New(std::size(sample_raw));
					assert(tuple);
					for (int i = 0; i < std::size(sample_raw); ++i)
					{	auto obj = PyLong_FromLong(sample_raw[i]);
						assert(obj);
						verify(0 == PyTuple_SetItem(tuple, i, obj));
					}
					verify(0 == PyTuple_SetItem(args, 0, tuple));
				}
				{
					static PyMethodDef func_def = {nullptr, c_descending, METH_VARARGS | METH_STATIC, nullptr};
					auto func = PyCFunction_NewEx(&func_def, NULL, NULL);
					assert(func);
					verify(0 == PyTuple_SetItem(args, 1, func));
				}
			FINISH;

			START(" 5.2 Call bubble_sort (call)");
				// Вызываем функцию и получаем результат
				auto func = PyDict_GetItemString(dict, "bubble_sort");
				assert(func && PyCallable_Check(func));
				// Создаем аргументы для вызова функции (tuple с одним элементом - нашим списком)
				verify(result = PyObject_CallObject(func, args));
			FINISH;

			for (auto&& i : sample_descending)
			{	auto obj = PyList_GetItem(result, &i - sample_descending);
				assert(obj);
				int val = 0;
				verify(PyArg_Parse(obj, "i", &val));
				assert(val == i);
			}
			Py_DECREF(args);
			Py_DECREF(result);
		}
		{
			PyObject* result = nullptr;
			PyObject* args = nullptr;

			START(" 5.3 Call bubble_sort (arg init)");
			verify(args = PyTuple_New(1));
			{
				auto tuple = PyTuple_New(std::size(sample_raw));
				assert(tuple);
				for (int i = 0; i < std::size(sample_raw); ++i)
				{
					auto obj = PyLong_FromLong(sample_raw[i]);
					assert(obj);
					verify(0 == PyTuple_SetItem(tuple, i, obj));
				}
				verify(0 == PyTuple_SetItem(args, 0, tuple));
			}
			FINISH;

			START(" 5.4 Call bubble_sort (call)");
			// Вызываем функцию и получаем результат
			auto func = PyDict_GetItemString(dict, "bubble_sort");
			assert(func&& PyCallable_Check(func));
			// Создаем аргументы для вызова функции (tuple с одним элементом - нашим списком)
			verify(result = PyObject_CallObject(func, args));
			FINISH;

			for (auto&& i : sample_sorted)
			{
				auto obj = PyList_GetItem(result, &i - sample_sorted);
				assert(obj);
				int val = 0;
				verify(PyArg_Parse(obj, "i", &val));
				assert(val == i);
			}
			Py_DECREF(args);
			Py_DECREF(result);
		}

		START("98 close");
		FINISH;

		START("99 Finalize");
			Py_Finalize();
		FINISH;
	FINISH;

	std::cout << "Python test1 result:\n";
	VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
	endl(std::cout);
}
