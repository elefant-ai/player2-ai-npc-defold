#pragma once

#include <string>
#include <dmsdk/sdk.h>
#include "CallbackQueue.hpp"

using namespace std;

struct ExtensionSettings {
    // general config
    string client_id;
    // host
    string host;
    // endpoints
    string chat_completion;
    string npc_responses;
    string npc_spawn;
    string npc_chat;
    string npc_kill;
    string health;
    // string characters;
    string auth_start;
    string auth_poll;

    // Auth callback
    CallbackId auth_callback = INVALID_CALLBACK;
} g_ExtensionSettings;

void LoadExtensionSettings(dmExtension::AppParams* params) {
    dmConfigFile::HConfig cfg = dmEngine::GetConfigFile(params);

    // client_id needs to be set manually via function

    g_ExtensionSettings.host = dmConfigFile::GetString(cfg, "player2_endpoints.host", "https://api.player2.game");

    // health
    g_ExtensionSettings.health = dmConfigFile::GetString(cfg, "player2_endpoints.health", "/v1/health");
    // chat
    g_ExtensionSettings.chat_completion = dmConfigFile::GetString(cfg, "player2_endpoints.chat_completion", "/v1/chat/completions");
    // npc
    g_ExtensionSettings.npc_spawn = dmConfigFile::GetString(cfg, "player2_endpoints.npc_spawn", "/v1/npc/games/{game_id}/npcs/spawn");
    g_ExtensionSettings.npc_chat = dmConfigFile::GetString(cfg, "player2_endpoints.npc_spawn", "/v1/npc/games/{game_id}/npcs/{npc_id}/chat");
    g_ExtensionSettings.npc_kill = dmConfigFile::GetString(cfg, "player2_endpoints.npc_spawn", "/v1/npc/games/{game_id}/npcs/{npc_id}/kill");
    g_ExtensionSettings.npc_responses = dmConfigFile::GetString(cfg, "player2_endpoints.npc_spawn", "/v1/npc/games/{game_id}/npcs/responses");
    // auth
    g_ExtensionSettings.auth_start = dmConfigFile::GetString(cfg, "player2_endpoints.auth_start", "/v1/login/device/new");
    g_ExtensionSettings.auth_poll = dmConfigFile::GetString(cfg, "player2_endpoints.auth_poll", "/v1/login/device/token");

    // TODO: Fill in:
    // characters

    // temporary override for testing
    // printf("OVERRIDE host + some endpoints. DELETE EM!\n");
    // g_ExtensionSettings.host = "http://localhost:3000";
    // g_ExtensionSettings.npc_responses = "/stream";
}

