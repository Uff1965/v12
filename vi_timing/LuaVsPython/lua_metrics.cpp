// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://www.lua.org/manual/5.4/

#include "header.h"
#include "LuaVsPython.h"

#include <vi_timing/timing.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cassert>

namespace
{
	// Текст скрипта на Lua используемый в тесте:
	constexpr char sample_lua[] = R"(
global_string = "global string"

function empty_func()
end

function simple_func(a)
	return a
end

function bubble_sort(aa, cmp)
    cmp = cmp or function(l, r) return l < r end

    local a = {table.unpack(aa)}
    local swapped = true
    while swapped do
        swapped = false
        for i = 2, #a do
            if cmp(a[i], a[i - 1]) then
                a[i], a[i - 1] = a[i - 1], a[i]
                swapped = true
            end
        end
    end
    return a
end
)";
	// Эталон строки для проверки получения глобальной переменной из скрипта:
	constexpr char sample[] = "global string";
}

extern "C"
{
	// c_ascending - C-функция которая будет зарегистрирована в Python под именем "c_ascending" и будет использована внутри скрипта
	static int c_ascending(lua_State *L)
	{
		assert(2 == lua_gettop(L));
		const auto l = lua_tointeger(L, 1);
		const auto r = lua_tointeger(L, 2);
		lua_pushboolean(L, l < r);
		return 1;
	}

	// c_descending - C-функция которая будет передана из C-кода в Python-скрипт в качестве аргумента функции bubble_sort
	extern "C" static int c_descending(lua_State *L)
	{
		assert(2 == lua_gettop(L));
		const auto l = lua_tointeger(L, 1);
		const auto r = lua_tointeger(L, 2);
		lua_pushboolean(L, l > r);
		return 1;
	}
}

struct test_lua_t final: test_interface_t
{
	std::string title() const override { return "LUA"; };

	void InitializeEngine(const char* tm) const override;
	void* CompileScript(const char* tm) const override;
	std::string ExportCode(const char* tm, void*) const override;
	void* ImportCode(const char* tm, const std::string& p_code) const override;
	void ExecutionScript(const char* tm, void*) const override;
	void CloseScript(const char* tm) const override;
	void FinalizeEngine(const char* tm) const override;

	void WorkGetString(const char* tm) const override;
	void WorkCallEmpty(const char* tm) const override;
	void WorkCallSimple(const char* tm) const override;
	void* WorkBubbleSortArgs(const char* tm, bool descending) const override;
	void WorkBubbleSortRun(const char* tm, void* py_args, bool descending) const override;

	mutable lua_State *L = nullptr;

	inline static const auto _ = registrar(std::make_unique<test_lua_t>());
};

void test_lua_t::InitializeEngine(const char *tm) const
{	START(tm);
	FINISH;
}

void* test_lua_t::CompileScript(const char* tm) const
{	START(tm);
		verify(L = luaL_newstate());
		luaL_openlibs(L);
		verify(LUA_OK == luaL_loadstring(L, sample_lua));
		assert(1 == lua_gettop(L)); // На стеке скрипт
	FINISH;
	return nullptr;
}

std::string test_lua_t::ExportCode(const char* tm, void*) const
{	std::string result;
	START(tm);
		auto writer = [](lua_State*, const void* p, std::size_t sz, void* ud)
			{	auto& buf = *static_cast<std::string*>(ud);
				buf.append(static_cast<const char*>(p), sz);
				return 0;
			};
		verify(LUA_OK == lua_dump(L, writer, &result, 0));
		lua_pop(L, 1);
		assert(0 == lua_gettop(L)); // Стек пуст
	FINISH;
	return result;
}

void* test_lua_t::ImportCode(const char* tm, const std::string& p_code) const
{	START(tm);
		verify(LUA_OK == luaL_loadbuffer(L, p_code.data(), p_code.size(), "<script>"));
		assert(1 == lua_gettop(L)); // На стеке скрипт
	FINISH;
	return nullptr;
}

