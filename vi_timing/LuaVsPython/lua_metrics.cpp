// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://www.lua.org/manual/5.4/

#include "header.h"
#include "LuaVsPython.h"

#include <vi_timing/timing.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

namespace
{
	constexpr char script_path[] = "sample.lua";
/* Содержимое файла "sample.lua":
global_string = "global string"

function empty_func()
end

function bubble_sort(raw_data, comparator)
    if not comparator then
        comparator = c_ascending
    end
    local result = {table.unpack(raw_data)}
    local swapped = true
    while swapped do
        swapped = false
        for i = 2, #result do
            if comparator(result[i], result[i - 1]) then
                result[i], result[i - 1] = result[i - 1], result[i]
                swapped = true
            end
        end
    end
    return result
end
-- Файл дополнен неиспользуемым кодом для увеличения размера.
*/
	constexpr char global_string_sample[] = "global string";
	constexpr char global_string_name[] = "global_string";
	constexpr char empty_func_name[] = "empty_func";
	constexpr char bubble_sort_func_name[] = "bubble_sort";
} // namespace

extern "C"
{
	// c_ascending - C-функция которая будет зарегистрирована в Python под именем "c_ascending" и использована внутри скрипта
	static int c_ascending(lua_State *L)
	{	assert(2 == lua_gettop(L));
		const auto l = lua_tointeger(L, 1);
		const auto r = lua_tointeger(L, 2);
		lua_pushboolean(L, l < r);
		return 1;
	}

	// c_descending - C-функция которая будет передана из C-кода в Python-скрипт в качестве аргумента функции bubble_sort
	static int c_descending(lua_State *L)
	{	assert(2 == lua_gettop(L));
		const auto l = lua_tointeger(L, 1);
		const auto r = lua_tointeger(L, 2);
		lua_pushboolean(L, l > r);
		return 1;
	}
}

struct test_lua_t final: test_interface_t
{
	const std::string script_ = read_file(script_path);

	std::string title() const override { return "LUA"; };

	void InitializeEngine(const char* tm) const override;
	void* CompileScript(const char* tm) const override;
	std::string ExportCode(const char* tm, void*) const override;
	void* ImportCode(const char* tm, const std::string& p_code) const override;
	void ExecutionScript(const char* tm, void*) const override;
	void FunctionRegister(const char* tm) const override;
	void FinalizeEngine(const char* tm) const override;

	void WorkGetGlobalString(const char* tm) const override;
	void WorkCallEmptyFunction(const char* tm) const override;
	void* WorkBubbleSortPreparingArguments(const char* tm, bool descending) const override;
	void WorkBubbleSortRun(const char* tm, void* py_args, bool descending) const override;

	mutable lua_State *L_ = nullptr;

	inline static const auto _ = registrar(std::make_unique<test_lua_t>());
};

void test_lua_t::InitializeEngine(const char *tm) const
{	VI_TM(tm);
	verify(L_ = luaL_newstate());
	luaL_openlibs(L_);
	assert(0 == lua_gettop(L_)); // Стек пуст
}

void* test_lua_t::CompileScript(const char* tm) const
{	VI_TM(tm);
	verify(LUA_OK == luaL_loadstring(L_, script_.c_str()));
	assert(1 == lua_gettop(L_)); // На стеке скрипт
	return nullptr;
}

std::string test_lua_t::ExportCode(const char* tm, void*) const
{	VI_TM(tm);
	auto writer = [](lua_State*, const void* p, std::size_t sz, void* ud)
		{	auto& str = *static_cast<std::string*>(ud);
			str.append(static_cast<const char*>(p), sz);
			return 0;
		};
	std::string result;
	verify(LUA_OK == lua_dump(L_, writer, &result, 0));
	lua_pop(L_, 1);
	assert(0 == lua_gettop(L_)); // Стек пуст
	return result;
}

