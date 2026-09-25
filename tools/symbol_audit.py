#!/usr/bin/env python3
"""
symbol_audit.py — сверка целей хуков/сигнатур проекта Solstice (1.21.44)
с заголовками LeviLamina (текущая версия, по умолчанию 26.51).

Что делает:
  1. Вытаскивает из src/Hook/** все цели хуков  -> Detour>("Class::method"
  2. Вытаскивает из src/SDK/OffsetProvider.hpp все DEFINE_* записи (имя вида Class_member)
  3. Для каждого имени ищет класс в заголовках LeviLamina (src/**, src-client/**)
     и проверяет, есть ли там такой метод/поле
  4. Пишет отчёт:
       docs/migration-1.26/audit.md       — таблицы со статусами
       docs/migration-1.26/symbol-map.csv — то же самое в CSV
     и (по флагу --annotate) расставляет в исходниках пометки
       // [1.26] ...

Запуск:
    python3 tools/symbol_audit.py                       # отчёт
    python3 tools/symbol_audit.py --annotate            # + пометки в исходниках
    python3 tools/symbol_audit.py --ll /path/to/LeviLamina

Ничего не компилирует и не ломает сборку: пометки — это только комментарии.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEFAULT_LL = Path("/home/user/LeviLamina")

OUT_DIR = REPO / "docs" / "migration-1.26"


# ─────────────────────────── модели ───────────────────────────


@dataclass
class Entry:
    kind: str            # "hook" | "sig"
    raw: str             # как записано в проекте: "Actor::baseTick" или "ClientInstance_getLocalPlayer"
    cls: str             # имя класса
    member: str          # имя метода/поля
    file: str            # файл проекта
    line: int
    status: str = ""
    symbol: str = ""
    ll_file: str = ""
    ll_line: str = ""
    decl: str = ""
    note: str = ""


# ─────────────────────────────────────────────────────────────────────────────
# РУЧНЫЕ ВЕРДИКТЫ (проверено глазами по хидерам LeviLamina 26.51).
# Формат: цель -> (статус, куда делось, пояснение)
#   RENAMED    — переименовано, новый символ найден и подтверждён
#   MOVED      — переехало в другой класс/компонент, найдено
#   NO_LAYOUT  — класс есть, но поля не описаны -> оффсет только из IDA
#   GONE       — в 1.26 нет (удалено/переименовано неизвестно куда) -> реверс
# ─────────────────────────────────────────────────────────────────────────────
MANUAL = {
    # ── хуки ──
    "Mob::getCurrentSwingDuration": (
        "RENAMED", "Mob::getModifiedSwingDuration  (src/mc/world/actor/Mob.h:338)",
        "в 1.26 это int getModifiedSwingDuration(); Item::getSwingDuration (Item.h:231) — "
        "длительность взмаха предмета, НЕ моба: для хука бери Mob::getModifiedSwingDuration"),
    "bobHurt": (
        "MOVED", "LevelRendererPlayer::bobHurt(Matrix&, float)  "
        "(src-client/mc/client/renderer/game/LevelRendererPlayer.h:365)",
        "метод переехал из анонимного класса в LevelRendererPlayer"),
    "entityHurt": (
        "RENAMED", "Actor::_hurt(ActorDamageSource const&, float, HurtParameters const&)  "
        "(src/mc/world/actor/Actor.h:612, транк $_hurt:1478)",
        "сигнатура изменилась: появился третий аргумент HurtParameters"),
    "entityHealthChanged": (
        "RENAMED", "onActorHealthChanged  (src/mc/scripting/modules/minecraft/events/IScriptWorldAfterEvents.h:127)",
        "это scripting-событие; для нативного хука ищи Actor::_hurt / heal"),
    "projectileHitBlock": (
        "RENAMED", "onProjectileHitBlock  (IScriptWorldAfterEvents.h:341) + ProjectileHitEvent",
        "scripting-событие; нативный вариант — хук ProjectileComponent / Actor::_onHit"),
    "projectileHitEntity": (
        "RENAMED", "onProjectileHitEntity  (IScriptWorldAfterEvents.h:344) + ProjectileHitEvent",
        "см. projectileHitBlock"),
    "Keyboard::feed": (
        "MOVED", "ll::event::input::KeyInputEvent + ll::input::KeyRegistry  "
        "(src-client/ll/api/event/input/)",
        "в 1.26 вместо хука клавиатуры лучше взять готовое событие LeviLamina"),
    "CameraDirectLookSystemUtil::_handleLookInput": (
        "GONE", "CameraDirectLookComponent / CameraDirectLookDefinition  "
        "(src-client/mc/deps/minecraft_camera/components/, src/mc/deps/shared_types/v1_21_100/camera/)",
        "класс переехал на компонентную систему камеры — реверс"),
    "Unknown::renderNametag": (
        "GONE", "NameTagRenderObject / NameTagRenderer  "
        "(src-client/mc/deps/minecraft_renderer/objects/NameTagRenderObject.h, "
        "src-client/mc/client/gui/controls/renderers/NameTagRenderer.h)",
        "в 1.26 неймтеги — объекты рендера, отдельной функции нет"),
    "PacketHandlerDispatcherInstance<": (
        "MOVED", "src/mc/network/PacketHandlerDispatcherInstance.h",
        " имя в Detour() обрезано, уточни шаблонные параметры"),

    # ── ClientInstance ──
    "ClientInstance_getBlockSource": (
        "RENAMED", "ClientInstance::getRegion()  "
        "(src-client/mc/client/game/ClientInstance.h:475, транк $getRegion:1470)",
        "getBlockSource переименован в getRegion"),
    "ClientInstance_mLevelRenderer": (
        "MOVED", "ClientInstance::getLevelRenderer()  (ClientInstance.h:796, $getLevelRenderer:1787)",
        "поля класса не описаны (в 1.26 у ClientInstance всего 2 описанных поля) — "
        "бери через геттер, а не по оффсету"),
    "ClientInstance_mPacketSender": (
        "MOVED", "ClientInstance::getPacketSender()  (ClientInstance.h:994)", "см. mLevelRenderer"),
    "ClientInstance_mGuiData": (
        "MOVED", "ClientInstance::getGuiData()  (ClientInstance.h:857/859)", "см. mLevelRenderer"),
    "ClientInstance_getInputHandler": (
        "RENAMED", "ClientInstance::getInput() -> ClientInputHandler*  (ClientInstance.h:1040); "
        "getMinecraftInput() (968)", "getInputHandler -> getInput"),
    "ClientInstance_mMinecraftSim": (
        "NO_LAYOUT", "src-client/mc/client/game/ClientInstance.h",
        "в хидере описаны только mUITexture/mUICursorTexture — оффсет только из IDA"),

    # ── MinecraftGame ──
    "MinecraftGame_playUi": ("NO_LAYOUT", "src-client/mc/client/game/MinecraftGame.h",
        "у MinecraftGame в 1.26 не описано НИ ОДНОГО поля, метода playUi нет — реверс"),
    "MinecraftGame_mClientInstances": ("NO_LAYOUT", "src-client/mc/client/game/MinecraftGame.h",
        "полей не описано; геттеры primaryClientInstance ищи в MinecraftGame.h — реверс"),
    "MinecraftGame_mProfanityContext": ("NO_LAYOUT", "src-client/mc/client/game/MinecraftGame.h",
        "полей не описано — реверс"),
    "MinecraftGame_mMouseGrabbed": ("NO_LAYOUT", "src-client/mc/client/game/MinecraftGame.h",
        "полей не описано; мышь — ClientInstance::grabMouse()/isMouseGrabbed()"),

    # ── рендер ──
    "LevelRenderer_mRendererPlayer": (
        "RENAMED", "LevelRenderer::mLevelRendererPlayer  (shared_ptr<LevelRendererPlayer>, "
        "src-client/mc/client/renderer/game/LevelRenderer.h:125)",
        "mRendererPlayer -> mLevelRendererPlayer, тип shared_ptr"),
    "LevelRendererPlayer_mFovX": (
        "RENAMED", "LevelRendererPlayer::mFov  (float, "
        "src-client/mc/client/renderer/game/LevelRendererPlayer.h:131)",
        "в 1.26 один float mFov (есть ещё mOFov — предыдущее значение); "
        "вертикальный FOV считается из aspect ratio"),
    "LevelRendererPlayer_mFovY": (
        "RENAMED", "LevelRendererPlayer::mOFov  (float, LevelRendererPlayer.h:132)",
        "в 1.26 отдельного «FovY» нет: mFov + mOFov (предыдущее)"),
    "LevelRendererPlayer_mCameraPos": (
        "MOVED", "LevelRendererCamera::mCameraPos  (Vec3, "
        "src-client/mc/client/renderer/game/LevelRendererCamera.h:263)",
        "позиция камеры переехала в LevelRendererCamera"),

    # ── мир / актор ──
    "LevelData_mTick": (
        "RENAMED", "LevelData::mCurrentTick  (Tick, src/mc/world/level/storage/LevelData.h:77)",
        "mTick -> mCurrentTick (тип Tick, 8 байт)"),
    "GameSession_mEventCallback": (
        "RENAMED", "GameSession::getNetEventCallback() / mLegacyClientNetworkHandler  "
        "(src/mc/world/GameSession.h:30, 60)", "mEventCallback -> getNetEventCallback()"),
    "Actor_mGameMode": (
        "MOVED", "ECS ActorGameTypeComponent  (src/mc/entity/components/ActorGameTypeComponent.h)",
        "gamemode ушёл в ECS-компонент, бери через getEntityContext()"),
    "Actor_mHurtTimeComponent": (
        "MOVED", "ECS MobHurtTimeComponent : IntComponent  (src/mc/entity/components/MobHurtTimeComponent.h)",
        "hurt time ушёл в ECS-компонент (mValue)"),
    "Actor_mSwinging": (
        "MOVED", "actor data flags (ActorDataFlagComponent / getStatusFlag(ActorFlags::Swinging))",
        "флаги актора в 1.26 — биты в ECS-компоненте, не поле Actor"),
    "Actor_mDestroying": (
        "MOVED", "actor data flags (ActorDataFlagComponent)",
        "см. Actor_mSwinging"),
    "Actor_mSupplies": (
        "MOVED", "ECS ActorEquipmentComponent  (src/mc/entity/components/ActorEquipmentComponent.h)",
        "mHand/mArmor — unique_ptr<SimpleContainer>"),
    "Actor_mContainerManagerModel": (
        "MOVED", "PlayerInventory::mHudContainerManager  (weak_ptr<HudContainerManagerModel>, "
        "src/mc/world/actor/player/PlayerInventory.h:26)", "контейнер-менеджер живёт в PlayerInventory"),
    "Actor_mSerializedSkin": (
        "GONE", "SerializedSkin в 1.26 не найден", "ищи PlayerSkinComponent / SerializedSkinComponent — реверс"),
    "BlockSource_mBuildHeight": (
        "NO_LAYOUT", "src/mc/world/level/BlockSource.h",
        "поля нет; есть getHeight()/getHeightmapPos(); высота мира — DimensionHeightRange.h"),
    "PlayerInventory_mContainer": (
        "RENAMED", "PlayerInventory::mInventory  (unique_ptr<Inventory>, "
        "src/mc/world/actor/player/PlayerInventory.h:24)", "mContainer -> mInventory"),
    "ContainerManagerModel_getSlot": (
        "RENAMED", "ContainerManagerModel::getFullContainerSlot(int, FullContainerName const&)  "
        "(src/mc/world/containers/managers/models/ContainerManagerModel.h:107, $getFullContainerSlot:182)",
        "getSlot -> getFullContainerSlot, добавился аргумент FullContainerName"),

    # ── прочее ──
    "BlockLegacy_mBlockId": ("GONE", "класса BlockLegacy в 1.26 нет", "переименован/вынесен — реверс"),
    "BlockLegacy_mayPlaceOn": ("GONE", "класса BlockLegacy в 1.26 нет", "см. BlockLegacy_mBlockId"),
    "BlockLegacy_getCollisionShape": ("GONE", "класса BlockLegacy в 1.26 нет", "см. BlockLegacy_mBlockId"),
    "bgfx_d3d12_RendererContextD3D12_m_commandQueue": (
        "NO_LAYOUT", "src-client/mc/external/bgfx/bgfx.h", "bgfx — внешняя библиотека, layout не в хидерах БДС"),
    "bgfx_context_m_renderCtx": ("NO_LAYOUT", "src-client/mc/external/bgfx/bgfx.h", "см. выше"),
    "ClientInputMappingFactory_mKeyboardMouseSettings": (
        "NO_LAYOUT", "src-client/mc/client/input/ClientInputMappingFactory.h", "полей не описано — реверс"),
    "MinecraftSim_mGameSim": ("GONE", "MinecraftSim в 1.26 нет", "кастомное имя из Flarial — реверс"),
    "MinecraftSim_mRenderSim": ("GONE", "MinecraftSim в 1.26 нет", "см. выше"),
    "MinecraftSim_mGameSession": ("GONE", "MinecraftSim в 1.26 нет", "см. выше"),
    "MainView_bedrockPlatform": ("GONE", "MainView в 1.26 нет", "кастомное имя — реверс"),
    "BedrockPlatformUWP_mcGame": ("GONE", "BedrockPlatformUWP в 1.26 нет", "кастомное имя — реверс"),
    "UIProfanityContext_mEnabled": ("GONE", "UIProfanityContext в 1.26 нет", "кастомное имя — реверс"),
    "Bone_mPartModel": ("GONE", "Bone — собственная структура проекта", "твой реверс, как и раньше"),
}

# Цели, для которых глобальный поиск даёт ложные срабатывания (разобрано вручную)
SKIP_GLOBAL = {
    "Keyboard::feed",   # глобальный поиск находит Actor::feed(int) — это не то
}

STATUS_ORDER = {
    "FOUND": 0,
    "FOUND_THUNK": 1,
    "FOUND_ELSEWHERE": 2,
    "RENAMED": 2,
    "MOVED": 2,
    "EMPTY_CLASS": 3,
    "NOT_IN_CLASS": 6,
    "CLASS_MISSING": 7,
    "NO_LAYOUT": 8,
    "GONE": 9,
}

# Переименования Mojang между 1.21.44 и 1.26 (проверено по хидерам вручную)
SYNONYMS = {
    "setupAndRender": ["render"],                       # ScreenView::setupAndRender -> ScreenView::render
    "getCurrentSwingDuration": ["getSwingDuration"],    # Mob:: -> Item::getSwingDuration
    "entityHurt": ["_hurt"],                            # Actor::_hurt
    "entityHealthChanged": ["onActorHealthChanged"],
    "projectileHitBlock": ["onProjectileHitBlock"],
    "projectileHitEntity": ["onProjectileHitEntity"],
}

# Ручные пометки: что делать с конкретной целью
NOTES = {
    "Keyboard::feed":
        "в 1.26 хук клавиатуры не нужен: ll::event::input::KeyInputEvent + "
        "ll::input::KeyRegistry::getOrCreateKey (src-client/ll/api/event/input)",
    "MouseDevice::feed":
        "альтернатива без хука: ll::event::input::MouseInputEvent",
    "ScreenView::setupAndRender":
        "в 1.26 это ScreenView::render; LeviLamina уже хукает его и даёт "
        "Before/AfterUIRenderEvent (src-client/ll/api/event/render)",
    "RakNet::RakPeer::GetLastPing":
        "RakNet в хидерах ещё есть (src/mc/deps/raknet/RakPeer.h), но появился "
        "src/mc/external/webrtc — транспорт менялся, проверять вживую",
    "RakNet::RakPeer::RunUpdateCycle": "см. RakNet::RakPeer::GetLastPing",
    "RakNet::RakPeer::SendImmediate":
        "в RakPeer.h есть Send + транк $Send; SendImmediate ищите рядом",
    "CameraDirectLookSystemUtil::_handleLookInput":
        "класс переехал: теперь CameraDirectLookComponent / CameraDirectLookDefinition",
    "mce::framebuilder::RenderItemInHandDescription::RenderItemInHandDescription":
        "структура в хидерах пустая (layout неизвестен) — только ручной реверс",
    "Unknown::renderNametag":
        "в 1.26 неймтеги — это NameTagRenderObject / NameTagRenderer (объекты рендера), "
        "не отдельная функция",
    "Mob::getCurrentSwingDuration": "в 1.26: uint Item::getSwingDuration() (Item.h:231)",
    "PacketHandlerDispatcherInstance<":
        "класс есть: src/mc/network/PacketHandlerDispatcherInstance.h (шаблон, имя в Detour обрезано)",
    "bobHurt": "найдено как LevelRendererPlayer::bobHurt(Matrix&, float) — класс сменился",
    "ActorAnimationControllerPlayer::applyToPose":
        "символ есть, но сигнатура изменилась: (ApplyAnimationContext const&, RenderParams&, "
        "unordered_map<SkeletalHierarchyIndex, vector<BoneOrientation>>&, float) — тело хука переписывать",
    "projectileHitBlock": "в 1.26 есть scripting-событие onProjectileHitBlock + ProjectileHitEvent",
    "projectileHitEntity": "в 1.26 есть scripting-событие onProjectileHitEntity + ProjectileHitEvent",
    "entityHealthChanged": "в 1.26: onActorHealthChanged (IScriptWorldAfterEvents.h:127)",
    "ClientInstance::isPreGame": "готовый символ + транк $isPreGame",
    "ContainerScreenController::tick": "готовый символ",
    "Actor::baseTick": "готовый символ",
    "LoopbackPacketSender::send": "готовый символ (send/sendTo/sendToServer)",
}


# ─────────────────────── парсинг проекта ───────────────────────


def collect_hooks(root: Path) -> list[Entry]:
    out: list[Entry] = []
    detour_re = re.compile(r'Detour>\("([^"]+)"')
    for path in sorted((root / "src" / "Hook").rglob("*.cpp")):
        for i, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            for m in detour_re.finditer(line):
                target = m.group(1).strip()
                if "::" in target:
                    cls, member = target.rsplit("::", 1)   # учитываем неймспейсы: RakNet::RakPeer::Send
                    cls = cls.rsplit("::", 1)[-1]
                else:
                    cls, member = "?", target
                out.append(Entry("hook", target, cls.strip(), member.strip(),
                                 str(path.relative_to(root)), i))
    return out


def collect_sigs(root: Path) -> list[Entry]:
    out: list[Entry] = []
    path = root / "src" / "SDK" / "OffsetProvider.hpp"
    if not path.exists():
        return out
    # у TYPED-варианта первый аргумент — тип (float/uint8_t), имя идёт вторым
    define_re = re.compile(
        r'^\s*(//\s*)?(DEFINE_INDEX_FIELD_TYPED|DEFINE_INDEX_FIELD|DEFINE_FIELD)\s*\(\s*([^,]+?)\s*,\s*([^,]+?)\s*,'
    )
    for i, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        m = define_re.match(line)
        if not m:
            continue
        commented = bool(m.group(1))
        name = (m.group(4) if m.group(2) == "DEFINE_INDEX_FIELD_TYPED" else m.group(3)).strip()
        name = re.sub(r'^Flarial_', '', name)   # их же префикс оффсетов
        if "_" in name:
            cls, member = name.split("_", 1)
        else:
            cls, member = "?", name
        e = Entry("sig", name, cls, member, str(path.relative_to(root)), i)
        if commented:
            e.note = "закомментировано в проекте"
        out.append(e)
    return out


# ─────────────────── индекс заголовков LeviLamina ───────────────────


def build_header_index(ll: Path) -> dict[str, list[Path]]:
    index: dict[str, list[Path]] = {}
    for base in ("src-client", "src-server", "src"):
        d = ll / base / "mc"
        if not d.exists():
            continue
        for h in d.rglob("*.h"):
            index.setdefault(h.stem, []).append(h)
    return index


def _declares(hfile: Path, cls: str) -> bool:
    """Есть ли в файле объявление (а не форвард-декларация) класса cls."""
    try:
        text = hfile.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return False
    return re.search(r"^\s*(class|struct)\s+" + re.escape(cls) + r"\b\s*(:|\{)", text, re.M) is not None


def pick_class_file(index: dict[str, list[Path]], cls: str) -> Path | None:
    """Файл, где класса описано больше всего.

    У одного имени часто два хидера: настоящий и короткая клиентская заглушка
    (например BlockSource.h — 776 строк против 75). Раньше побеждала заглушка,
    и метод «не находился в своём классе» — это ломало весь аудит.
    """
    cands = [p for p in index.get(cls, []) if _declares(p, cls)] or index.get(cls, [])
    if not cands:
        return None

    def richness(p: Path) -> int:
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            return 0
        return len(text.splitlines()) + 20 * len(re.findall(r"::ll::TypedStorage<", text))

    def score(p: Path) -> int:
        s = p.as_posix()
        return 0 if "/src-client/" in s else (1 if "/src/" in s else 2)

    return sorted(cands, key=lambda p: (-richness(p), score(p)))[0]


def global_search(ll: Path, names: list[str]) -> tuple[str, str, str] | None:
    """Ищем имя по всем хидерам (когда в «своём» классе его нет)."""
    import subprocess
    for nm in names:
        pat = r"\b" + re.escape(nm) + r"\s*\("
        try:
            out = subprocess.run(
                ["grep", "-rn", "-m1", "-E", pat, "--include=*.h", "src", "src-client"],
                cwd=ll, capture_output=True, text=True, timeout=120,
            )
        except (OSError, subprocess.SubprocessError):
            return None
        for line in out.stdout.splitlines():
            parts = line.split(":", 2)
            if len(parts) == 3:
                return parts[0], parts[1], parts[2].strip()
    return None


def find_member(hfile: Path, member: str) -> tuple[str, str, str]:
    """Ищет member в файле класса. Возвращает (status, line_no, decl)."""
    text = hfile.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    plain = re.compile(r"\b" + re.escape(member) + r"\s*\(")
    thunk = re.compile(r"\$" + re.escape(member) + r"\s*\(")
    field = re.compile(r"\b" + re.escape(member) + r"\b")

    hits: list[tuple[int, str, str]] = []  # (prio, line_no, decl)
    for i, line in enumerate(lines, 1):
        if thunk.search(line):
            hits.append((0, i, line.strip()))   # транк предпочтительнее: виртуалку хукают через $
        elif plain.search(line):
            hits.append((1, i, line.strip()))
    if hits:
        hits.sort()
        prio, ln, decl = hits[0]
        return ("FOUND_THUNK" if prio == 1 else "FOUND"), str(ln), decl

    for i, line in enumerate(lines, 1):
        s_line = line.strip()
        if field.search(line) and not s_line.startswith("//"):
            if re.search(r"(class|struct)\s+" + re.escape(member) + r"\s*\{\s*\}\s*;", s_line):
                return "EMPTY_CLASS", str(i), s_line
            return "FOUND", str(i), s_line
    return "NOT_IN_CLASS", "", ""


# ─────────────────────────── отчёты ───────────────────────────


def render_md(entries: list[Entry], ll: Path, header_index_n: int) -> str:
    by_status: dict[str, list[Entry]] = {}
    for e in entries:
        by_status.setdefault(e.status, []).append(e)

    def table(items: list[Entry]) -> str:
        rows = ["| Цель в проекте (1.21.44) | Статус | LeviLamina 26.51 | Объявление | Примечание |",
                "|---|---|---|---|---|"]
        for e in sorted(items, key=lambda x: (STATUS_ORDER.get(x.status, 9), x.cls, x.member)):
            loc = f"`{e.ll_file}:{e.ll_line}`" if e.ll_file else "—"
            decl = e.decl.replace("|", "\\|")
            if len(decl) > 90:
                decl = decl[:87] + "…"
            note = e.note.replace("|", "\\|")
            rows.append(f"| `{e.raw}` | {e.status} | {loc} | `{decl or '—'}` | {note} |")
        return "\n".join(rows)

    total = len(entries)
    found = len(by_status.get("FOUND", []))
    thunks = len(by_status.get("FOUND_THUNK", []))
    elsewhere = len(by_status.get("FOUND_ELSEWHERE", []))
    pct = ((found + thunks + elsewhere) / total * 100) if total else 0.0

    parts = [
        "# Аудит миграции Solstice 1.21.44 → 1.26 (LeviLamina)",
        "",
        f"- Проект: `{REPO.name}` (Solstice, гейм-версия 1.21.44)",
        f"- Заголовки LeviLamina: `{ll}`",
        f"- Проиндексировано классов в хидерах: **{header_index_n}**",
        f"- Проверено целей: **{total}** (хуков: {len([e for e in entries if e.kind=='hook'])}, "
        f"сигнатур/полей: {len([e for e in entries if e.kind=='sig'])})",
        f"- Найдено по имени в 1.26: **{found + thunks + elsewhere}** ({pct:.0f}%) "
        f"— из них точно в том же классе: **{found + thunks}** "
        f"(транков `$`: {thunks}), в другом классе (на проверку): **{elsewhere}**",
        "",
        "Статусы:",
        "",
        "- `FOUND` — метод/поле с таким именем есть в заголовке 1.26 (адрес резолвит symdb)",
        "- `FOUND_THUNK` — найден `$`-транк: виртуальную функцию хукают через `&Class::$method`",
        "- `FOUND_ELSEWHERE` — в «своём» классе имени нет, но оно найдено в другом классе (переехало)",
        "- `RENAMED` — переименовано в 1.26, новый символ найден и подтверждён вручную",
        "- `MOVED` — переехало в другой класс/компонент (или заменяется событием), найдено вручную",
        "- `NO_LAYOUT` — класс есть, но поля не описаны: оффсет только из IDA",
        "- `GONE` — в 1.26 нет (удалено или переименовано неизвестно куда): реверс",
        "- `EMPTY_CLASS` — класс/структура в хидерах пустая (`struct X {};`) — layout неизвестен, только реверс",
        "- `NOT_IN_CLASS` — класс есть, но такого члена нет (переименовано/удалено/переехало)",
        "- `CLASS_MISSING` — класса с таким именем в хидерах 1.26 нет вообще",
        "",
        "## Хуки",
        "",
        table([e for e in entries if e.kind == "hook"]),
        "",
        "## Сигнатуры и поля (OffsetProvider.hpp)",
        "",
        table([e for e in entries if e.kind == "sig"]),
        "",
        "## Что это значит",
        "",
        "- Всё, что попало в `FOUND*` — можно перестать искать сигнатурой:",
        "  адрес даст symdb, имя и типы даст хидер.",
        "- `NOT_IN_CLASS` / `CLASS_MISSING` — это и есть объём ручного реверса.",
        "- Отчёт сгенерирован `tools/symbol_audit.py`, перезапускайте после обновления хидеров.",
        "",
    ]
    return "\n".join(parts)


def write_csv(entries: list[Entry], path: Path) -> None:
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["kind", "target", "class", "member", "status",
                    "ll_file", "ll_line", "decl", "project_file", "project_line", "note"])
        for e in entries:
            w.writerow([e.kind, e.raw, e.cls, e.member, e.status,
                        e.ll_file, e.ll_line, e.decl, e.file, e.line, e.note])


# ─────────────────────── пометки в исходниках ───────────────────────


def status_tag(e: "Entry") -> str:
    """Человеческая пометка для комментария в исходнике."""
    return {
        "FOUND": "ОБНОВЛЕНО",
        "FOUND_THUNK": "ОБНОВЛЕНО ($-транк)",
        "FOUND_ELSEWHERE": "ПРОВЕРИТЬ (найдено в другом классе)",
        "RENAMED": "ПЕРЕИМЕНОВАНО В 1.26",
        "MOVED": "ПЕРЕЕХАЛО В 1.26",
        "NO_LAYOUT": "РЕВЕРС (поля класса не описаны в 1.26)",
        "GONE": "РЕВЕРС (в 1.26 нет)",
        "EMPTY_CLASS": "РЕВЕРС (структура в хидерах пустая)",
        "NOT_IN_CLASS": "РЕВЕРС (нет в классе 1.26)",
        "CLASS_MISSING": "РЕВЕРС (класс не найден в 1.26)",
    }.get(e.status, e.status)


def verdict_text(e: "Entry") -> str:
    """Строчка пометки: куда делось и что делать."""
    where = f"{e.ll_file}:{e.ll_line}".rstrip(":") if e.ll_file else "не найдено"
    text = f"{status_tag(e)}: {e.raw} -> {where}"
    if e.note:
        text += f" — {e.note}"
    return text

MARK = "// [1.26]"


def annotate_sigs(root: Path, entries: list[Entry]) -> int:
    path = root / "src" / "SDK" / "OffsetProvider.hpp"
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    by_line = {e.line: e for e in entries if e.kind == "sig"}
    out: list[str] = []
    n = 0
    for i, line in enumerate(lines, 1):
        if MARK in line:            # идемпотентность: стираем старые пометки
            continue
        e = by_line.get(i)
        if e:
            out.append(f"{MARK} {verdict_text(e)}")
            n += 1
        out.append(line)
    path.write_text("\n".join(out) + "\n", encoding="utf-8")
    return n


def annotate_hooks(root: Path, entries: list[Entry]) -> int:
    by_file: dict[str, list[Entry]] = {}
    for e in entries:
        if e.kind == "hook":
            by_file.setdefault(e.file, []).append(e)
    n = 0
    for rel, items in by_file.items():
        path = root / rel
        text = path.read_text(encoding="utf-8", errors="replace")
        if MARK in text:
            lines = [l for l in text.splitlines() if MARK not in l]
            text = "\n".join(lines)
        lines = text.splitlines()
        # вставляем блок пометок после последнего #include
        last_inc = max((i for i, l in enumerate(lines) if l.strip().startswith("#include")), default=-1)
        block = ["", "// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):"]
        for e in items:
            block.append(f"{MARK} {verdict_text(e)}")
        lines[last_inc + 1:last_inc + 1] = block
        text = "\n".join(lines)
        # убираем лишние пустые строки, появившиеся после вставки
        text = re.sub(r"(\[1\.26\][^\n]*\n)(\n{2,})", r"\1\n", text)
        path.write_text(text.rstrip() + "\n", encoding="utf-8")
        n += 1
    return n


# ─────────────────────────── main ───────────────────────────


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--ll", default=str(DEFAULT_LL), help="путь к клону LeviLamina")
    ap.add_argument("--annotate", action="store_true", help="расставить пометки в исходниках")
    ap.add_argument("--root", default=str(REPO), help="корень проекта Solstice")
    args = ap.parse_args()

    root = Path(args.root)
    ll = Path(args.ll)
    if not (ll / "src" / "mc").exists():
        print(f"не найдены заголовки LeviLamina по пути {ll} (указать: --ll PATH)", file=sys.stderr)
        return 2

    index = build_header_index(ll)
    entries = collect_hooks(root) + collect_sigs(root)

    for e in entries:
        e.note = NOTES.get(e.raw, e.note)
        verdict = MANUAL.get(e.raw)
        if verdict:
            e.status, where, note = verdict
            e.ll_file, e.ll_line = where, ""
            e.decl = ""
            e.note = note
            continue
        hfile = pick_class_file(index, e.cls)
        names = [e.member] + SYNONYMS.get(e.member, [])
        if hfile is not None:
            hit = False
            for nm in names:
                status, ln, decl = find_member(hfile, nm)
                if status != "NOT_IN_CLASS":
                    e.status, e.ll_line, e.decl = status, ln, decl
                    e.ll_file = hfile.relative_to(ll).as_posix()
                    if nm != e.member:
                        e.note = " | ".join(filter(None, [e.note, f"переименовано: {nm}"]))
                    hit = True
                    break
            if not hit:
                e.status, e.ll_file = "NOT_IN_CLASS", hfile.relative_to(ll).as_posix()

        if e.raw in SKIP_GLOBAL:
            e.ll_file = ""
            continue
        if hfile is None or e.status in ("NOT_IN_CLASS", "CLASS_MISSING"):
            g = global_search(ll, names)
            if g:
                e.status = "FOUND_ELSEWHERE"
                e.ll_file, e.ll_line, e.decl = g
                e.note = " | ".join(filter(None, [e.note, "найдено в другом классе — проверь, тот ли это метод"]))
            elif hfile is None:
                e.status = "CLASS_MISSING"

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "audit.md").write_text(render_md(entries, ll, len(index)), encoding="utf-8")
    write_csv(entries, OUT_DIR / "symbol-map.csv")

    print(f"целей: {len(entries)}")
    for st, n in sorted({e.status: sum(1 for x in entries if x.status == e.status) for e in entries}.items()):
        print(f"  {st}: {n}")
    print(f"отчёт: {OUT_DIR/'audit.md'}")
    print(f"csv:   {OUT_DIR/'symbol-map.csv'}")

    if args.annotate:
        print(f"пометок в OffsetProvider.hpp: {annotate_sigs(root, entries)}")
        print(f"файлов хуков помечено: {annotate_hooks(root, entries)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
