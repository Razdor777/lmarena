#include "ItemTextures.hpp"
#include <Utils/Resources.hpp>
#include <Hook/Hooks/RenderHooks/D3DHook.hpp>
#include <algorithm>
#include <cctype>

static std::string toLowerStr(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return r;
}

void ItemTextures::init()
{
    if (sInitialized) return;
    if (ResourceLoader::Resources.empty()) {
        spdlog::warn("[ItemTextures] Resources not loaded yet, deferring...");
        return;
    }

    spdlog::info("[ItemTextures] Initializing...");

    static const char* itemNames[] = {
        "netherite_helmet", "netherite_chestplate", "netherite_leggings", "netherite_boots",
        "netherite_sword", "netherite_pickaxe", "netherite_axe", "netherite_shovel",
        "diamond_helmet", "diamond_chestplate", "diamond_leggings", "diamond_boots",
        "diamond_sword", "diamond_pickaxe", "diamond_axe", "diamond_shovel",
        "iron_helmet", "iron_chestplate", "iron_leggings", "iron_boots",
        "iron_sword", "iron_pickaxe", "iron_axe", "iron_shovel",
        "gold_helmet", "gold_chestplate", "gold_leggings", "gold_boots",
        "gold_sword", "gold_pickaxe", "gold_axe", "gold_shovel",
        "chainmail_helmet", "chainmail_chestplate", "chainmail_leggings", "chainmail_boots",
        "leather_chestplate", "leather_helmet", "leather_leggings", "leather_boots",
        "stone_sword", "stone_pickaxe", "stone_axe", "stone_shovel",
        "wood_sword", "wood_pickaxe", "wood_axe", "wood_shovel",
        "bow_standby", "crossbow_standby", "trident", "totem", "elytra",
        "turtle_helmet", "arrow", "apple", "apple_golden", "bread",
        "beef_cooked", "ender_pearl", "carrot_golden", "chicken_cooked", "cookie",
        "leather",
        "potion_bottle_drinkable", "potion_bottle_empty",
        "potion_bottle_heal", "potion_bottle_harm",
        "potion_bottle_fireresistance", "potion_bottle_movespeed",
        "potion_bottle_moveslowdown", "potion_bottle_regeneration",
        "potion_bottle_invisibility", "potion_bottle_nightvision",
        "potion_bottle_jump", "potion_bottle_poison",
        "potion_bottle_weakness", "potion_bottle_damageboost",
        "potion_bottle_slowfall", "potion_bottle_waterbreathing",
        "potion_bottle_turtlemaster",
        "potion_bottle_splash", "potion_bottle_lingering",
        "potion_bottle_splash_heal", "potion_bottle_splash_harm",
        "potion_bottle_splash_fireresistance", "potion_bottle_splash_movespeed",
        "potion_bottle_splash_moveslowdown", "potion_bottle_splash_regeneration",
        "potion_bottle_splash_invisibility", "potion_bottle_splash_nightvision",
        "potion_bottle_splash_jump", "potion_bottle_splash_poison",
        "potion_bottle_splash_weakness", "potion_bottle_splash_damageboost",
        "potion_bottle_splash_slowfall", "potion_bottle_splash_waterbreathing",
        "potion_bottle_splash_turtlemaster",
        "potion_bottle_splash_absorption", "potion_bottle_splash_blindness",
        "potion_bottle_splash_confusion", "potion_bottle_splash_digslowdown",
        "potion_bottle_splash_digspeed", "potion_bottle_splash_healthboost",
        "potion_bottle_splash_hunger", "potion_bottle_splash_levitation",
        "potion_bottle_splash_resistance", "potion_bottle_splash_saturation",
        "potion_bottle_splash_wither",
        "potion_bottle_lingering_heal", "potion_bottle_lingering_harm",
        "potion_bottle_lingering_fireresistance", "potion_bottle_lingering_movespeed",
        "potion_bottle_lingering_moveslowdown", "potion_bottle_lingering_regeneration",
        "potion_bottle_lingering_invisibility", "potion_bottle_lingering_nightvision",
        "potion_bottle_lingering_jump", "potion_bottle_lingering_poison",
        "potion_bottle_lingering_weakness", "potion_bottle_lingering_damageboost",
        "potion_bottle_lingering_slowfall", "potion_bottle_lingering_waterbreathing",
        "potion_bottle_lingering_turtlemaster",
        "potion_bottle_lingering_luck", "potion_bottle_lingering_wither",
        "potion_bottle_lingering_empty",
        "potion_bottle_absorption", "potion_bottle_blindness",
        "potion_bottle_confusion", "potion_bottle_digslowdown",
        "potion_bottle_digspeed", "potion_bottle_healthboost",
        "potion_bottle_hunger", "potion_bottle_levitation",
        "potion_bottle_resistance", "potion_bottle_saturation",
        "potion_bottle_wither",
        "potion_overlay",
    };

    static const char* effectNames[] = {
        "bad_omen_effect", "blindness_effect", "fire_resistance_effect",
        "haste_effect", "hunger_effect", "invisibility_effect",
        "jump_boost_effect", "levitation_effect", "mining_fatigue_effect",
        "nausea_effect", "night_vision_effect", "poison_effect",
        "regeneration_effect", "resistance_effect", "slowness_effect",
        "speed_effect", "strength_effect", "village_hero_effect",
        "water_breathing_effect", "weakness_effect", "wither_effect"
    };

    int loaded = 0;
    auto loadList = [&](const char* names[], int count) {
        for (int i = 0; i < count; i++) {
            const char* name = names[i];
            if (ResourceLoader::Resources.find(name) == ResourceLoader::Resources.end()) {
                spdlog::warn("[ItemTextures] Resource key '{}' not found", name);
                continue;
            }
            ID3D11ShaderResourceView* srv = nullptr;
            int width = 0, height = 0;
            if (D3DHook::loadTextureFromEmbeddedResource(name, &srv, &width, &height)) {
                sTextures[name] = { srv, width, height };
                loaded++;
            } else {
                spdlog::warn("[ItemTextures] Failed to create texture: {}", name);
            }
        }
    };

    loadList(itemNames, sizeof(itemNames)/sizeof(itemNames[0]));
    loadList(effectNames, sizeof(effectNames)/sizeof(effectNames[0]));

    spdlog::info("[ItemTextures] Loaded {} textures", loaded);
    if (loaded == 0) return;
    sInitialized = true;
}

