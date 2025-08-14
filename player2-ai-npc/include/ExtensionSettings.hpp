#pragma once

#include <string>
#include <dmsdk/sdk.h>

using namespace std;

struct ExtensionSettings {
    string project_key;
    string host;
    string chat_completion;
    string health;
    string characters;
    string npc_responses;
    string npc_spawn;
    string npc_chat;
    string npc_kill;
} g_ExtensionSettings;

void LoadExtensionSettings(dmExtension::AppParams* params) {
    dmConfigFile::HConfig cfg = dmEngine::GetConfigFile(params);

    // Use the project title as the key (keep it simple!)
    g_ExtensionSettings.project_key = dmConfigFile::GetString(cfg, "project.title", "undefined_defold_project_key");

    g_ExtensionSettings.host = dmConfigFile::GetString(cfg, "player2_endpoints.host", "http://localhost:4315");

    // chat
    g_ExtensionSettings.chat_completion = dmConfigFile::GetString(cfg, "player2_endpoints.chat_completion", "/v1/chat/completions");
    // npc
    g_ExtensionSettings.npc_spawn = dmConfigFile::GetString(cfg, "player2_endpoints.npc_spawn", "/v1/npc/games/{game_id}/npcs/spawn");
    g_ExtensionSettings.npc_chat = dmConfigFile::GetString(cfg, "player2_endpoints.npc_spawn", "/v1/npc/games/{game_id}/npcs/{npc_id}/chat");
    g_ExtensionSettings.npc_kill = dmConfigFile::GetString(cfg, "player2_endpoints.npc_spawn", "/v1/npc/games/{game_id}/npcs/{npc_id}/kill");
    g_ExtensionSettings.npc_responses = dmConfigFile::GetString(cfg, "player2_endpoints.npc_spawn", "/v1/npc/games/{game_id}/npcs/responses");

    // TODO: Fill in the others once this is done
}

