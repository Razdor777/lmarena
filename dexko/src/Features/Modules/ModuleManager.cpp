//
// Dexko ModuleManager — форк движкового ModuleManager с урезанным списком.
//
// Модули Dexko:
//   DeviceSpoof      — всегда включён (форсится в Dexko::shutdownThread)
//   CustomCrosshair  — включён по умолчанию
//   HitParticles     — включён по умолчанию
//   Reach            — только combat reach, включается через .reach
//   Interface        — инфраструктурный (цвета темы для HitParticles "Theme Color")
//   AntiBot          — инфраструктурный (ActorUtils жёстко ждёт его наличие)
//
// Остальные методы — 1:1 с движком (serialize/deserialize/onClientTick/...).
//

#include <Features/Modules/ModuleManager.hpp>

#include <spdlog/spdlog.h>
#include <Utils/StringUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

#include "Misc/DeviceSpoof.hpp"
#include <Features/Modules/Misc/AntiBot.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include "Visual/CustomCrosshair.hpp"
#include "Visual/HitParticles.hpp"
#include "Combat/Reach.hpp"

void ModuleManager::init() {
  // ── Infrastructure (не отображаются пользователю как "читерские" модули) ──
  mModules.emplace_back(std::make_shared<Interface>());
  mModules.emplace_back(std::make_shared<AntiBot>());

  // ── Dexko modules ──────────────────────────────────────────────────────────
  mModules.emplace_back(std::make_shared<DeviceSpoof>());     // always on
  mModules.emplace_back(std::make_shared<CustomCrosshair>()); // on by default
  mModules.emplace_back(std::make_shared<HitParticles>());    // on by default
  mModules.emplace_back(std::make_shared<Reach>());           // enabled via .reach

  for (auto &module : mModules) {
    try {
      module->onInit();
    } catch (const std::exception &e) {
      spdlog::error("Failed to initialize module {}: {}", module->mName,
                    e.what());
    } catch (const nlohmann::json::exception &e) {
      spdlog::error("Failed to initialize module {}: {}", module->mName,
                    e.what());
    } catch (...) {
      spdlog::error("Failed to initialize module {}: unknown", module->mName);
    }
  }
}

void ModuleManager::shutdown() {
  for (auto &module : mModules) {
    if (module->mEnabled) {
      module->mEnabled = false;
      module->onDisable();
    }
  }
  mModules.clear();
}

void ModuleManager::registerModule(const std::shared_ptr<Module> &module) {
  mModules.push_back(module);
}

std::vector<std::shared_ptr<Module>> &ModuleManager::getModules() {
  return mModules;
}

Module *ModuleManager::getModule(const std::string &name) const {
  for (const auto &module : mModules) {
    if (StringUtils::equalsIgnoreCase(module->mName, name)) {
      return module.get();
    }
    // Also check all name variants (Lowercase, LowercaseSpaced, Normal, NormalSpaced)
    for (const auto &[style, variant] : module->mNames) {
      if (StringUtils::equalsIgnoreCase(variant, name)) {
        return module.get();
      }
    }
  }
  return nullptr;
}

void ModuleManager::removeModule(const std::string &name) {
  for (auto it = mModules.begin(); it != mModules.end(); ++it) {
    if (StringUtils::equalsIgnoreCase((*it)->mName, name)) {
      mModules.erase(it);
      return;
    }
  }
}

std::vector<std::shared_ptr<Module>> &
ModuleManager::getModulesInCategory(int catId) {
  static std::unordered_map<int, std::vector<std::shared_ptr<Module>>>
      categoryMap = {};
  if (categoryMap.contains(catId)) {
    return categoryMap[catId];
  }

  std::vector<std::shared_ptr<Module>> modules;
  for (const auto &module : mModules) {
    if (static_cast<int>(module->mCategory) == catId) {
      modules.push_back(module);
    }
  }

  categoryMap[catId] = modules;
  return categoryMap[catId];
}