void ItemTextures::shutdown()
{
    for (auto& [name, tex] : sTextures) {
        if (tex.srv) { tex.srv->Release(); tex.srv = nullptr; }
    }
    sTextures.clear();
    sInitialized = false;
}

ItemTextures::TextureData* ItemTextures::getTexture(const std::string& itemName)
{
    if (!sInitialized) return nullptr;
    std::string lower = toLowerStr(itemName);
    auto it = sTextures.find(lower);
    return (it != sTextures.end()) ? &it->second : nullptr;
}

ItemTextures::TextureData* ItemTextures::getTextureForItem(const std::string& fullItemName)
{
    std::string name = fullItemName;
    size_t colon = name.find(':');
    if (colon != std::string::npos) name = name.substr(colon + 1);
    name = toLowerStr(name);

    if (auto* t = getTexture(name)) return t;

    if (name.find("golden_") == 0)
        if (auto* t = getTexture("gold_" + name.substr(7))) return t;
    if (name.find("wooden_") == 0)
        if (auto* t = getTexture("wood_" + name.substr(7))) return t;

    if (name == "bow") return getTexture("bow_standby");
    if (name == "crossbow") return getTexture("crossbow_standby");
    if (name.find("totem") != std::string::npos) return getTexture("totem");
    if (name == "cooked_beef") return getTexture("beef_cooked");
    if (name == "cooked_chicken") return getTexture("chicken_cooked");
    if (name == "golden_apple" || name == "enchanted_golden_apple")
        return getTexture("apple_golden");
    if (name == "golden_carrot") return getTexture("carrot_golden");

    return nullptr;
}