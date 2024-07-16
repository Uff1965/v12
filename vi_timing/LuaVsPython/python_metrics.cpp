// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://docs.python.org/3/extending/index.html
// https://docs.python.org/3/extending/embedding.html
// https://docs.python.org/3/c-api/index.html

#include "header.h"
#include "lua_metrics.h"

#include <vi_timing/timing.h>
#include "LuaVsPython.h"
#include <Python.h>

#include <cassert>
#include <iostream>
#include <string>
#include <thread>

#define START(s) \
	std::this_thread::yield(); \
	for (auto n = 5; n--;) \
	{	VI_TM(s); \
	} \
	VI_TM_CLEAR(s); \
	do { VI_TM(s)

#define END \
	} while(0)

using namespace std::string_literals;

namespace
{
	constexpr char sample[] = "global string";
}

void python_test()
{	VI_TM_CLEAR();

	START(" *** PYTHON ***");

		PyObject* module = nullptr;
		PyObject* dict = nullptr;

		START("1 Initialize");
			Py_Initialize();
		END;

		START("2 dofile");
			verify(module = PyImport_ImportModule("sample"));
			verify(dict = PyModule_GetDict(module));
		END;

		START("3 Get string");
			auto p = PyDict_GetItemString(dict, "global_string");
			assert(p);
			char* sz{};
			verify(PyArg_Parse(p, "s", &sz));
			assert(sz && 0 == strcmp(sz, sample));
		END;

		START("4 Call empty");
			auto func = PyDict_GetItemString(dict, "empty_func");
			assert(func);
			auto ret = PyObject_CallNoArgs(func);
			assert(ret);
			Py_DECREF(ret);
		END;

		START("5 Call strlen");
			auto func = PyDict_GetItemString(dict, "strlen_func");
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
		END;

		{	PyObject* list = nullptr;

			START("6 Call bubble_sort (arg init)");
				list = PyList_New(std::size(sample_raw));
				assert(list);
				for (int i = 0; i < std::size(sample_raw); ++i)
				{	auto obj = PyLong_FromLong(sample_raw[i]);
					assert(obj);
					verify(0 == PyList_SetItem(list, i, obj));
				}
			END;

			START("7 Call bubble_sort (call)");
				// Вызываем функцию и получаем результат
				auto func = PyDict_GetItemString(dict, "bubble_sort");
				assert(func && PyCallable_Check(func));
				// Создаем аргументы для вызова функции (tuple с одним элементом - нашим списком)
				auto args = PyTuple_Pack(1, list);
				auto result = PyObject_CallObject(func, args);
				Py_DECREF(result);
				Py_DECREF(args);
			END;

			for(auto &&i: sample_sorted)
			{	auto obj = PyList_GetItem(list, &i - sample_sorted);
				assert(obj);
				int val = 0;
				verify(PyArg_Parse(obj, "i", &val));
				assert(val == i);
			}
			Py_DECREF(list);
		}

		START("98 close");
			Py_DECREF(module);
		END;

		START("99 Finalize");
			Py_Finalize();
		END;
	END;

	std::cout << "Python test1 result:\n";
	VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
}
