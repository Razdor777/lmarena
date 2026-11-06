# -*- coding: utf-8 -*-
# Разрешение конфликтов слияния 26.62 (Dexko): свои фиксы + правки базы под 26.62
import re, pathlib

BASE = pathlib.Path('/home/user/lmarena/26.62')
ROOT = pathlib.Path('/home/user/lmarena')

PAT = re.compile(r'<<<<<<< /tmp/mine\n(.*?)\|\|\|\|\|\|\| /tmp/older\n(.*?)=======\n(.*?)>>>>>>> /tmp/yours\n?', re.S)

def norm(t):
    return t if (t.endswith('\n') or t == '') else t + '\n'

def resolve(path, res):
    p = BASE / path
    text = p.read_text(encoding='utf-8', errors='replace')
    idx = [0]
    def repl(m):
        i = idx[0]; idx[0] += 1
        r = res[i]
        if r == 'M': return m.group(1)
        if r == 'Y': return norm(m.group(3))
        return r
    new, n = PAT.subn(repl, text)
    assert n == len(res), f"{path}: найдено конфликтов {n}, ожидается {len(res)}"
    p.write_text(new, encoding='utf-8')
    print(f"OK  {path} ({n})")

RES = {
 # хуки: база поменяла сигнатуры/механику под 26.62 — берём базу, свои фичи добавляем
 'src/Hook/Hooks/ActorHooks/ActorModelHook.cpp': ['Y', 'Y'],
 'src/Hook/Hooks/RenderHooks/RenderItemInHandHook.cpp': ['Y'],
 'src/Hook/Hooks/MiscHooks/MouseHook.cpp': ['M', 'M'],

 # SDK: свои null-safe фиксы + правки базы (WeakPtr, оффсеты 26.62)
 'src/SDK/Minecraft/Actor/Actor.cpp': ['M', 'M', 'M', 'M'],
 'src/SDK/Minecraft/Inventory/ItemStack.hpp': ['Y', 'Y'],
 'src/SDK/Minecraft/Network/Packets/TextPacket.hpp': ['Y'],
}

resolve('src/Hook/Hooks/ActorHooks/BaseTickHook.cpp', ["""    if (ClientInstance::get()->getPacketSender()) {
        std::vector<QueuedPacket> packets;
        {
            std::lock_guard<std::mutex> lock(mPacketQueueMutex);
            packets.swap(mQueuedPackets);
        }
        for (auto& [mTime, mPacket, mBypassHook] : packets)
        {
            spdlog::trace("Sending packet with ID: {} [queued {}ms ago] [{}]", magic_enum::enum_name(mPacket->getId()), NOW - mTime, mBypassHook ? "bypassing hook" : "not bypassing hook");
            if (mBypassHook) ClientInstance::get()->getPacketSender()->sendToServer(mPacket.get());
            else ClientInstance::get()->getPacketSender()->send(mPacket.get());
        }
    }

    // Чат-бинды: клавишу ловит KeyHook, а сама отправка — здесь, в потоке
    // игры, как и обычный чат.
    ChatBinds::flushPending();
"""])

resolve('src/Hook/Hooks/MiscHooks/KeyHook.cpp', ["""
    // Чат-бинды (ClickGui -> Chat Binds): клавиша забиндена — текст ушёл в чат,
    // а игре клавиша не отдаётся ни нажатием, ни отпусканием (T иначе открыл бы
    // ещё и ванильный чат). Только в игре: в меню/чате/ClickGui не срабатывает.
    if (ChatBinds::handleKey(static_cast<int>(key), isDown))
        return;

    if (isDown) gGameSeenKeysDown.insert(static_cast<int>(key));
    else        gGameSeenKeysDown.erase(static_cast<int>(key));

    //oFunc(key, isDown);
    mCurrentCancel = false;
"""])

