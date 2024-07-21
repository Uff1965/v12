// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://docs.python.org/3/extending/index.html
// https://docs.python.org/3/extending/embedding.html
// https://docs.python.org/3/c-api/index.html

#include "header.h"
#include "LuaVsPython.h"

#include <vi_timing/timing.h>

#include <Python.h>
#include <marshal.h> // For PyMarshal_WriteObjectToString, PyMarshal_ReadObjectFromString

#include <cassert>

// verify реализован следующим образом, чтобы не засорять код проверками на nullptr:
// inline bool verify(bool b) { assert(b); return b; }

namespace
{
	// Текст скрипта на Python используемый в тесте:
	constexpr char sample_py[] = R"(
global_string = "global string"

def empty_func():
	pass

def simple_func(a):
	return a

def bubble_sort(t, cmp = None):
	if not cmp:
		cmp = c_ascending
	a = list(t)
	swapped = True
	while swapped:
		swapped = False
		for i in range(1, len(a)):
			if cmp(a[i], a[i - 1]):
				a[i], a[i - 1] = a[i - 1], a[i]
				swapped = True
	return a
)";
	// Эталон строки для проверки получения глобальной переменной из скрипта:
	constexpr char sample[] = "global string";
}

extern "C"
{
	// c_ascending - C-функция которая будет зарегистрирована в Python под именем "c_ascending" и будет использована внутри скрипта
	static PyObject *c_ascending(PyObject *, PyObject *args)
	{	int l, r;
		assert(PyTuple_Check(args) && 2 == PyTuple_Size(args));
		const auto ret = PyArg_ParseTuple(args, "ii", &l, &r); // Парсим аргументы
		return verify(0 != ret) ? PyBool_FromLong(l < r) : nullptr;
	}

	// c_descending - C-функция которая будет передана из C-кода в Python-скрипт в качестве аргумента функции bubble_sort
	static PyObject *c_descending(PyObject *, PyObject *args)
	{	int l, r;
		assert(PyTuple_Check(args) && 2 == PyTuple_Size(args));
		const auto ret = PyArg_ParseTuple(args, "ii", &l, &r);
		return  verify(0 != ret) ? PyBool_FromLong(r < l) : nullptr;
	}
}

struct test_python_t final: test_interface_t
{
	std::string title() const override { return "PYTHON"; };

	void InitializeEngine(const char* tm) const override;
	void* CompileScript(const char* tm) const override;
	std::string ExportCode(const char* tm, void* py_obj) const override;
	void* ImportCode(const char* tm, const std::string& p_code) const override;
	void ExecutionScript(const char* tm, void* py_obj) const override;
	void CloseScript(const char* tm) const override;
	void FinalizeEngine(const char* tm) const override;

	void WorkGetString(const char* tm) const override;
	void WorkCallEmpty(const char* tm) const override;
	void WorkCallSimple(const char* tm) const override;
	void* WorkBubbleSortArgs(const char* tm, bool descending) const override;
	void WorkBubbleSortRun(const char* tm, void* py_args, bool descending) const override;

	mutable PyObject *dict_ = nullptr;

	inline static const auto _ = registrar(std::make_unique<test_python_t>());
};

void test_python_t::InitializeEngine(const char* tm) const
{	START(tm);
		Py_Initialize();
	FINISH;
}

void* test_python_t::CompileScript(const char* tm) const
{	PyObject *result = nullptr;
	START(tm);
		verify(result = Py_CompileString(sample_py, "sample.py", Py_file_input));
	FINISH;
	return result;
}

std::string test_python_t::ExportCode(const char* tm, void* code) const
{	auto py_code = static_cast<PyObject*>(code);
	assert(PyCode_Check(py_code));
	std::string result;
	START(tm);
		auto py_buffer = PyMarshal_WriteObjectToString(py_code, Py_MARSHAL_VERSION); // Сериализуем код
		assert(py_buffer);
		Py_DECREF(py_code); // Удаляем скомпилированный код
		char *buffer = nullptr;
		Py_ssize_t buffer_size = 0;
		verify(0 == PyBytes_AsStringAndSize(py_buffer, &buffer, &buffer_size)); // Сохраняем P-код
		result.assign(buffer, buffer_size);
		Py_DECREF(py_buffer);
	FINISH;
	return result;
}

void* test_python_t::ImportCode(const char* tm, const std::string &code) const
{	void *result = nullptr;
	START(tm);
		verify(result = PyMarshal_ReadObjectFromString(code.data(), code.size())); // Десериализуем код
	FINISH;
	return result;
}

