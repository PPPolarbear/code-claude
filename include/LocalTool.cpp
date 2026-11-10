#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "LocalTool.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/// @brief 本地调用工具并返回json格式的调用结果，目前仅能实现一次工具调用
/// @param message AI返回的包含调用请求内容
nlohmann::json tool_invoke(const nlohmann::json& message)
{
    const json& messTool = message["tool_calls"].back();
    if(messTool.contains("function") 
        && messTool.contains("name") 
        && messTool["function"].empty()) perror("AI返回的工具调用格式解析错误");
    std::string toolName = messTool["function"]["name"].get<std::string>();
    if(toolName == "Read")
    {
        std::string argPath = messTool["function"]["arguments"].get<std::string>();
        if(argPath.empty()) perror("读取文件路径为空！");
        std::string toolid = messTool["id"].get<std::string>();
        json argPath2json = json::parse(argPath);
        std::string RealPath = argPath2json["file_path"].get<std::string>();
        std::ifstream fileread(RealPath, std::ios::binary);
        if(!fileread) 
        {
            return {
                {"role", "tool"},
                {"tool_call_id", toolid},
                {"content", "Failed to open file: " + RealPath}
            };
        }
        else
        {
            std::ostringstream buffer;
            buffer << fileread.rdbuf();
            std::string content = buffer.str();
            return {
                {"role", "tool"},
                {"tool_call_id", toolid},
                {"content", content}
            };
        }
        std::cerr << "tool_invoke函数错误，不应该执行到这里\n";
        std::exit(EXIT_FAILURE); // 异常退出出
    }
    // 其他工具函数待添加
    std::cerr << "访问到未定义工具\n";
    return 1;
}
