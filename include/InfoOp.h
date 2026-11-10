#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool CheckRearInfo(const json& message);

void AppendRearInfo(const json& massage, json& body);
