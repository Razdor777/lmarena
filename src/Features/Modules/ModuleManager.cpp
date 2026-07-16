// ModuleManager.cpp
//
// Created by vastrakai on 6/28/2024.
//

#include <Features/Modules/ModuleManager.hpp>
#include <Utils/OAuthUtils.hpp>
#include <build_info.h>

#include "Combat/Aimbot.hpp"
#include "Combat/ArrowTP.hpp"
#include "Combat/Aura.hpp"
#include "Combat/AutoClicker.hpp"
#include "Combat/Criticals.hpp"
#include "Combat/GhostMode.hpp"
#include "Combat/HitBoxes.hpp"
#include "Combat/InfiniteAura.hpp"
#include "Combat/InfiniteChestAura.hpp"
#include "Combat/PearlStopper.hpp"
#include "Combat/Reach.hpp"
#include "Combat/TriggerBot.hpp"

#include "Misc/AntiBot.hpp"
#include "Misc/AntiCheatDetector.hpp"
#include "Misc/Anticheat.hpp"
#include "Misc/ArmorSlotUnlock.hpp"
#include "Misc/AutoDodge.hpp"
#include "Misc/AutoLoot.hpp"
#include "Misc/AutoMessage.hpp"
#include "Misc/DeviceSpoof.hpp"
#include "Misc/Disabler.hpp"
#include "Misc/FakeChat.hpp"
#include "Misc/Friends.hpp"
#include "Misc/ItemDupe.hpp"
#include "Misc/ItemUseDelayFix.hpp"
#include "Misc/JavaInventoryHotkeys.hpp"
#include "Misc/KickSounds.hpp"
#include "Misc/Killsults.hpp"
#include "Misc/NetSkip.hpp"
#include "Misc/NoFilter.hpp"
#include "Misc/NoPacket.hpp"
#include "Misc/NoPause.hpp"
#include "Misc/PacketLogger.hpp"
#include "Misc/PlayerLogger.hpp"
#include "Misc/SkinBlinker.hpp"
#include "Misc/SkinChanger.hpp"
#include "Misc/SkinSpoofer.hpp"
#include "Misc/SkinStealer.hpp"
#include "Misc/Spammer.hpp"
#include "Misc/SpectatorDetector.hpp"
#include "Misc/StaffAlert.hpp"
#include "Misc/TestModule.hpp"
#include "Misc/ToggleSounds.hpp"

#include "Movement/AirJump.hpp"
#include "Movement/AntiImmobile.hpp"
#include "Movement/AutoPath.hpp"
#include "Movement/AutoPatrol.hpp"
#include "Movement/AutoSneak.hpp"
#include "Movement/Fly.hpp"
#include "Movement/InventoryMove.hpp"
#include "Movement/Jesus.hpp"
#include "Movement/Jetpack.hpp"
#include "Movement/LongJump.hpp"
#include "Movement/NoClip.hpp"
#include "Movement/NoJumpDelay.hpp"
#include "Movement/NoSlowDown.hpp"
#include "Movement/ReverseStep.hpp"
#include "Movement/SafeWalk.hpp"
#include "Movement/ServerSneak.hpp"
#include "Movement/Speed.hpp"
#include "Movement/Spider.hpp"
#include "Movement/Sprint.hpp"
#include "Movement/Step.hpp"
#include "Movement/TargetStrafe.hpp"
#include "Movement/Velocity.hpp"

#include "Player/AntiVoid.hpp"
#include "Player/AutoArmor.hpp"
#include "Player/AutoTool.hpp"
#include "Player/Cage.hpp"
#include "Player/ChainTP.hpp"
#include "Player/ChestAura.hpp"
#include "Player/ChestStealer.hpp"
#include "Player/ClickTp.hpp"
#include "Player/ContainerActions.hpp"
#include "Player/FastEat.hpp"
#include "Player/FastMine.hpp"
#include "Player/Freecam.hpp"
#include "Player/InvManager.hpp"
#include "Player/MidclickAction.hpp"
#include "Player/NoFall.hpp"
#include "Player/NoRotate.hpp"
#include "Player/Nuker.hpp"
#include "Player/OreMiner.hpp"
#include "Player/OreMinerV2.hpp"
#include "Player/PlayerTracker.hpp"
#include "Player/Regen.hpp"
#include "Player/RegionFill.hpp"
#include "Player/Scaffold.hpp"
#include "Player/Schematic.hpp"
#include "Player/SchematicBuilder.hpp"
#include "Player/SpawnerAura.hpp"
#include "Player/Teams.hpp"
#include "Player/Timer.hpp"

#include "spdlog/spdlog.h"

