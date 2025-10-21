#pragma once

// httplib allow for https
// I hate how defold does not print exceptions properly!
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
#include <unordered_map>
#include <queue>
#include <chrono>
#include <thread>

#include "httplib.h"
#include "json.hpp"
#include "ExtensionSettings.hpp"
#include <dmsdk/sdk.h>
#include "OpenUrl.hpp"

enum RequestMethod {
    GET, POST
};

typedef std::function<void(std::string data, httplib::Headers headers)> ReceiveFunction;
typedef std::function<void(std::string data)> ReceiveStreamFunction;
typedef std::function<void(std::string data, int code)> FailFunction;
typedef std::function<void(httplib::Headers headers)> FinishFunction;

namespace {

    struct AuthCallback {
        std::function<void()> on_success;
        FailFunction on_fail;
    };
    struct AuthPollLoop {
        bool running;
        std::thread *thread;
    };

    // auth (queue and key)
    // TODO: mutex?
    std::string auth_key;
    bool auth_is_calling;
    std::queue<AuthCallback> auth_callback_queue;
    AuthPollLoop auth_poll_loop;

    httplib::Headers GenerateHeaders(const httplib::Headers &headers) {

        httplib::Headers headers_to_send = httplib::Headers();
        headers_to_send.insert(headers.begin(), headers.end());

        if (auth_key.size() != 0) {
            httplib::Headers merge_headers = {
                { "Authorization", "Bearer " + auth_key }
            };
            headers_to_send.insert(merge_headers.begin(), merge_headers.end());
        }
        return headers_to_send;
    }

    httplib::Client MakeClient() {
        std::string host = g_ExtensionSettings.host;

        // printf("HOST pre %s\n", host.c_str());
        httplib::Client cli(host);
        // printf("HOST POST\n");

        return cli;
    }

    bool ResSucceeded(const httplib::Result &res) {
        return res && res->status / 100 == 2;
    }

    void CallAuthQueue() {
        while (!auth_callback_queue.empty()) {
            auto c = auth_callback_queue.front();
            if (c.on_success) {
                c.on_success();
            }
            auth_callback_queue.pop();
        }
    }
    void FailAuthQueue(std::string data, int code) {
        printf("asdf FAIL 0\n");
        while (!auth_callback_queue.empty()) {
            printf("asdf FAIL 1\n");
            auto c = auth_callback_queue.front();
            printf("asdf FAIL 2\n");
            if (c.on_fail) {
                c.on_fail(data, code);
            }
            printf("asdf FAIL 3\n");
            auth_callback_queue.pop();
            printf("asdf FAIL 4\n");
        }
        printf("asdf finished FAIL\n");
    }

