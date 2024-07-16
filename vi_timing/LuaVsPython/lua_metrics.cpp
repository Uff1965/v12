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

void lua_test()
{	VI_TM_CLEAR(nullptr);

	START(" *** LUA ***   ");

		lua_State* L{};

		START("1 Initialize");
		END;

		START("2 dofile");
			L = luaL_newstate();
			luaL_openlibs(L);
			verify(LUA_OK == luaL_dofile(L, "sample.lua"));
		END;

		START("3 Get string");
			verify(LUA_TSTRING == lua_getglobal(L, "global_string"));
			std::size_t len = 0;
			auto sz = lua_tolstring(L, -1, &len);
			assert(sz && len == strlen(sz) && 0 == strcmp(sz, "global string"));
			lua_pop(L, 1);
		END;

		START("4 Call empty");
			verify(LUA_TFUNCTION == lua_getglobal(L, "empty_func"));
			verify(LUA_OK == lua_pcall(L, 0, 0, 0));
		END;

		START("5 Call strlen");
			verify(LUA_TFUNCTION == lua_getglobal(L, "strlen_func"));
			verify(lua_pushstring(L, sample));
			verify(LUA_OK == lua_pcall(L, 1, 1, 0));
			verify(strlen(sample) == lua_tointeger(L, -1)); //-V2513
			lua_pop(L, 1);
		END;

		{
			START("6 Call bubble_sort");
				// Создаем и заполняем таблицу с числами для сортировки
				lua_createtable(L, static_cast<unsigned>(std::size(sample_sorted)), 0);
				for (int i = 1; i <= std::size(sample_sorted); ++i) {
					lua_pushnumber(L, i); // Ключ
					lua_pushnumber(L, sample_raw[i - 1]); // Значение (5, 4, 3, 2, 1)
					lua_settable(L, -3);
				}

				// Помещаем функцию bubble_sort на вершину стека
				verify(LUA_TFUNCTION == lua_getglobal(L, "bubble_sort"));

				lua_pushvalue(L, -2);

				START("7 Call call(bubble_sort)");
					verify(LUA_OK == lua_pcall(L, 1, 0, 0));
				END;
				assert(1 == lua_gettop(L));
			END;

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
			assert(0 == lua_gettop(L));
		}

		START("98 close");
			lua_close(L);
		END;

		START("99 Finalize");
		END;
	END;

	std::cout << "Lua test result:\n";
	VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
}
