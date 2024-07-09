// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// https://www.lua.org/manual/5.4/

#include "header.h"
#include "lua_metrics.h"

#include <vi_timing/timing.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <array>
#include <cassert>
#include <iostream>
#include <string>
#include <thread>

using namespace std::string_literals;

#define YIELD(s) /*std::this_thread::yield(); VI_TM_CLEAR(s)*/

namespace
{
	constexpr std::size_t CNT = 1'000;
	constexpr char sample[] = "global string";
}

void lua_test()
{	VI_TM_CLEAR(nullptr);

	lua_State* L{};
	std::array<lua_State*, CNT> arr;

	YIELD("1 Initialize");
	{	VI_TM("1 Initialize");
	}

	YIELD("2 dofile");
	{	VI_TM("2 dofile");
		L = luaL_newstate();
//		luaL_openlibs(L);
		verify(LUA_OK == luaL_dofile(L, "sample.lua"));
	}

	YIELD("*2 dofile");
	for (auto &ptr : arr)
	{	VI_TM("*2 dofile");
		ptr = luaL_newstate();
//		luaL_openlibs(ptr);
		verify(LUA_OK == luaL_dofile(ptr, "sample.lua"));
	}

	YIELD("3 Get string");
	{	VI_TM("3 Get string");
		verify(LUA_TSTRING == lua_getglobal(L, "str"));
		std::size_t len = 0;
		auto sz = lua_tolstring(L, -1, &len);
		assert(len == strlen(sz) && 0 == strcmp(sz, "global string"));
		lua_pop(L, 1);
	}

	YIELD("*3 Get string");
	for (auto ptr : arr)
	{	VI_TM("*3 Get string");
		verify(LUA_TSTRING == lua_getglobal(ptr, "str"));
		std::size_t len = 0;
		auto sz = lua_tolstring(ptr, -1, &len);
		assert(sz && len == strlen(sz) && 0 == strcmp(sz, "global string"));
		lua_pop(ptr, 1);
	}

	YIELD("4 Call empty");
	{	VI_TM("4 Call empty");
		verify(LUA_TFUNCTION == lua_getglobal(L, "empty_func"));
		verify(LUA_OK == lua_pcall(L, 0, 0, 0));
	}

	YIELD("*4 Call empty");
	for (auto ptr : arr)
	{	VI_TM("*4 Call empty");
		verify(LUA_TFUNCTION == lua_getglobal(ptr, "empty_func"));
		verify(LUA_OK == lua_pcall(ptr, 0, 0, 0));
	}

	YIELD("5 Call strlen");
	{	VI_TM("5 Call strlen");
		verify(LUA_TFUNCTION == lua_getglobal(L, "strlen_func"));
		verify(lua_pushstring(L, sample));
		verify(LUA_OK == lua_pcall(L, 1, 1, 0));
		verify(strlen(sample) == lua_tointeger(L, -1)); //-V2513
		lua_pop(L, 1);
	}

	YIELD("*5 Call strlen");
	for (auto ptr: arr)
	{	VI_TM("*5 Call strlen");
		verify(LUA_TFUNCTION == lua_getglobal(ptr, "strlen_func"));
		verify(lua_pushstring(ptr, sample));
		verify(LUA_OK == lua_pcall(ptr, 1, 1, 0));
		verify(strlen(sample) == lua_tointeger(ptr, -1)); //-V2513
		lua_pop(ptr, 1);
	}

	YIELD("98 close");
	{	VI_TM("98 close");
		lua_close(L);
	}

	YIELD("*98 close");
	for (auto ptr : arr)
	{	VI_TM("*98 close");
		lua_close(ptr);
	}

	YIELD("99 Finalize");
	{	VI_TM("99 Finalize");
	}

	std::cout << "Lua test result:\n";
	static constexpr auto flags = vi_tmSortByName | vi_tmSortAscending | vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowDiscreteness;
	VI_TM_REPORT(flags);
}
