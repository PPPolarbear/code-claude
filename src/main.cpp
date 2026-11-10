#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include "InfoOp.h"
#include "LocalTool.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

cpr::Response Repost(json& request_body, std::string api_key, std::string base_url)
{
    cpr::Response response = cpr::Post(
        cpr::Url{base_url + "/chat/completions"},
        cpr::Header{
            {"Authorization", "Bearer " + api_key},
            {"Content-Type", "application/json"}
        },
        cpr::Body{request_body.dump()}
    );

    if (response.status_code != 200) {
        std::cerr << "HTTP error: " << response.status_code << std::endl;
        exit(EXIT_FAILURE);// 异常退出
    }
    return response;
}


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

    if (response.status_code != 200) {
        std::cerr << "HTTP error: " << response.status_code << std::endl;
        return 1;
    }

    json result = json::parse(response.text);

    if (!result.contains("choices") || result["choices"].empty()) {
        std::cerr << "No choices in response" << std::endl;
        return 1;
    }

    json message = result["choices"][0]["message"];

    /*
        改request_body中的message字段，每次返回看工具调用是不是空
    不空则继续调用并把信息追加到mesage中
    空则认为成功结束
    */

    while(CheckRearInfo(message)) // 工具调用不通过
    {
        json toolres = tool_invoke(message); // 本地调用工具，返回成json类型
        AppendRearInfo(message, request_body); // 先加AI返回的对话历史
        AppendRearInfo(toolres, request_body); // 再加入本地调用工具的的结果
        result = json::parse(Repost(request_body, api_key, base_url).text);
        if (!result.contains("choices") || result["choices"].empty()) 
        {
        std::cerr << "No choices in response" << std::endl;
        return 1;
        }
        message = result["choices"][0]["message"];
    }
    
    /// 处理完工具调用后输出内容
    if (message.contains("content") && message["content"].is_string()) {
        std::cout << message["content"].get<std::string>();
    }


    return 0;
}
