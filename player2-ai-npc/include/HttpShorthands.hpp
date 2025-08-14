#pragma once

#include <iostream>
#include <string>

#include "httplib.h"
#include "json.hpp"
#include "ExtensionSettings.hpp"
#include <dmsdk/sdk.h>


// TODO: Set to true once its verified
#define COMPRESS_BODY false

enum RequestMethod {
    GET, POST
};

typedef std::function<void(std::string data, httplib::Headers headers)> ReceiveFunction;
typedef std::function<void(std::string data, int code)> FailFunction;
typedef std::function<void()> FinishFunction;

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

        httplib::Client cli(host);
        // printf("override client TODO make it better\n");
        // httplib::Client cli("http://localhost:3000");
        // httplib::Client cli("127.0.0.1", 3000);

        // printf("CLIENT: %s\n", cli.);

        return cli;
    }

    void Receive(const httplib::Result &res, ReceiveFunction onReceive, FailFunction onFail) {
        if (!res) {
            printf("No connection made, socket did not succeed probably. Using Host = %s.\n", g_ExtensionSettings.host.c_str());
            if (onFail) {
                onFail("No connection made, socket did not succeed probably. Using Host = " + g_ExtensionSettings.host + ".\n", -1);
            }
            return;
        }
        bool succeed = res->status / 100 == 2;
        if (succeed) {
            if (onReceive) {
                onReceive(res->body, res->headers);
            }
        } else {
            if (onFail) {
                onFail(res->body, res->status);
            }
        }
    }

    void DoPlayer2Request(const char *path, RequestMethod requestMethod, const char *body, const char *content_type, const httplib::Headers &headers, ReceiveFunction onReceive, FailFunction onFail) {

        httplib::Headers headers_to_send = GenerateHeaders(headers);

        // request in a separate thread
        new std::thread([](std::string path, RequestMethod requestMethod, std::string body, std::string content_type, httplib::Headers headers, ReceiveFunction onReceive, FailFunction onFail) {
            httplib::Client cli = MakeClient();

            switch (requestMethod) {
                case GET: {
                    Receive(cli.Get(path, headers), onReceive, onFail);
                    break;
                }
                case POST: {
                    Receive(cli.Post(path, headers, body, content_type), onReceive, onFail);
                    break;
                }
                default:
                    throw "Invalid/Not implemented request method: " + requestMethod;
            }
        }, std::string(path), requestMethod, std::string(body ? body : ""), std::string(content_type ? content_type : ""), headers_to_send, onReceive, onFail);

    }
}

void PerformGet(const char *path, const httplib::Headers &headers, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {
    DoPlayer2Request(path, GET, 0, 0, headers, onReceive, onFail);
}
void PerformGet(const char *path, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {
    PerformGet(path, httplib::Headers(), onReceive, onFail);
}
void PerformPost(const char *path, const char *body, const char *content_type, const httplib::Headers &headers, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {
    DoPlayer2Request(path, POST, body, content_type, headers, onReceive, onFail);
}
void PerformPost(const char *path, const char *body, const char *content_type, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {
    PerformPost(path, body, content_type, httplib::Headers(), onReceive, onFail);
}
void PerformPost(const char *path, const nlohmann::json &json, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {

    std::string body = json.dump();

    char *content_type = "application/json";

    PerformPost(path, body.c_str(), content_type, onReceive, onFail);
}

void PerformGetStream(const httplib::Headers &headers, bool loop, ReceiveFunction onReceive, FinishFunction onFinish = 0, FailFunction onFail = 0) {

    httplib::Client cli = MakeClient();

    httplib::Headers headers_to_send = GenerateHeaders(headers);

    throw "unimplemented";
    // TODO: Reference http stream like before
}
