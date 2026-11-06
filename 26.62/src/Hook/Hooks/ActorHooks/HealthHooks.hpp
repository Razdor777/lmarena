#pragma once
#include <Hook/Hook.hpp>
//
// Хуки урона: EntityHurtEvent. Без этого не работают DamageLog и LastTP
// (оба уже слушают событие), а также всё, что считает входящий урон.
//

class HealthHooks : public Hook {
public:
    HealthHooks() : Hook() {
        mName = "Actor::hurt";
    }

    static std::unique_ptr<Detour> mDetourEntityHurt;

    // По прологу: RCX = Actor* (this), RDX = ActorDamageSource*, R8d = int damage,
    // R9b = bool knockback, стек = bool ignites. Возвращает bool.
    static bool onEntityHurt(class Actor* actor, class ActorDamageSource* source,
                             int damage, bool knockback, bool ignites);

    void init() override;
};
