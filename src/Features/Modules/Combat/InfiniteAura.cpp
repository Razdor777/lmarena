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
#include <SDK/Minecraft/Network/Packets/ActorEventPacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MobEquipmentPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/SetActorMotionPacket.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
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
  mGhostHistoryCount = 0;
  mLastLockedHealth = -1.f;
  mLastHpDropTime = 0;
  mLastTargetRuntimeID = -1;
  mLastTargetSwitch = 0;
  mLockedTargetRuntimeID = -1;
  mLockedTargetName.clear();
  mIsBindingLockKey = false;
  mBindWaitRelease = false;
  mRecentAttackers.clear();
  mLastAttacksByID.clear();
  mHitTiming.clear();
  mLockKeyStates.clear();
  mLastPositions.clear();
  dbgReset();

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
  mGhostHistoryCount = 0;
  mLastLockedHealth = -1.f;
  mLastHpDropTime = 0;
  mLockedTargetRuntimeID = -1;
  mLockedTargetName.clear();
  mIsBindingLockKey = false;
  mBindWaitRelease = false;
  mRecentAttackers.clear();
  mLastAttacksByID.clear();
  mHitTiming.clear();
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

  // ── ИСПРАВЛЕНИЕ: yaw атакующего, знаки оригинальные ──
  float playerYaw = 0.f;
  auto playerRot = player->getActorRotationComponent();
  if (playerRot) playerYaw = playerRot->mYaw;

  float yawRad = playerYaw * (PI / 180.0f);
  float offset = mKBOffset.mValue;
  glm::vec3 forward = {-sinf(yawRad), 0.f, cosf(yawRad)};
  glm::vec3 right   = {cosf(yawRad), 0.f, sinf(yawRad)};

  switch (mKBDirection.mValue) {
  // Удар сзади → цель летит назад (ко мне)
  case KBDirection::PushBack:    return targetPos + forward * offset;
  // Удар спереди → цель летит вперёд (прочь)
  case KBDirection::PushForward: return targetPos - forward * offset;
  // Удар справа → цель летит влево
  case KBDirection::PushLeft:    return targetPos + right * offset;
  // Удар слева → цель летит вправо
  case KBDirection::PushRight:   return targetPos - right * offset;
  case KBDirection::Custom: {
    float rad = (playerYaw + mKBCustomAngle.mValue + 180.f) * (PI / 180.0f);
    return targetPos + glm::vec3(-sinf(rad) * offset, 0.f, cosf(rad) * offset);
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
  // Match Nametags: the live player registry/components are authoritative.
  // isDead()/health can remain stale for remotely streamed players.
  if (!actor->isPlayer()) return false;
  if (!actor->getAABBShapeComponent() || !actor->getRenderPositionComponent()) return false;

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

  // Тайминг живёт столько же, сколько и история ударов по цели.
  for (auto it = mHitTiming.begin(); it != mHitTiming.end();)
  {
    const uint64_t stamp = std::max(it->second.lastSent, it->second.lastConfirmed);
    it = (stamp != 0 && NOW - stamp > 30000) ? mHitTiming.erase(it) : ++it;
  }

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
    // призрак НЕ гасим — он должен дорисовать свой эффект (см. onRenderEvent)
    mHasTarget = false;
    return;
  }

  auto sender = ClientInstance::get()->getPacketSender();
  if (!sender) return;

  bool targetFound = false;
  glm::vec3 savedPlayerPos = *player->getPos();

  for (auto actor : actors) {
    if (!isActorSafeToUse(actor)) continue;

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

    // ── Тайминг по подтверждениям ────────────────────────────────────
    // Пока предыдущий удар по этой цели не подтверждён (или пока сервер
    // ещё держит иммунитет после попадания) — не бьём. Раньше здесь был
    // один общий таймер CPS: 20 ударов в секунду, из которых сервер
    // засчитывал 3 (у остальных не было урона — они пришли в окно
    // неуязвимости и были выброшены).
    {
      const uint64_t nowTick = static_cast<uint64_t>(NOW);
      uint64_t waitMs = 0;
      if (!canAttackTarget(actorID, nowTick, waitMs))
      {
        targetFound = true;   // цель есть, просто бить рано
        if (mDebug.mValue)
        {
          std::lock_guard lock(mDebugMutex);
          mDbgNextAt = nowTick + waitMs;
        }
        continue;
      }
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
        if (attackPkt) {
          sender->sendToServer(attackPkt.get());
          auto& timing = mHitTiming[actorID];
          timing.lastSent = static_cast<uint64_t>(NOW);
          timing.pending = true;
          timing.sentSinceConfirm++;
          dbgRecordSent(actorID);
        }
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

    // История ударов для шлейфа (новые точки — в конце, старые вытесняются).
    if (mHitTrail.mValue) {
      const int maxTrail =
          std::clamp(static_cast<int>(mTrailPoints.mValue), 1, kMaxTrailPoints);
      if (mGhostHistoryCount >= maxTrail) {
        for (int i = 1; i < mGhostHistoryCount; i++)
          mGhostHistory[i - 1] = mGhostHistory[i];
        mGhostHistoryCount = maxTrail - 1;
      }
      mGhostHistory[mGhostHistoryCount++] = attackPos;
    } else if (mGhostHistoryCount != 0) {
      mGhostHistoryCount = 0;
    }
    mLastTargetRuntimeID = actorID;
    mLastTargetSwitch = NOW;
    mLastAttacksByID[actorID] = NOW;
    targetFound = true;

    if (mMode.mValue == Mode::Single || mMode.mValue == Mode::Switch) break;
  }

  mHasTarget = targetFound;
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

  // HURT приходит на того, кому сервер реально засчитал урон. Это единственный
  // честный сигнал «удар дошёл»: по нему и строится весь тайминг (а не по CPS).
  if (event.mPacket && event.mPacket->getId() == PacketID::ActorEvent) {
    auto ev = event.getPacket<ActorEventPacket>();
    if (ev && ev->mEvent == ActorEvent::HURT) {
      onHitConfirmed(ev->mRuntimeID);
      if (mDebug.mValue) dbgRecordHit(ev->mRuntimeID);
    }
  }

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
// DEBUG: РЕАЛЬНАЯ ЗАДЕРЖКА УДАРОВ (Task 16)
// ═══════════════════════════════════════════════════════════════
//
// Нажатие (ActorUtils::createAttackTransaction) — это ещё не удар: сервер может
// его проглотить (не тот тик, дистанция, античит, дебафф). Подтверждение —
// ActorEventPacket HURT по той же цели. Между ними лежит настоящая задержка,
// и именно она объясняет, почему 20 CPS за 5 секунд — не 100 ударов.
//
// Считаем: сколько отправлено, сколько подтверждено, задержку (последнюю /
// среднюю / мин / макс) и интервал между подтверждениями; отдельно — histogram
// задержек и реальный APS по подтверждённым ударам за последние 5 секунд.

// Можно ли бить эту цель прямо сейчас.
//   - удар отправлен и ещё не подтверждён → ждём (второй удар всё равно
//     придёт в окно неуязвимости и будет выброшен);
//   - сервер подтвердил попадание → ждём измеренный интервал до следующего.
bool InfiniteAura::canAttackTarget(int64_t targetId, uint64_t now, uint64_t& waitMs)
{
  waitMs = 0;

  auto it = mHitTiming.find(targetId);
  if (it == mHitTiming.end()) return true;   // по этой цели ещё не били

  HitTiming& t = it->second;

  if (t.pending)
  {
    // Сколько ждать ответа: по измеренной задержке, а пока её нет —
    // по темпу CPS (чтобы не зависнуть, если сервер молчит).
    const float cap = t.confirmMs > 0.f
        ? std::clamp(t.confirmMs * 2.2f, 250.f, 1500.f)
        : std::max(120.f, 1000.f / std::max(1.f, mAPS.mValue));

    const uint64_t age = now - t.lastSent;
    if (age < static_cast<uint64_t>(cap))
    {
      waitMs = static_cast<uint64_t>(cap) - age;
      return false;
    }

    // Ответа нет — удар потерян. Значит, рано: отодвигаем темп и пробуем снова.
    t.pending = false;
    t.intervalMs = std::min(kMaxHurtMs, std::max(t.intervalMs, kHurtCooldownMs) + 70.f);
    t.nextAt = now + static_cast<uint64_t>(std::max(120.f, t.intervalMs));

    if (mDebug.mValue)
    {
      std::lock_guard lock(mDebugMutex);
      mDbgPaceMs = t.intervalMs;
      mDbgNextAt = t.nextAt;
    }
    return false;
  }

  if (t.nextAt > now)
  {
    waitMs = t.nextAt - now;
    return false;
  }

  return true;
}

// Сервер подтвердил урон по цели. Обновляем измеренный темп и ставим
// следующий удар чуть раньше конца иммунитета — так он приходит ровно в
// тот тик, когда цель снова можно бить (то, что делает скиловый игрок).
void InfiniteAura::onHitConfirmed(int64_t targetId)
{
  auto it = mHitTiming.find(targetId);
  if (it == mHitTiming.end() || !it->second.pending) return;   // не наше попадание

  HitTiming& t = it->second;
  const uint64_t now = static_cast<uint64_t>(NOW);

  // Сглаживаем: одиночный выброс не должен ломать темп.
  const float dt = static_cast<float>(now > t.lastSent ? now - t.lastSent : 0);
  t.confirmMs = t.confirmMs > 0.f ? (t.confirmMs * 0.6f + dt * 0.4f) : dt;

  // Темп между ударами — не константа, а то, что подстраивается по ответам
  // сервера: попали с первого раза — можно на 20 мс раньше, потеряли удар —
  // на 70 мс позже. Так он сам сходится к самому быстрому темпу, при котором
  // КАЖДЫЙ удар засчитывается (у скилового игрока это ~2 удара в секунду).
  if (t.intervalMs <= 0.f)
  {
    const float measured = (t.lastConfirmed != 0 && now > t.lastConfirmed)
        ? static_cast<float>(now - t.lastConfirmed)
        : kHurtCooldownMs;
    t.intervalMs = std::clamp(measured, kHurtCooldownMs, kMaxHurtMs);
  }
  else if (t.sentSinceConfirm <= 1)
  {
    t.intervalMs = std::max(kHurtCooldownMs, t.intervalMs - 20.f);
  }

  t.lastConfirmed = now;
  t.pending = false;
  t.sentSinceConfirm = 0;

  const float jitter = MathUtils::randomFloat(-35.f, 35.f);
  float next = t.intervalMs + jitter;

  // Настройка APS — это ПОТОЛОК, а не цель: чаще неё не бьём никогда.
  next = std::max(next, 1000.f / std::max(1.f, mAPS.mValue));
  t.nextAt = now + static_cast<uint64_t>(std::max(next, 120.f));

  if (mDebug.mValue)
  {
    std::lock_guard lock(mDebugMutex);
    mDbgPaceMs = t.intervalMs;
    mDbgNextAt = t.nextAt;
  }
}

void InfiniteAura::dbgReset()
{
  std::lock_guard lock(mDebugMutex);
  mDebugQueueCount = 0;
  mDbgSent = mDbgConfirmed = mDbgLost = mDbgForeign = 0;
  mDbgLatSum = 0;
  mDbgLastMs = mDbgAvgMs = mDbgMinMs = mDbgMaxMs = 0.f;
  for (int& h : mDbgHist) h = 0;
  mDbgConfirmCount = 0;
  mDbgLastConfirmAt = 0;
  mDbgLastIntervalMs = mDbgAvgIntervalMs = 0.f;
  mDbgIntervalSum = 0;
  mDbgRealAps = 0.f;
  mDbgPaceMs = 0.f;
  mDbgNextAt = 0;
}

void InfiniteAura::dbgRecordSent(int64_t targetId)
{
  std::lock_guard lock(mDebugMutex);
  if (mDebugQueueCount >= kDebugQueueSize)
  {
    // Старому нажатию уже нечего ждать — оно потерялось в очереди.
    for (int i = 1; i < mDebugQueueCount; i++) mDebugQueue[i - 1] = mDebugQueue[i];
    mDebugQueueCount--;
    mDbgLost++;
  }
  mDebugQueue[mDebugQueueCount++] = { static_cast<uint64_t>(NOW), targetId };
  mDbgSent++;
}

void InfiniteAura::dbgRecordHit(int64_t targetId)
{
  std::lock_guard lock(mDebugMutex);

  int found = -1;
  for (int i = 0; i < mDebugQueueCount; i++)
  {
    if (mDebugQueue[i].target == targetId) { found = i; break; }
  }
  if (found < 0)
  {
    mDbgForeign++;   // урон не от наших нажатий (или по другой цели)
    return;
  }

  const uint64_t at = mDebugQueue[found].at;
  for (int i = found + 1; i < mDebugQueueCount; i++) mDebugQueue[i - 1] = mDebugQueue[i];
  mDebugQueueCount--;

  const uint64_t now = static_cast<uint64_t>(NOW);
  const uint64_t dt = now > at ? now - at : 0;
  if (dt > 1500)
  {
    mDbgLost++;      // ответ на давнее нажатие — это не «наш» удар
    return;
  }

  mDbgConfirmed++;
  mDbgLatSum += dt;
  mDbgLastMs = static_cast<float>(dt);
  mDbgAvgMs = static_cast<float>(mDbgLatSum / mDbgConfirmed);
  if (mDbgMinMs == 0.f || mDbgLastMs < mDbgMinMs) mDbgMinMs = mDbgLastMs;
  if (mDbgLastMs > mDbgMaxMs) mDbgMaxMs = mDbgLastMs;

  const int bucket = dt < 50 ? 0 : dt < 100 ? 1 : dt < 200 ? 2 : dt < 500 ? 3 : 4;
  mDbgHist[bucket]++;

  // Интервал между подтверждёнными ударами — самый честный показатель
  // «сколько ударов в секунду реально засчитывает сервер».
  if (mDbgLastConfirmAt != 0 && now > mDbgLastConfirmAt)
  {
    mDbgLastIntervalMs = static_cast<float>(now - mDbgLastConfirmAt);
    mDbgIntervalSum += static_cast<uint64_t>(mDbgLastIntervalMs);
    mDbgAvgIntervalMs = static_cast<float>(mDbgIntervalSum / (mDbgConfirmed - 1));
  }
  mDbgLastConfirmAt = now;

  if (mDbgConfirmCount < kDebugConfirmWindow) mDbgConfirmTimes[mDbgConfirmCount++] = now;
  else
  {
    for (int i = 1; i < kDebugConfirmWindow; i++) mDbgConfirmTimes[i - 1] = mDbgConfirmTimes[i];
    mDbgConfirmTimes[kDebugConfirmWindow - 1] = now;
  }

  int last5s = 0;
  for (int i = 0; i < mDbgConfirmCount; i++)
    if (now - mDbgConfirmTimes[i] <= 5000) last5s++;
  mDbgRealAps = static_cast<float>(last5s) / 5.f;
}

void InfiniteAura::dbgRender(ImDrawList* drawList)
{
  if (!drawList) return;
  if (!ImGui::GetCurrentContext()) return;

  uint64_t sent = 0, confirmed = 0, lost = 0, foreign = 0;
  float last = 0.f, avg = 0.f, mn = 0.f, mx = 0.f, lastInt = 0.f, avgInt = 0.f, aps = 0.f;
  float pace = 0.f;
  uint64_t nextIn = 0;
  int hist[5]{};
  {
    std::lock_guard lock(mDebugMutex);
    sent = mDbgSent; confirmed = mDbgConfirmed; lost = mDbgLost; foreign = mDbgForeign;
    last = mDbgLastMs; avg = mDbgAvgMs; mn = mDbgMinMs; mx = mDbgMaxMs;
    lastInt = mDbgLastIntervalMs; avgInt = mDbgAvgIntervalMs; aps = mDbgRealAps;
    pace = mDbgPaceMs;
    nextIn = mDbgNextAt > static_cast<uint64_t>(NOW) ? mDbgNextAt - static_cast<uint64_t>(NOW) : 0;
    for (int i = 0; i < 5; i++) hist[i] = mDbgHist[i];
  }

  ImGuiIO& io = ImGui::GetIO();
  ImFont* font = ImGui::GetFont();
  if (!font) return;

  const float px = 13.f;
  const float pad = 10.f;
  const float lineH = px + 5.f;
  const char* labels[10];
  char text[10][128]{};

  const float pct = sent ? (100.f * static_cast<float>(confirmed) / static_cast<float>(sent)) : 0.f;
  snprintf(text[0], sizeof(text[0]), "InfiniteAura delay");
  snprintf(text[1], sizeof(text[1]), "sent %llu   hit %llu (%.0f%%)   lost %llu",
           static_cast<unsigned long long>(sent), static_cast<unsigned long long>(confirmed), pct,
           static_cast<unsigned long long>(lost));
  snprintf(text[2], sizeof(text[2]), "delay  last %.0f  avg %.0f  min %.0f  max %.0f ms",
           last, avg, mn, mx);
  snprintf(text[3], sizeof(text[3]), "interval  last %.0f  avg %.0f ms", lastInt, avgInt);
  snprintf(text[4], sizeof(text[4]), "real APS %.1f   (requested %.1f)", aps, mAPS.mValue);
  snprintf(text[5], sizeof(text[5]), "0-50   %d", hist[0]);
  snprintf(text[6], sizeof(text[6]), "50-100 %d", hist[1]);
  snprintf(text[7], sizeof(text[7]), "100-200 %d", hist[2]);
  snprintf(text[8], sizeof(text[8]), "200+   %d", hist[3] + hist[4]);
  snprintf(text[9], sizeof(text[9]), "pace   %.0f ms   next hit in %llu ms",
           pace, static_cast<unsigned long long>(nextIn));

  const int lines = 10;
  const float w = 268.f;
  const float h = pad * 2.f + lineH * lines;

  ImVec2 p = { 12.f, io.DisplaySize.y - h - 12.f };
  drawList->AddRectFilled(p, { p.x + w, p.y + h }, IM_COL32(10, 11, 17, 225), 6.f);
  drawList->AddRect(p, { p.x + w, p.y + h }, IM_COL32(255, 255, 255, 26), 6.f);

  for (int i = 0; i < lines; i++)
  {
    const ImU32 col = i == 0 ? IM_COL32(150, 226, 172, 255)
                    : i == 4 ? IM_COL32(240, 200, 120, 255)
                             : IM_COL32(226, 229, 236, 235);
    drawList->AddText(font, px, { p.x + pad, p.y + pad + lineH * static_cast<float>(i) }, col, text[i]);
  }
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
        // свечение под тропой: широкая полупрозрачная линия, основная — поверх
        for (size_t i = 0; i + 1 < points.size(); i++) {
          ImColor g = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
          g.Value.w = 0.16f * alphaMultiplier;
          drawList->AddLine(points[i], points[i + 1], g, mPathWidth.mValue * 3.f);
        }
        for (size_t i = 0; i + 1 < points.size(); i++) {
          ImColor c = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
          c.Value.w *= alphaMultiplier;
          drawList->AddLine(points[i], points[i + 1], c, mPathWidth.mValue);
        }

        // наконечник в конце тропы — показывает, куда уходит телепорт
        if (points.size() >= 2) {
          const ImVec2 tip = points.back();
          const ImVec2 prev = points[points.size() - 2];
          float dx = tip.x - prev.x;
          float dy = tip.y - prev.y;
          const float segLen = std::sqrt(dx * dx + dy * dy);
          if (segLen > 2.f) {
            dx /= segLen;
            dy /= segLen;
            const float headLen = 9.f + mPathWidth.mValue * 2.f;
            const float halfW = headLen * 0.55f;
            const ImVec2 back(tip.x - dx * headLen, tip.y - dy * headLen);
            const ImVec2 left(back.x - dy * halfW, back.y + dx * halfW);
            const ImVec2 right(back.x + dy * halfW, back.y - dx * halfW);
            ImColor hc = ColorUtils::getThemedColor(static_cast<float>(points.size()) * 0.05f);
            hc.Value.w = alphaMultiplier;
            drawList->AddTriangleFilled(tip, left, right, hc);
          }
        }

        // «Поток» — точки, бегущие по тропе от игрока к цели. Показывает
        // направление телепорта в моменте, а не только сам маршрут.
        if (mPathFlow.mValue && points.size() >= 2) {
          std::vector<float> segmentLengths;
          segmentLengths.reserve(points.size() - 1);
          float totalLength = 0.f;
          for (size_t i = 0; i + 1 < points.size(); i++) {
            const float dx = points[i + 1].x - points[i].x;
            const float dy = points[i + 1].y - points[i].y;
            const float len = std::sqrt(dx * dx + dy * dy);
            segmentLengths.push_back(len);
            totalLength += len;
          }

          if (totalLength > 1.f) {
            constexpr int kDots = 9;
            const float flow = static_cast<float>(currentTime % 1000) / 1000.f;
            for (int k = 0; k < kDots; k++) {
              float travel = std::fmod(flow + static_cast<float>(k) / kDots, 1.f) * totalLength;
              for (size_t i = 0; i < segmentLengths.size(); i++) {
                const bool last = (i + 1 == segmentLengths.size());
                if (travel > segmentLengths[i] && !last) {
                  travel -= segmentLengths[i];
                  continue;
                }
                const float t = segmentLengths[i] > 0.f
                                    ? std::clamp(travel / segmentLengths[i], 0.f, 1.f)
                                    : 0.f;
                const ImVec2 dot(points[i].x + (points[i + 1].x - points[i].x) * t,
                                 points[i].y + (points[i + 1].y - points[i].y) * t);
                ImColor dc = ColorUtils::getThemedColor(static_cast<float>(k) * 0.08f);
                dc.Value.w = alphaMultiplier;
                drawList->AddCircleFilled(dot, mPathWidth.mValue + 1.f, dc, 10);
                break;
              }
            }
          }
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

  // ── GHOST: призрак атаки + эффект удара ──
  // Живёт по своему таймеру (не гаснет вместе с тропой), дышит, бьёт вспышкой
  // и пускает рябь по земле. Новых настроек нет — всё завязано на Draw Ghost.
  if (mDrawGhost.mValue && mGhostVisible && mLastAttack != 0) {
    const float ageMs   = static_cast<float>(NOW - mLastAttack);
    const float kLife   = 1400.f;  // общий срок жизни эффекта
    const float kRise   = 90.f;    // всплеск сразу после удара
    const float kFall   = 700.f;   // затухание хвоста
    const float kRipple = 620.f;   // рябь по земле
    const float kFlash  = 150.f;   // вспышка удара

    if (ageMs <= kLife) {
      // огибающая: всплеск → держимся → гасим хвост
      float env = 1.f;
      if (ageMs < kRise)      env = ageMs / kRise;
      if (ageMs > kLife - kFall) env = std::max(0.f, (kLife - ageMs) / kFall);
      // «дыхание» бокса — лёгкая пульсация, чтобы не выглядело статикой
      const float pulse = 0.82f + 0.18f * std::sin(ageMs * (2.f * PI / 1000.f));
      const float a = env * pulse;

      const bool isLocked = (mLockedTargetRuntimeID != -1 && mHighlightLocked.mValue);
      ImColor base = isLocked ? ImColor(255, 215, 0, 255)
                              : ColorUtils::getStaticAccentColor(0);

      glm::vec3 feet = mGhostPos - glm::vec3(0.f, 1.62f, 0.f);
      AABB gAABB;
      gAABB.mMin = feet - glm::vec3(0.3f, 0.f, 0.3f);
      gAABB.mMax = feet + glm::vec3(0.3f, 1.8f, 0.3f);
      auto pts = MathUtils::getImBoxPoints(gAABB);

      if (!pts.empty()) {
        ImColor glow = base;
        glow.Value.w = 0.22f * a;
        ImColor fill = base;
        fill.Value.w = 0.12f * a;
        ImColor outline = base;
        outline.Value.w = 0.9f * a;

        // свечение: широкая мягкая линия под контуром, сверху яркая
        drawList->AddPolyline(pts.data(), (int)pts.size(), glow, true, 5.f);
        drawList->AddConvexPolyFilled(pts.data(), (int)pts.size(), fill);
        drawList->AddPolyline(pts.data(), (int)pts.size(), outline, true, 2.f);
      }

      // шлейф: призраки прошлых ударов, чем старше — тем бледнее и тоньше.
      // Последняя точка истории — это текущий удар, поэтому её пропускаем.
      if (mHitTrail.mValue && mGhostHistoryCount > 1) {
        for (int i = 0; i + 1 < mGhostHistoryCount; i++) {
          // история[0] — самый старый удар, он и должен быть самым бледным
          const float age = static_cast<float>(mGhostHistoryCount - 1 - i) /
                            static_cast<float>(mGhostHistoryCount);
          const float trailAlpha = (1.f - age) * 0.55f * env;
          if (trailAlpha <= 0.02f) continue;

          const glm::vec3 trailFeet = mGhostHistory[i] - glm::vec3(0.f, 1.62f, 0.f);
          AABB trailBox;
          trailBox.mMin = trailFeet - glm::vec3(0.3f, 0.f, 0.3f);
          trailBox.mMax = trailFeet + glm::vec3(0.3f, 1.8f, 0.3f);
          auto trailPts = MathUtils::getImBoxPoints(trailBox);
          if (trailPts.empty()) continue;

          ImColor tc = base;
          tc.Value.w = trailAlpha;
          drawList->AddPolyline(trailPts.data(), (int)trailPts.size(), tc, true, 1.5f);

          ImVec2 a, b;
          if (RenderUtils::worldToScreen(trailFeet, a) &&
              RenderUtils::worldToScreen(mGhostHistory[i], b)) {
            ImColor lc2 = base;
            lc2.Value.w = trailAlpha * 0.6f;
            drawList->AddLine(a, b, lc2, 1.f);
          }
        }
      }

      // искры: разлетаются из точки удара и гаснут за ~0.4 с
      if (mHitSparks.mValue && ageMs < 420.f) {
        const float p = ageMs / 420.f;
        ImVec2 sparkCenter;
        if (RenderUtils::worldToScreen(mGhostPos, sparkCenter)) {
          constexpr int kSparks = 12;
          for (int i = 0; i < kSparks; i++) {
            const float ang = (static_cast<float>(i) / kSparks) * 6.2831853f + p * 2.2f;
            const float r0 = 6.f + 30.f * p;
            const float r1 = r0 + 8.f + 16.f * (1.f - p);
            ImColor sc = base;
            sc.Value.w = (1.f - p) * 0.85f;
            drawList->AddLine(ImVec2(sparkCenter.x + std::cos(ang) * r0,
                                     sparkCenter.y + std::sin(ang) * r0),
                              ImVec2(sparkCenter.x + std::cos(ang) * r1,
                                     sparkCenter.y + std::sin(ang) * r1),
                              sc, 1.5f);
          }
        }
      }

      // луч «игрок → точка атаки» + яркая точка в конце
      ImVec2 ps, gs;
      if (RenderUtils::worldToScreen(*player->getPos(), ps) &&
          RenderUtils::worldToScreen(mGhostPos, gs)) {
        ImColor lc = base;
        lc.Value.w = 0.45f * env;
        drawList->AddLine(ps, gs, lc, 1.5f);

        ImColor dot = base;
        dot.Value.w = 0.9f * a;
        drawList->AddCircleFilled(gs, 3.5f, dot, 16);
      }

      // рябь по земле — расходится от точки удара (как в JumpCircles)
      if (ageMs < kRipple) {
        const float p = ageMs / kRipple;
        const float radius = 0.35f + 1.9f * p;
        const int kSeg = 24;
        std::vector<ImVec2> ring;
        ring.reserve(kSeg);
        for (int i = 0; i < kSeg; i++) {
          const float ang = static_cast<float>(i) / static_cast<float>(kSeg) * 2.f * PI;
          const glm::vec3 wp(mGhostPos.x + std::cos(ang) * radius, feet.y,
                             mGhostPos.z + std::sin(ang) * radius);
          ImVec2 sp;
          if (RenderUtils::worldToScreen(wp, sp)) ring.push_back(sp);
        }
        if (ring.size() >= 3) {
          ImColor rc = base;
          rc.Value.w = (1.f - p) * 0.85f;
          drawList->AddPolyline(ring.data(), (int)ring.size(), rc, true, 2.f);
        }
      }

      // вспышка удара — белый крест в точке атаки
      if (ageMs < kFlash) {
        const float p = ageMs / kFlash;
        ImVec2 c;
        if (RenderUtils::worldToScreen(mGhostPos, c)) {
          const float len = 5.f + 15.f * p;
          ImColor fc = base;
          fc.Value.x = fc.Value.y = fc.Value.z = 1.f;
          fc.Value.w = (1.f - p) * 0.85f;
          drawList->AddLine(ImVec2(c.x - len, c.y), ImVec2(c.x + len, c.y), fc, 1.5f);
          drawList->AddLine(ImVec2(c.x, c.y - len), ImVec2(c.x, c.y + len), fc, 1.5f);
        }
      }

      // подпись под боксом — дистанция до точки атаки
      ImVec2 lp;
      if (RenderUtils::worldToScreen(
              glm::vec3(mGhostPos.x, feet.y - 0.15f, mGhostPos.z), lp)) {
        const std::string label = fmt::format(
            "{:.1f}m", glm::distance(*player->getPos(), mGhostPos));
        const ImVec2 sz = ImGui::CalcTextSize(label.c_str());
        ImColor tc = base;
        tc.Value.w = 0.95f * env;
        ImRenderUtils::drawShadowText(drawList, label,
                                      ImVec2(lp.x - sz.x * 0.5f, lp.y), tc, 15.f);
      }
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

    // ── Сколько у него осталось и не прилетело ли только что ──
    // Скачок HP вниз подкрашивает бокс красным: видно, что удар дошёл.
    float lockedHp = 20.f;
    try { lockedHp = locked->getHealth(); } catch (...) { lockedHp = 20.f; }
    if (mLastLockedHealth >= 0.f && lockedHp < mLastLockedHealth - 0.01f)
      mLastHpDropTime = NOW;
    mLastLockedHealth = lockedHp;

    const bool hpFlashed = mLastHpDropTime != 0 && (NOW - (int64_t)mLastHpDropTime) < 350;
    const ImU32 boxColor = hpFlashed ? IM_COL32(255, 95, 95, 235) : IM_COL32(255, 215, 0, 180);

    if (!pts.empty()) {
      drawList->AddPolyline(pts.data(), (int)pts.size(), boxColor, true,
                            hpFlashed ? 4.f : 3.f);

      // прицел под ногами цели: три вращающиеся дуги, краснеют при ударе.
      // Аргумент считаем в double, иначе NOW (ms от эпохи) теряет точность во float.
      {
        constexpr float kRingR = 0.7f;
        constexpr int kArcs = 3;
        constexpr int kArcSeg = 6;
        const float spin = static_cast<float>(
            std::fmod(static_cast<double>(NOW) / 1000.0 * 1.6, 2.0 * PI));
        const ImU32 ringCol = hpFlashed ? IM_COL32(255, 110, 110, 230)
                                        : IM_COL32(255, 215, 0, 200);
        const float arcSpan = (2.f * PI / kArcs) * 0.6f;
        for (int arc = 0; arc < kArcs; arc++) {
          const float start = spin + arc * (2.f * PI / kArcs);
          ImVec2 seg[kArcSeg + 1];
          bool allOk = true;
          for (int s = 0; s <= kArcSeg && allOk; s++) {
            const float ang = start + arcSpan * static_cast<float>(s) / kArcSeg;
            const glm::vec3 wp(feet.x + std::cos(ang) * kRingR, feet.y + 0.02f,
                               feet.z + std::sin(ang) * kRingR);
            allOk = RenderUtils::worldToScreen(wp, seg[s]);
          }
          // если хоть одна точка за экраном — дугу пропускаем целиком, без «прыжков»
          if (allOk) drawList->AddPolyline(seg, kArcSeg + 1, ringCol, false, 2.f);
        }
      }

      if (mTargetInfo.mValue) {
        float minX = pts[0].x, maxX = pts[0].x, minY = pts[0].y, maxY = pts[0].y;
        for (const auto& p : pts) {
          minX = std::min(minX, p.x);
          maxX = std::max(maxX, p.x);
          minY = std::min(minY, p.y);
          maxY = std::max(maxY, p.y);
        }

        // Уголки — «рамка» вокруг цели, не мешает обзору внутри бокса.
        const float len = std::clamp((maxX - minX) * 0.25f, 6.f, 26.f);
        const float th  = 2.f;
        const ImU32 cornerCol = hpFlashed ? IM_COL32(255, 120, 120, 255)
                                          : IM_COL32(255, 235, 150, 230);
        const ImVec2 corners[4][3] = {
            { {minX, minY + len}, {minX, minY}, {minX + len, minY} },
            { {maxX - len, minY}, {maxX, minY}, {maxX, minY + len} },
            { {maxX, maxY - len}, {maxX, maxY}, {maxX - len, maxY} },
            { {minX + len, maxY}, {minX, maxY}, {minX, maxY - len} }
        };
        for (const auto& c : corners) {
          drawList->AddLine(c[0], c[1], cornerCol, th);
          drawList->AddLine(c[1], c[2], cornerCol, th);
        }

        // HP-бар над боксом (у игроков максимум 20 HP).
        const float barW = std::max(24.f, maxX - minX);
        const float barH = 4.f;
        const float barY = minY - 9.f;
        const float hpFrac = std::clamp(lockedHp / 20.f, 0.f, 1.f);
        const ImU32 hpColor = hpFrac > 0.6f ? IM_COL32(120, 255, 120, 235)
                            : hpFrac > 0.3f ? IM_COL32(255, 215, 0, 235)
                                            : IM_COL32(255, 80, 80, 235);
        drawList->AddRectFilled(ImVec2(minX, barY), ImVec2(minX + barW, barY + barH),
                                IM_COL32(0, 0, 0, 170), 2.f);
        drawList->AddRectFilled(ImVec2(minX, barY), ImVec2(minX + barW * hpFrac, barY + barH),
                                hpColor, 2.f);
      }
    }

    ImVec2 screenPos;
    if (RenderUtils::worldToScreen(targetPos + glm::vec3(0.f, 0.5f, 0.f), screenPos)) {
      std::string lockText;
      try { lockText = "Locked: " + locked->getRawName() + "  [" + getKeyName(mLockTargetKey) + "]"; }
      catch (...) { lockText = "Locked  [" + std::string(getKeyName(mLockTargetKey)) + "]"; }

      if (mTargetInfo.mValue) {
        float dist = 0.f;
        try { dist = locked->distanceTo(player); } catch (...) { dist = 0.f; }
        const int armor = getArmorPieces(locked);
        lockText += fmt::format("  {:.1f}m  HP {:.0f}/20  Armor {}", dist, lockedHp, armor);
      }

      ImVec2 textSize = ImGui::CalcTextSize(lockText.c_str());
      screenPos.x -= textSize.x * 0.5f;
      screenPos.y -= textSize.y + 5.f;

      drawList->AddRectFilled({screenPos.x - 4, screenPos.y - 2}, {screenPos.x + textSize.x + 4, screenPos.y + textSize.y + 2}, IM_COL32(0, 0, 0, 140), 3.f);
      drawList->AddText(screenPos, hpFlashed ? IM_COL32(255, 130, 130, 255) : IM_COL32(255, 215, 0, 255), lockText.c_str());
    }
  } else {
    // Цели нет — забываем её HP, чтобы следующая цель не «мигала» с чужого урона.
    mLastLockedHealth = -1.f;
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

  // DEBUG: реальная задержка ударов (см. dbgRender выше).
  if (mDebug.mValue) dbgRender(drawList);
}