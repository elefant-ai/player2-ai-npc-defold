#pragma once

#include <functional>
#include <unordered_map>

#include <dmsdk/sdk.h>
#include <stdlib.h>

typedef std::function<int(lua_State *L)> PopulateCallbackArgsFunction;

typedef unsigned long CallbackId;

const CallbackId INVALID_CALLBACK = 0;

namespace {
    // Internal stuff

    // Lua callback config
    struct LuaCallbackInfo
    {
        LuaCallbackInfo() : m_L(0), m_Callback(LUA_NOREF), m_Self(LUA_NOREF) {}
        lua_State *m_L;
        int m_Callback;
        int m_Self;
    };

    struct CallbackSubscription {
        // dmMutex::HMutex call_mutex;
        LuaCallbackInfo callback;

        CallbackSubscription()
        {
            callback.m_Callback = LUA_NOREF;
            callback.m_Self = LUA_NOREF;
            // call_mutex = dmMutex::New();
        }
    };

    void RegisterCallback(lua_State *L, int index, LuaCallbackInfo *cbk)
    {
        if (cbk->m_Callback != LUA_NOREF)
        {
            dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
            dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
        }

        cbk->m_L = dmScript::GetMainThread(L);
        luaL_checktype(L, index, LUA_TFUNCTION);

        lua_pushvalue(L, index);
        cbk->m_Callback = dmScript::Ref(L, LUA_REGISTRYINDEX);

        dmScript::GetInstance(L);
        cbk->m_Self = dmScript::Ref(L, LUA_REGISTRYINDEX);
    }
    void UnregisterCallback(LuaCallbackInfo *cbk)
    {
        if (cbk->m_Callback != LUA_NOREF)
        {
            dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
            dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
            cbk->m_Callback = LUA_NOREF;
        }
    }

    void InvokeCallback(LuaCallbackInfo *cbk, PopulateCallbackArgsFunction populateCallbackArgs) {
        if (cbk->m_Callback == LUA_NOREF)
        {
            printf("empty!!");
            return;
        }

        lua_State *L = cbk->m_L;
        DM_LUA_STACK_CHECK(L, 0);

        lua_rawgeti(L, LUA_REGISTRYINDEX, cbk->m_Callback);
        lua_rawgeti(L, LUA_REGISTRYINDEX, cbk->m_Self);
        lua_pushvalue(L, -1);

        dmScript::SetInstance(L);


        // arg(s) + 1
        int number_of_arguments = populateCallbackArgs(L) + 1;

        // example of what populateResult could look like:
        // int populateResult(lua_State *L) {
        // lua_createtable(L, 0, 0); // Main Table
        //     lua_pushstring(L, cmd->message.c_str());
        //     lua_setfield(L, -2, "result");
        //     return 1;    
        // }

        int ret = lua_pcall(L, number_of_arguments, 0, 0);
        if (ret != 0)
        {
            dmLogError("Error running callback: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

    // Subscriptions
    dmMutex::HMutex subscriptions_mutex;
    std::unordered_map<CallbackId, CallbackSubscription*> subscriptions;
    CallbackId subscription_id_counter = 1;

    // Invoke
    dmMutex::HMutex command_queue_mutex;
    struct InvokeCommand
    {
        CallbackId callback_id;
        PopulateCallbackArgsFunction populateCallbackArgs;
    };
    dmArray<InvokeCommand *> command_queue;

}

// Grabs a lua callback from the stack and registers it so we can call it later
// returns the ID of the registered callback (so we can call it later!)
CallbackId RegisterCallback(lua_State *L, int index) {

    DM_MUTEX_SCOPED_LOCK(subscriptions_mutex);

    CallbackId callback_id = ++subscription_id_counter;

    CallbackSubscription *new_sub = new CallbackSubscription();

    // DM_MUTEX_SCOPED_LOCK(new_sub->call_mutex);

    subscriptions[callback_id] = new_sub;

    // second arg = stream callback
    RegisterCallback(L, index, &new_sub->callback);

    // printf("(new: %lu)\n", callback_id);

    return callback_id;
}

void UnregisterCallback(CallbackId callback_id) {
    DM_MUTEX_SCOPED_LOCK(subscriptions_mutex);

    if (subscriptions.find(callback_id) == subscriptions.end()) {
        printf("Invalid callback id: %lu\n", callback_id);
        throw "invalid callback id.";
    }

    CallbackSubscription *sub = subscriptions[callback_id];

    LuaCallbackInfo *cbk = &sub->callback;
    if (cbk->m_Callback != LUA_NOREF)
    {
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
        cbk->m_Callback = LUA_NOREF;
    }

    // free
    delete sub;

    // remove from map
    subscriptions.erase(callback_id);
}

void InvokeCallback(lua_State *L, CallbackId callback_id, PopulateCallbackArgsFunction populateCallbackArgs) {
    DM_MUTEX_SCOPED_LOCK(command_queue_mutex);
    DM_MUTEX_SCOPED_LOCK(subscriptions_mutex);

    // Valid subscription must exist at time of invoke queue (fine if it gets removed later)
    if (subscriptions.find(callback_id) == subscriptions.end()) {
        printf("Invalid callback id: %lu\n", callback_id);
        throw "invalid callback id.";
    }

    // Create invoke command
    InvokeCommand *cmd = new InvokeCommand();
    cmd->callback_id = callback_id;
    cmd->populateCallbackArgs = populateCallbackArgs;

    if (command_queue.Full())
    {
        // why are we doing this?
        command_queue.OffsetCapacity(8);
    }
    command_queue.Push(cmd);

}

void InitQueue() {
    command_queue_mutex = dmMutex::New();
    subscriptions_mutex = dmMutex::New();
}

void RunQueue() {
    DM_MUTEX_SCOPED_LOCK(command_queue_mutex);

    // go through entire queue, invoke callbacks
    for (uint32_t i = 0; i != command_queue.Size(); ++i)
    {
        InvokeCommand *cmd = command_queue[i];
        CallbackId callback_id = cmd->callback_id;

        DM_MUTEX_SCOPED_LOCK(subscriptions_mutex);

        if (subscriptions.find(callback_id) == subscriptions.end()) {
            printf("(tried calling callback that was unqueued: %lu. Ignoring)", callback_id);
            continue;
        }
        CallbackSubscription *sub = subscriptions[callback_id];

        // Actually invoke it!
        InvokeCallback(&sub->callback, cmd->populateCallbackArgs);

        // free
        delete cmd;

        //remove from map
        command_queue.EraseSwap(i--);
    }
    command_queue.SetSize(0);
}