// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi_timing/timing.h>
#include "LuaVsPython.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern "C" int l_cmp(lua_State* L)
{	const double l = lua_tonumber(L, 1);
	const double r = lua_tonumber(L, 2);
	lua_pushboolean(L, (l < r)); // result
	return 1; // number of results
}

void lua_test()
{	VI_TM_CLEAR(nullptr);

	{	VI_TM(" *** ALL ***");

		lua_State* L = nullptr;
		{	VI_TM("1. Initialize");
			L = luaL_newstate(); // Создаём новое Lua-состояние
			luaL_openlibs(L); // Загружаем стандартные библиотеки
		}

		std::string text;
		{	VI_TM("2. Load text from file");

			std::ifstream file("sample.lua");
			std::ostringstream ss;
			ss << file.rdbuf();
			text = ss.str();
		}

		{	VI_TM("3. Compile");
			luaL_loadstring(L, text.c_str()); // Компилируем скрипт
			// Теперь на вершине стека лежит скомпилированный скрипт.
			// Его дамп можно сохранить и использовать много раз сэкономив ресурсы на загрузке и компиляции.
			// Но мы этого делать не будем.
		}

		{	VI_TM("4. Execution");
			lua_pcall(L, 0, 0, 0); // Выполняем скрипт
			// В случае ошибки на вершине стека будет лежать сообщение об ошибке
			// Иначе скрипт будет удалён со стека
		}

		{	VI_TM("5.1 Get string");
			lua_getglobal(L, "global_string"); // Получаем значение глобальной переменной
			std::size_t len = 0;
			auto sz = lua_tolstring(L, -1, &len); // Получаем указатель на строку и её длину
			lua_pop(L, 1); // Удаляем строку со стека
		}

		{	VI_TM("5.2 Call empty");
			lua_getglobal(L, "empty_func"); // Получаем функцию по имени
			lua_pcall(L, 0, 0, 0); // Выполняем функцию. При этом она удаляется со стека.
		}

		// несколько раз повтрояем вызов функции и убеждаемся, что время выполнения уменьшается
		{	VI_TM("5.3 Call empty");
			lua_getglobal(L, "empty_func");
			lua_pcall(L, 0, 0, 0);
		}

		{	VI_TM("5.4 Call empty");
			lua_getglobal(L, "empty_func");
			lua_pcall(L, 0, 0, 0);
		}

		// 1000 раз вызываем функцию и получаем установившееся время выполнения
		for (unsigned n = 0; n < 1'000; ++n)
		{	VI_TM("5.5 Call empty (1000 times)");
			lua_getglobal(L, "empty_func");
			lua_pcall(L, 0, 0, 0);
		}

		{	VI_TM("5.6 bubble_sort (arg init)");
			// Копируем неотсортированный массив в Lua-таблицу.
			lua_createtable(L, static_cast<unsigned>(std::size(sample_raw)), 0);
			for (int i = 1; i <= std::size(sample_raw); ++i) {
				lua_pushnumber(L, i); // Ключ
				lua_pushnumber(L, sample_raw[i - 1]); // Значение
				lua_settable(L, -3);
			}
			// На стеке теперь таблица
		}

		{	VI_TM("5.7 bubble_sort");

			lua_getglobal(L, "bubble_sort");
			lua_insert(L, -2); // Перемещаем таблицу на второе место
			lua_pushcfunction(L, l_cmp); // Добавляем компаратор
			lua_pcall(L, 2, 1, 0); // Вызываем функцию с двумя аргументами и одним результатом
			// Аргументы удалены. На стеке теперь результат, тоже таблица.
		}

		{	// читаем отсортированный массив.
			lua_pushnil(L); // Первый ключ
			while (lua_next(L, -2) != 0)
			{	const auto idx = lua_tointeger(L, -2);
				const auto val = lua_tointeger(L, -1);
				lua_pop(L, 1);
				assert(sample_sorted[idx - 1] == val);
			}
			lua_pop(L, 1); // Удаляем таблицу
		}

		{	VI_TM("6 close");
			lua_close(L); // Закрываем Lua-состояние
		}

		{	VI_TM("7 Finalize");
			// В Lua не нужно выполнять финализацию библиотек.
		}
	}

	std::cout << "Lua test result:\n";
	VI_TM_REPORT(vi_tmSortByName | vi_tmSortAscending);
	endl(std::cout);
}

struct impl_test_t: test_t
{
	void test() const override { lua_test(); }
	std::string name() const override { return "LUA slim"; }
	inline static auto _ = (registrar(std::make_unique<impl_test_t>()), 0);
};
