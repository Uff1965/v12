// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://www.lua.org/manual/5.4/

#include "header.h"
#include "lua_metrics.h"
#include "LuaVsPython.h"

#include <vi_timing/timing.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

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

extern "C" int l_cmp(lua_State * L)
{	const double l = lua_tonumber(L, 1);
	const double r = lua_tonumber(L, 2);
	lua_pushboolean(L, (l < r)); // result
	return 1; // number of results
}

namespace
{
	constexpr char sample[] = "global string";
}

void lua_test()
{	VI_TM_CLEAR(nullptr);

	START("  *** ALL ***");

		std::string text;
		START(" 0 Load text");
			std::ifstream file("sample.lua");
			assert(file);
			std::ostringstream ss;
			ss << file.rdbuf();
			text = ss.str();
		FINISH;

		lua_State* L{};

		START(" 1 Initialize");
			L = luaL_newstate();
			luaL_openlibs(L);
			assert(0 == lua_gettop(L)); // Стек пуст
		FINISH;

//			verify(LUA_OK == luaL_dofile(L, "sample.lua"));
		START(" 2.1 dofile (load+compile)");
//				verify(LUA_OK == luaL_loadfile(L, "sample.lua"));
			verify(LUA_OK == luaL_loadstring(L, text.c_str()));
			/* !!!NOTE!!!
			Чтобы выполнить скомпилированную строку Lua в разных lua_State,
			можно использовать функцию lua_dump для сохранения скомпилированного кода в байт-код,
			а затем загрузить и выполнить этот байт-код в других состояниях Lua с помощью lua_load*/
			assert(1 == lua_gettop(L)); // На стеке скомпилированный код
		FINISH;
		START(" 2.2 dofile (call)");
			verify(LUA_OK == lua_pcall(L, 0, 0, 0));
			assert(0 == lua_gettop(L)); // Стек пуст
		FINISH;

		START(" 3 Get string");
			verify(LUA_TSTRING == lua_getglobal(L, "global_string"));
			std::size_t len = 0;
			auto sz = lua_tolstring(L, -1, &len);
			assert(sz && len == strlen(sz) && 0 == strcmp(sz, "global string"));
			lua_pop(L, 1);
			assert(0 == lua_gettop(L)); // Стек пуст
		FINISH;

		START(" 4.1 Call empty");
			verify(LUA_TFUNCTION == lua_getglobal(L, "empty_func"));
			verify(LUA_OK == lua_pcall(L, 0, 0, 0));
			assert(0 == lua_gettop(L)); // Стек пуст
		FINISH;

		for (unsigned n = 0; n < CNT; ++n)
		{	VI_TM(" 4.2 Call empty (many times)");
			verify(LUA_TFUNCTION == lua_getglobal(L, "empty_func"));
			verify(LUA_OK == lua_pcall(L, 0, 0, 0));
			assert(0 == lua_gettop(L)); // Стек пуст
		}

		{
			START(" 5.1 Call bubble_sort (arg init)");
				// Создаем и заполняем таблицу с числами для сортировки
				lua_createtable(L, static_cast<unsigned>(std::size(sample_sorted)), 0);
				for (int i = 1; i <= std::size(sample_sorted); ++i) {
					lua_pushnumber(L, i); // Ключ
					lua_pushnumber(L, sample_raw[i - 1]); // Значение (5, 4, 3, 2, 1)
					lua_settable(L, -3);
				}
				assert(1 == lua_gettop(L)); // На стеке таблица
			FINISH;

			START(" 5.2 Call bubble_sort (call)");
				// Помещаем функцию bubble_sort на вершину стека
				verify(LUA_TFUNCTION == lua_getglobal(L, "bubble_sort"));
				// перемещаем таблицу на вершину стека
				lua_insert(L, -2);
				lua_pushcfunction(L, l_cmp);
				assert(3 == lua_gettop(L)); // На стеке: ф-я и таблица и компаратор
				verify(LUA_OK == lua_pcall(L, 2, 1, 0));
				assert(1 == lua_gettop(L)); // На вершине стека остался результат
			FINISH;

			lua_pushnil(L); // Первый ключ
			while (lua_next(L, -2) != 0)
			{	// Используем 'ключ' (по индексу -2) и 'значение' (по индексу -1)
				assert(lua_isinteger(L, -2));
				assert(lua_isnumber(L, -1));
				const auto i = lua_tointeger(L, -2);
				const auto v = lua_tointeger(L, -1);
				assert(sample_sorted[i - 1] == v);
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			assert(0 == lua_gettop(L)); // Стек пуст
		}

		START("98 close");
			lua_close(L);
		FINISH;

		START("99 Finalize");
		FINISH;
	FINISH;

	std::cout << "Lua test result:\n";
	VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
	endl(std::cout);
}
