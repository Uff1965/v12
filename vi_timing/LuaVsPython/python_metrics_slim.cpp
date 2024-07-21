#include <vi_timing/timing.h>
#include "LuaVsPython.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <Python.h>



#ifdef NDEBUG
constexpr auto sample_size = 10'000U;
#else
constexpr auto sample_size = 1'000U;
#endif
extern const int(&sample_raw)[sample_size];
extern const int(&sample_ascending_sorted)[sample_size];

extern "C" PyObject* p_cmp(PyObject*, PyObject* args)
{	int l, r;
	const auto ret = PyArg_ParseTuple(args, "ii", &l, &r); // ������ ���������
	return  (0 != ret) ? PyBool_FromLong(l < r) : nullptr;
}

void python_test()
{	VI_TM_CLEAR();

	{	VI_TM(" *** ALL ***");

		{	VI_TM("1 Initialize");
			Py_Initialize(); // ������������� �������������
		}

		std::string text;
		{	VI_TM(" 0 Load text");
			std::ifstream file("sample.py");
			std::ostringstream ss;
			ss << file.rdbuf();
			text = ss.str();
		}

		PyObject* dict = nullptr;
		{	PyObject* code = nullptr;

			{	VI_TM(" 2.1 dofile (load+compile)");
				code = Py_CompileString(text.c_str(), "sample.py", Py_file_input); // ����������� ������
				dict = PyDict_New();
			}
			// ����� ����� ��������� ���������������� ������ � ������������ ��� ����� ���.
			{	VI_TM(" 2.2 dofile (call)");
				auto module = PyEval_EvalCode(code, dict, nullptr); // ��������� ������
				Py_DECREF(module);
			}
			Py_DECREF(code);
		}

		{	VI_TM(" 3 Get string");
			auto p = PyDict_GetItemString(dict, "global_string"); // ������ ���������� ����������
			char* sz{};
			PyArg_Parse(p, "s", &sz); // ��������� ������
		}

		{	VI_TM(" 4.1 Call empty");
			auto func = PyDict_GetItemString(dict, "empty_func");
			auto ret = PyObject_CallNoArgs(func); // �������� ������� ��� ���������� empty_func
			Py_DECREF(ret);
		}
		// ��������� ����� ������� ��������� ��� � ����������, ��� ���������� ������ ��� ����������
		{	VI_TM(" 4.11 Call empty");
			auto func = PyDict_GetItemString(dict, "empty_func");
			auto ret = PyObject_CallNoArgs(func);
			Py_DECREF(ret);
		}

		{	VI_TM(" 4.12 Call empty");
			auto func = PyDict_GetItemString(dict, "empty_func");
			auto ret = PyObject_CallNoArgs(func);
			Py_DECREF(ret);
		}
		// �������� ����������� ����� ���������� ������ �������
		for (unsigned n = 0; n < 1'000; ++n)
		{	VI_TM(" 4.2 Call empty (many times)");
			auto func = PyDict_GetItemString(dict, "empty_func");
			auto ret = PyObject_CallNoArgs(func);
			Py_DECREF(ret);
		}

		{
			PyObject* args = nullptr;
			{	VI_TM(" 5.1 Call bubble_sort (arg init)");
				args = PyTuple_New(2); // ������� tuple � ����� ���������� ��� �������� ���� ���������� � �������
				{	auto tuple = PyTuple_New(std::size(sample_raw)); // ������ �������� - ������ ����� ��� ����������
					for (int i = 0; i < std::size(sample_raw); ++i)
					{	auto obj = PyLong_FromLong(sample_raw[i]); // ������� ������ ��� ������� �����
						PyTuple_SetItem(tuple, i, obj); // ��������� ������ � ������
					}
					PyTuple_SetItem(args, 0, tuple); // ��������� ������ ����� � tuple ����������
				}
				{
					static PyMethodDef cmp_def =
					{	nullptr,		// ��� �������
						p_cmp,			// ��������� �� �������
						METH_VARARGS,	// ��� ����������
						nullptr			// ������������
					};

					auto func = PyCFunction_NewEx(&cmp_def, NULL, NULL); // ������� ������� ��� ���������
					PyTuple_SetItem(args, 1, func); // ��������� ������� ������ ����������
				}
			}

			PyObject* result = nullptr;
			{	VI_TM(" 5.2 Call bubble_sort (call)");
				// �������� ������� � �������� ���������
				auto func = PyDict_GetItemString(dict, "bubble_sort");
				// ������� ��������� ��� ������ ������� (tuple � ����� ��������� - ����� �������)
				result = PyObject_CallObject(func, args); // �������� ������� � �����������
			}
			// ���������, ��� ��������� ���������� ������������� ����������
			for (auto&& i : sample_ascending_sorted)
			{	int val = 0;
				auto obj = PyList_GetItem(result, &i - sample_ascending_sorted); // �������� ������� �� ������
				PyArg_Parse(obj, "i", &val);
				assert(i == val);
			}
			Py_DECREF(args);
			Py_DECREF(result);
		}

		{	VI_TM("98 close");
		}

		{	VI_TM("99 Finalize");
			Py_Finalize(); // ��������� ������ ��������������
		}
	}

	std::cout << "Python test1 result:\n";
	VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
	endl(std::cout);
}

struct impl_test_t: test_t
{
	void test() const override { python_test(); }
	const std::string& name() const override { return "PYTHON slim"; }
	inline static auto _ = (registrar(std::make_unique<impl_test_t>()), 0);
};
