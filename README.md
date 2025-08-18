# Player 2 AI NPC Plugin for Defold
The Official Player2 AI NPC Plugin for Defold

The Player 2 AI NPC Defold plugin allows developers to easily create AI NPCs in their Defold projects.

The plugin uses free AI APIs from the [player2 App](https://player2.game/)

Just open Player2, and the plugin connects automatically, so you can dive right into building your world instead of wrestling with keys or settings. When your game is ready, we’ll share it with our community of 40,000+ active players eager for AI-driven adventures [on our discord](https://player2.game/discord)

## Usage Guide

### Installing the plugin
You can use Player 2 AI NPC Plugin in your own project by adding this project as a [Defold library dependency](http://www.defold.com/manuals/libraries/). Open your game.project file and in the dependencies field under project add:

	https://github.com/elefant-ai/player2-ai-npc-defold/archive/master.zip
	
---

### NPC API

The Player 2 NPC API is accessible via the `ainpc` global.

Player2 NPCs can be spawned in (`ainpc.npc_spawn`), spoken to (`ainpc.npc_chat`), and killed (`ainpc.npc_kill`). The responses for all NPCs can be read as a stream (`ainpc.npc_responses`).

#### [A simple NPC example can be found here](https://github.com/elefant-ai/player2-ai-npc-defold/blob/main/assets/scripts/npc_test.script)

### Chat completion API

If you just wish to use simple chat completion, you can do so via the `ainpc.chat_completion` function.

```lua
local t = {
    ["messages"] = {
        {
            ["content"] = "You are an assistant.",
            ["role"] = "system"
        },
        {
            ["content"] = "Hello, how are you doing today?",
            ["role"] = "user"
        }
    }
}
ainpc.chat_completion(t,
function(self, result)
    print("REPLY:", json.encode(result))
end,
function(self, err, id)
    print("Failed", err, id)
end)
```
Example reply:
```json
{
    "choices": [
        {
            "finish_reason": "stop",
            "index": 0,
            "message": {
                "content": "Hello! I'm functioning well and ready to chat or help with whatever you need! 🌟 I always enjoy our conversations. How are you doing today? I'd love to hear from you!",
                "role": "assistant"
            }
        }
    ]
}
```
The chat completion response is a table following the [Player 2 API](http://localhost:4315/docs/#/Chat/chat_completion).

If you wish, you can also pass a string to `ainpc.chat_completion` instead of a table:
```lua
ainpc.chat_completion("Hello, how are you doing today?", ...)
```
This is identical to running:
```lua
local t = {
    ["messages"] = {
        {
            ["content"] = "Hello, how are you doing today?",
            ["role"] = "user"
        }
    }
}
ainpc.chat_completion(t, ...)
```

### Configuration

Custom configuration must be added to the `game.project` file manually via text editor.

All configuration values for the extension are defined in the [`ext.properties`](https://github.com/elefant-ai/player2-ai-npc-defold/blob/main/player2-ai-npc/ext.properties) file. For example, override the host by appending this to the `game.project` file:
```properties
[player2_endpoints]
host = http://localhost:8080
```

### Game key

The game key passed to the APIs is the project title (found opening the `game.project` file)