void* test_lua_t::ImportCode(const char* tm, const std::string& p_code) const
{	VI_TM(tm);
	verify(LUA_OK == luaL_loadbuffer(L_, p_code.data(), p_code.size(), "<script>"));
	assert(1 == lua_gettop(L_)); // На стеке скрипт
	return nullptr;
}

void test_lua_t::ExecutionScript(const char* tm, void*) const
{	VI_TM(tm);
	verify(LUA_OK == lua_pcall(L_, 0, 0, 0));
}

void test_lua_t::FunctionRegister(const char* tm) const
{	VI_TM(tm);
	lua_register(L_, "c_ascending", c_ascending);
	assert(0 == lua_gettop(L_));
}

void test_lua_t::FinalizeEngine(const char* tm) const
{	VI_TM(tm);
	lua_close(L_);
}

void test_lua_t::WorkGetGlobalString(const char* tm) const
{	VI_TM(tm);
	verify(LUA_TSTRING == lua_getglobal(L_, global_string_name));
	std::size_t len = 0;
	auto const sz = lua_tolstring(L_, -1, &len);
	assert(sz && len == strlen(sz) && 0 == strcmp(sz, global_string_sample));
	lua_pop(L_, 1);
	assert(0 == lua_gettop(L_));
}

void test_lua_t::WorkCallEmptyFunction(const char* tm) const
{	VI_TM(tm);
	verify(LUA_TFUNCTION == lua_getglobal(L_, empty_func_name));
	verify(LUA_OK == lua_pcall(L_, 0, 0, 0));
	assert(0 == lua_gettop(L_));
}

void* test_lua_t::WorkBubbleSortPreparingArguments(const char* tm, bool desc) const
{	VI_TM(tm);
	// Создаем и заполняем таблицу с числами для сортировки
	lua_createtable(L_, static_cast<unsigned>(std::size(sample_raw)), 0);
	for (int i = 1; i <= std::size(sample_raw); ++i)
	{	lua_pushnumber(L_, sample_raw[i - 1]);
		lua_rawseti(L_, -2, i);
	}
	assert(1 == lua_gettop(L_) && LUA_TTABLE == lua_type(L_, -1)); // На стеке таблица

	if (desc)
	{	lua_pushcfunction(L_, c_descending); // добавляем функцию сравнения
		assert(2 == lua_gettop(L_)); // На стеке: таблица и компаратор
	}
	return nullptr;
}

void test_lua_t::WorkBubbleSortRun(const char* tm, void*, bool desc) const
{	
	{	VI_TM(tm);
		const int nargs = desc? 2: 1; // Количество аргументов для функции bubble_sort
		assert(nargs == lua_gettop(L_)); // Аргументы уже на стеке
		verify(LUA_TFUNCTION == lua_getglobal(L_, bubble_sort_func_name)); // Помещаем функцию bubble_sort на вершину стека
		lua_insert(L_, -(nargs + 1)); // перемещаем функцию под аргументы
		assert(1 + nargs == lua_gettop(L_)); // На стеке: ф-я и аргументы
		verify(LUA_OK == lua_pcall(L_, nargs, 1, 0));
		assert(1 == lua_gettop(L_) && LUA_TTABLE == lua_type(L_, -1)); // На стеке остался результат
	}

	// Проверяем результат сортировки:
	auto const sample = desc? sample_descending_sorted: sample_ascending_sorted;
	lua_pushnil(L_); // Первый ключ
	while (lua_next(L_, -2) != 0)
	{	assert(3 == lua_gettop(L_) && lua_isinteger(L_, -2) && lua_isnumber(L_, -1)); // Таблица, ключ, значение
		const auto k = lua_tointeger(L_, -2); // Индекс в таблице
		const auto v = lua_tointeger(L_, -1); // Значение
		assert(sample[k - 1] == v); // Проверяем, что значение в таблице совпадает с эталоном
		lua_pop(L_, 1); // Удаляем значение, оставляем ключ для следующего lua_next
	}
	lua_pop(L_, 1); // Удаляем таблицу
	assert(0 == lua_gettop(L_));
}
