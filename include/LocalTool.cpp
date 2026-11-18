#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include "LocalTool.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <sys/wait.h>

using json = nlohmann::json;


json Get_Path(const json& message)
{
    if(message.empty()) 
    {
        std::cerr << "写操作中message丢失\n";
        std::exit(EXIT_FAILURE);
    }
    if(message.contains("tool_calls") &&
     !message["tool_calls"].empty() &&
     message["tool_calls"][0].contains("function") &&
      message["tool_calls"][0]["function"].contains("arguments") &&
       !message["tool_calls"][0]["function"]["arguments"].empty())
       {
            return json::parse(message["tool_calls"][0]["function"]["arguments"].get<std::string>());
       }
    else{
        std::cerr << "写操作中message缺少arguments字段\n";
        std::exit(EXIT_FAILURE);
    }
    return "";
}

/// @brief 从message的message["tool_calls"][i]["function"]["arguments"]字段提取出命令
/// @param message 模型返回信息
/// @return 命令字符串
std::string Get_command(const nlohmann::json& message)
{
std::string Get_command(const nlohmann::json& message)
{
    if (message.contains("tool_calls") &&
        !message["tool_calls"].empty() &&
        message["tool_calls"][0].contains("function") &&
        message["tool_calls"][0]["function"].contains("arguments") &&
        !message["tool_calls"][0]["function"]["arguments"].empty())
    {
        json arguments = json::parse(
            message["tool_calls"][0]["function"]["arguments"].get<std::string>());
        return arguments["command"].get<std::string>();
    }

    std::cerr << "模型信息返回错误，无法调用bash";
    _exit(126);
}
}

/// @brief 利用POSIX原语完成bash执行工作
/// @param command 命令字符串
/// @return shell在标准输入输出流上的执行结果
std::string ExecRes(const std::string& command)
{
    int fds[2];
    if(pipe(fds) == -1)
    {
        std::cerr << "pipe创建失败";
        return "";
    }

    pid_t pid = fork();
    if(pid == -1) // fork失败
    {
        close(fds[0]);
        close(fds[1]);
        std::cerr << "fork失败";
        return "";
    }

    if(!pid) // 子进程
    {
        if(dup2(fds[1], STDOUT_FILENO) == -1 || dup2(fds[1], STDERR_FILENO) == -1)
        {
            std::cerr << "标准输入/输出重定向到管道过程失败";
            _exit(126);
        }
        close(fds[0]);
        close(fds[1]);

        // 交给shell执行command，成功则无信息返回直接结束
        execl("/bin/sh", "sh", "-c", command.c_str(), (char*)nullptr);

        // 只有 exec 失败才会走到这里
        _exit(127);
    }

    // 父进程
    close(fds[1]); // 关闭写端，防止read等不到EOF

    std::string output;
    std::array<char, 4096> buf;

    while(1)
    {
        int n = read(fds[0], buf.data(), buf.size());
        if(n > 0)
        {
            // 成功读取
            output.append(buf.data(), static_cast<size_t>(n));
        }
        else if(n == 0){
            // 已经读完，关闭写端
            break; 
        }
        else{
            std::cerr << "读端异常";
            break;
        }
    }
    close(fds[0]); // 用完关闭读端

    int status = 0;
    waitpid(pid, &status, 0);
    return output;
}


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
        std::exit(EXIT_FAILURE); // 异常退出
    }
    else if(toolName == "Write")
    {
        // 解析获取待写入路径和内容
        json Arg = Get_Path(message);
        std::string file_path = Arg["file_path"].get<std::string>();
        std::string content = Arg["content"].get<std::string>();
        std::string toolid = message["tool_calls"][0]["id"].get<std::string>();

        std::ofstream file(file_path);
        if (!file) {
        // 创建失败
        std::cerr << "文件创建失败\n";
        return{
                {"role", "tool"},
                {"tool_call_id", toolid},
                {"content", "写入失败"}}; 
        }
        
        // 直接覆写
        file << content;
        return{
                {"role", "tool"},
                {"tool_call_id", toolid},
                {"content", "成功写入"}}; 
    }
    else if(toolName == "Bash")
    {
        std::string toolid = message["tool_calls"][0]["id"].get<std::string>();
        std::string command = Get_command(message);
        std::string res = ExecRes(command);
        return {
            {"role", "tool"},
            {"tool_call_id", toolid},
            {"content", res}
        };
    }
    
    // 其他工具函数待添加
    std::cerr << "访问到未定义工具\n";
    return 1;
}
