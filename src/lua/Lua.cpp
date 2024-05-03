#include "Lua.h"
#include <lua.hpp>
#include "../Main.h"
#include "Lua_ENet.h"
#include "../ByteStream.h"

lua_State* gL;

void PushFunctionTable(lua_State* L, const char* name, const FUNCTION_TABLE* table, int length, int pop)
{
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_setfield(L, -3, name);

	for (int i = 0; i < length; ++i)
	{
		lua_pushcfunction(L, table[i].f);
		lua_setfield(L, -2, table[i].name);
	}

	if (pop)
		lua_pop(L, 1);
}

static int lua_SetNetVersion(lua_State* L)
{
	int ver = (int)luaL_checknumber(L, 1);

	gNetVersion = ver;

	return 0;
}

static int lua_GetNetVersion(lua_State* L)
{
	lua_pushnumber(L, gNetVersion);
	return 1;
}

static int lua_GetPacketCode(lua_State* L)
{
	// Get the event type from Lua stack
	lua_pushnumber(L, packetcode);
	return 1;
}

static int lua_GetModulePath(lua_State* L) {
	lua_pushstring(L, gModulePath);
	return 1;
}

static int lua_GetSpecialPacketCode(lua_State* L)
{
	// Get the event type from Lua stack
	lua_pushnumber(L, specialPacketCode);
	return 1;
}

static int lua_GetSpecialPacketData(lua_State* L) {
	if (specialData == nullptr)
		lua_pushstring(L, "No data");
	else
		lua_pushstring(L, specialData);

	return 1;
}

static int lua_GetUsername(lua_State* L) {
	lua_pushstring(L, lua_Username);
	return 1;
}

static int lua_GetPlayerNum(lua_State* L) {
	lua_pushnumber(L, lua_PlayerNum);
	return 1;
}



int InitNetScript()
{
	char scriptpath[MAX_PATH];
	char path[MAX_PATH];
	gL = luaL_newstate();

	sprintf(path, "%s\\main.lua", gScriptsPath);
	sprintf(scriptpath, "%s\\?.lua", gScriptsPath);

	luaL_openlibs(gL);

	lua_getglobal(gL, "package");
	lua_pushstring(gL, scriptpath);
	lua_setfield(gL, -2, "path");

	// Push functions before any netplay related ones here


	// Set the global to "CaveNet"
	lua_newtable(gL);
	lua_pushvalue(gL, -1);
	lua_setglobal(gL, "CaveNet");

	lua_pushcfunction(gL, lua_SetNetVersion);
	lua_setfield(gL, -2, "SetNetVersion");

	lua_pushcfunction(gL, lua_GetNetVersion);
	lua_setfield(gL, -2, "GetNetVersion");

	lua_pushcfunction(gL, lua_GetPacketCode);
	lua_setfield(gL, -2, "eventCode");

	lua_pushcfunction(gL, lua_GetModulePath);
	lua_setfield(gL, -2, "GetModulePath");

	lua_pushcfunction(gL, lua_GetSpecialPacketCode);
	lua_setfield(gL, -2, "specialEventCode");

	lua_pushcfunction(gL, lua_GetSpecialPacketData);
	lua_setfield(gL, -2, "GetSpecialPacketData");

	lua_pushcfunction(gL, lua_GetUsername);
	lua_setfield(gL, -2, "GetUsername");

	lua_pushcfunction(gL, lua_GetPlayerNum);
	lua_setfield(gL, -2, "GetPlayerID");

	PushFunctionTable(gL, "ENet", EnetFunctionTable, FUNCTION_TABLE_ENET_SIZE, TRUE);

	if (luaL_dofile(gL, path) != LUA_OK)
	{
		const char* error = lua_tostring(gL, -1);;
		printf("ERROR: %s\n", error);
		return BOOLFALSE;
	}

	return BOOLTRUE;
}