    void Receive(const httplib::Result &res, ReceiveFunction onReceive, FailFunction onFail) {
        if (!res) {
            // printf("Connection failed, socket did not succeed probably. Using Host = %s.\n", g_ExtensionSettings.host.c_str());
            if (onFail) {
                onFail("No connection made, socket did not succeed probably. Using Host = " + g_ExtensionSettings.host + ".\n", -1);
            }
            return;
        }
        bool succeed = ResSucceeded(res);
        if (succeed) {
            if (onReceive) {
                onReceive(res->body, res->headers);
            }
        } else {
            if (onFail) {
                onFail(res ? res->body : "Failed to open stream", res ? res->status : -1);
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

    void _RunAuthPollLoop(std::string client_id, std::string device_code, std::string grant_type, double interval, double expires_in) {
        // Hit g_ExtensionSettings.auth_poll at interval

        std::thread *old_auth_loop_thread = auth_poll_loop.thread;

        auth_poll_loop.thread = new std::thread([old_auth_loop_thread, client_id, device_code, grant_type, interval, expires_in]() {
            // If previous auth loop is active, CANCEL previous auth loop and WAIT
            // TODO: Could result in some extra waiting but whatever
            if (old_auth_loop_thread) {
                auth_poll_loop.running = false;
                old_auth_loop_thread->join();
            }

            // http
            auto cli = MakeClient();
            std::string poll_path = g_ExtensionSettings.auth_poll;
            // body
            unordered_map<string, string> body;
            body["client_id"] = g_ExtensionSettings.client_id;
            nlohmann::json body_json = body;
            std::string body_json_string = body_json.dump();
            // headers
            httplib::Headers headers= {
                { "Content-Type", "application/json" }
            };

            // Start running!
            auth_poll_loop.running = true;
            while (auth_poll_loop.running) {

                // Get results
                Receive(cli.Post(poll_path.c_str(), headers, body_json_string, "application/json"),
                    [](std::string data, httplib::Headers headers) {
                        // Success! We got the key.
                        auto body = nlohmann::json::parse(data);
                        std::string p2_key = body["p2Key"];
                        auth_key = p2_key;
                        CallAuthQueue();
                        auth_poll_loop.running = false;
                    }, [](std::string data, int code) {
                        // Request failed, keep going if we get 400 otherwise we STOP
                        if (code != 400) {
                            // We failed for real! Nothing we can do.
                            FailAuthQueue(data, code);

                            auth_poll_loop.running = false;
                        }
                    }
                );

                // Wait
                if (auth_poll_loop.running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds((long)(interval * 1000)));
                }
            }
        });
    }

    void _RunAuthStart() {
        printf("AUTH START\n");
        // body
        unordered_map<string, string> body;
        body["client_id"] = g_ExtensionSettings.client_id;
        nlohmann::json body_json = body;
        std::string body_json_string = body_json.dump();

        // headers
        httplib::Headers headers= {
            { "Content-Type", "application/json" }
        };

        DoPlayer2Request(g_ExtensionSettings.auth_start.c_str(), RequestMethod::POST, body_json_string.c_str(), "application/json", headers,
            [](std::string data, httplib::Headers headers) {
                printf("AUTH SUCCESS\n");
                // Success! Start procedure.
                auto body = nlohmann::json::parse(data);
                std::string verification_url = body["verificationUriComplete"];
                std::string device_code = body["deviceCode"];
                // std::string user_code = body["userCode"]; // unused
                double expires_in = body["expiresIn"];
                double interval = body["interval"];

                if (g_ExtensionSettings.auth_callback != INVALID_CALLBACK) {
                    // TODO: Invoke g_ExtensionSettings.auth_callback with arguments:
                    // verification_url
                    throw "You heard the man";
                } else {
                    // Just Open the URL by default
                    invoke_browser(verification_url);
                }

                // Run the auth loop, assuming that things are opened!
                _RunAuthPollLoop(g_ExtensionSettings.client_id, device_code, "urn:ietf:params:oauth:grant-type:device_code", interval, expires_in);
            },
            [](std::string data, int code) {
                // We failed! Nothing we can do.
                FailAuthQueue(data, code);
            }
        );
    }

    void PerformWithAuth(AuthCallback callback) {
        auth_callback_queue.push(callback);
        printf("asdf 8\n");
        /// We have auth? just call.
        if (auth_key.size() != 0) {
            printf("asdf BAD 1\n");
            CallAuthQueue();
            return;
        }
        // If auth is calling, add to queue and continue.
        if (auth_is_calling) {
            return;
        }

        // We have started our auth process!
        auth_is_calling = true;

        // client_id must be populated
        if (g_ExtensionSettings.client_id.empty()) {
            printf("asdf BAD? 2\n");
            FailAuthQueue("No client_id is set.\nPlease find your client_id from https://player2.game/profile/developer and then call ainpc.init(client_id) with your project client_id.", -1);
            return;
        }
        printf("asdf 9\n");

        _RunAuthStart();

        // TODO: Step 1: POST g_ExtensionSettings.auth_start
        // TODO: - On success: From the dictionary, grab verification_url, deviceCode, userCode, expiresIn, interval 
        // TODO: Call a global "open_auth_prompt" function? Set it to some kind of default here where an asset is loaded
        // TODO: Cancel auth function (just a global flag here) that FAILS auth and CLOSES the auth prompt window (part of a function passed to open_auth_prompt?)
        // TODO: Hit g_ExtensionSettings.auth_poll at interval interval (threads ahoy)
        //          - If it has a player2 key, finish (set key and CallAuthQueue)
    }

    void DoPlayer2RequestAuth(const char *path, RequestMethod requestMethod, const char *body, const char *content_type, const httplib::Headers &headers, ReceiveFunction onReceive, FailFunction onFail) {
        printf("asdf 5\n");
        std::string p = path;
        std::string b = body ? body : "";
        std::string c_type = content_type ? content_type : "";
        printf("asdf 6\n");

        AuthCallback c;
        c.on_success = [=]() {
            DoPlayer2Request(p.c_str(), requestMethod, b.c_str(), c_type.c_str(), headers, onReceive, onFail);
        };
        c.on_fail = onFail;

        printf("asdf 7\n");
        PerformWithAuth(c);
    }
}

void PerformGet(const char *path, const httplib::Headers &headers, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {
    printf("asdf 4\n");
    DoPlayer2Request(path, GET, 0, 0, headers, onReceive, onFail);
}
void PerformGet(const char *path, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {
    PerformGet(path, httplib::Headers(), onReceive, onFail);
}
void PerformPost(const char *path, const char *body, const char *content_type, const httplib::Headers &headers, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {
    DoPlayer2RequestAuth(path, POST, body, content_type, headers, onReceive, onFail);
}
void PerformPost(const char *path, const char *body, const char *content_type, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {
    PerformPost(path, body, content_type, httplib::Headers(), onReceive, onFail);
}
void PerformPost(const char *path, const nlohmann::json &json, ReceiveFunction onReceive = 0, FailFunction onFail = 0) {

    std::string body = json.dump();

    char *content_type = "application/json";

    PerformPost(path, body.c_str(), content_type, onReceive, onFail);
}

// http get stream
typedef unsigned long StreamId;
namespace {
    struct StreamContainer {
        std::thread *thread;
        bool running;
    };

    dmMutex::HMutex stream_mutex;
    std::unordered_map<StreamId, StreamContainer*> streams;
    StreamId stream_id_counter;    
}

StreamId _PerformGetStream(const char *path, const httplib::Headers &headers, bool loop, ReceiveStreamFunction onReceive, FinishFunction onFinish = 0, FailFunction onFail = 0) {
    DM_MUTEX_SCOPED_LOCK(stream_mutex);

    StreamContainer *container = new StreamContainer();
    container->running = true;

    StreamId id = ++stream_id_counter;

    streams[id] = container;

    std::string path_string(path);

    container->thread = new std::thread([id, container, loop, headers, path_string, onReceive, onFinish, onFail] (){

        httplib::Client cli = MakeClient();

        httplib::Headers headers_to_send = GenerateHeaders(headers);

        do {
            bool running = true;
            auto res = cli.Get(path_string, headers_to_send, [container, onReceive](const char *data, size_t data_length) {
                std::string body(data, data_length);
                onReceive(body);
                return container->running;
            });
            bool res_empty = !res;
            httplib::Headers headers = res_empty ? httplib::Headers() : res->headers;
            Receive(res, [onFinish](std::string data, httplib::Headers headers) {
                if (onFinish) {
                    onFinish(headers);
                }
            }, [onFail, onFinish, res_empty, headers, container](std::string data, int err) {
                // we may also cancel if our result is no longer present and we are no longer running
                if (res_empty && !container->running) {
                    if (onFinish) {
                        onFinish(headers);
                    }
                } else {
                    if (onFail) {
                        onFail(data + " (response could have cut out mid stream!)", err);                        
                    }
                    if (onFinish) {
                        onFinish(headers);
                    }
                }
            } );
        } while (container->running && loop);

        // When thread finishes, DETATCH the thread, FREE the stream container and REMOVE stream container from map
        container->thread->detach();
        DM_MUTEX_SCOPED_LOCK(stream_mutex);
        streams.erase(id);
        delete container;
    });

    return id;
}

StreamId PerformGetStream(const char *path, const httplib::Headers &headers, bool loop, ReceiveStreamFunction onReceive, FinishFunction onFinish = 0, FailFunction onFail = 0) {
    std::string p = path;

    AuthCallback c;
    c.on_success = [=]() {
        _PerformGetStream(path, headers, loop, onReceive, onFinish, onFail);
    };
    c.on_fail = onFail;

    PerformWithAuth(c);
}

StreamId PerformGetStream(const char *path, bool loop, ReceiveStreamFunction onReceive, FinishFunction onFinish = 0, FailFunction onFail = 0) {
    return PerformGetStream(path, httplib::Headers(), loop, onReceive, onFinish, onFail);
}
StreamId PerformGetStream(const char *path, ReceiveStreamFunction onReceive, FinishFunction onFinish = 0, FailFunction onFail = 0) {
    return PerformGetStream(path, false, onReceive, onFinish, onFail);
}


void StopGetStream(StreamId id) {
    DM_MUTEX_SCOPED_LOCK(stream_mutex);
    if (streams.find(id) != streams.end()) {
        // should be all we need, the thread will finish on its own
        streams[id]->running = false;
        // streams[id]->thread->detach();
    }

}

// Lua/Defold Init
void InitHttpShorthands() {
    stream_mutex = dmMutex::New();
}