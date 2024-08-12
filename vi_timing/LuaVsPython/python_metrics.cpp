// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://docs.python.org/3/extending/embedding.html
// https://docs.python.org/3/c-api/index.html
// https://docs.python.org/3/library/marshal.html

#include "header.h"
#include "LuaVsPython.h"

#include <vi_timing/timing.h>

#include <Python.h>
#include <marshal.h> // For PyMarshal_WriteObjectToString, PyMarshal_ReadObjectFromString

#include <cassert>
#include <vector>

#ifndef _DLL
	#error "For the PyRun_SimpleFile function to work, the MS RTL must be loaded from a DLL (MS compiler option: /MD or /MDd). Established experimentally on Python v.3.12.2"
#endif

namespace
{
	const char script_path[] = "sample.py";
/* ���������� ����� "sample.py":
global_string = "global string"

def empty_func():
	pass

def bubble_sort(raw_data, comparator = None):
	if not comparator:
		comparator = c_ascending
	result = list(raw_data)
	swapped = True
	while swapped:
		swapped = False
		for i in range(1, len(result)):
			if comparator(result[i], result[i - 1]):
				result[i], result[i - 1] = result[i - 1], result[i]
				swapped = True
	return result
# ���� �������� �������������� ����� ��� ���������� �������.
*/
	constexpr char global_string_name[] = "global_string";
	constexpr char global_string_sample[] = "global string";
	constexpr char empty_func_name[] = "empty_func";
	constexpr char bubble_sort_func_name[] = "bubble_sort";
} // namespace

extern "C"
{
	// c_ascending - C-������� ������� ����� ���������������� � Python ��� ������ "c_ascending" � ����� ������������ ������ �������
	static PyObject *c_ascending(PyObject *, PyObject *args)
	{	assert(args && PyTuple_Check(args) && 2 == PyTuple_Size(args));
		int l, r;
		const auto ret = PyArg_ParseTuple(args, "ii", &l, &r); // ������ ���������
		return verify(0 != ret) ? PyBool_FromLong(l < r) : nullptr;
	}

	// c_descending - C-������� ������� ����� �������� �� C-���� � Python-������ � �������� ��������� ������� bubble_sort
	static PyObject *c_descending(PyObject *, PyObject *args)
	{	assert(args && PyTuple_Check(args) && 2 == PyTuple_Size(args));
		int l, r;
		const auto ret = PyArg_ParseTuple(args, "ii", &l, &r);
		return  verify(0 != ret) ? PyBool_FromLong(r < l) : nullptr;
	}
}

struct test_python_t: test_interface_t
{
	inline static const auto _ = registrar(std::make_unique<test_python_t>());
	const std::string script_ = read_file(script_path);
	mutable PyObject *py_dict_ = nullptr;

	std::string title() const override { return "PYTHON"; };

	void InitializeEngine(const char* tm) const override;
	void* CompileScript(const char* tm) const override;
	std::string ExportCode(const char* tm, void* py_obj) const override;
	void RestartEngine(const char* tm) const override;
	void* ImportCode(const char* tm, const std::string& p_code) const override;
	void ExecutionScript(const char* tm, void* py_obj) const override;
	void FunctionRegister(const char* tm) const override;
	void FinalizeEngine(const char* tm) const override;

	void WorkGetGlobalString(const char* tm) const override;
	void WorkCallEmptyFunction(const char* tm) const override;
	void* WorkBubbleSortPreparingArguments(const char* tm, bool descending) const override;
	void WorkBubbleSortRun(const char* tm, void* py_args, bool descending) const override;
};

void test_python_t::InitializeEngine(const char* tm) const
{	VI_TM(tm);
	Py_InitializeEx(1);
	assert(Py_IsInitialized());
}

void* test_python_t::CompileScript(const char* tm) const
{	VI_TM(tm);
	auto result = Py_CompileString(script_.c_str(), "sample.py", Py_file_input);
	assert(result && PyCode_Check(result));
	return result;
}

std::string test_python_t::ExportCode(const char* tm, void* code) const
{	VI_TM(tm);
	auto py_code = static_cast<PyObject*>(code);
	assert(py_code && PyCode_Check(py_code));
	auto py_buffer = PyMarshal_WriteObjectToString(py_code, Py_MARSHAL_VERSION); // ����������� ���
	assert(py_buffer && PyBytes_Check(py_buffer));
	Py_DECREF(py_code); // ������� ���������������� ���
	char *buffer = nullptr;
	Py_ssize_t buffer_size = 0;
	verify(0 == PyBytes_AsStringAndSize(py_buffer, &buffer, &buffer_size)); // ��������� P-���
	std::string result(buffer, buffer_size);
	Py_DECREF(py_buffer);
	return result;
}

void test_python_t::RestartEngine(const char* tm) const
{	VI_TM(tm);
	assert(Py_IsInitialized());
	verify(0 == Py_FinalizeEx());
	assert(!Py_IsInitialized());
	Py_InitializeEx(1);
	assert(Py_IsInitialized());
}

