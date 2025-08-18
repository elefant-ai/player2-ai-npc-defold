#pragma once

/**
 * Credits to https://stackoverflow.com/questions/31889691/read-lua-table-and-put-into-json-object-using-c
 */

#include <dmsdk/sdk.h>
#include <stdlib.h>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

namespace {
    void parseTable(lua_State* L, json& result);
    json parseLuaValue(lua_State* L);
    
    /* Find the size of the array on the top of the Lua stack
    * -1   object (not a pure array)
    * >=0  elements in array
    */
    int lua_array_length(lua_State *l)
    {
        double k;
        int max;
        int items;

        max = 0;
        items = 0;

        lua_pushnil(l);
        /* table, startkey */
        while (lua_next(l, -2) != 0) {
            /* table, key, value */
            if (lua_type(l, -2) == LUA_TNUMBER &&
                (k = lua_tonumber(l, -2))) {
                /* Integer >= 1 ? */
                if (floor(k) == k && k >= 1) {
                    if (k > max)
                        max = k;
                    items++;
                    lua_pop(l, 1);
                    continue;
                }
            }

            /* Must not be an array (non integer key) */
            lua_pop(l, 2);
            return -1;
        }

        return max;
    }

    /* json_append_array args:
 * - lua_State
 * - JSON strbuf
 * - Size of passwd Lua array (top of stack) */
// static void json_append_array(lua_State *l, json_config_t *cfg, strbuf_t *json,
//                               int array_length)
// {
//     int comma, i;

//     json_encode_descend(l, cfg);

//     strbuf_append_char(json, '[');

//     comma = 0;
//     for (i = 1; i <= array_length; i++) {
//         if (comma)
//             strbuf_append_char(json, ',');
//         else
//             comma = 1;

//         lua_rawgeti(l, -1, i);
//         json_append_data(l, cfg, json);
//         lua_pop(l, 1);
//     }

//     strbuf_append_char(json, ']');

//     cfg->current_depth--;
// }
    std::vector<json> parseArray(lua_State *L, int length) {
        std::vector<json> result;
        for (int i = 1; i <= length; i++) {
            lua_rawgeti(L, -1, i);
            auto value = parseLuaValue(L);
            result.push_back(value);
            lua_pop(L, 1);
        }
        return result;
    }

    json parseLuaValue(lua_State* L)
    {
        json result;

        auto type = lua_type(L, -1);
        if (type == LUA_TBOOLEAN) {
            result = lua_toboolean(L, -1) != 0;
        } else if (type == LUA_TNUMBER) {
            result = lua_tonumber(L, -1);
        } else if (type == LUA_TSTRING) {
            result = lua_tostring(L, -1);
        } else if (type == LUA_TTABLE) {
            int len = lua_array_length(L);
            if (len > 0)
                result = parseArray(L, len);
            else
                parseTable(L, result);
        };
        // lua_pop(L, 1);
        return result;
    }
    void parseTable(lua_State* L, json& result)
    {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            auto key = lua_tostring(L, -2);
            auto value = parseLuaValue(L);

            result[key] = value;
            lua_pop(L, 1);
        }
    }
}

json LuaTableToJson(lua_State *L) {
    json result;
    parseTable(L, result);
    return result;
}

// Read a json string as a table or a string
std::string ReadJsonString(lua_State *L, int index) {
    if (lua_istable(L, index))
    {
        // copy table to top temporarily and process it

        lua_pushvalue(L, index);
        json j = LuaTableToJson(L);
        std::string result = j.dump();
        lua_pop(L, 1);

        return result;
    } else if (lua_isstring(L, index)) {
        std::string result = luaL_checkstring(L, index);
        return result;
    }
    printf("Invalid argument passed, must be either a table or string.\n");
    throw "invalid argument passed, must be either a table or a string.";
}