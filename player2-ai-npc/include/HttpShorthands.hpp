#pragma once

#include "httplib.h"
#include "ExtensionSettings.hpp"
#include <dmsdk/sdk.h>

#include <iostream>
#include <string>

enum RequestMethod {
    GET, POST
};

namespace {
    httplib::Headers GenerateHeaders(const httplib::Headers &headers) {
        std::string project_key = g_ExtensionSettings.project_key;

        httplib::Headers merge_headers = {
            { "player2-game-key", project_key.c_str() }
        };
        httplib::Headers headers_to_send = merge_headers;
        headers_to_send.insert(headers.begin(), headers.end());

        return headers_to_send;
    }

    httplib::Client MakeClient() {
         std::string host = g_ExtensionSettings.host;
        int port = g_ExtensionSettings.port;

        httplib::Client cli(host.c_str(), port);

        return cli;
    }
}


void DoPlayer2Request(const char *path, RequestMethod requestMethod, const httplib::Headers &headers, TODO SUCCEED FUNCTION, TODO FUNCTION FAIL) {
    httplib::Client cli = MakeClient();

    httplib::Headers headers_to_send = GenerateHeaders(headers);

    switch (requestMethod) {
        case GET:
            cli.Get(path, headers_to_send, TODO SUCCEED FUNCTION);
            break;
        case POST:
            cli.Post(path, headers_to_send, TODO SUCCEED FUNCTION);
            break;
        default:
            throw "Invalid request method: " + requestMethod;
    }
}

void PerformGetStream(const httplib::Headers &headers, bool loop, TODO ON DATA, TODO ON FINISH, TODO ON FAIL) {

    httplib::Client cli = MakeClient();

    httplib::Headers headers_to_send = GenerateHeaders(headers);

    // TODO: Reference http stream like before
}