std::unordered_map<std::string, std::shared_ptr<Module>>
ModuleManager::getModuleCategoryMap() {
  static std::unordered_map<std::string, std::shared_ptr<Module>> map;

  if (!map.empty()) {
    return map;
  }

  for (const auto &module : mModules) {
    map[module->getCategory()] = module;
  }

  return map;
}

void ModuleManager::onClientTick() {
  for (auto &module : mModules) {
    try {
      if (module->mWantedState != module->mEnabled) {
        module->mEnabled = module->mWantedState;
        spdlog::trace("onClientTick: calling {} on module {}",
                      module->mEnabled ? "onEnable" : "onDisable",
                      module->mName);
        if (module->mEnabled) {
          module->onEnable();
        } else {
          module->onDisable();
        }
      }
    } catch (const std::exception &e) {
      spdlog::error("Failed to enable/disable module {}: {}", module->mName,
                    e.what());
    } catch (const nlohmann::json::exception &e) {
      spdlog::error("Failed to enable/disable module {}: {}", module->mName,
                    e.what());
    } catch (...) {
      spdlog::error("Failed to enable/disable module {}: unknown",
                    module->mName);
    }
  }
}

nlohmann::json ModuleManager::serialize() const {
  nlohmann::json j;
  j["client"] = "Dexko";
  j["version"] = DEXKO_VERSION;
  j["modules"] = nlohmann::json::array();

  for (const auto &module : mModules) {
    j["modules"].push_back(module->serialize());
  }

  return j;
}

nlohmann::json ModuleManager::serializeModule(Module *module) {
  nlohmann::json j;
  j["client"] = "Dexko";
  j["version"] = DEXKO_VERSION;
  j["modules"] = nlohmann::json::array();

  j["modules"].push_back(module->serialize());

  return j;
}

