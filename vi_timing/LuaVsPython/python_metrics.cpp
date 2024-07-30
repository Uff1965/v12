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
#include <fstream>
#include <sstream>

#ifndef _DLL
	#error "For the PyRun_SimpleFile function to work, the MS RTL must be loaded from a DLL (MS compiler option: /MD or /MDd). Established experimentally on Python v.3.12.2"
#endif
constexpr auto py_ver = PY_VERSION_HEX;
constexpr char py_ver_s[] = PY_VERSION;

namespace
{
	const char filename[] = "sample.py";

	auto read_file = [](const char *filename) -> std::string
	{
//*
		std::ifstream file(filename);
		assert(file);
		std::ostringstream ss;
		ss << file.rdbuf();
		return ss.str();
/*/
		auto f = std::fopen(filename, "r");
		assert(f);
		verify(0 == fseek(f, 0, SEEK_END));
		auto sz = ftell(f);
		verify(0 == fseek(f, 0, SEEK_SET));
		std::string result;
		result.resize(sz);
		verify(sz == fread(result.data(), 1, sz, f));
		verify(0 == fclose(f));
		return result;
//*/
	};

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
	std::string sample_py = read_file(filename);

	std::string title() const override { return "PYTHON"; };

	void InitializeEngine(const char* tm) const override;
	void* CompileScript(const char* tm) const override;
	std::string ExportCode(const char* tm, void* py_obj) const override;
	void* ImportCode(const char* tm, const std::string& p_code) const override;
	void ExecutionScript(const char* tm, void* py_obj) const override;
	void FunctionRegister(const char* tm) const override;
	void FinalizeEngine(const char* tm) const override;

	void WorkGetString(const char* tm) const override;
	void WorkCallEmpty(const char* tm) const override;
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
		verify(result = Py_CompileString(sample_py.c_str(), "sample.py", Py_file_input));
	FINISH;
	return result;
}

std::string test_python_t::ExportCode(const char* tm, void* code) const
{	auto py_code = static_cast<PyObject*>(code);
	assert(py_code && PyCode_Check(py_code));
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
	FINISH;
}

void test_python_t::FunctionRegister(const char* tm) const
{	START(tm);
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

void test_python_t::FinalizeEngine(const char* tm) const
{	START(tm);
		Py_DECREF(dict_);
		dict_ = nullptr;
		verify(0 == Py_FinalizeEx()); // Завершаем работу интерпретатора
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

void* test_python_t::WorkBubbleSortArgs(const char* tm, bool descending) const
{	PyObject *result = nullptr;
	START(tm);
		// descending определяет передаём ли мы в функцию сортировки дополнительный аргумент
		verify(result = PyTuple_New(descending? 2: 1)); // Создаём кортеж аргументов из 1 или 2 элементов
		assert(result && PyTuple_Check(result));

		{	auto tuple = PyTuple_New(std::size(sample_raw)); // первым аргументом передаём список для сортировки
			assert(tuple && PyTuple_Check(tuple));
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

struct test2_python_t: test_t
{
	void test() const override;
	std::string title() const override { return "test2_python_t"; }

	inline static const auto _ = registrar(std::make_unique<test2_python_t>());
};

void test2_python_t::test() const
{
	for(int i = 0; i < 100; ++i)
	{ // Warming up file reading.
		read_file(filename);
	}

	{	VI_TM("*P* 1. PyRun_SimpleFile");
		Py_InitializeEx(0);

#pragma warning(suppress: 4996)
		FILE *file = std::fopen(filename, "r");
		assert(file && !std::ferror(file));
		verify( 0 == PyRun_SimpleFile(file, filename));
		verify(0 == std::fclose(file));

		verify(0 == Py_FinalizeEx());
	}

	{	VI_TM("*P* 2. read_file + PyRun_String");
		Py_Initialize();

		const auto &&text = read_file(filename);

		auto py_dict = PyDict_New();
		verify(py_dict);

		auto py_obj = PyRun_String(text.data(), Py_file_input, py_dict, nullptr);
		assert(py_obj);

		Py_DECREF(py_dict);
		verify(0 == Py_FinalizeEx());
	}

	{	VI_TM("*P* 3. read_file + Py_CompileString + PyEval_EvalCode");
		Py_Initialize();

		const auto &&text = read_file(filename);

		PyObject *py_code = Py_CompileString(text.data(), "sample.py", Py_file_input);
		assert(py_code);
		auto py_dict = PyDict_New();
		verify(py_dict);
		auto py_module = PyEval_EvalCode(py_code, py_dict, nullptr);
		assert(py_module);
		Py_DECREF(py_module);
		Py_DECREF(py_dict);
		Py_DECREF(py_code);

		verify(0 == Py_FinalizeEx());
	}

	{	std::string buffer;

		Py_InitializeEx(0);
		{
			VI_TM("*P* 4. Export");

			const auto &&text = read_file(filename);
			PyObject *py_code = Py_CompileString(text.data(), "sample.py", Py_file_input);
			assert(py_code && PyCode_Check(py_code));

			auto py_buffer = PyMarshal_WriteObjectToString(py_code, Py_MARSHAL_VERSION); // Сериализуем код
			assert(py_buffer);
			Py_DECREF(py_code); // Удаляем скомпилированный код
			char *buff = nullptr;
			Py_ssize_t buff_size = 0;
			verify(0 == PyBytes_AsStringAndSize(py_buffer, &buff, &buff_size)); // Сохраняем P-код
			buffer.assign(buff, buff_size);
			Py_DECREF(py_buffer);
		}

		{	VI_TM("*P* 5. Import");

			auto py_code = PyMarshal_ReadObjectFromString(buffer.data(), buffer.size());
			verify(py_code && PyCode_Check(py_code));

			auto py_dict = PyDict_New();
			verify(py_dict);
			auto py_module = PyEval_EvalCode(py_code, py_dict, nullptr);
			assert(py_module);
			Py_DECREF(py_module);
			Py_DECREF(py_dict);
			Py_DECREF(py_code);
		}

		verify(0 == Py_FinalizeEx());
	}
}
