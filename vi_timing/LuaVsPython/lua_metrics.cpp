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

#define YIELD(s) \
	std::this_thread::yield(); \
	for(int n = 0; n < 5; ++n) \
	{	VI_TM(s); \
	} \
	VI_TM_CLEAR(s)

#define START(s) \
	YIELD(" " s); \
	{	VI_TM(" " s)

#define START_F(s) \
	YIELD("*" s); \
	for (auto &ptr : arr) \
	{	VI_TM("*" s)

#define END } \
	do{} while(0)

namespace
{
	constexpr std::size_t CNT = 1'000;
	constexpr char sample[] = "global string";
}

void lua_test()
{	VI_TM_CLEAR(nullptr);

	lua_State* L{};
	std::array<lua_State*, CNT> arr;

	START("1 Initialize");
	END;

	START("2 dofile");
		L = luaL_newstate();
//		luaL_openlibs(L);
		verify(LUA_OK == luaL_dofile(L, "sample.lua"));
	END;

	START_F("2 dofile");
		ptr = luaL_newstate();
//		luaL_openlibs(ptr);
		verify(LUA_OK == luaL_dofile(ptr, "sample.lua"));
	END;

	START("3 Get string");
		verify(LUA_TSTRING == lua_getglobal(L, "str"));
		std::size_t len = 0;
		auto sz = lua_tolstring(L, -1, &len);
		assert(len == strlen(sz) && 0 == strcmp(sz, "global string"));
		lua_pop(L, 1);
	END;

	START_F("3 Get string");
		verify(LUA_TSTRING == lua_getglobal(ptr, "str"));
		std::size_t len = 0;
		auto sz = lua_tolstring(ptr, -1, &len);
		assert(sz && len == strlen(sz) && 0 == strcmp(sz, "global string"));
		lua_pop(ptr, 1);
	END;

	START("4 Call empty");
		verify(LUA_TFUNCTION == lua_getglobal(L, "empty_func"));
		verify(LUA_OK == lua_pcall(L, 0, 0, 0));
	END;

	START_F("4 Call empty");
		verify(LUA_TFUNCTION == lua_getglobal(ptr, "empty_func"));
		verify(LUA_OK == lua_pcall(ptr, 0, 0, 0));
	END;

	START("5 Call strlen");
		verify(LUA_TFUNCTION == lua_getglobal(L, "strlen_func"));
		verify(lua_pushstring(L, sample));
		verify(LUA_OK == lua_pcall(L, 1, 1, 0));
		verify(strlen(sample) == lua_tointeger(L, -1)); //-V2513
		lua_pop(L, 1);
	END;

	START_F("5 Call strlen");
		verify(LUA_TFUNCTION == lua_getglobal(ptr, "strlen_func"));
		verify(lua_pushstring(ptr, sample));
		verify(LUA_OK == lua_pcall(ptr, 1, 1, 0));
		verify(strlen(sample) == lua_tointeger(ptr, -1)); //-V2513
		lua_pop(ptr, 1);
	END;

	START("98 close");
		lua_close(L);
	END;

	START_F("98 close");
		lua_close(ptr);
	END;

	START("99 Finalize");
	END;

	std::cout << "Lua test result:\n";
	static constexpr auto flags = vi_tmSortByName | vi_tmSortAscending | vi_tmShowOverhead | vi_tmShowDuration | vi_tmShowUnit | vi_tmShowDiscreteness;
	VI_TM_REPORT(flags);
}