void test_python_t::ExecutionScript(const char* tm, void* code) const
{	auto py_code = static_cast<PyObject*>(code);
	assert(PyCode_Check(py_code));
	START(tm);
		verify(dict_ = PyDict_New());
		auto py_module = PyEval_EvalCode(py_code, dict_, nullptr);
		assert(py_module);
		Py_DECREF(py_module);
		Py_DECREF(py_code);

		static PyMethodDef func_def =
		{	nullptr, // Имя функции
			c_ascending, // Указатель на функцию
			METH_VARARGS, // Тип аргументов
			nullptr // Документация
		};
		auto func = PyCFunction_NewEx(&func_def, NULL, NULL);
		assert(func);
		verify(0 == PyDict_SetItemString(dict_, "c_ascending", func));
		Py_DECREF(func);
	FINISH;
}

void test_python_t::WorkGetString(const char* tm) const
{	START(tm);
		auto p = PyDict_GetItemString(dict_, "global_string");
		assert(p);
		char* sz = nullptr;
		verify(0 != PyArg_Parse(p, "s", &sz));
		assert(sz && 0 == strcmp(sz, sample));
	FINISH;
}

void test_python_t::WorkCallEmpty(const char* tm) const
{	START(tm);
		auto func = PyDict_GetItemString(dict_, "empty_func");
		assert(func);
		auto ret = PyObject_CallNoArgs(func);
		assert(ret);
		Py_DECREF(ret);
	FINISH;
}

void test_python_t::WorkCallSimple(const char* tm) const
{	START(tm);
		auto func = PyDict_GetItemString(dict_, "simple_func");
		assert(func);
		auto arg = PyLong_FromLong(777);
		auto ret = PyObject_CallFunctionObjArgs(func, arg, nullptr);
		assert(ret);
		auto result = PyLong_AsLong(ret);
		assert(result == 777);
		Py_DECREF(ret);
		Py_DECREF(arg);
	FINISH;
}

void* test_python_t::WorkBubbleSortArgs(const char* tm, bool descending) const
{	PyObject *result = nullptr;
	START(tm);
		// descending определяет передаём ли мы в функцию сортировки дополнительный аргумент
		verify(result = PyTuple_New(descending? 2: 1)); // Создаём кортеж аргументов из 1 или 2 элементов

		{	auto tuple = PyTuple_New(std::size(sample_raw)); // первым аргументом передаём список для сортировки
			assert(tuple);
			for (int i = 0; i < std::size(sample_raw); ++i)
			{	auto obj = PyLong_FromLong(sample_raw[i]);
				assert(obj);
				verify(0 == PyTuple_SetItem(tuple, i, obj));
			}
			verify(0 == PyTuple_SetItem(result, 0, tuple));
		}

		if(descending)
		{	// Вторым аргументом передаём функцию сравнения для сортировки по убыванию
			static PyMethodDef func_def = { nullptr, c_descending, METH_VARARGS | METH_STATIC, nullptr };
			auto func = PyCFunction_NewEx(&func_def, NULL, NULL);
			assert(func);
			verify(0 == PyTuple_SetItem(result, 1, func));
		}
	FINISH;
	return result;
}

void test_python_t::WorkBubbleSortRun(const char* tm, void *args, bool descending) const
{	auto py_args = static_cast<PyObject*>(args);
	assert(PyTuple_Check(py_args));
	PyObject *result = nullptr;
	START(tm);
		auto func = PyDict_GetItemString(dict_, "bubble_sort");
		assert(func && PyCallable_Check(func));
		verify(result = PyObject_CallObject(func, py_args));
		Py_DECREF(py_args);
	FINISH;

	// Проверяем результат
	auto &smpl = descending? sample_descending_sorted: sample_ascending_sorted;
	for (auto&& i : smpl)
	{	auto obj = PyList_GetItem(result, &i - smpl);
		assert(obj);
		int val = 0;
		verify(0 != PyArg_Parse(obj, "i", &val));
		assert(val == i);
	}
	Py_DECREF(result);
}

void test_python_t::CloseScript(const char* tm) const
{	START(tm);
		Py_DECREF(dict_);
		dict_ = nullptr;
	FINISH;
}

void test_python_t::FinalizeEngine(const char* tm) const
{	START(tm);
		Py_Finalize(); // Завершаем работу интерпретатора
	FINISH;
}
