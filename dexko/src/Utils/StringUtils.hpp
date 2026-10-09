
#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <cctype>
//
// Created by vastrakai on 6/28/2024.
//

class StringUtils {
public:
    static std::vector<std::string> split(std::string_view str, char delimiter);
    static std::string toLower(std::string str);
    static std::string toUpper(std::string str);
    static bool equalsIgnoreCase(const std::string& str1, const std::string& str2);
    static bool containsIgnoreCase(const std::string& str, const std::string& subStr);
    static bool containsAnyIgnoreCase(const std::string& str, const std::vector<std::string>& strVector);
    static std::string replaceAll(std::string& string, const std::string& from, const std::string& to);
    static std::string generateUUID(int index);
    static std::string generateMboard(int index);
    static int64_t generateCID();
    static std::string fromBase64(const std::string& str);
    static std::string toBase64(const std::string& str);
};