#include "Visual/AfterImage.hpp"
#include "Visual/Ambience.hpp"
#include "Visual/AmbientCubes.hpp"
#include "Visual/Animations.hpp"
#include "Visual/ArmorHUD.hpp"
#include "Visual/Arraylist.hpp"
#include "Visual/BlockESP.hpp"
#include "Visual/ChinaHat.hpp"
#include "Visual/ClickGui.hpp"
#include "Visual/PlayerPositionOffset.hpp"
#include "Visual/CustomFog.hpp"
#include "Visual/DamageLog.hpp"
#include "Visual/DestroyProgress.hpp"
#include "Visual/ESP.hpp"
#include "Visual/EnemyIndicator.hpp"
#include "Visual/EntityRadar.hpp"
#include "Visual/FakePlayer.hpp"
#include "Visual/Freelook.hpp"
#include "Visual/FullBright.hpp"
#include "Visual/Glint.hpp"
#include "Visual/Goofy.hpp"
#include "Visual/HealthBar.hpp"
#include "Visual/HealthDisplay.hpp"
#include "Visual/HudEditor.hpp"
#include "Visual/Interface.hpp"
#include "Visual/ItemESP.hpp"
#include "Visual/ItemPhysics.hpp"
#include "Visual/Kagune.hpp"
#include "Visual/LevelInfo.hpp"
#include "Visual/PlayerHUD.hpp"
#include "Visual/InventoryHUD.hpp"
#include "Visual/MotionBlur.hpp"
#include "Visual/NameProtect.hpp"
#include "Visual/Nametags.hpp"
#include "Visual/NearbyPlayers.hpp"
#include "Visual/NoCameraClip.hpp"
#include "Visual/NoDebuff.hpp"
#include "Visual/CustomCrosshair.hpp"
#include "Visual/NoHurtcam.hpp"
#include "Visual/NoRender.hpp"
#include "Visual/Notifications.hpp"
#include "Visual/OpponentReach.hpp"
#include "Visual/PathTracer.hpp"
#include "Visual/PotionHUD.hpp"
#include "Visual/ProjectileTracer.hpp"
#include "Visual/ReachCounter.hpp"
#include "Visual/RobloxCamera.hpp"
#include "Visual/HitColor.hpp"
#include "Visual/SessionInfo.hpp"
#include "Visual/TargetHUD.hpp"
#include "Visual/TeamHealthBars.hpp"
#include "Visual/Tracers.hpp"
#include "Visual/UpdateForm.hpp"
#include "Visual/VSync.hpp"
#include "Visual/ViewModel.hpp"
#include "Visual/Watermark.hpp"
#include "Visual/Zoom.hpp"