resolve('src/Hook/Hooks/MiscHooks/MouseHook.hpp', ["""    // Pending scroll delta: stored here so it can be injected AFTER ImGui::NewFrame()
    // (before NewFrame the value gets wiped by ImGui internals)
    static inline float mScrollDelta = 0.0f;
    static inline bool mCurrentCancel;
"""])

resolve('src/Hook/Hooks/RenderHooks/ItemRendererHook.cpp', ["""    if (_this) sItemRenderer = _this;
    if (ctx)   sBarContext   = ctx;

    auto holder = nes::make_holder<ItemRendererEvent>(_this, ctx, a3);
    gFeatureManager->mDispatcher->trigger(holder);

    return original(_this, ctx, a3, a4, a5, a6);
}"""])

resolve('src/SDK/Minecraft/Inventory/Item.hpp', ["""    // 26.62: оффсеты mItemId/mName обновлены по новой базе Solstice
    CLASS_FIELD(short, mItemId, 0xAA);
    CLASS_FIELD(std::string, mName, 0xD8);
    // TODO(26.62): оффсеты ниже остались от 1.21.44 — перепроверить на новой версии!
    CLASS_FIELD(int, mArmorItemType, 0x244);
    CLASS_FIELD(int, mProtection, 0x264);

    // vtable index 35 = Item::getMaxDamage() (снято на 1.21.44 — TODO перепроверить)
    int getMaxDamage() const {
        return MemUtils::callVirtualFunc<short>(35, const_cast<Item*>(this));
    }

    // vtable index 15 = Item::isBlockItem() — "является ли предмет блоком".
    // Надёжный способ отличить блоки (в т.ч. campfire) от инструментов.
    // (снято на 1.21.44 — TODO перепроверить индекс на 26.62)
    bool isBlockItem() const {
        return MemUtils::callVirtualFunc<bool>(15, const_cast<Item*>(this));
    }
"""])

resolve('src/Utils/GameUtils/ActorUtils.cpp', ["""    cit->mHand = 0;
    cit->mItemInHand = NetworkItemStackDescriptor(*stack);
"""])

resolve('src/Utils/GameUtils/ItemUtils.cpp', ['Y', 'M', """        ItemStack* stack = container->getItem(i);
        if (!stack || !stack->getItem()) continue;
"""])

resolve('src/Utils/GameUtils/PacketUtils.cpp', ["""    if (!textPacket) return;
    textPacket->Messeage(player, msg);
"""])

resolve('src/SDK/Minecraft/World/Level.cpp', ["""#include <windows.h>
#include <SDK/heap_rd.hpp>
""", 'Y'])

resolve('src/SDK/Minecraft/World/Level.hpp', ["""#include <SDK/Minecraft/Network/Packets/PlayerListPacket.hpp>
#include <glm/glm.hpp>
#include <Utils/MemUtils.hpp>

enum class ParticleType : int {
    Bubble = 1,
    Crit = 2,               // Синие криты
    BlockCrack = 3,         // Осколки блока
    Smoke = 4,              // Дым
    Explode = 5,            // Взрыв
    Heart = 12,             // Сердечки
    Flame = 15,             // Огонь
    Lava = 16,              // Брызги лавы
    Portal = 18,            // Фиолетовый портал
    EnchantingTable = 21,   // Символы чародейства
    Totem = 27,             // Искры тотема
    DragonBreath = 35,      // Дыхание дракона
    SonicBoom = 53          // Волна Вардена
};
""", 'Y', 'Y', """\tCLASS_FIELD(uintptr_t**, mVfTable, 0x0);
\t//CLASS_FIELD(class BlockPalette*, mBlockPalette, 0x1C8);
\t///CLASS_FIELD(std::unordered_map<mce::UUID, PlayerListEntry>, pl, 0x1BC8); // class_field doesn't support std::unordered_map lol

\tstd::unordered_map<mce::UUID, PlayerListEntry>* getPlayerList();
\tclass HitResult* getHitResult();
\tclass SyncedPlayerMovementSettings* getPlayerMovementSettings();
\tstd::vector<Actor*> getRuntimeActorList();
\tLevelData* getLevelData();
\tclass BlockPalette* getBlockPalette();

\t// Спавн партиклов.
\t// TODO(26.62): оффсет addParticle в Level.cpp снят с 1.21.44 — переснять!
\tvoid addParticle(ParticleType type, const glm::vec3& pos, const glm::vec3& dir = glm::vec3(0.f, 0.05f, 0.f), int data = 0);
"""])

