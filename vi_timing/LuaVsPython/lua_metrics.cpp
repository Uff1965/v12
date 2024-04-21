// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"
#include "lua_metrics.h"

#include <vi_timing/timing.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <string>

#define TM(s) VI_TM(((s) + suffix).c_str())

namespace
{
	using namespace std::string_literals;

	void test(const std::string &suffix)
	{
		static constexpr char sample[] = "global string";

		TM("Test");

		lua_State* L = nullptr;
		{	TM("Initialize");
			L = luaL_newstate();
			assert(L);

			luaL_openlibs(L);
		}

		auto top = lua_gettop(L);

		{	TM("Loadfile");
			verify(LUA_OK == luaL_loadfile(L, "sample.lua"));
			verify(LUA_OK == lua_pcall(L, 0, 0, 0));
		}
		assert(lua_gettop(L) == top);

		{	const char* sz{};
			std::size_t len = 0;
			TM("Get string");
			verify(LUA_TSTRING == lua_getglobal(L, "str"));
			sz = lua_tolstring(L, -1, &len);
			assert(len == strlen(sz) && 0 == strcmp(sz, "global string"));
			lua_pop(L, 1);
		}
		assert(lua_gettop(L) == top);

		{	TM("Call empty");
			verify(LUA_TFUNCTION == lua_getglobal(L, "empty_func"));
			verify(LUA_OK == lua_pcall(L, 0, 0, 0));
		}
		assert(lua_gettop(L) == top);

		{
			TM("Call strlen");
			verify(LUA_TFUNCTION == lua_getglobal(L, "strlen_func"));
			verify(lua_pushstring(L, sample));
			verify(LUA_OK == lua_pcall(L, 1, 1, 0));
			verify(strlen(sample) == lua_tointeger(L, -1)); //-V2513
			lua_pop(L, 1);
		}
		assert(lua_gettop(L) == top);

		{
			TM("Close");
			lua_close(L);
		}
	}
} // namespace {

void lua_test()
{
	test(" (lua)"s);
	test(" bis (lua)"s);
}
