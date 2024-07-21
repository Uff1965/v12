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
	const auto ret = PyArg_ParseTuple(args, "ii", &l, &r); // Парсим аргументы
	return  (0 != ret) ? PyBool_FromLong(l < r) : nullptr;
}

void python_test()
{	VI_TM_CLEAR();

	{	VI_TM(" *** ALL ***");

		{	VI_TM("1 Initialize");
			Py_Initialize(); // Иницализируем интерпретатор
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
				code = Py_CompileString(text.c_str(), "sample.py", Py_file_input); // Компилируем скрипт
				dict = PyDict_New();
			}
			// Здесь можно сохранить скомпилированный скрипт и использовать его много раз.
			{	VI_TM(" 2.2 dofile (call)");
				auto module = PyEval_EvalCode(code, dict, nullptr); // Выполняем скрипт
				Py_DECREF(module);
			}
			Py_DECREF(code);
		}

		{	VI_TM(" 3 Get string");
			auto p = PyDict_GetItemString(dict, "global_string"); // Читаем глобальную переменную
			char* sz{};
			PyArg_Parse(p, "s", &sz); // Извлекаем строку
		}

		{	VI_TM(" 4.1 Call empty");
			auto func = PyDict_GetItemString(dict, "empty_func");
			auto ret = PyObject_CallNoArgs(func); // Вызываем функцию без параметров empty_func
			Py_DECREF(ret);
		}
		// Повторяем вызов функции несколько раз и убеждаемся, что выполнение каждый раз ускоряется
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
		// Получаем устоявшееся время выполнения вызова функции
		for (unsigned n = 0; n < 1'000; ++n)
		{	VI_TM(" 4.2 Call empty (many times)");
			auto func = PyDict_GetItemString(dict, "empty_func");
			auto ret = PyObject_CallNoArgs(func);
			Py_DECREF(ret);
		}

		{
			PyObject* args = nullptr;
			{	VI_TM(" 5.1 Call bubble_sort (arg init)");
				args = PyTuple_New(2); // Создаем tuple с двумя элементами для передачи двух аргументов в функцию
				{	auto tuple = PyTuple_New(std::size(sample_raw)); // Первый аргумент - кортеж чисел для сортировки
					for (int i = 0; i < std::size(sample_raw); ++i)
					{	auto obj = PyLong_FromLong(sample_raw[i]); // Создаем объект для каждого числа
						PyTuple_SetItem(tuple, i, obj); // Добавляем объект в кортеж
					}
					PyTuple_SetItem(args, 0, tuple); // Добавляем кортеж числе в tuple аргументов
				}
				{
					static PyMethodDef cmp_def =
					{	nullptr,		// Имя функции
						p_cmp,			// Указатель на функцию
						METH_VARARGS,	// Тип аргументов
						nullptr			// Документация
					};

					auto func = PyCFunction_NewEx(&cmp_def, NULL, NULL); // Создаем функцию для сравнения
					PyTuple_SetItem(args, 1, func); // Добавляем функцию вторым аргументом
				}
			}

			PyObject* result = nullptr;
			{	VI_TM(" 5.2 Call bubble_sort (call)");
				// Вызываем функцию и получаем результат
				auto func = PyDict_GetItemString(dict, "bubble_sort");
				// Создаем аргументы для вызова функции (tuple с одним элементом - нашим списком)
				result = PyObject_CallObject(func, args); // Вызываем функцию с аргументами
			}
			// Проверяем, что результат сортировки соответствует ожидаемому
			for (auto&& i : sample_ascending_sorted)
			{	int val = 0;
				auto obj = PyList_GetItem(result, &i - sample_ascending_sorted); // Получаем элемент из списка
				PyArg_Parse(obj, "i", &val);
				assert(i == val);
			}
			Py_DECREF(args);
			Py_DECREF(result);
		}

		{	VI_TM("98 close");
		}

		{	VI_TM("99 Finalize");
			Py_Finalize(); // Завершаем работу интерпретатора
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
