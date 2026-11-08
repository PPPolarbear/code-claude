#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;




int main(int argc, char* argv[]) {
    if (argc < 3 || std::string(argv[1]) != "-p") {
        std::cerr << "Expected first argument to be '-p'" << std::endl;
        return 1;
    }

    std::string prompt = argv[2];

    if (prompt.empty()) {
        std::cerr << "Prompt must not be empty" << std::endl;
        return 1;
    }

    const char* api_key_env = std::getenv("OPENROUTER_API_KEY");
    const char* base_url_env = std::getenv("OPENROUTER_BASE_URL");

    std::string api_key = api_key_env ? api_key_env : "";
    std::string base_url = base_url_env ? base_url_env : "https://openrouter.ai/api/v1";

    if (api_key.empty()) {
        std::cerr << "OPENROUTER_API_KEY is not set" << std::endl;
        return 1;
    }



    json request_body = {
        {"model", "anthropic/claude-haiku-4.5"},
        {"messages", json::array({
            {{"role", "user"}, {"content", prompt}}
        })},
        {"tools", json::array({
            {
                {"type", "function"},
                {"function", {
                    {"name", "Read"},
                    {"description", "Read and return the contents of a file"},
                    {"parameters", {
                        {"type", "object"},
                        {"properties", {
                            {"file_path", {
                                {"type", "string"},
                                {"description", "The path to the file to read"}
                            }}
                        }},
                        {"required", json::array({"file_path"})}
                    }}
                }}
            }
})}
    };

    cpr::Response response = cpr::Post(
        cpr::Url{base_url + "/chat/completions"},
        cpr::Header{
            {"Authorization", "Bearer " + api_key},
            {"Content-Type", "application/json"}
        },
        cpr::Body{request_body.dump()}
    );

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
                "function": {
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

    if (response.status_code != 200) {
        std::cerr << "HTTP error: " << response.status_code << std::endl;
        return 1;
    }

    json result = json::parse(response.text);

    if (!result.contains("choices") || result["choices"].empty()) {
        std::cerr << "No choices in response" << std::endl;
        return 1;
    }

    json& message = result["choices"][0]["message"];

    /*
        改request_body中的message字段，每次返回看工具调用是不是空
    不空则继续调用并把信息追加到mesage中
    空则认为成功结束
    */

    
    request_body.append(message);      // 把返回信息里面最后一轮的AI内容返回，request_body后续需要改写为一个类

    while(message.contains("tool_calls") && 
        message["tool_calls"].is_array() &&
        !message["tool_calls"].empty())
        {
            /// 发现存在工具调用
            std::string tool_res = tool_invoke(message);        // 执行工具调用，把返回字符串形式，记得注释里面写好格式
            request_body.append(tool_res);                      // 把工具调用结果加入
            repost(request_body)                                // 重新发送请求并更新message，result，response


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

        }
    
    /// 处理完工具调用后输出内容
        if (message.contains("content") && message["content"].is_string()) {
            std::cout << message["content"].get<std::string>();
        }

    /// 下面后续内容等待下次处理时合并整理到tool_invoke函数
    if (message.contains("tool_calls") && message["tool_calls"].is_array() &&
        !message["tool_calls"].empty())
        {
            const json& messTool = result["choices"][0]["message"]["tool_calls"][0];
            std::string toolName = messTool["function"]["name"].get<std::string>();
            if(toolName == "Read")
            {
                std::string argPath = messTool["function"]["arguments"].get<std::string>();
                if(argPath.empty()) perror("读取文件路径为空！");
                json argPath2json = json::parse(argPath);
                std::string RealPath = argPath2json["file_path"].get<std::string>();
                std::ifstream fileread(RealPath, std::ios::binary);
                if(!fileread) 
                {
                    std::cerr << "Failed to open file: " << RealPath << std::endl;
                    return 1;
                }
                std::cout << fileread.rdbuf();
            }
        }



    return 0;
}
