#include <iostream>
#include "InfoOp.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

/// @brief 检查"tool_calls"字段，判断是否需要工具调用， 有 返回 1 ，没有 返回 0 
/// @param message   response[choices][message]引用
bool CheckRearInfo(const json& message)
{
    if(message.empty()) return 0;
    if(message.contains("tool_calls") && 
        message["tool_calls"].is_array() &&
        !message["tool_calls"].empty()) return 1;
    return 0;
}

/// @brief 将massage字段追加到requestbody的message历史列表中
/// @param massage response[choices][message]引用
/// @param body request_body
void AppendRearInfo(const json& message, json& body)
{
    if(message.empty()) perror("错误，有空信息要被加到requestbody中");
    if(!body.contains("messages")) perror("我草怎么message历史消息不见了！");
    if (!body.contains("messages") || !body["messages"].is_array()) {
    std::cerr << "RequestBody格式不对\n";return;}
    body["messages"].push_back(message);
}

