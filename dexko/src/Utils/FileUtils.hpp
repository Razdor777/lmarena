#pragma once
//
// Created by vastrakai on 6/29/2024.
//

#include <string>
#include <vector>

class FileUtils {
public:
  static std::string getRoamingStatePath();
  static std::string getDexkoDir();
  static bool fileExists(const std::string &path);
  static void createDirectory(const std::string &path);
  static void validateDirectories();
  static bool deleteFile(const std::string &path);
  static std::vector<std::string> listFiles(const std::string &path);
  static void createFile(const std::string &path);
};
