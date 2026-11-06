#pragma once
#include <Hook/Hook.hpp>

class HealthHooks : public Hook {
public:
  static std::unique_ptr<Detour> mDetourEntityHurt;
  static std::unique_ptr<Detour> mDetourEntityHealthChanged;

  // Assumptions:
  // RCX: Actor* (this)
  // RDX: ActorDamageSource*
  // R8: int damage
  // R9: bool knockback
  // Stack: bool ignites
  static bool onEntityHurt(class Actor *actor, class ActorDamageSource *source,
                           int damage, bool knockback, bool ignites);

  // EntityHealthChanged often: void onHealthChanged(Actor* actor, int
  // oldHealth, int newHealth, AttributeModifier modifier) But check arguments
  // based on 1.21.44 User says: "entityHealthChanged sub_1423A0B80" We'll hook
  // it but proceed with caution on args
  static void onEntityHealthChanged(class Actor *actor, int oldHealth,
                                    int newHealth,
                                    const class AttributeModifier &modifier);

  void init() override;
};
