
#include <iostream>

#include "ExtensionSettings.hpp"
#include "Player2AINPC.hpp"
#include "CallbackQueue.hpp"
#include "HttpShorthands.hpp"
#include "LuaTableToJson.hpp"
#include "StringReplace.hpp"

#include "json.hpp"

#include <dmsdk/sdk.h>

using json = nlohmann::json;
// using namespace nlohmann::literals;

int PostAPICall(lua_State *L, const char *path, int offsetArgs = 0) {
    DM_LUA_STACK_CHECK(L, 0);

    // Read input as json string
    std::string json_string = ReadJsonString(L, offsetArgs + 1);

    auto callback = RegisterCallback(L, offsetArgs + 2);
    auto callback_fail = RegisterCallback(L, offsetArgs + 3);

    bool json_input = true; // All content is json yeah

    httplib::Headers headers;
    if (json_input) {
        httplib::Headers merge_headers = {
            { "Content-Type", "application/json" }
        };
        headers.insert(merge_headers.begin(), merge_headers.end());
    }

    PerformPost(path, json_string.c_str(), "application/json", headers, [callback, L] (std::string data, httplib::Headers headers) {
        // We got some data!
        InvokeCallback(L, callback, [data] (lua_State *L) {

            // printf("ASDF got em %s\n", data.c_str());

            bool json_reply = true; // All content is json yeah
            if (json_reply) {
                // TODO: convert data to string, then convert to lua table if json
                // TODO: Push the table
            }

            lua_createtable(L, 0, 0);
            lua_pushstring(L, data.c_str());
            lua_setfield(L, -2, "result");

            return 1;
        });
    }, [callback_fail, L](std::string data, int error_code) {
        InvokeCallback(L, callback_fail, [data, error_code] (lua_State *L) {

            lua_pushstring(L, data.c_str());
            lua_pushinteger(L, error_code);

            return 2;
        });
    });

    return 0;
}

// Post but allow for game_id as an optional param and replace game_id and npc_id in the query path
int PostAPICallFirstGameIdOrNpcId(lua_State *L, const char *path, bool gameid_only = false) {
    // Here's how it works:
    // game id first
    // npc id second
    // OR
    // npc id first
    // UNLESS gameid_only = true, where:
    // game id first
    int argOffset;
    std::string game_id = "{game_id}";
    std::string npc_id = "{npc_id}";
    bool first = lua_isstring(L, 1);
    bool second = lua_isstring(L, 2);

    if (gameid_only) {
        // OPTIONAL game id
        if (first) {
            argOffset = 1;
            game_id = luaL_checkstring(L, 1);
        } else {
            argOffset = 0;
            game_id = "DEFAULT";
        }
    } else {
        if (second) {
            // OPTIONAL game id present
            argOffset = 2;
            game_id = luaL_checkstring(L, 1);
            npc_id = luaL_checkstring(L, 2);
        } else {
            // MANDATORY npc id
            argOffset = 1;
            npc_id = luaL_checkstring(L, 1);
            game_id = "DEFAULT";
        }
    }

    // replace params
    std::string path_result = path;
    path_result = replace_all(path, "{game_id}", game_id);
    path_result = replace_all(path, "{npc_id}", npc_id);

    printf("CREATING NPC: %s with arg offsets: %d\n", path_result.c_str(), argOffset);

    return PostAPICall(L, path_result.c_str(), argOffset);
}

// chat_completion(table, function, function)
int FuncChatCompletion(lua_State* L) {
    return PostAPICall(L, g_ExtensionSettings.chat_completion.c_str());
}

// npc_spawn(game_id, table, function)
// npc_spawn(table, function)
int FuncNpcSpawn(lua_State* L) {
    return PostAPICallFirstGameIdOrNpcId(L, g_ExtensionSettings.npc_spawn.c_str(), true);
}
// npc_chat(game_id, npc_id, table, function)
// npc_chat(npc_id, table, function)
int FuncNpcChat(lua_State* L) {
    return PostAPICallFirstGameIdOrNpcId(L, g_ExtensionSettings.npc_chat.c_str());
}
// npc_kill(game_id, npc_id, table, function)
// npc_kill(npc_id, table, function)
int FuncNpcKill(lua_State* L) {
    return PostAPICallFirstGameIdOrNpcId(L, g_ExtensionSettings.npc_kill.c_str());
}

int FuncNpcResponses(lua_State* L) {
    // TODO: Write stream using example from before
    // TODO: test locally
    // TODO: Then, test spawn endpoint
    // TODO: Then, test responses endpoint after chat is called as well
    return 0;
}

// Functions exposed to Lua
const luaL_reg Module_methods[] =
{
    {"chat_completion", FuncChatCompletion},
    {"npc_spawn", FuncNpcSpawn},
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

    // Init other things
    InitQueue();

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
    // Flush/execute queue calls
    RunQueue();
    return dmExtension::RESULT_OK;

}


DM_DECLARE_EXTENSION(Player2AINPC, LIB_NAME, AppInitializePlayer2AINPC, AppFinalizePlayer2AINPC, InitializePlayer2AINPC, UpdatePlayer2AINPC, 0, FinalizePlayer2AINPC)