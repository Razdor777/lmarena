#include "InfiniteAura.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>

#include <Features/Events/PacketInEvent.hpp>
#include <Features/Modules/Misc/Friends.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MobEquipmentPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/SetActorMotionPacket.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// ═══════════════════════════════════════════════════════════════
// SAFE ACTOR CHECK
// ═══════════════════════════════════════════════════════════════

bool InfiniteAura::isActorSafeToUse(Actor *actor) {
  if (!actor)
    return false;

  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return false;

  if (!player->mContext.mRegistry->valid(actor->mContext.mEntityId))
    return false;

  return true;
}

// ═══════════════════════════════════════════════════════════════
// KEY NAME
// ═══════════════════════════════════════════════════════════════

const char *InfiniteAura::getKeyName(int vk) {
  static char buf[32];
  switch (vk) {
  case 0: return "None";
  case VK_LBUTTON: return "LMB";
  case VK_RBUTTON: return "RMB";
  case VK_MBUTTON: return "MMB";
  case VK_BACK: return "Back";
  case VK_TAB: return "Tab";
  case VK_RETURN: return "Enter";
  case VK_SHIFT: return "Shift";
  case VK_CONTROL: return "Ctrl";
  case VK_MENU: return "Alt";
  case VK_ESCAPE: return "Esc";
  case VK_SPACE: return "Space";
  case VK_DELETE: return "Del";
  case VK_INSERT: return "Ins";
  case VK_HOME: return "Home";
  case VK_END: return "End";
  case VK_CAPITAL: return "CapsLock";
  default: break;
  }
  if (vk >= VK_F1 && vk <= VK_F12) { snprintf(buf, 32, "F%d", vk - VK_F1 + 1); return buf; }
  if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) { snprintf(buf, 32, "Num%d", vk - VK_NUMPAD0); return buf; }
  if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) { snprintf(buf, 32, "%c", (char)vk); return buf; }
  snprintf(buf, 32, "0x%02X", vk);
  return buf;
}

// ═══════════════════════════════════════════════════════════════
// KEY HELPERS
// ═══════════════════════════════════════════════════════════════

bool InfiniteAura::sIsAnyKeyHeld() {
  for (int vk = 8; vk <= 254; vk++) {
    if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) continue;
    if (vk == VK_LSHIFT || vk == VK_RSHIFT) continue;
    if (vk == VK_LCONTROL || vk == VK_RCONTROL) continue;
    if (vk == VK_LMENU || vk == VK_RMENU) continue;
    if (vk == VK_LWIN || vk == VK_RWIN) continue;
    if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
    if (vk == VK_ESCAPE) continue;
    if (GetAsyncKeyState(vk) & 0x8000) return true;
  }
  return false;
}

int InfiniteAura::sFindHeldKey() {
  for (int vk = 8; vk <= 254; vk++) {
    if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) continue;
    if (vk == VK_LSHIFT || vk == VK_RSHIFT) continue;
    if (vk == VK_LCONTROL || vk == VK_RCONTROL) continue;
    if (vk == VK_LMENU || vk == VK_RMENU) continue;
    if (vk == VK_LWIN || vk == VK_RWIN) continue;
    if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
    if (vk == VK_ESCAPE) continue;
    if (GetAsyncKeyState(vk) & 0x8000) return vk;
  }
  return -1;
}

// ═══════════════════════════════════════════════════════════════
// ENABLE / DISABLE
// ═══════════════════════════════════════════════════════════════

void InfiniteAura::onEnable() {
  gFeatureManager->mDispatcher
      ->listen<BaseTickEvent, &InfiniteAura::onBaseTickEvent>(this);
  gFeatureManager->mDispatcher
      ->listen<PacketOutEvent, &InfiniteAura::onPacketOutEvent,
               nes::event_priority::ABSOLUTE_LAST>(this);
  gFeatureManager->mDispatcher
      ->listen<PacketInEvent, &InfiniteAura::onPacketInEvent>(this);
  gFeatureManager->mDispatcher
      ->listen<RenderEvent, &InfiniteAura::onRenderEvent>(this);

  mHasTarget = false;
  mGhostVisible = false;
  mLastTargetRuntimeID = -1;
  mLastTargetSwitch = 0;
  mLockedTargetRuntimeID = -1;
  mLockedTargetName.clear();
  mIsBindingLockKey = false;
  mBindWaitRelease = false;
  mRecentAttackers.clear();
  mLastAttacksByID.clear();
  mLockKeyStates.clear();
  mLastPositions.clear();

  auto player = ClientInstance::get()->getLocalPlayer();
  if (player) {
    mLastHealth = player->getHealth();
    auto rot = player->getActorRotationComponent();
    if (rot)
      mRots = {rot->mPitch, rot->mYaw, rot->mYaw};
  }
}

