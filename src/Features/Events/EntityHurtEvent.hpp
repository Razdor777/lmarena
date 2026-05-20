#pragma once

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/EntityId.hpp>

// Minimal definition based on typical Bedrock structure
class ActorDamageSource {
public:
  virtual ~ActorDamageSource() = default;
  virtual bool isEntitySource() const = 0;
  virtual bool isChildEntitySource() const = 0;

  // We can assume offset 8 is damage cause enum
  // But importantly, we might want EntityId?
  // Actually, usually getEntity() is a virtual method or similar.
  // For now, let's treat it as opaque and hook into getEntity later if needed

  // According to reverse engineering:
  // virtual methods...
  // 0x18: EntityId mEntityId; (example)
  // Or getEntity() virtual function at index ~8?
};

struct EntityHurtEvent {
  class Actor *mEntity;
  class ActorDamageSource *mSource;
  int mDamage;
  bool mKnockback;
  bool mIgnites;
};
