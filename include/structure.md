
    /*
    response结构：
    {
    "choices": [
        {
        "index": 0,
        "message": {
            "role": "assistant",
            "content": null,
            "tool_calls": [
            {
                "id": "call_abc123",
                "type": "function",
                "function": 
                {
                "name": "Read",
                "arguments": "{\"file_path\": \"/path/to/file.txt\"}"
                }
            }
            ]
        },
        "finish_reason": "tool_calls"
        }
    ]
    }
    */


    /*
    ----------------------------------
    AI返回型
    {
        "role": "assistant",
        "content": null,
        "tool_calls": [
            {
            "id": "call_abc123",
            "type": "function",
            "function": {
                "name": "Read",
                "arguments": "{\"file_path\": \"README.md\"}"
            }
            }
        ]
    }
    ----------------------------------
    工具返回型
    {
        "role": "tool",
        "tool_call_id": "call_abc123",
        "content": "# My Project\n\nChemical expiry period: 6 months"
    }
    ----------------------------------
    */



Bash Tool example：
{
  "type": "function",
  "function": {
    "name": "Bash",
    "description": "Execute a shell command",
    "parameters": {
      "type": "object",
      "required": ["command"],
      "properties": {
        "command": {
          "type": "string",
          "description": "The command to execute"
        }
      }
    }
  }
}