void test_lua_t::ExecutionScript(const char* tm, void*) const
{	START(tm);
		lua_register(L, "c_ascending", c_ascending);
		verify(LUA_OK == lua_pcall(L, 0, 0, 0));
		assert(0 == lua_gettop(L)); // Стек пуст
	FINISH;
}

void test_lua_t::CloseScript(const char* tm) const
{	START(tm);
		lua_close(L);
	FINISH;
}

void test_lua_t::FinalizeEngine(const char* tm) const
{	START(tm);
	FINISH;
}

void test_lua_t::WorkGetString(const char* tm) const
{	START(tm);
		verify(LUA_TSTRING == lua_getglobal(L, "global_string"));
		std::size_t len = 0;
		auto sz = lua_tolstring(L, -1, &len);
		assert(sz && len == strlen(sz) && 0 == strcmp(sz, sample));
		lua_pop(L, 1);
		assert(0 == lua_gettop(L)); // Стек пуст
	FINISH;
}

void test_lua_t::WorkCallEmpty(const char* tm) const
{	START(tm);
		verify(LUA_TFUNCTION == lua_getglobal(L, "empty_func"));
		verify(LUA_OK == lua_pcall(L, 0, 0, 0));
		assert(0 == lua_gettop(L)); // Стек пуст
	FINISH;
}

void test_lua_t::WorkCallSimple(const char* tm) const
{	START(tm);
		verify(LUA_TFUNCTION == lua_getglobal(L, "simple_func"));
		lua_pushinteger(L, 777);
		verify(LUA_OK == lua_pcall(L, 1, 1, 0));
		auto val = lua_tointeger(L, -1);
		assert(777 == val);
		lua_pop(L, 1);
		assert(0 == lua_gettop(L)); // Стек пуст
	FINISH;
}

void* test_lua_t::WorkBubbleSortArgs(const char* tm, bool descending) const
{	START(tm);
		// Создаем и заполняем таблицу с числами для сортировки
		lua_createtable(L, static_cast<unsigned>(std::size(sample_raw)), 0);
		for (int i = 1; i <= std::size(sample_raw); ++i)
		{	lua_pushnumber(L, i); // Ключ
			lua_pushnumber(L, sample_raw[i - 1]); // Значение
			lua_settable(L, -3);
		}
		assert(1 == lua_gettop(L)); // На стеке таблица
	FINISH;
	return nullptr;
}

void test_lua_t::WorkBubbleSortRun(const char* tm, void* py_args, bool descending) const
{	START(tm);
		// Помещаем функцию bubble_sort на вершину стека
		verify(LUA_TFUNCTION == lua_getglobal(L, "bubble_sort"));
		// перемещаем таблицу на вершину стека
		lua_insert(L, -2);
		if (descending)
		{	lua_pushcfunction(L, c_descending);
			assert(3 == lua_gettop(L)); // На стеке: ф-я и таблица и компаратор
			verify(LUA_OK == lua_pcall(L, 2, 1, 0));
		}
		else
		{	assert(2 == lua_gettop(L)); // На стеке: ф-я и таблица
			verify(LUA_OK == lua_pcall(L, 1, 1, 0));
		}
		assert(1 == lua_gettop(L)); // На стеке остался результат
	FINISH;

	// Проверяем результат сортировки:
	auto sample = descending? sample_descending_sorted: sample_ascending_sorted;
	lua_pushnil(L); // Первый ключ
	while (lua_next(L, -2) != 0)
	{	// Используем 'ключ' (по индексу -2) и 'значение' (по индексу -1)
		assert(lua_isinteger(L, -2));
		assert(lua_isnumber(L, -1));
		const auto i = lua_tointeger(L, -2);
		const auto v = lua_tointeger(L, -1);
		assert(sample[i - 1] == v);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	assert(0 == lua_gettop(L)); // Стек пуст
}