void ModuleManager::deserialize(const nlohmann::json &j, bool showMessages) {
  if (!j.is_object() || !j.contains("modules") || !j["modules"].is_array()) {
    spdlog::error("Invalid config root: expected object with modules array");
    if (showMessages) {
      ChatUtils::displayClientMessage(
          "§cInvalid config format. Using defaults.");
    }
    return;
  }

  const std::string version = j.value("version", std::string{});
  std::string currentVersion = DEXKO_VERSION;

  if (version != currentVersion) {
    spdlog::warn("Config version mismatch. Expected: {}, Got: {}",
                 currentVersion, version);
    ChatUtils::displayClientMessage(
        "§eWarning: The specified config is from a different version of "
        "Dexko. §cSome settings may not be loaded§e.");
  }

  int modulesLoaded = 0;
  int settingsLoaded = 0;

  std::vector<std::string> moduleNames;
  for (const auto &module : mModules) {
    moduleNames.push_back(module->mName);
  }

  for (const auto &module : j["modules"]) {
    if (!module.is_object() || !module.contains("name")) {
      spdlog::warn("Skipping malformed module entry in config");
      continue;
    }

    const std::string name = module.value("name", std::string{});
    if (name.empty()) {
      spdlog::warn("Skipping unnamed module entry in config");
      continue;
    }
    std::erase(moduleNames, name);
    const bool enabled = module.value("enabled", false);
    const int keybind = module.value("key", 0);

    auto *mod = getModule(name);
    if (mod) {
      mod->mWantedState = enabled;
      mod->mKey = keybind;

      std::vector<std::string> settingNames;
      for (const auto &setting : mod->mSettings) {
        settingNames.push_back(setting->mName);
      }

      if (module.contains("settings") &&
          (module["settings"].is_array() || module["settings"].is_object())) {
        for (const auto &setting : module["settings"].items()) {
          try {
            const auto &settingValue = setting.value();
            if (!settingValue.is_object() || !settingValue.contains("name")) {
              spdlog::warn("Skipping malformed setting in module {}", name);
              continue;
            }
            const std::string settingName = settingValue["name"];
            std::erase(settingNames, settingName);

            auto *set = mod->getSetting(settingName);
            if (set) {
              if (set->mType == SettingType::Bool) {
                auto *boolSetting = static_cast<BoolSetting *>(set);
                boolSetting->mValue = settingValue["boolValue"];
                if (settingValue.contains("key")) {
                  boolSetting->mKey = settingValue["key"];
                } else {
                  boolSetting->mKey = -1;
                }
              } else if (set->mType == SettingType::Number) {
                auto *numberSetting = static_cast<NumberSetting *>(set);
                numberSetting->mValue = settingValue["numberValue"];
              } else if (set->mType == SettingType::Enum) {
                auto *enumSetting = static_cast<EnumSetting *>(set);
                if (settingValue["enumValue"] >= 0 &&
                    settingValue["enumValue"] < enumSetting->mValues.size())
                  enumSetting->mValue = settingValue["enumValue"];
                else {
                  spdlog::warn("Invalid enum value for setting {} in module {}",
                               settingName, name);
                  if (showMessages)
                    ChatUtils::displayClientMessage(
                        "§cInvalid enum value for setting §6" + settingName +
                        "§c in module §6" + name + "§c.");
                }
              } else if (set->mType == SettingType::Color) {
                auto *colorSetting = static_cast<ColorSetting *>(set);
                for (int i = 0; i < 4; i++) {
                  colorSetting->mValue[i] = settingValue["colorValue"][i];
                }
              } else if (set->mType == SettingType::String) {
                auto *stringSetting = static_cast<StringSetting *>(set);
                if (settingValue.contains("stringValue") && settingValue["stringValue"].is_string()) {
                  stringSetting->setValue(settingValue["stringValue"].get<std::string>());
                }
              }

              settingsLoaded++;
            } else {
              spdlog::warn("Setting {} not found for module {}", settingName,
                           name);
              if (showMessages)
                ChatUtils::displayClientMessage("§cSetting §6" + settingName +
                                                "§c not found for module §6" +
                                                name + "§c.");
            }
          } catch (const std::exception &e) {
            spdlog::warn("Failed to load setting {} for module {}: {}",
                         setting.key(), name, e.what());
            if (showMessages)
              ChatUtils::displayClientMessage(
                  "§cFailed to load setting §6" + setting.key() +
                  "§c for module §6" + name + "§c.");
          }
        }

        modulesLoaded++;
      }

      // ===== GENERIC CUSTOM DATA — работает для ЛЮБОГО модуля =====
      if (mod->hasCustomData()) {
        try {
          if (module.contains("customData") &&
              !module["customData"].is_null()) {
            mod->deserializeCustomData(module["customData"]);
          } else {
            mod->deserializeCustomData(nlohmann::json());
          }
        } catch (const std::exception &e) {
          spdlog::warn("Failed to load custom data for {}: {}", name, e.what());
          if (showMessages)
            ChatUtils::displayClientMessage(
                "§cFailed to load custom data for §6" + name + "§c.");
        }
      }
      // =============================================================

      for (const auto &settingName : settingNames) {
        spdlog::warn(
            "Setting {} not found for module {}, default value will be used",
            settingName, name);
        if (showMessages)
          ChatUtils::displayClientMessage("§cSetting §6" + settingName +
                                          "§c not found for module §6" + name +
                                          "§c, default value will be used.");
      }
    } else {
      spdlog::warn("Module {} not found", name);
      if (showMessages)
        ChatUtils::displayClientMessage("§cModule §6" + name + "§c not found.");
    }
  }

  for (const auto &moduleName : moduleNames) {
    spdlog::warn("Module {} not found in config, using default settings",
                 moduleName);
    if (showMessages)
      ChatUtils::displayClientMessage(
          "§cModule §6" + moduleName +
          "§c not found in config, using default settings.");
  }

  spdlog::info("Loaded {} modules and {} settings from config", modulesLoaded,
               settingsLoaded);
}
