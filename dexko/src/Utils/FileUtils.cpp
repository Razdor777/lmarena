//
// Created by vastrakai on 6/29/2024.
//

#include "FileUtils.hpp"

#include <Windows.h>
#include <filesystem>
#include <fstream>

#include "spdlog/spdlog.h"

std::string FileUtils::getRoamingStatePath() {
  // Get the appdata environment variable
  char *appdata = nullptr;
  size_t len = 0;
  _dupenv_s(&appdata, &len, "APPDATA");
  if (appdata == nullptr)
    return "";
  return std::string(appdata) + "\\..\\Local\\Packages\\Microsoft.MinecraftUWP_"
                                "8wekyb3d8bbwe\\RoamingState\\";
}

std::string FileUtils::getDexkoDir() {
  return getRoamingStatePath() + xorstr_("Dexko\\");
}

bool FileUtils::fileExists(const std::string &path) {
  return std::filesystem::exists(path);
}

void FileUtils::createDirectory(const std::string &path) {
  if (!std::filesystem::exists(path)) {
    std::filesystem::create_directory(path);
    spdlog::info("Created directory: {}", path);
  }
}

void FileUtils::validateDirectories() {
  createDirectory(getDexkoDir());
  createDirectory(getDexkoDir() + "Configs\\");
  spdlog::info("Directories created successfully.");
}

bool FileUtils::deleteFile(const std::string &path) {
  if (fileExists(path)) {
    DeleteFileA(path.c_str());
    return true;
  } else {
    spdlog::error("Failed to delete file: {}", path);
    return false;
  }
}

std::vector<std::string> FileUtils::listFiles(const std::string &path) {
  std::vector<std::string> files;

  WIN32_FIND_DATAA findFileData;
  HANDLE hFind = FindFirstFileA((path + "*").c_str(), &findFileData);
  if (hFind == INVALID_HANDLE_VALUE) {
    spdlog::error("Failed to list files in directory: {}", path);
    return files;
  }

  do {
    if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      continue;

    files.push_back(findFileData.cFileName);
  } while (FindNextFileA(hFind, &findFileData));

  FindClose(hFind);

  return files;
}

void FileUtils::createFile(const std::string &path) {
  std::ofstream file(path);
  file.close();
  spdlog::info("Created file: {}", path);
}
