//
// Created by vastrakai on 7/19/2024.
//

#include "MessageTemplate.hpp"

std::string MessageTemplate::getEntry() {
    auto entry = mDefaultMessage;
    if (entry.empty()) return "";

    for (const auto& [variable, value] : mVariables)
    {
        entry = StringUtils::replaceAll(entry, variable, value);
    }

    return entry;
}

void MessageTemplate::defineVariable(const std::string& variableName, const std::string& variableValue)
{
    mVariables["!" + variableName + "!"] = variableValue;
}

void MessageTemplate::setVariableValue(const std::string& variableName, const std::string& variableValue)
{
    mVariables["!" + variableName + "!"] = variableValue;
}