resolve('src/Utils/MiscUtils/BlockUtils.cpp', ["""#include <src/Features/Modules/Player/Regen.hpp>
#include <src/Features/Modules/Player/OreMiner.hpp>
""", 'M', 'M', 'Y', 'M'])

# ---- Level.cpp: чистый хвост (getLevelData + getBlockPalette + addParticle) ----
p = BASE / 'src/SDK/Minecraft/World/Level.cpp'
t = p.read_text(encoding='utf-8', errors='replace')
i = t.find('LevelData* Level::getLevelData()')
assert i > 0, "не найден getLevelData в Level.cpp"
tail = """LevelData* Level::getLevelData()
{
    // TODO(26.62): у авторов новой базы этот вызов закомментирован — если будут
    // проблемы, закомментируй return тоже (как в их версии).
    return hat::member_at<LevelData*>(this, OffsetProvider::Level_mLevelData);
}

class BlockPalette* Level::getBlockPalette()
{
    static auto vIndex = OffsetProvider::Level_getBlockPalette;
    return MemUtils::callVirtualFunc<class BlockPalette*>(vIndex, this);
}

// Спавн партиклов.
// TODO(26.62): оффсет функции sub_1432A4800 снят с 1.21.44 — ОБЯЗАТЕЛЬНО переснять
// с нового Minecraft.Windows.exe (26.62), иначе вызов уйдёт по неверному адресу!
void Level::addParticle(ParticleType type, const glm::vec3& pos, const glm::vec3& dir, int data)
{
    static uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    static uintptr_t func = base + 0x32A4800; // sub_1432A4800 (снят с 1.21.44)

    MemUtils::callFastcall<void>(func, this, static_cast<int>(type), &pos, &dir, data);
}
"""
p.write_text(t[:i] + tail, encoding='utf-8')
print("OK  Level.cpp хвост переписан")

# ---- FileUtils: версия юзера + Dexko-нейминг + все каталоги ----
fu = (ROOT / 'src/Utils/FileUtils.cpp').read_text(encoding='utf-8', errors='replace')
fu = fu.replace('Juzdex', 'Dexko')
fu = fu.replace('\n// Alias for compatibility\nstd::string FileUtils::getSolsticeDir() { return getDexkoDir(); }\n', '\n')
old_v = 'void FileUtils::validateDirectories() {\n  createDirectory(getDexkoDir());\n  createDirectory(getDexkoDir() + "Configs\\\\");\n  spdlog::info("Directories created successfully.");\n}'
new_v = ('void FileUtils::validateDirectories() {\n'
         '  createDirectory(getDexkoDir());\n'
         '  createDirectory(getDexkoDir() + "Configs\\\\");\n'
         '  createDirectory(getDexkoDir() + "Templates\\\\");\n'
         '  createDirectory(getDexkoDir() + "Databases\\\\");\n'
         '  createDirectory(getDexkoDir() + "Skins\\\\");        // SkinChanger / SkinStealer\n'
         '  createDirectory(getDexkoDir() + "BlinkerSkins\\\\"); // skinblinker\n'
         '  createDirectory(getDexkoDir() + "Audio\\\\");        // ToggleSounds\n'
         '  spdlog::info("Directories created successfully.");\n}')
assert old_v in fu, "validateDirectories не найден в ожидаемом виде"
fu = fu.replace(old_v, new_v)
(BASE / 'src/Utils/FileUtils.cpp').write_text(fu, encoding='utf-8')

