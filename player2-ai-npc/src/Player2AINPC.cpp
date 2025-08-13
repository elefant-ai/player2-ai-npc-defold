
#include <iostream>

#include "ExtensionSettings.hpp"
#include "Player2AINPC.hpp"
#include "HttpShorthands.hpp"
#include <dmsdk/sdk.h>



int FuncChatCompletion(lua_State* L) {
    DM_LUA_STACK_CHECK(L, 0);

    // TODO: input table
    // TODO: Make a local interface for endpoints
    DoPlayer2Request("asdf");

    return 0;
}



// Functions exposed to Lua
const luaL_reg Module_methods[] =
{
    {"chat_completion", FuncChatCompletion},
    {0, 0}
};

void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializePlayer2AINPC(dmExtension::AppParams* params)
{
    LoadExtensionSettings(params);

    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializePlayer2AINPC(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);

    printf("Registered %s Extension\n", MODULE_NAME);

    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizePlayer2AINPC(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizePlayer2AINPC(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result UpdatePlayer2AINPC(dmExtension::Params *params)
{
    // TODO: flush queues
    return dmExtension::RESULT_OK;

}


DM_DECLARE_EXTENSION(Player2AINPC, LIB_NAME, AppInitializePlayer2AINPC, AppFinalizePlayer2AINPC, InitializePlayer2AINPC, UpdatePlayer2AINPC, 0, FinalizePlayer2AINPC)