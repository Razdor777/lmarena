#include "ConfigManager.hpp"

#include <fstream>
#include <filesystem>
#include <chrono>
#include <Features/FeatureManager.hpp>
#include <Utils/FileUtils.hpp>
#include <nlohmann/json.hpp>

#include "spdlog/spdlog.h"

namespace {
bool backupCorruptedConfig(const std::string& path)
{
    try
    {
        if (!std::filesystem::exists(path)) return false;

        const auto ts = std::chrono::system_clock::now().time_since_epoch().count();
        const std::filesystem::path src(path);
        const auto backupPath = src.parent_path() / (src.stem().string() + ".corrupted." + std::to_string(ts) + src.extension().string());
        std::filesystem::copy_file(src, backupPath, std::filesystem::copy_options::overwrite_existing);
        spdlog::warn("Backed up corrupted config to {}", backupPath.string());
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to backup corrupted config {}: {}", path, e.what());
        return false;
    }
}
}

std::string ConfigManager::getConfigPath()
{
    return FileUtils::getDexkoDir() + "Configs\\";
}

bool ConfigManager::configExists(const std::string& name)
{
    return FileUtils::fileExists(getConfigPath() + name + ".json");
}

void ConfigManager::loadConfig(const std::string& name)
{
    std::string path = getConfigPath() + name + ".json";

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            spdlog::error("Failed to open config file: {}", path);
            return;
        }

        if (file.peek() == std::ifstream::traits_type::eof()) {
            spdlog::error("Config {} is empty", path);
            backupCorruptedConfig(path);
            saveConfig(name);
            return;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        if (!j.is_object() || !j.contains("modules") || !j["modules"].is_array()) {
            spdlog::error("Config {} has invalid structure", path);
            backupCorruptedConfig(path);
            saveConfig(name);
            return;
        }

        gFeatureManager->mModuleManager->deserialize(j);

        LastLoadedConfig = name;

        spdlog::info("Loaded config " + name + " successfully.");
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("JSON error loading config {}: {}", name, e.what());
        backupCorruptedConfig(path);
        saveConfig(name);
    } catch (const std::exception& e) {
        spdlog::error("Error loading config {}: {}", name, e.what());
    }
}

void ConfigManager::saveConfig(const std::string& name)
{
    std::string path = getConfigPath() + name + ".json";

    try {
        // Сначала сериализуем в строку (до открытия файла!)
        // Если тут exception — файл НЕ БУДЕТ повреждён
        nlohmann::json j = gFeatureManager->mModuleManager->serialize();
        std::string content = j.dump(4);

        // Только теперь пишем в файл
        std::ofstream file(path);
        if (!file.is_open()) {
            spdlog::error("Failed to open config file for writing: {}", path);
            return;
        }
        file << content;
        file.close();

        LastLoadedConfig = name;

        spdlog::info("Config saved successfully.");
    } catch (const std::exception& e) {
        spdlog::error("Failed to save config {}: {}", name, e.what());
    }
}