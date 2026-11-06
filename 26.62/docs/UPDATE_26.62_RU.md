# Dexko → Minecraft 26.62: статус переноса

Дата: 2026-11-06. База: свежий Solstice 26.62 (surukotonai/solstice, коммит cc6463d).
Старый проект: корень репозитория (Solstice 1.21.44 + модули Dexko).

---

## Что сделано

### Каркас
- Папка `26.62/` пересобрана начисто со свежей базы Solstice 26.62 (старый
  поломанный мердж с конфликтами `<<<<<<<` убран полностью — маркеров не осталось).
- Ребрендинг: `project(Dexko)`, `Dexko.cpp/.hpp` (класс и неймспейс `Dexko`),
  `DEXKO_VERSION`, `DexkoShaders`, строки `solstice → dexko`, `DEXKO FATAL`.
- CMake: одна цель `add_library(Dexko SHARED)`, `__DEBUG__` определён всегда —
  как в старом проекте, чтобы ненайденная сигнатура не убивала клиент через
  `__fastfail`.

### Авторизация и инициализация (Dexko.cpp)
- Твоя авторизация из корня (`Features/Auth/Authorization.*`, `CustomCryptor`),
  вызов включён как в корне: `#ifdef __PRIVATE_BUILD__` → `auth.init()` +
  `isPrivateUser()`.
- `DeviceSpoof` принудительно включается после загрузки конфига (как у тебя).
- Проверка обновлений через `OAuthUtils::getLatestCommitHash()` (у авторов базы
  она была вырезана — `latestHash = "Invalid"`).
- Безопасный eject: `mUnloading` (atomic), отдельный поток выгрузки
  `unloadThreadProc` вместо `FreeLibrary` в текущем потоке, null-safe
  `playUi`/`displayClientMessage`, пауза 300 мс после снятия хуков.

### Перенесено из старого проекта
- **Все ~76 своих модулей** (+5 возвращены после вырезания авторами базы:
  VoiceChat, Regen, RegenRecode, BoneEsp, Goofy).
- 11 своих команд (`.spammer`, `.fakechat`, `.pos1/.pos2`, `.playeroffset`,
  `.spos1/.spos2`, `.scopy/.spaste/.sbuild/.scancel`) — зарегистрированы.
- 5 своих событий (ContainerSlotHovered, EntityHurt, HurtColor, ProjectileHit,
  SetupAndRender) + их диспатчи в хуках.
- ChatBinds (flushPending в BaseTick, handleKey в KeyHook — в 26.62 клавиатура
  идёт через WndProcHook → KeyHook::onKey, проверено).
- GUI-панели: ChatBindsPanel, CombatantGui (интеграция в ClickGui перенесена).
- Свои хуки: ProjectileHooks, AmbienceHook, HurtColorHook (+ HealthHooks
  скопирован, но не регистрируется — как в корне, см. Вопросы).
- Утилиты: ItemAtlas, ItemTextures, ClientPack, CubemapGenerator, PngWriter,
  NurikIcons, CPSTracker, HealthTracker, PlatformTracker и т.д.
- SDK-файлы, KeyboardMouseSettings (нужен GhostMode — база его вырезала),
  vendored `src/glm`, ресурсы (`effect/`, `items/`, шрифты nurik/mntsb/Roboto).
- 7 «базовых» модулей, где твои правки — это целые фичи, смержены твоими
  версиями с учётом API 26.62: ClickGui, CustomChat, Freecam, Glint, Nametags,
  RobloxCamera, TargetHUD.
- Твои незавершённые внутренние мерджи (CombatantGui, OreMinerV2, Kagune,
  TazikHat, Trails, WorldParticles) разрешены в пользу HEAD-версий.

### Совместимость с SDK 26.62
- `ItemStack::mItem` теперь `WeakPtr<Item>` (база): у WeakPtr есть
  `operator bool` — булевы проверки `!stack->mItem` работают как раньше;
  разыменования исправлены (`JavaInventoryHotkeys`, `SetupAndRenderHook`,
  `ContainerActions`).
- `Item`: оффсеты `mItemId 0xAA`, `mName 0xD8` взяты с базы 26.62.
- `TextPacket`: новый variant-based layout + `Messeage()` (используется в
  PacketUtils).
- 3-way слияние твоих правок SDK/Utils/хуков со свежей базой (~60 конфликтов
  разрешено вручную).

### Сигнатуры и оффсеты
- В `SigManager.hpp` добавлен блок **«DEXKO: кастомные сигнатуры»** — 46 твоих
  сигнатур, перенесённых с 1.21.44.
- В `OffsetProvider.hpp` добавлен блок **«DEXKO: кастомные оффсеты»** — 33 шт.
- Твои RE-заметки сохранены: `docs/RE_notes_supervazhno.txt`.

---

## Что осталось (по приоритету)

1. **Собрать в Visual Studio** (x64):
   ```
   cd 26.62
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```
   Первый билд наверняка покажет остаточные ошибки компиляции (перенос делался
   без MSVC под рукой) — кидай список сюда, правим.

2. **Кастомные сигнатуры (46 шт.)** — паттерны сняты с 1.21.44, большинство
   НЕ найдётся на 26.62. Ненайденная сигнатура == 0 → зависимый модуль не
   включать, пока не переснимем. Для переснятия нужен `Minecraft.Windows.exe`
   от 26.62 или дамп IDA/Ghidra (см. docs/RE_notes_supervazhno.txt — там уже
   есть паттерны для 1.21.13x по ItemRenderer/Cubemap).

3. **Кастомные оффсеты (33 шт.)** и помеченные `TODO(26.62)` места в коде:
   - `Item::mArmorItemType 0x244`, `mProtection 0x264` — с 1.21.44;
   - vtable 35 (`getMaxDamage`), 15 (`isBlockItem`) — с 1.21.44;
   - `Level::addParticle` — оффсет функции `sub_1432A4800` с 1.21.44 (!).

4. **OreMinerV2**: runtime ID руд — сверить с 26.62 (сдвигаются каждый контент-апдейт).

5. Проверить живость: `.help`, телепорт, рендер (ESP/ClickGui), чат-хуки,
   OreMiner, Scaffold.

---

## Вопросы (не блокируют сборку)

- **HealthHooks** скопирован, но не регистрируется — ровно как в твоём корне
  (EntityHurtEvent, получается, не диспатчится). Зарегистрировать?
- **SkinCommand.hpp** существует, но нигде не добавлен — как в корне.
- Базовые модули, которые ты сам отключал в корне (IRC, AirSpeed, AutoAccept и
  ~40 других) — оставлены отключёнными (твой кураторский список), плюс добавлены
  5 новых модулей базы 26.62: AutoEat, AutoWalk, PlaceHighlights, Surround,
  ZipLine.
- `RenderItemInHandHook` отключён авторами базы (закомментирован) — оставлено как у них.

## Структура ссылок

- Старый проект (1.21.44): корень репозитория — не тронут.
- Свежая база 26.62 (референс): `/home/user/solstice-ref`.
- Старая база 1.21.44 (референс для сравнений): `/home/user/solstice-old-12144`.