(BASE / 'src/Utils/FileUtils.hpp').write_text("""#pragma once
//
// Created by vastrakai on 6/29/2024.
//

#include <string>
#include <vector>

class FileUtils {
public:
    static std::string getRoamingStatePath();
    static std::string getDexkoDir();
    static bool fileExists(const std::string& path);
    static void createDirectory(const std::string& path);
    static void validateDirectories();
    static bool deleteFile(const std::string& path);
    static void writeResourceToFile(Resource* resource, const std::string& path);
    static void writeResourceToFile(const std::string& path, const unsigned char* data, size_t size);
    static std::vector<std::string> listFiles(const std::string& path);
    static void createFile(const std::string& path);
    static size_t getFileSize(const std::string& path);
    static std::vector<unsigned char> readFile(const std::string& path);
};
""", encoding='utf-8')
print("OK  FileUtils переписан (Dexko + все каталоги)")

# ---- ItemRendererHook.cpp: статсы юзера + событие базы ----
(BASE / 'src/Hook/Hooks/RenderHooks/ItemRendererHook.cpp').write_text("""#include "ItemRendererHook.hpp"

#include <SDK/Minecraft/Rendering/ItemRenderer.hpp>
#include <SDK/Minecraft/Rendering/BaseActorRenderContext.hpp>
#include <Features/Events/ItemRendererEvent.hpp>

void ItemRendererHook::init()
{
    mItemRendererDetour = std::make_unique<Detour>(
        "ItemRenderer::render",
        reinterpret_cast<void*>(SigManager::ItemRenderer_render),
        &ItemRendererHook::onRender);
    mItemRendererDetour->enable();
}

void* ItemRendererHook::onRender(ItemRenderer* _this, BaseActorRenderContext* ctx,
                                  void* a3, void* a4, void* a5, void* a6)
{
    auto original = mItemRendererDetour->getOriginal<&ItemRendererHook::onRender>();

    if (_this) sItemRenderer = _this;
    if (ctx)   sBarContext   = ctx;

    auto holder = nes::make_holder<ItemRendererEvent>(_this, ctx, a3);
    gFeatureManager->mDispatcher->trigger(holder);

    return original(_this, ctx, a3, a4, a5, a6);
}
""", encoding='utf-8')
print("OK  ItemRendererHook.cpp переписан")

# ---- KeyHook.cpp: гарантия инклудов и статика ----
p = BASE / 'src/Hook/Hooks/MiscHooks/KeyHook.cpp'
t = p.read_text(encoding='utf-8', errors='replace')
if '#include <Features/ChatBinds/ChatBinds.hpp>' not in t:
    t = t.replace('#include "KeyHook.hpp"', '#include "KeyHook.hpp"\n\n#include <Features/ChatBinds/ChatBinds.hpp>', 1)
if '#include <unordered_set>' not in t:
    t = t.replace('#include <SDK/Minecraft/ClientInstance.hpp>', '#include <SDK/Minecraft/ClientInstance.hpp>\n#include <unordered_set>', 1)
if 'gGameSeenKeysDown;' not in t:
    t = re.sub(r'(#include <unordered_set>\n)', r'\1\nstatic std::unordered_set<int> gGameSeenKeysDown;\n', t, count=1)
p.write_text(t, encoding='utf-8')
print("OK  KeyHook.cpp инклуды/статика проверены")

# ---- финальная проверка: маркеров не осталось ----
bad = []
for f in BASE.rglob('*'):
    if f.suffix in ('.cpp', '.hpp', '.h', '.txt', '.cmake') and f.is_file():
        try:
            t = f.read_text(encoding='utf-8', errors='ignore')
        except Exception:
            continue
        if '<<<<<<< ' in t or '>>>>>>> ' in t:
            bad.append(str(f.relative_to(BASE)))
print("Остались маркеры конфликтов:", bad if bad else "нигде ✓")