void InfiniteAura::onDisable() {
  gFeatureManager->mDispatcher
      ->deafen<BaseTickEvent, &InfiniteAura::onBaseTickEvent>(this);
  gFeatureManager->mDispatcher
      ->deafen<PacketOutEvent, &InfiniteAura::onPacketOutEvent>(this);
  gFeatureManager->mDispatcher
      ->deafen<PacketInEvent, &InfiniteAura::onPacketInEvent>(this);
  gFeatureManager->mDispatcher
      ->deafen<RenderEvent, &InfiniteAura::onRenderEvent>(this);

  mHasTarget = false;
  mGhostVisible = false;
  mLockedTargetRuntimeID = -1;
  mLockedTargetName.clear();
  mIsBindingLockKey = false;
  mBindWaitRelease = false;
  mRecentAttackers.clear();
  mLastAttacksByID.clear();
  mLastPositions.clear();
}

// ═══════════════════════════════════════════════════════════════
// SERIALIZATION
// ═══════════════════════════════════════════════════════════════

nlohmann::json InfiniteAura::serializeCustomData() {
  nlohmann::json j;
  j["lockTargetKey"] = mLockTargetKey;
  return j;
}

void InfiniteAura::deserializeCustomData(const nlohmann::json &j) {
  if (j.is_null() || j.empty()) return;
  if (j.contains("lockTargetKey") && j["lockTargetKey"].is_number())
    mLockTargetKey = j["lockTargetKey"].get<int>();
}

// ═══════════════════════════════════════════════════════════════
// LOCK KEY
// ═══════════════════════════════════════════════════════════════

bool InfiniteAura::isLockKeyJustPressed(int vk) {
  if (vk <= 0) return false;
  bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
  bool was = mLockKeyStates[vk];
  mLockKeyStates[vk] = down;
  return down && !was;
}

// ═══════════════════════════════════════════════════════════════
// GET LOCKED TARGET
// ═══════════════════════════════════════════════════════════════

Actor *InfiniteAura::getLockedTarget() {
  if (mLockedTargetRuntimeID == -1) return nullptr;
  Actor *actor = ActorUtils::getActorFromRuntimeID(mLockedTargetRuntimeID);
  if (!actor) return nullptr;
  if (!isActorSafeToUse(actor)) return nullptr;
  return actor;
}

void InfiniteAura::clearLockedTarget() { mLockedTargetRuntimeID = -1; }

// ═══════════════════════════════════════════════════════════════
// RAYCAST TO ACTOR
// ═══════════════════════════════════════════════════════════════

Actor *InfiniteAura::raycastToActor(float maxAngle) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player) return nullptr;

  auto rot = player->getActorRotationComponent();
  if (!rot) return nullptr;

  float yaw = rot->mYaw * (PI / 180.f);
  float pitch = rot->mPitch * (PI / 180.f);

  glm::vec3 lookDir(-sinf(yaw) * cosf(pitch), -sinf(pitch), cosf(yaw) * cosf(pitch));
  lookDir = glm::normalize(lookDir);

  glm::vec3 playerPos = *player->getPos();
  auto actors = ActorUtils::getActorList(true, true);

  Actor *bestActor = nullptr;
  float bestAngle = maxAngle;
  float bestDist = FLT_MAX;

  for (auto actor : actors) {
    if (!isValidTarget(actor, player)) continue;

    glm::vec3 toActor = *actor->getPos() - playerPos;
    float dist = glm::length(toActor);
    if (dist < 0.01f) continue;

    toActor = glm::normalize(toActor);
    float dot = std::clamp(glm::dot(lookDir, toActor), -1.f, 1.f);
    float angle = acosf(dot) * (180.f / PI);

    if (angle < bestAngle) {
      bestAngle = angle;
      bestDist = dist;
      bestActor = actor;
    } else if (std::fabs(angle - bestAngle) < 0.01f && dist < bestDist) {
      bestDist = dist;
      bestActor = actor;
    }
  }

  return bestActor;
}

// ═══════════════════════════════════════════════════════════════
// IS LOCKED TARGET VALID
// ═══════════════════════════════════════════════════════════════

bool InfiniteAura::isLockedTargetValid() {
  if (mLockedTargetRuntimeID == -1) return false;

  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player) return false;

  Actor *locked = ActorUtils::getActorFromRuntimeID(mLockedTargetRuntimeID);
  if (!locked) return false;

  if (!isActorSafeToUse(locked)) return false;

  if (locked->isDead()) return false;
  if (locked->getHealth() <= 0.f) return false;

  if (!isValidTarget(locked, player)) return false;

  if (mAutoUnlock.mValue && !mInfiniteRange.mValue) {
    if (locked->distanceTo(player) > mMaxLockDistance.mValue) return false;
  }

  return true;
}

