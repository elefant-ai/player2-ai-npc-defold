#pragma once

#include <string>
#include <dmsdk/sdk.h>

using namespace std;

struct ExtensionSettings {
    string project_key;
    string host;
    int port;
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

    g_ExtensionSettings.host = dmConfigFile::GetString(cfg, "player2_endpoints.host", "127.0.0.1");
    g_ExtensionSettings.port = dmConfigFile::GetInt(cfg, "player2_endpoints.port", 4315);

    // TODO: Fill in the others once this is done
}