void ModuleManager::init() {
  // Visual (must be initialized first)
  mModules.emplace_back(std::make_shared<HudEditor>());

  // Combat
  mModules.emplace_back(std::make_shared<Aura>());
  mModules.emplace_back(std::make_shared<TriggerBot>());
  mModules.emplace_back(std::make_shared<AutoClicker>());
  mModules.emplace_back(std::make_shared<Reach>());
  mModules.emplace_back(std::make_shared<Criticals>());
  mModules.emplace_back(std::make_shared<InfiniteAura>());
  mModules.emplace_back(std::make_shared<InfiniteChestAura>());
  mModules.emplace_back(std::make_shared<Aimbot>());
  mModules.emplace_back(std::make_shared<ArrowTP>());
  mModules.emplace_back(std::make_shared<PearlStopper>());
  mModules.emplace_back(std::make_shared<GhostMode>());
  mModules.emplace_back(std::make_shared<HitBoxes>());

  // Movement
  mModules.emplace_back(std::make_shared<Fly>());
  mModules.emplace_back(std::make_shared<Velocity>());
  mModules.emplace_back(std::make_shared<NoSlowDown>());
  mModules.emplace_back(std::make_shared<AntiImmobile>());
  mModules.emplace_back(std::make_shared<Sprint>());
  mModules.emplace_back(std::make_shared<Speed>());
  mModules.emplace_back(std::make_shared<InventoryMove>());
  mModules.emplace_back(std::make_shared<SafeWalk>());
  mModules.emplace_back(std::make_shared<NoJumpDelay>());
  mModules.emplace_back(std::make_shared<Step>());
  mModules.emplace_back(std::make_shared<LongJump>());
  mModules.emplace_back(std::make_shared<Spider>());
  mModules.emplace_back(std::make_shared<ServerSneak>());
  mModules.emplace_back(std::make_shared<AutoSneak>());
  mModules.emplace_back(std::make_shared<AirJump>());
  mModules.emplace_back(std::make_shared<TargetStrafe>());
  mModules.emplace_back(std::make_shared<Jesus>());
  mModules.emplace_back(std::make_shared<ReverseStep>());
  mModules.emplace_back(std::make_shared<Jetpack>());
  mModules.emplace_back(std::make_shared<AutoPatrol>());

  // Player
  mModules.emplace_back(std::make_shared<Timer>());
  mModules.emplace_back(std::make_shared<ChestStealer>());
  mModules.emplace_back(std::make_shared<InvManager>());
  mModules.emplace_back(std::make_shared<PlayerTracker>());
  mModules.emplace_back(std::make_shared<Regen>());
  mModules.emplace_back(std::make_shared<SchematicBuilder>());
  mModules.emplace_back(std::make_shared<NoClip>());
  mModules.emplace_back(std::make_shared<ContainerActions>());
  mModules.emplace_back(std::make_shared<RegionFill>());
  mModules.emplace_back(std::make_shared<OreMinerV2>());
  mModules.emplace_back(std::make_shared<AutoArmor>());
  mModules.emplace_back(std::make_shared<Scaffold>());
  mModules.emplace_back(std::make_shared<Nuker>());
  mModules.emplace_back(std::make_shared<OreMiner>());
  mModules.emplace_back(std::make_shared<AutoTool>());
  mModules.emplace_back(std::make_shared<MidclickAction>());
  mModules.emplace_back(std::make_shared<Freecam>());
  mModules.emplace_back(std::make_shared<NoFall>());
  mModules.emplace_back(std::make_shared<Teams>());
  mModules.emplace_back(std::make_shared<AntiVoid>());
  mModules.emplace_back(std::make_shared<FastMine>());
  mModules.emplace_back(std::make_shared<ClickTp>());
  mModules.emplace_back(std::make_shared<ChestAura>());
  mModules.emplace_back(std::make_shared<NoRotate>());
  mModules.emplace_back(std::make_shared<FastEat>());
  mModules.emplace_back(std::make_shared<SpawnerAura>());
  mModules.emplace_back(std::make_shared<ChainTP>());
  mModules.emplace_back(std::make_shared<AutoLoot>());
  mModules.emplace_back(std::make_shared<Cage>());

  // Misc
  mModules.emplace_back(std::make_shared<ToggleSounds>());
  mModules.emplace_back(std::make_shared<PacketLogger>());
  mModules.emplace_back(std::make_shared<ArmorSlotUnlock>());
  mModules.emplace_back(std::make_shared<DeviceSpoof>());
  mModules.emplace_back(std::make_shared<ItemUseDelayFix>());
  mModules.emplace_back(std::make_shared<KickSounds>());
  mModules.emplace_back(std::make_shared<AntiBot>());
  mModules.emplace_back(std::make_shared<AntiCheatDetector>());
  mModules.emplace_back(std::make_shared<FakeChat>());
  mModules.emplace_back(std::make_shared<Friends>());
  mModules.emplace_back(std::make_shared<NoPacket>());
  mModules.emplace_back(std::make_shared<NoFilter>());
  mModules.emplace_back(std::make_shared<AutoMessage>());
  mModules.emplace_back(std::make_shared<Killsults>());
  mModules.emplace_back(std::make_shared<NetSkip>());
  mModules.emplace_back(std::make_shared<Disabler>());
  mModules.emplace_back(std::make_shared<StaffAlert>());
  mModules.emplace_back(std::make_shared<JavaInventoryHotkeys>());
  mModules.emplace_back(std::make_shared<Spammer>());
  mModules.emplace_back(std::make_shared<SkinStealer>());
  mModules.emplace_back(std::make_shared<AutoDodge>());
  mModules.emplace_back(std::make_shared<TestModule>());
  mModules.emplace_back(std::make_shared<SkinChanger>());
  mModules.emplace_back(std::make_shared<SpectatorDetector>());
  mModules.emplace_back(std::make_shared<PlayerLogger>());
  mModules.emplace_back(std::make_shared<ItemDupe>());
  mModules.emplace_back(std::make_shared<SkinSpoofer>());

  // Visual
  mModules.emplace_back(std::make_shared<Watermark>());
  mModules.emplace_back(std::make_shared<ClickGui>());
  mModules.emplace_back(std::make_shared<Interface>());
  mModules.emplace_back(std::make_shared<Arraylist>());
  mModules.emplace_back(std::make_shared<LevelInfo>());
  mModules.emplace_back(std::make_shared<PlayerHUD>());
  mModules.emplace_back(std::make_shared<InventoryHUD>());
  mModules.emplace_back(std::make_shared<Notifications>());
  mModules.emplace_back(std::make_shared<HitColor>());
  mModules.emplace_back(std::make_shared<DestroyProgress>());
  mModules.emplace_back(std::make_shared<ESP>());
  mModules.emplace_back(std::make_shared<BlockESP>());
  mModules.emplace_back(std::make_shared<MotionBlur>());
  mModules.emplace_back(std::make_shared<PlayerPositionOffset>());
  mModules.emplace_back(std::make_shared<Animations>());
  mModules.emplace_back(std::make_shared<NoCameraClip>());
  mModules.emplace_back(std::make_shared<RobloxCamera>());
  mModules.emplace_back(std::make_shared<TargetHUD>());
  mModules.emplace_back(std::make_shared<ItemESP>());
  mModules.emplace_back(std::make_shared<ArmorHUD>());
  mModules.emplace_back(std::make_shared<Nametags>());
  mModules.emplace_back(std::make_shared<CustomCrosshair>());
  mModules.emplace_back(std::make_shared<NoHurtcam>());
  mModules.emplace_back(std::make_shared<FullBright>());
  mModules.emplace_back(std::make_shared<ViewModel>());
  mModules.emplace_back(std::make_shared<SessionInfo>());
  mModules.emplace_back(std::make_shared<ReachCounter>());
  mModules.emplace_back(std::make_shared<OpponentReach>());
  mModules.emplace_back(std::make_shared<VSync>());
  mModules.emplace_back(std::make_shared<Tracers>());
  mModules.emplace_back(std::make_shared<ChinaHat>());
  mModules.emplace_back(std::make_shared<NameProtect>());
  mModules.emplace_back(std::make_shared<Zoom>());
  mModules.emplace_back(std::make_shared<Glint>());
  mModules.emplace_back(std::make_shared<NoDebuff>());
  mModules.emplace_back(std::make_shared<Freelook>());
  mModules.emplace_back(std::make_shared<NoRender>());
  mModules.emplace_back(std::make_shared<Kagune>());
  mModules.emplace_back(std::make_shared<Ambience>());
  mModules.emplace_back(std::make_shared<AmbientCubes>());
  mModules.emplace_back(std::make_shared<EntityRadar>());
  mModules.emplace_back(std::make_shared<AfterImage>());
  mModules.emplace_back(std::make_shared<PathTracer>());
  mModules.emplace_back(std::make_shared<FakePlayer>());
  mModules.emplace_back(std::make_shared<CustomFog>());
  mModules.emplace_back(std::make_shared<PotionHUD>());
  mModules.emplace_back(std::make_shared<NearbyPlayers>());
  mModules.emplace_back(std::make_shared<EnemyIndicator>());

  mModules.emplace_back(std::make_shared<ProjectileTracer>());
  mModules.emplace_back(std::make_shared<DamageLog>());
  mModules.emplace_back(std::make_shared<HealthBar>());
  mModules.emplace_back(std::make_shared<TeamHealthBars>());
  mModules.emplace_back(std::make_shared<HealthDisplay>());

#ifdef __PRIVATE_BUILD__
  mModules.emplace_back(std::make_shared<SkinBlinker>());
  mModules.emplace_back(std::make_shared<Anticheat>());
#endif

#ifdef __DEBUG__
  mModules.emplace_back(std::make_shared<AutoPath>());
  mModules.emplace_back(std::make_shared<Goofy>());
#endif

  // Determine if we should add UpdateForm
  std::string oldHash = OAuthUtils::getLastCommitHash();
  std::string latestHash = SOLSTICE_BUILD_VERSION;
  if (oldHash != latestHash && oldHash != "") {
    spdlog::info("Adding UpdateForm module, oldHash: {}, latestHash: {}",
                 oldHash, latestHash);
    mModules.emplace_back(std::make_shared<UpdateForm>());
  } else {
    spdlog::info("Not adding UpdateForm module, oldHash: {}, latestHash: {}",
                 oldHash, latestHash);
  }

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
  j["client"] = "Solstice";
  j["version"] = SOLSTICE_VERSION;
  j["modules"] = nlohmann::json::array();

  for (const auto &module : mModules) {
    j["modules"].push_back(module->serialize());
  }

  return j;
}

nlohmann::json ModuleManager::serializeModule(Module *module) {
  nlohmann::json j;
  j["client"] = "Solstice";
  j["version"] = SOLSTICE_VERSION;
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
  std::string currentVersion = SOLSTICE_VERSION;

  if (version != currentVersion) {
    spdlog::warn("Config version mismatch. Expected: {}, Got: {}",
                 currentVersion, version);
    ChatUtils::displayClientMessage(
        "§eWarning: The specified config is from a different version of "
        "Solstice. §cSome settings may not be loaded§e.");
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
            // Конфиг НЕ содержит customData → СБРОС К ДЕФОЛТАМ!
            // Это фиксит баг когда данные от старого конфига "протекают"
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