#pragma once

#define LIB_NAME "Player2AINPC"
#define MODULE_NAME "ainpc"
#define DLIB_LOG_DOMAIN "player2"

// TODO: try to add this in later cause why not
// #ifndef WIN32_LEAN_AND_MEAN
// #define WIN32_LEAN_AND_MEAN
// #endif

#include <dmsdk/sdk.h>
#include <stdlib.h>

// Lua callback config
struct LuaCallbackInfo
{
    LuaCallbackInfo() : m_L(0), m_Callback(LUA_NOREF), m_Self(LUA_NOREF) {}
    lua_State *m_L;
    int m_Callback;
    int m_Self;
};