// ═══════════════════════════════════════════════════════════════
// FIND ACTOR BY NAME
// ═══════════════════════════════════════════════════════════════

Actor *InfiniteAura::findActorByLockedName() {
  if (mLockedTargetName.empty()) return nullptr;

  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player) return nullptr;

  auto actors = ActorUtils::getActorList(true, true);
  for (auto actor : actors) {
    if (!isActorSafeToUse(actor)) continue;
    if (actor == player) continue;
    if (actor->isDead()) continue;

    try {
      if (actor->getRawName() == mLockedTargetName) return actor;
    } catch (...) {}
  }
  return nullptr;
}

// ═══════════════════════════════════════════════════════════════
// UPDATE LOCK TARGET
// ═══════════════════════════════════════════════════════════════

void InfiniteAura::updateLockTarget() {
  if (mIsBindingLockKey) return;

  if (mLockedTargetRuntimeID != -1 && !isLockedTargetValid()) {
    Actor *newActor = findActorByLockedName();
    if (newActor && isActorSafeToUse(newActor)) {
      int64_t newID = newActor->getRuntimeID();
      if (newID != mLockedTargetRuntimeID) {
        mLockedTargetRuntimeID = newID;
        return;
      }
    }

    std::string reason = "lost";

    Actor *old = ActorUtils::getActorFromRuntimeID(mLockedTargetRuntimeID);
    if (old && isActorSafeToUse(old)) {
      bool dead = false;
      float hp = 1.f;
      try { dead = old->isDead(); } catch (...) {}
      try { hp = old->getHealth(); } catch (...) {}

      if (dead || hp <= 0.f) {
        reason = "dead";
      } else if (mAutoUnlock.mValue && !mInfiniteRange.mValue) {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (player) {
          float dist = 0.f;
          try { dist = old->distanceTo(player); } catch (...) {}
          if (dist > mMaxLockDistance.mValue) reason = "too far";
        }
      }
    }

    NotifyUtils::notify("§cTarget unlocked: " + reason, 2.f, Notification::Type::Warning);
    clearLockedTarget();
    mLockedTargetName.clear();
  }

  if (isLockKeyJustPressed(mLockTargetKey)) {
    Actor *raycastTarget = raycastToActor(mLockFOV.mValue);
    int64_t raycastID = (raycastTarget && isActorSafeToUse(raycastTarget))
                            ? raycastTarget->getRuntimeID() : -1;

    if (raycastID != -1 && raycastID == mLockedTargetRuntimeID) {
      clearLockedTarget();
      mLockedTargetName.clear();
      NotifyUtils::notify("§eTarget unlocked", 2.f, Notification::Type::Info);
      ClientInstance::get()->playUi("random.orb", 1.0f, 0.8f);
    } else if (raycastID != -1) {
      mLockedTargetRuntimeID = raycastID;
      try { mLockedTargetName = raycastTarget->getRawName(); } catch (...) { mLockedTargetName.clear(); }
      NotifyUtils::notify("§aLocked: §f" + mLockedTargetName, 2.5f, Notification::Type::Info);
      ClientInstance::get()->playUi("random.orb", 1.0f, 1.2f);
    } else {
      if (mLockedTargetRuntimeID != -1) {
        clearLockedTarget();
        mLockedTargetName.clear();
        NotifyUtils::notify("§eTarget unlocked", 2.f, Notification::Type::Info);
        ClientInstance::get()->playUi("random.orb", 1.0f, 0.8f);
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// CREATE MOVE PACKET
// ═══════════════════════════════════════════════════════════════

std::shared_ptr<MovePlayerPacket> InfiniteAura::createPacketForPos(glm::vec3 pos) {
  auto player = ClientInstance::get()->getLocalPlayer();
  auto packet = MinecraftPackets::createPacket<MovePlayerPacket>();
  packet->mPos = pos;
  packet->mPlayerID = player->getRuntimeID();
  packet->mRot = {mRots.x, mRots.y};
  packet->mYHeadRot = mRots.z;
  packet->mResetPosition = PositionMode::Teleport;
  packet->mOnGround = true;
  packet->mRidingID = -1;
  packet->mCause = TeleportationCause::Unknown;
  packet->mSourceEntityType = ActorType::Player;
  packet->mTick = 0;
  return packet;
}

// ═══════════════════════════════════════════════════════════════
// STRAIGHT LINE TP
// ═══════════════════════════════════════════════════════════════

void InfiniteAura::straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender) {
  auto sender = ClientInstance::get()->getPacketSender();
  if (!sender) return;

  float stepSize = mStepDistance.mValue;
  glm::vec3 diff = to - from;
  float totalDist = glm::length(diff);

  if (totalDist < 0.01f) {
    sender->sendToServer(createPacketForPos(to).get());
    return;
  }

  glm::vec3 direction = glm::normalize(diff);
  glm::vec3 currentPos = from;
  std::vector<glm::vec3> positions;

  while (glm::distance(currentPos, to) > stepSize) {
    currentPos += direction * stepSize;
    positions.push_back(currentPos);
    sender->sendToServer(createPacketForPos(currentPos).get());
  }

  positions.push_back(to);
  sender->sendToServer(createPacketForPos(to).get());

  if (saveForRender) {
    std::lock_guard<std::mutex> guard(mMutex);
    mPacketPositions = positions;
    mLastPathTime = NOW;
  }
}

// ═══════════════════════════════════════════════════════════════
// KB DIRECTION + POSITION PREDICTION
// ═══════════════════════════════════════════════════════════════

glm::vec3 InfiniteAura::getAttackPosition(Actor *target, Actor *player) {
  glm::vec3 targetPos = *target->getPos();
  int64_t targetID = target->getRuntimeID();

  glm::vec3 velocity(0.f);
  auto now = NOW;

  auto it = mLastPositions.find(targetID);
  if (it != mLastPositions.end()) {
    auto &prev = it->second;
    float dt = (float)(now - prev.timestamp) / 1000.f;
    if (dt > 0.01f && dt < 2.0f) {
      velocity = (targetPos - prev.position) / dt;
    }
  }

  mLastPositions[targetID] = {targetPos, (uint64_t)now};

  float dist = glm::distance(*player->getPos(), targetPos);
  float predictTicks = std::clamp(dist / 50.f, 0.1f, 0.35f);
  glm::vec3 predictedPos = targetPos + velocity * predictTicks;

  float maxPredictDist = 3.f;
  if (glm::distance(predictedPos, targetPos) > maxPredictDist) {
    glm::vec3 predDir = glm::normalize(predictedPos - targetPos);
    predictedPos = targetPos + predDir * maxPredictDist;
  }

  if (mKBDirection.mValue == KBDirection::Default) return predictedPos;

  float targetYaw = 0.f;
  auto targetRot = target->getActorRotationComponent();
  if (targetRot) targetYaw = targetRot->mYaw;

  float yawRad = targetYaw * (PI / 180.0f);
  float offset = mKBOffset.mValue;
  glm::vec3 forward = {-sinf(yawRad), 0.f, cosf(yawRad)};
  glm::vec3 right = {cosf(yawRad), 0.f, sinf(yawRad)};

  switch (mKBDirection.mValue) {
  case KBDirection::PushBack:   return predictedPos + forward * offset;
  case KBDirection::PushForward: return predictedPos - forward * offset;
  case KBDirection::PushLeft:   return predictedPos + right * offset;
  case KBDirection::PushRight:  return predictedPos - right * offset;
  case KBDirection::Custom: {
    float rad = (targetYaw + mKBCustomAngle.mValue + 180.f) * (PI / 180.0f);
    return predictedPos + glm::vec3(-sinf(rad) * offset, 0.f, cosf(rad) * offset);
  }
  default: return predictedPos;
  }
}

// ═══════════════════════════════════════════════════════════════
// BEST WEAPON
// ═══════════════════════════════════════════════════════════════

int InfiniteAura::getBestWeapon() {
  if (mSwapMode.mValue == SwapMode::None) return -1;
  return ItemUtils::getBestItem(SItemType::Sword, mHotbarOnly.mValue);
}

// ═══════════════════════════════════════════════════════════════
// ARMOR PIECES
// ═══════════════════════════════════════════════════════════════

int InfiniteAura::getArmorPieces(Actor *actor) {
  if (!isActorSafeToUse(actor)) return 0;
  try {
    auto container = actor->getArmorContainer();
    if (!container) return 0;
    int count = 0;
    for (int i = 0; i < 4; i++) {
      auto item = container->getItem(i);
      if (item && item->mItem) count++;
    }
    return count;
  } catch (...) { return 0; }
}

// ═══════════════════════════════════════════════════════════════
// SHIELD CHECK — по имени предмета (SItemType::Shield нет)
// ═══════════════════════════════════════════════════════════════

bool InfiniteAura::hasShieldInOffhand(Actor *target)
{
  if (!isActorSafeToUse(target)) return false;
  try {
    auto *offhand = target->getOffhandContainer();
    if (!offhand) return false;
    auto *stack = offhand->getItem(0);
    if (!stack || !stack->mItem) return false;
    auto *itemObj = stack->getItem();
    if (!itemObj) return false;
    std::string name = itemObj->mName;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return name.find("shield") != std::string::npos;
  } catch (...) {}
  return false;
}

// ═══════════════════════════════════════════════════════════════
// TARGET VALIDATION
// ═══════════════════════════════════════════════════════════════

bool InfiniteAura::isValidTarget(Actor *actor, Actor *player) {
  if (!actor || !player) return false;
  if (actor == player) return false;
  if (!isActorSafeToUse(actor)) return false;
  if (actor->isDead()) return false;

  float hp = 0.f;
  try { hp = actor->getHealth(); } catch (...) { return false; }
  if (hp <= 0.f) return false;

  if (!mInfiniteRange.mValue) {
    try { if (actor->distanceTo(player) > mRange.mValue) return false; } catch (...) { return false; }
  }

  if (mIgnoreFriends.mValue && gFriendManager && gFriendManager->isFriend(actor)) return false;

  if (mOnlySameY.mValue) {
    try {
      float yDiff = std::fabs(actor->getPos()->y - player->getPos()->y);
      if (yDiff > mYOffset.mValue) return false;
    } catch (...) { return false; }
  }

  return true;
}

// ═══════════════════════════════════════════════════════════════
// MAIN TICK
// ═══════════════════════════════════════════════════════════════

void InfiniteAura::onBaseTickEvent(BaseTickEvent &event) {
  auto player = event.mActor;
  if (!player) return;

  auto supplies = player->getSupplies();
  if (!supplies) return;

  if (mOnlyOnGround.mValue && !player->isOnGround()) return;

  // --------------------------------------------------
  // KEYBIND CAPTURE
  // --------------------------------------------------
  if (mIsBindingLockKey) {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
      mIsBindingLockKey = false;
      mBindWaitRelease = false;
      return;
    }
    if (mBindWaitRelease) {
      if (!sIsAnyKeyHeld()) mBindWaitRelease = false;
      return;
    }
    int pressed = sFindHeldKey();
    if (pressed > 0) {
      mLockTargetKey = pressed;
      mIsBindingLockKey = false;
      mBindWaitRelease = false;
      mLockKeyStates[pressed] = true;
      NotifyUtils::notify(std::string("§aLock key set to: §f") + getKeyName(pressed), 2.f, Notification::Type::Info);
    }
    return;
  }

  updateLockTarget();

  // --------------------------------------------------
  // HURT-BY TRACKING
  // --------------------------------------------------
  float currentHealth = player->getHealth();
  if (currentHealth < mLastHealth) {
    auto nearby = ActorUtils::getActorList(true, true);
    for (auto a : nearby) {
      if (!isActorSafeToUse(a) || a == player) continue;
      try { if (a->distanceTo(player) < 8.f) mRecentAttackers[a->getRuntimeID()] = NOW; } catch (...) {}
    }
  }
  mLastHealth = currentHealth;

  for (auto it = mRecentAttackers.begin(); it != mRecentAttackers.end();)
    it = (NOW - it->second > 10000) ? mRecentAttackers.erase(it) : ++it;

  for (auto it = mLastAttacksByID.begin(); it != mLastAttacksByID.end();)
    it = (NOW - it->second > 30000) ? mLastAttacksByID.erase(it) : ++it;

  for (auto it = mLastPositions.begin(); it != mLastPositions.end();)
    it = (NOW - it->second.timestamp > 5000) ? mLastPositions.erase(it) : ++it;

  // --------------------------------------------------
  // GET & FILTER TARGETS
  // --------------------------------------------------
  auto actors = ActorUtils::getActorList(true, true);
  actors.erase(std::remove_if(actors.begin(), actors.end(),
      [&](Actor *a) { return !isValidTarget(a, player); }), actors.end());

  // --------------------------------------------------
  // LOCK OVERRIDE
  // --------------------------------------------------
  if (mLockedTargetRuntimeID != -1 && isLockedTargetValid()) {
    Actor *locked = getLockedTarget();
    actors = locked ? std::vector<Actor *>{locked} : std::vector<Actor *>{};
  } else if (mLockedTargetRuntimeID != -1) {
    actors.clear();
  } else {
    // --------------------------------------------------
    // SORTING
    // --------------------------------------------------
    if (mMode.mValue == Mode::Switch) {
      std::ranges::sort(actors, [&](Actor *a, Actor *b) {
        uint64_t tA = mLastAttacksByID.count(a->getRuntimeID()) ? mLastAttacksByID[a->getRuntimeID()] : 0;
        uint64_t tB = mLastAttacksByID.count(b->getRuntimeID()) ? mLastAttacksByID[b->getRuntimeID()] : 0;
        return tA < tB;
      });
    } else {
      switch (mPriority.mValue) {
      case Priority::Closest:
        std::ranges::sort(actors, [&](Actor *a, Actor *b) {
          return a->distanceTo(player) < b->distanceTo(player);
        });
        break;
      case Priority::Armor:
        std::ranges::sort(actors, [&](Actor *a, Actor *b) {
          int armorA = getArmorPieces(a), armorB = getArmorPieces(b);
          if (armorA != armorB) return armorA > armorB;
          return a->distanceTo(player) < b->distanceTo(player);
        });
        break;
      case Priority::HurtBy:
        std::ranges::sort(actors, [&](Actor *a, Actor *b) {
          uint64_t tA = mRecentAttackers.count(a->getRuntimeID()) ? mRecentAttackers[a->getRuntimeID()] : 0;
          uint64_t tB = mRecentAttackers.count(b->getRuntimeID()) ? mRecentAttackers[b->getRuntimeID()] : 0;
          if (tA != tB) return tA > tB;
          return a->distanceTo(player) < b->distanceTo(player);
        });
        break;
      }
    }
  }

  // --------------------------------------------------
  // APS DELAY
  // --------------------------------------------------
  uint64_t delay = static_cast<uint64_t>(1000.f / mAPS.mValue);
  if (NOW - mLastAttack < delay) return;

  if (actors.empty()) {
    mHasTarget = mGhostVisible = false;
    return;
  }

  auto sender = ClientInstance::get()->getPacketSender();
  if (!sender) return;

  bool targetFound = false;
  glm::vec3 savedPlayerPos = *player->getPos();

  for (auto actor : actors) {
    if (!isActorSafeToUse(actor)) continue;
    if (actor->isDead()) continue;

    float hp = 0.f;
    try { hp = actor->getHealth(); } catch (...) { continue; }
    if (hp <= 0.f) continue;

    int64_t actorID = 0;
    try { actorID = actor->getRuntimeID(); } catch (...) { continue; }
    if (actorID == 0) continue;

    if (mMode.mValue == Mode::Switch && mSwitchDelay.mValue > 0) {
      if (actorID != mLastTargetRuntimeID && NOW - mLastTargetSwitch < static_cast<uint64_t>(mSwitchDelay.mValue))
        continue;
    }

    int bestWeapon = getBestWeapon();
    int oldSlot = supplies->mSelectedSlot;

    glm::vec3 attackPos = getAttackPosition(actor, player);

    // ── AutoShield ──
    if (mAutoShield.mValue) {
      bool isSneaking = false;
      try { isSneaking = actor->getStatusFlag(ActorFlags::Sneaking); } catch (...) {}

      if (isSneaking && hasShieldInOffhand(actor)) {
        // Priority 1: axe disables shield
        int axeSlot = ItemUtils::getBestItem(SItemType::Axe, mHotbarOnly.mValue);
        if (axeSlot != -1) {
          bestWeapon = axeSlot;
        } else {
          // Priority 2: attack from behind
          auto targetRot = actor->getActorRotationComponent();
          if (targetRot) {
            float yaw = targetRot->mYaw * (PI / 180.f);
            glm::vec3 behindDir(sinf(yaw), 0, -cosf(yaw));
            attackPos = *actor->getPos() + behindDir * 0.5f;
          }
        }
      }
    }

    // STEP 1: TP
    straightLineTP(savedPlayerPos, attackPos, true);

    // STEP 2: Criticals
    if (mCriticals.mValue) {
      auto upPkt = createPacketForPos(attackPos + glm::vec3(0.f, 0.1f, 0.f));
      upPkt->mOnGround = false;
      sender->sendToServer(upPkt.get());

      auto downPkt = createPacketForPos(attackPos);
      downPkt->mOnGround = false;
      sender->sendToServer(downPkt.get());
    }

    // STEP 3: Weapon
    if (bestWeapon != -1)
      sender->sendToServer(PacketUtils::createMobEquipmentPacket(bestWeapon).get());

    // STEP 4: Final check + attack
    bool shouldAttack = false;
    if (isActorSafeToUse(actor)) {
      try { shouldAttack = !actor->isDead() && actor->getHealth() > 0.f; } catch (...) {}
    }

    if (shouldAttack) {
      try {
        player->swing();
        auto attackPkt = ActorUtils::createAttackTransaction(actor, bestWeapon != -1 ? bestWeapon : supplies->mSelectedSlot);
        if (attackPkt) sender->sendToServer(attackPkt.get());
      } catch (...) {}
    }

    // STEP 5: Restore weapon
    if (bestWeapon != -1)
      sender->sendToServer(PacketUtils::createMobEquipmentPacket(oldSlot).get());

    // STEP 6: TP back
    if (mFollow.mValue)
      player->setPosition(*actor->getPos());
    else
      straightLineTP(attackPos, savedPlayerPos, false);

    mGhostPos = attackPos;
    mGhostVisible = true;
    mLastAttack = NOW;
    mLastTargetRuntimeID = actorID;
    mLastTargetSwitch = NOW;
    mLastAttacksByID[actorID] = NOW;
    targetFound = true;

    if (mMode.mValue == Mode::Single || mMode.mValue == Mode::Switch) break;
  }

  mHasTarget = targetFound;
  if (!targetFound) mGhostVisible = false;
}

// ═══════════════════════════════════════════════════════════════
// PACKET OUT
// ═══════════════════════════════════════════════════════════════

void InfiniteAura::onPacketOutEvent(PacketOutEvent &event) {
  if (event.mPacket->getId() == PacketID::MovePlayer) {
    auto packet = event.getPacket<MovePlayerPacket>();
    mRots = {packet->mRot.x, packet->mRot.y, packet->mYHeadRot};
  }
}

// ═══════════════════════════════════════════════════════════════
// PACKET IN
// ═══════════════════════════════════════════════════════════════

void InfiniteAura::onPacketInEvent(PacketInEvent &event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player) return;

  if (mFullVelocity.mValue && event.mPacket->getId() == PacketID::SetActorMotion) {
    auto motionPkt = std::reinterpret_pointer_cast<SetActorMotionPacket>(event.mPacket);
    if (motionPkt && motionPkt->mRuntimeID == player->getRuntimeID()) {
      event.setCancelled(true);
      return;
    }
  }

  if (!mSilentAccept.mValue) return;
  if (event.mPacket->getId() != PacketID::MovePlayer) return;

  auto packet = event.getPacket<MovePlayerPacket>();
  if (packet->mPlayerID != player->getRuntimeID()) return;

  event.cancel();
  ClientInstance::get()->getPacketSender()->sendToServer(packet.get());
}

// ═══════════════════════════════════════════════════════════════
// RENDER
// ═══════════════════════════════════════════════════════════════

void InfiniteAura::onRenderEvent(RenderEvent &event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player) return;

  uint64_t currentTime = NOW;
  uint64_t fadeTime = 500;
  float alphaMultiplier = 1.0f;

  {
    std::lock_guard<std::mutex> guard(mMutex);
    if (mLastPathTime + fadeTime < currentTime) mPacketPositions.clear();
    if (mLastPathTime + fadeTime > currentTime) {
      alphaMultiplier = 1.0f - static_cast<float>(currentTime - mLastPathTime) / static_cast<float>(fadeTime);
      alphaMultiplier = std::clamp(alphaMultiplier, 0.0f, 1.0f);
    }
  }

  auto drawList = ImGui::GetBackgroundDrawList();

  // PATH
  {
    std::lock_guard<std::mutex> guard(mMutex);
    if (mDrawPath.mValue && !mPacketPositions.empty()) {
      if (mRenderMode.mValue == RenderMode::Lines) {
        std::vector<ImVec2> points;
        for (auto &pos : mPacketPositions) {
          ImVec2 pt;
          if (RenderUtils::worldToScreen(pos, pt)) points.emplace_back(pt);
        }
        for (size_t i = 0; i + 1 < points.size(); i++) {
          ImColor c = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
          c.Value.w *= alphaMultiplier;
          drawList->AddLine(points[i], points[i + 1], c, 2.0f);
        }
      } else {
        for (auto &pos : mPacketPositions) {
          AABB aabb = AABB(pos, glm::vec3(0.2f));
          auto pts = MathUtils::getImBoxPoints(aabb);
          drawList->AddConvexPolyFilled(pts.data(), (int)pts.size(), IM_COL32(255, 0, 0, (int)(100 * alphaMultiplier)));
          drawList->AddPolyline(pts.data(), (int)pts.size(), IM_COL32(255, 0, 0, (int)(255 * alphaMultiplier)), true, 2.f);
        }
      }
    }
  }

  // GHOST
  if (mDrawGhost.mValue && mGhostVisible && alphaMultiplier > 0.01f) {
    glm::vec3 feet = mGhostPos - glm::vec3(0.f, 1.62f, 0.f);
    AABB gAABB;
    gAABB.mMin = feet - glm::vec3(0.3f, 0.f, 0.3f);
    gAABB.mMax = feet + glm::vec3(0.3f, 1.8f, 0.3f);
    auto pts = MathUtils::getImBoxPoints(gAABB);

    if (!pts.empty()) {
      bool isLocked = (mLockedTargetRuntimeID != -1);
      ImColor fill = isLocked && mHighlightLocked.mValue
                         ? ImColor(255, 215, 0, (int)(50 * alphaMultiplier))
                         : ColorUtils::getThemedColor(0);
      fill.Value.w = 0.2f * alphaMultiplier;

      ImColor outline = isLocked && mHighlightLocked.mValue
                            ? ImColor(255, 215, 0, (int)(200 * alphaMultiplier))
                            : ColorUtils::getThemedColor(0);
      outline.Value.w = 0.7f * alphaMultiplier;

      drawList->AddConvexPolyFilled(pts.data(), (int)pts.size(), fill);
      drawList->AddPolyline(pts.data(), (int)pts.size(), outline, true, 2.f);
    }

    ImVec2 ps, gs;
    if (RenderUtils::worldToScreen(*player->getPos(), ps) && RenderUtils::worldToScreen(mGhostPos, gs)) {
      ImColor lc = ColorUtils::getThemedColor(0);
      lc.Value.w = 0.4f * alphaMultiplier;
      drawList->AddLine(ps, gs, lc, 1.5f);
    }
  }

  // LOCKED TARGET HIGHLIGHT
  Actor *locked = getLockedTarget();
  if (locked && mHighlightLocked.mValue && isActorSafeToUse(locked)) {
    glm::vec3 targetPos = *locked->getPos();
    glm::vec3 feet = targetPos - glm::vec3(0.f, 1.62f, 0.f);

    AABB targetAABB;
    targetAABB.mMin = feet - glm::vec3(0.3f, 0.f, 0.3f);
    targetAABB.mMax = feet + glm::vec3(0.3f, 1.8f, 0.3f);
    auto pts = MathUtils::getImBoxPoints(targetAABB);

    if (!pts.empty())
      drawList->AddPolyline(pts.data(), (int)pts.size(), IM_COL32(255, 215, 0, 180), true, 3.f);

    ImVec2 screenPos;
    if (RenderUtils::worldToScreen(targetPos + glm::vec3(0.f, 0.5f, 0.f), screenPos)) {
      std::string lockText;
      try { lockText = "Locked: " + locked->getRawName() + "  [" + getKeyName(mLockTargetKey) + "]"; }
      catch (...) { lockText = "Locked  [" + std::string(getKeyName(mLockTargetKey)) + "]"; }

      ImVec2 textSize = ImGui::CalcTextSize(lockText.c_str());
      screenPos.x -= textSize.x * 0.5f;
      screenPos.y -= textSize.y + 5.f;

      drawList->AddRectFilled({screenPos.x - 4, screenPos.y - 2}, {screenPos.x + textSize.x + 4, screenPos.y + textSize.y + 2}, IM_COL32(0, 0, 0, 140), 3.f);
      drawList->AddText(screenPos, IM_COL32(255, 215, 0, 255), lockText.c_str());
    }
  }

  // BIND PANEL
  if (mShowBindPanel.mValue) {
    ImVec2 ss = ImGui::GetIO().DisplaySize;
    float panX = 20.f;
    float panY = ss.y - 80.f;

    std::string btnLabel;
    if (mIsBindingLockKey)
      btnLabel = mBindWaitRelease ? "Release keys..." : "Press key... (ESC = cancel)";
    else
      btnLabel = std::string("Lock Key: [") + getKeyName(mLockTargetKey) + "]";

    ImVec2 labelSize = ImGui::CalcTextSize(btnLabel.c_str());
    float btnW = labelSize.x + 24.f;
    float btnH = labelSize.y + 10.f;

    ImVec2 btnMin{panX, panY};
    ImVec2 btnMax{panX + btnW, panY + btnH};

    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool hovered = (mouse.x >= btnMin.x && mouse.x <= btnMax.x && mouse.y >= btnMin.y && mouse.y <= btnMax.y);
    bool clicked = hovered && ImGui::GetIO().MouseClicked[0];

    ImU32 bgColor = mIsBindingLockKey ? IM_COL32(200, 100, 0, 220)
                                      : (hovered ? IM_COL32(70, 70, 110, 220) : IM_COL32(30, 30, 50, 180));

    drawList->AddRectFilled(btnMin, btnMax, bgColor, 6.f);
    drawList->AddRect(btnMin, btnMax, mIsBindingLockKey ? IM_COL32(255, 150, 50, 255) : IM_COL32(100, 100, 160, 200), 6.f, 0, 1.5f);
    drawList->AddText({panX + (btnW - labelSize.x) * 0.5f, panY + (btnH - labelSize.y) * 0.5f}, IM_COL32(255, 255, 255, 255), btnLabel.c_str());

    if (clicked && !mIsBindingLockKey) {
      mIsBindingLockKey = true;
      mBindWaitRelease = true;
    }
  }
}