void* test_python_t::ImportCode(const char* tm, const std::string &code) const
{	VI_TM(tm);
	auto result = PyMarshal_ReadObjectFromString(code.data(), code.size()); // ������������� ���
	assert(result && PyCode_Check(result));
	return result;
}

void test_python_t::ExecutionScript(const char* tm, void* code) const
{	VI_TM(tm);
	auto py_code = static_cast<PyObject*>(code);
	assert(py_code && PyCode_Check(py_code));
	verify(py_dict_ = PyDict_New());
	auto py_result = PyEval_EvalCode(py_code, py_dict_, nullptr);
	assert(Py_None == py_result); // ��� ������ �� ���������� ������� ��������.
	Py_DECREF(py_result);
	Py_DECREF(py_code);
}

void test_python_t::FunctionRegister(const char* tm) const
{	VI_TM(tm);
	static PyMethodDef func_def =
	{	nullptr, // ��� �������
		c_ascending, // ��������� �� �������
		METH_VARARGS, // ��� ����������
		nullptr // ������������
	};
	auto py_func = PyCFunction_New(&func_def, nullptr);
	assert(py_func && PyCFunction_Check(py_func));
	verify(0 == PyDict_SetItemString(py_dict_, "c_ascending", py_func));
	Py_DECREF(py_func);
}

void test_python_t::FinalizeEngine(const char* tm) const
{	VI_TM(tm);
	Py_DECREF(py_dict_);
	py_dict_ = nullptr;
	verify(0 == Py_FinalizeEx()); // ��������� ������ ��������������
	assert(!Py_IsInitialized());
}

void test_python_t::WorkGetGlobalString(const char* tm) const
{	VI_TM(tm);
	auto py_string = PyDict_GetItemString(py_dict_, global_string_name);
	assert(py_string);
	char* sz = nullptr;
	verify(0 != PyArg_Parse(py_string, "s", &sz));
	assert(sz && 0 == strcmp(sz, global_string_sample));
}

void test_python_t::WorkCallEmptyFunction(const char* tm) const
{	VI_TM(tm);
	auto func = PyDict_GetItemString(py_dict_, empty_func_name);
	assert(func);
	auto ret = PyObject_CallNoArgs(func);
	assert(ret);
	Py_DECREF(ret);
}

void* test_python_t::WorkBubbleSortPreparingArguments(const char* tm, bool desc) const
{	VI_TM(tm);
	// desc ���������� ������� �� �� � ������� ���������� �������������� �������� ����������
	auto result = PyTuple_New(desc? 2: 1); // ������ ������ ���������� �� 1 ��� 2 ���������
	assert(result && PyTuple_Check(result));

	{	auto raw_data = PyTuple_New(std::size(sample_raw)); // ������ ���������� ������� ������ ��� ����������
		assert(raw_data && PyTuple_Check(raw_data));
		for (int i = 0; i < std::size(sample_raw); ++i)
		{	auto obj = PyLong_FromLong(sample_raw[i]);
			assert(obj);
			verify(0 == PyTuple_SetItem(raw_data, i, obj));
		}
		verify(0 == PyTuple_SetItem(result, 0, raw_data));
	}

	if(desc)
	{	// ������ ���������� ������� ������� ��������� ��� ���������� �� ��������
		static PyMethodDef comparator_def = { nullptr, c_descending, METH_VARARGS | METH_STATIC, nullptr };
		auto comparator = PyCFunction_NewEx(&comparator_def, NULL, NULL);
		assert(comparator);
		verify(0 == PyTuple_SetItem(result, 1, comparator));
	}
	return result;
}

void test_python_t::WorkBubbleSortRun(const char* tm, void *args, bool desc) const
{	PyObject *result = nullptr;
	{	VI_TM(tm);
		auto py_args = static_cast<PyObject*>(args);
		assert(PyTuple_Check(py_args));
		auto func = PyDict_GetItemString(py_dict_, bubble_sort_func_name);
		assert(func && PyCallable_Check(func));
		verify(result = PyObject_CallObject(func, py_args));
		Py_DECREF(py_args);
	}

	// ��������� ���������
	auto &smpl = desc? sample_descending_sorted: sample_ascending_sorted;
	for (auto&& i : smpl)
	{	auto obj = PyList_GetItem(result, &i - smpl.data());
		assert(obj);
		int val = 0;
		verify(0 != PyArg_Parse(obj, "i", &val));
		assert(val == i);
	}
	Py_DECREF(result);
}

struct test_python_mt_t: test_python_t
{
	std::string title() const override { return "LUA MT"; };

	void test() const override;

//	inline static const auto _ = registrar(std::make_unique<test_python_mt_t>());
};

void test_python_mt_t::test() const
{
	std::vector<std::jthread> arr(2 * std::thread::hardware_concurrency());
	for (auto &&t : arr)
		t = std::jthread([] { test_python_t i; i.test(); });
}
