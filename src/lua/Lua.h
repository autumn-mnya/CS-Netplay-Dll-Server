#pragma once

#include <lua.hpp>

extern lua_State* gL;

typedef struct FUNCTION_TABLE
{
	const char* name;
	lua_CFunction f;
} FUNCTION_TABLE;

void PushFunctionTable(lua_State* L, const char* name, const FUNCTION_TABLE* table, int length, int pop);
int InitNetScript();