#include "lua.hpp"
#include <iostream>
#include "trading_engine.h"  // Use cpp_add_order from the trading engine

int lua_add_order(lua_State* L) {
    int id = lua_tointeger(L, 1);
    double price = lua_tonumber(L, 2);
    int quantity = lua_tointeger(L, 3);
    const char* side_str = lua_tostring(L, 4);
    char side = side_str[0];
    cpp_add_order(id, price, quantity, side);
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
