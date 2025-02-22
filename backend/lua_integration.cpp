#include "lua.hpp"
#include <iostream>
#include <string>
#include "trading_engine.h"

int lua_add_order(lua_State* L) {
    if (lua_gettop(L) < 5) {
        lua_pushstring(L, "Not enough arguments to add_order");
        lua_error(L);
        return 0;
    }
    int id = lua_tointeger(L, 1);
    const char* symbol = lua_tostring(L, 2);
    double price = lua_tonumber(L, 3);
    int quantity = lua_tointeger(L, 4);
    const char* side_str = lua_tostring(L, 5);
    char side = side_str[0];
    int order_type = 0;
    if (lua_gettop(L) >= 6) {
        order_type = lua_tointeger(L, 6);
    }
    cpp_add_order(id, std::string(symbol), price, quantity, side, order_type);
    return 0;
}

void registerLuaFunctions(lua_State* L) {
    lua_register(L, "add_order", lua_add_order);
}

int main(int argc, char** argv) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    registerLuaFunctions(L);
    if (luaL_dofile(L, "backtest_script.lua") != LUA_OK) {
        std::cerr << "Error running script: " << lua_tostring(L, -1) << std::endl;
    }
    lua_close(L);
    return 0;
}

