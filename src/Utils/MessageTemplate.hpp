#pragma once
//
// Created by vastrakai on 7/19/2024.
//

class MessageTemplate {
public:
    std::string mTemplateName = "";
    std::string mDefaultMessage = "";
    std::map<std::string, std::string> mVariables = {};
    std::map<std::string, std::string> mVariableDescriptions = {};

    template <typename... Args>
    explicit MessageTemplate(const std::string& templateName, const std::string defaultMessage, Args... args)
        : mTemplateName(templateName), mDefaultMessage(defaultMessage)
    {
        mVariableDescriptions = { args... };
    }

    [[nodiscard]] std::string getEntry();
    void defineVariable(const std::string& variableName, const std::string& variableValue);
    void setVariableValue(const std::string& variableName, const std::string& variableValue);
};