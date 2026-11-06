//
// HealthHooks — единственный источник EntityHurtEvent в 26.62.
//
// В сборке под 1.21.44 здесь стояли два жёстко прописанных адреса
// (base+0x23A09B0 / base+0x23A0B80). Такие адреса живут ровно один билд, поэтому
// теперь оба берутся из SigManager. Пока сигнатуры не сняты с бинарника 26.62,
// хука просто не будет (см. tools/sig_scan_checklist.py) — но игра не упадёт,
// потому что детур не ставится на ноль.
//

#include "HealthHooks.hpp"

#include <Features/Events/EntityHurtEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/SigManager.hpp>

std::unique_ptr<Detour> HealthHooks::mDetourEntityHurt;

bool HealthHooks::onEntityHurt(Actor* actor, ActorDamageSource* source,
                               int damage, bool knockback, bool ignites) {
    auto original = mDetourEntityHurt->getOriginal<&HealthHooks::onEntityHurt>();

    if (actor) {
        auto holder = nes::make_holder<EntityHurtEvent>(actor, source, damage,
                                                      knockback, ignites);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    return original(actor, source, damage, knockback, ignites);
}

void HealthHooks::init() {
    if (!SigManager::Actor_hurt) {
        spdlog::warn("[DamageLog] Actor::hurt signature not resolved - EntityHurtEvent disabled");
        return;
    }

    mDetourEntityHurt = std::make_unique<Detour>(
        "Actor::hurt", reinterpret_cast<void*>(SigManager::Actor_hurt),
        &HealthHooks::onEntityHurt);
    mDetourEntityHurt->enable();
}
