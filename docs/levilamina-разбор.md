# LeviLamina — что это, зачем, и как в ней искать «секции и книги»

> Репозиторий скачан по твоей просьбе: `/home/user/LeviLamina`
> (ветка `main`, коммит `b3ff77f2`, версия в `tooth.json` — `26.51.5`, таргет BDS `1.26.51`).
> Все пути и строки кода ниже — реальные, из этого клона.

---

## 0. Короткий ответ, если читать только один абзац

**Имя символа и есть твой «указатель».** Тебе не нужно искать адрес.

В LeviLamina функция объявляется в заголовке (`src/mc/...`), а адрес подставляется
**автоматически**: при сборке — через импорт-либу из пакета `bedrockdata`, при запуске —
через `symbolprovider`, который читает базу символов (`bedrock-runtime-data` / symdb)
**конкретно твоей версии** `bedrock_server.exe` и превращает
`?getPosition@Actor@@QEBAAEBVVec3@@XZ` → `0x1415B8D50`.
(На Linux BDS обычно без PDB, там включаются фоллбэки — сигнатуры/адреса, см. §3.3.)

Поэтому искать вручную приходится не адреса, а **смысл** (какой класс/функция за что отвечает)
и **смещения полей** (до них сообщество докапывается отдельно, через IDA). Подробности — ниже.

---

## 1. Что такое LeviLamina (без маркетинга)

Из `README.md`:

> A lightweight, modular, and versatile mod loader for Minecraft Bedrock Edition,
> formerly known as LiteLoaderBDS

То есть это **модлоадер**, а не мод и не «SDK от Mojang». Что она физически делает:

1. **Инжектится в процесс игры.** `tooth.json` показывает реальный набор зависимостей релиза:
   `PreLoader 1.16.3`, `CrashLogger`, `PeEditor` (патчит `bedrock_server.exe`), `bds 1.26.51`,
   `bedrock-runtime-data 26.51.1-server.6`. Мод в `manifest.json` помечен как `"type": "preload-native"`.
2. **Даёт своё API поверх игры** (`src/ll/api/...`): события, команды, формы, i18n, планировщик,
   KV-БД, хуки, логи, работа с пакетами, сервисы (`ll::service::getLevel()`, `getServerNetworkHandler()`, …).
3. **Даёт «фейковые заголовки» игры** (`src/mc/...`) — 17 356 `.h` файлов, сгенерированных
   из отладочных символов BDS. Это и есть твой «каталог библиотеки».
4. **Чинит главную боль нативной разработки** — привязку к версии: вместо
   `SigManager::ActorAnimationControllerPlayer_applyToPose` и таблицы оффсетов в `offset.txt`
   ты пишешь `&ActorAnimationControllerPlayer::$applyToPose`, а адрес/сигнатуру за тебя
   обновляет база символов под версию.

### Из чего состоит клон (карта)

| Путь | Что там |
|---|---|
| `src/ll/api/**` | Публичное API LeviLamina (то, что ты инклудишь в моде): `memory/Hook.h`, `event/`, `command/`, `form/`, `i18n/`, `service/`, `mod/`, `utils/` |
| `src/ll/core/**` | Внутренности самого лоадера (загрузка модов, команды, логи, твики) |
| `src/mc/**` | **Фейковые заголовки BDS** — общие для сервера и клиента (17 356 файлов) |
| `src-server/mc/**` | Оверлей хидеров для серверной сборки (3 836 файлов), подключается в `xmake.lua:202-204` |
| `src-client/mc/**` | Оверлей хидеров для клиентской сборки (3 910 файлов), `xmake.lua:210-213` |
| `src-test/**` | Юнит-тесты + автоген `include_all.cpp`, который компилирует **все** хидеры (проверка, что они валидны) |
| `docs/main/contents/**` | Документация, она же сайт <https://lamina.levimc.org> |
| `xmake.lua` | Сборка; зависимости `bedrockdata v26.51.1-server.6` (символы!) и `symbolprovider v1.3.0` |
| `tooth.json` | Описание пакета для `lip`/tooth: зависимости рантайма, включая `bedrock-runtime-data` |

---

## 2. Твоя аналогия: где она точная, где ломается

Ты сказал: *«Майнкрафт — огромная библиотека, LeviLamina — помощник. Мне нужно поменять цвет неба,
LeviLamina говорит: смотри книгу "Насмешка" в секции "комедия". Но какой смысл, если я не могу
найти ни секцию, ни книгу?»*

Поправлю аналогию — она почти верная, но в ней не хватает одного звена:

| Библиотека | Реальность Bedrock |
|---|---|
| Здание библиотеки | `bedrock_server.exe` / `Minecraft.Windows.exe` — ~100 МБ скомпилированного C++ |
| Книги | Функции и классы. У них есть **настоящие имена** (из PDB/символов): `Actor::getPosition`, `LevelRenderer::renderSky` |
| Секция / стеллаж | Класс и его иерархия (`Player : Mob : Actor : …`) |
| **Номер полки (адрес)** | RVA в бинарнике. **Вот это LeviLamina даёт сама**, из symdb под версию |
| Оглавление | `src/mc/**` — фейковые хидеры |
| Помощник | LeviLamina: лоадер + API + резолвер символов |

**Ломается аналогия вот где:** в библиотеке ты сначала ищешь полку, потом книгу.
Здесь — **наоборот**: ты знаешь название книги (потому что имена Functions честно лежат в PDB),
а номер полки тебе подставляет symdb автоматически, причём ровно для твоей версии игры.

Единственное, чего система за тебя **не** сделает — не скажет, какая книга тебе нужна
(«где именно цвет неба?») и **что написано внутри книги** (поля классов в PDB обычно отсутствуют).

---

## 3. Почему «книга» = «указатель»: механика символов

### 3.1 Заголовки — это `dllimport`

`src/mc/_HeaderOutputPredefine.h`:

```cpp
#define MCAPI  LL_SHARED_IMPORT     // Windows: __declspec(dllimport); Linux: пусто (резолвится в рантайме)
#define MCTAPI template<> MCAPI
#define MCFOLD MCAPI                /*Identical COMDAT Folding*/
#define MCNAPI [[deprecated("This API is not available. Open an issue if you need it. ...")]] MCAPI
```

То есть каждая `MCAPI`-функция в хидере — это **импорт из BDS**. Линкер берёт имя
(декорированное, mangled), а базу «имя → адрес» поставляет пакет `bedrockdata`
(`xmake.lua:55`: `add_requires("bedrockdata v26.51.1-server.6")`).

### 3.2 Рантайм-резолвер

`src/ll/api/memory/Symbol.cpp`:

```cpp
res = pl::symbol_provider::pl_resolve_symbol_silent_n(sym.data(), sym.size());
...
if (!disableErrorOutput && res == nullptr) {
    getLogger().fatal("Couldn't find: {}", toString());   // деманглит имя для читаемости
    getLogger().fatal("In module: {}", sys_utils::getCallerModuleFileName());
}
```

`toString()` — demangler: `?getPosition@Actor@@QEBAAEBVVec3@@XZ`
→ `public: class Vec3 const & __ptr64 Actor::getPosition(void) const __ptr64`.
Если в логе краша видишь «Couldn't find: …» — это ровно твоя ситуация «книга есть, полки нет»:
символа нет в symdb этой версии.

### 3.3 Хук принимает 4 вида «указателей»

`src/ll/api/memory/Hook.h` (строки ~81–100):

```cpp
template <FuncPtrType T>            constexpr FuncPtr resolveIdentifier(T identifier);             // &Class::func
template <class T>                  constexpr FuncPtr resolveIdentifier(SignatureView identifier);  // байт-паттерн
template <class T>                  constexpr FuncPtr resolveIdentifier(SymbolView identifier);     // "?foo@Bar@@..."
template <class T>                  constexpr FuncPtr resolveIdentifier(uintptr_t address);         // сырой адрес
```

и внутри `hook()`:

```cpp
if (!_HookTarget) _HookTarget = ::ll::memory::resolveIdentifier<_OriginFuncType>(IDENTIFIER);
```

Это официально задокументировано в `docs/.../hook_guide.md`:

> `IDENTIFIER`: The identifier used for the Hook lookup function, which can be:
> function's decorated name, function bytecode, or function definition.

То есть **адрес — это последний фоллбэк**, а не основной путь. Основной — указатель на функцию
из хидера или строка символа.

### 3.4 Виртуальные функции — ловушка `$`

В Hook.h есть `static_assert`:

```
#IDENTIFIER " is a virtual function, you need use prefix $ workaround to hook it."
```

Поэтому в хидерах у каждой виртуалки есть «транк» с `$`: `virtual void foo()` → `MCAPI void $foo()`,
и в хуке пишут `&Class::$foo`. (Пример из `hook_guide.md`: `&DedicatedServer::$ctor`.)

---

## 4. Что придётся искать самому (и это и есть твоя боль)

### 4.1 Смысл: «какая книга мне нужна?»

Здесь LeviLamina — только grep-база. Официальный гайд
`docs/main/contents/developer_guides/how_to_guides/find_function_guide.md` честно описывает процесс:

1. Ищешь в `src/mc` по ключевому слову. Пример из гайда: player coordinates → ищешь `getPosition`,
   не находишь в `Player` → смотришь иерархию `Player : public ::Mob` → `Mob : Actor` →
   в `Actor.h` находишь `[[nodiscard]] Vec3 const& getPosition() const`.
2. Открываешь IDA Pro с PDB от BDS и смотришь псевдокод, чтобы убедиться, что функция делает то,
   что ты думаешь (в гайде: `return *(Vec3*)(this + 84)` — «похоже на поле позиции»).
3. Хукуешь/вызываешь.

Второй пример из гайда — перехват чата: ищешь `TextPacket` → `ServerNetworkHandler::handle(...)`
→ читаешь иксрефы и псевдокод → понимаешь, что `this` надо сдвинуть на −16
(поэтому в `ServerNetworkHandler.h` есть обёртка `getServerPlayer(...)`, делающая
`ll::memory::dAccess<ServerNetworkHandler>(this, -16)`).

**Вывод:** «секцию и книгу» ищут через grep по `src/mc` + IDA с PDB. У клиента
(`Minecraft.Windows.exe`) PDB нет, поэтому там всё сводится к сигнатурам (как у тебя в `SigManager`)
или к клентским symdb от сообщества (сборка `src-client`).

### 4.2 Смещения полей: «книга есть, а страницы пустые»

Это главный подвох, которого ты ещё не осознал. Открой, например:

```cpp
// src/mc/client/renderer/scripting/ServerGraphicsOverrideManager.h
class ServerGraphicsOverrideManager {};

// src/mc/deps/minecraft_renderer/framebuilder/RenderSkyDescription.h
namespace mce::framebuilder { struct RenderSkyDescription {}; }

// src/mc/client/renderer/SkyWeatherParameters.h
namespace AtmosphericWeatherConfig { struct SkyWeatherParameters {}; }
```

Имя класса/структуры есть, а полей — **ноль**. PDB даёт имена функциям, но не даёт layout классов.
Поэтому поля в хидерах бывают трёх видов:

1. **Известные** — обычные C++ поля.
2. **`ll::TypedStorage<Align, Size, Type>`** — «здесь лежит `Type`, размер `Size`, выравнивание `Align`,
   но реальный layout мы не знаем» (`src/ll/api/base/Alias.h`: `TypedStorageImpl` с `.get()`, `operator->`,
   неявным приведением к `T&`). Пример из `GraphicsOverrideParameterPacketPayload.h`:

   ```cpp
   ::ll::TypedStorage<8, 64, ::std::unordered_map<float, ::Vec3>> mKeyframes;
   ::ll::TypedStorage<4, 8,  ::std::optional<float>>              mFloatValue;
   ::ll::TypedStorage<4, 16, ::std::optional<::Vec3>>             mVec3Value;
   ::ll::TypedStorage<8, 32, ::std::string>                       mBiomeID;
   ::ll::TypedStorage<8, 40, ::std::optional<::std::string>>      mPlayerID;
   ::ll::TypedStorage<1, 1,  ::GraphicsOverrideParameterType>     mParameterId;
   ::ll::TypedStorage<1, 1,  bool>                                mResetParameter;
   ```

   Здесь размеры (`64`, `8`, `16`, `32`, `40`) — это аккуратно задокументированные смещения,
   найденные кем-то в IDA. Они и есть твой `offset.txt`, только встроенный в типы.
3. **Ручной доступ по смещению** — `ll::memory::dAccess<T>(ptr, offset)` (`src/ll/api/memory/Memory.h`),
   когда поля нет вообще.

Вот точный ответ на «смысл от книги, если мы её найти не можем»:
**имя найти можно (оно в хидере), адрес найдётся сам (symdb), а вот внутренности класса
(какой `this+0x38` = что) часто не найдены никем** — и это нормальная рабочая ситуация,
а не «я чего-то не понимаю».

---

## 5. Разбор твоего примера: «хочу поменять цвет неба»

Давай пройдём путь поиска по-настоящему, по скачанному репо.

### Шаг 1. Кто владеет состоянием?

Главный вопрос, который надо задать **до** хука. Небо:

* **Рисуется на клиенте** (`LevelRenderer` / `mce::framebuilder::RenderSkyDescription` — пустая структура).
* На сервере неба нет вообще: BDS не рендерит.

Значит, у тебя ровно три пути:

| Путь | Как | Когда уместен |
|---|---|---|
| **Данные** | Дата-пак / биом-компонент `SkyColorClientBiomeJsonComponent` (`mSkyColor` типа `SharedTypes::Color255RGB`, см. `src/mc/deps/shared_types/v1_21_40/clientbiome/components/`) | «Хочу другой цвет неба в своём мире» — делается JSON-паком, **без C++** |
| **Пакет клиенту** | `GraphicsOverrideParameterPacket` с `SkyZenithColor` / `SkyHorizonColor` | Сервер хочет динамически менять небо игрокам |
| **Хук рендера** | Клиентская сборка LeviLamina (`src-client`), хук `LevelRenderer::...` | Только клиент-мод/чит, как твой Solstice |

### Шаг 2. Что реально нашлось в хидерах (и это удача)

`src/mc/network/packet/GraphicsOverrideParameterType.h` — энум, который Mojang использует
для графических оверрайдов:

```cpp
enum class GraphicsOverrideParameterType : uchar {
    SkyZenithColor          = 0,
    SkyHorizonColor         = 1,
    HorizonBlendMin         = 2,
    ...
    SunColor                = 41,
    MoonColor               = 43,
    AmbientColor            = 47,
    SkyIntensity            = 50,
};
```

И сам пакет: `GraphicsOverrideParameterPacket : public ::ll::PayloadPacket<GraphicsOverrideParameterPacketPayload>`,
у пэйлоада — поля выше и конструкторы:

```cpp
MCAPI GraphicsOverrideParameterPacketPayload(
    ::GraphicsOverrideParameterType paramId,
    ::std::unordered_map<float, ::Vec3>&& keyframes,   // время → цвет (для Vec3-параметров)
    ::std::string const& biomeId,
    ::std::optional<::std::string> const& playerId
);
```

Отправка — `src/mc/world/actor/player/Player.h:729`:

```cpp
virtual void sendNetworkPacket(::Packet& packet) const;
```

### Шаг 3. Скелет кода (иллюстрация; цифры/семантику проверяй по IDA/эксперименту)

```cpp
#include "mc/network/packet/GraphicsOverrideParameterPacket.h"
#include "mc/network/packet/GraphicsOverrideParameterType.h"
#include "mc/world/actor/player/Player.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"

static void setSkyZenith(Player& player, Vec3 color /* 0..1 */) {
    std::unordered_map<float, Vec3> keyframes{{0.0f, color}};  // keyframe на «всё время»

    GraphicsOverrideParameterPacket pkt(
        GraphicsOverrideParameterType::SkyZenithColor,
        std::move(keyframes),
        "*",            // biomeId: "*" вероятно = все биомы (проверь!)
        std::nullopt    // playerId: пусто = применить глобально
    );
    player.sendNetworkPacket(pkt);
}

// Подписка на событие (реальный паттерн из src/ll/api/service/PlayerInfo.cpp:50)
auto listener = ll::event::EventBus::getInstance()
    .emplaceListener<ll::event::PlayerJoinEvent>([](ll::event::PlayerJoinEvent& ev) {
        setSkyZenith(ev.self(), Vec3{0.9f, 0.2f, 0.2f});   // красное небо
    });
```

Обрати внимание: **ни одного адреса, ни одной сигнатуры, ни одного оффсета руками.**
Имена классов/пакетов → компилятор → symdb → работает.

А теперь сравните с тем же в твоём текущем стиле (Solstice):

```cpp
mDetour = std::make_unique<Detour>("...", reinterpret_cast<void*>(SigManager::ActorAnimationControllerPlayer_applyToPose), &onActorModel);
auto ent = *reinterpret_cast<Actor**>(a2 + 0x38);   // хардкод оффсета
```

### Шаг 4. Мораль примера

Для «цвета неба» **хук вообще не нужен** — состояние неба owned клиентом и управляется
данными/пакетами. Хук нужен там, где поведения нет в данных: «не дать ударить», «изменить
дамаг», «перехватить пакет», «добавить команду». Это и есть то самое «найти секцию»:
сначала понять, **кто владеет состоянием**, а потом уже искать имя.

---

## 6. Чеклист: как искать «секцию и книгу» (рабочий процесс)

1. **Сформулируй, что именно меняется** (состояние, которое рендерится / считается / передаётся по сети).
2. **Определи сторону**: сервер (`src-server`) или клиент (`src-client`). Ошибка тут = неделя впустую.
3. **grep по хидерам** — это самый быстрый способ:

   ```bash
   cd /home/user/LeviLamina
   grep -rn "Sky" src/mc --include=*.h -i        # ищем по смыслу
   grep -rn "sendNetworkPacket" src/mc --include=*.h
   ```
4. **Если имя нашлось** — посмотри, заполнен ли класс. Пустое тело `struct X {};` или куча
   `TypedStorage` → layout неизвестен, читай следующий пункт.
5. **IDA Pro (+ PDB для BDS)** — подтвердить семантику функции и найти оффсеты полей.
   Именно этому посвящён `find_function_guide.md` (примеры с `getPosition` и `TextPacket`).
6. **Выбери способ воздействия** в порядке приоритета:
   готовое событие LeviLamina (`ll/api/event/**`) → вызов функции из хидера → пакет/дата-пак →
   хук (`LL_AUTO_TYPE_INSTANCE_HOOK`) → ручной оффсет (`dAccess`).
7. **Зафиксируй версию.** Хидеры живут под конкретную версию BDS (`bedrockdata v26.51.1-server.6`),
   и обновление игры = обновление пакетов.

### Готовые события (чтобы не писать хуки)

`src/ll/api/event/` уже содержит: `player/PlayerJoinEvent`, `PlayerChatEvent`, `PlayerAttackEvent`,
`PlayerPlaceBlockEvent`, `PlayerDieEvent`, `PlayerUseItemEvent`, `world/ServerLevelTickEvent`,
`BlockChangedEvent`, … Плюс сервисы `ll::service::getLevel()`, `getServerNetworkHandler()`,
`getCommandRegistry()` (`src/ll/api/service/Bedrock.h`).

---

## 7. LeviLamina vs твой текущий подход (Solstice: MinHook + SigManager + offset.txt)

| | Сейчас у тебя | С LeviLamina |
|---|---|---|
| Поиск функции | `hat::compile_signature<...>` + `SigManager` | `&Class::$method` или строка символа |
| Обновление версии | все сигнатуры/оффсеты нужно переснимать (`offset.txt`, 34 КБ хардкода) | обновляется symdb пакетом, твой код не меняется |
| Оффсеты полей | хардкод (`a2 + 0x38`) | `TypedStorage` + `dAccess`, часть уже заполнена |
| Инфраструктура | свою писал сам (диспетчер событий, фичи) | события, команды, формы, i18n, БД, планировщик — готовы |
| Краши | свои | `CrashLogger` + демангленные стектрейсы |
| Ограничения | любой таргет | привязка к версии BDS/клиента, Windows-ориентированно, свой EULA, LGPL-3.0 для открытой части |

То есть LeviLamina тебе **как автору клиентского чита** даёт не «возможность менять небо»
(это ты и так умеешь), а: (1) исчезновение ада при обновлении версий, (2) готовую инфраструктуру,
(3) гигантский справочник имён игры, по которому можно grep'ать, вместо угадывания по ассемблеру.

---

## 8. Что почитать в самом клоне (порядок)

1. `README.md` → `docs/main/contents/developer_guides/tutorials/create_your_first_mod.md`
2. `docs/main/contents/developer_guides/how_to_guides/hook_guide.md` — хуки (10 минут)
3. `docs/main/contents/developer_guides/how_to_guides/find_function_guide.md` — **это ответ на твой вопрос «как искать»**
4. `src/ll/api/memory/Hook.h` — как резолвится идентификатор хука
5. `src/ll/api/memory/Symbol.h/.cpp` — как имя превращается в адрес
6. `src/mc/_HeaderOutputPredefine.h` — что такое `MCAPI` / `MCFOLD` / `MCNAPI`
7. `src/ll/api/base/Alias.h` — `TypedStorage` (почему поля «пустые»)
8. Любой хидер игры, например `src/mc/world/actor/Actor.h` — посмотри на структуру:
   виртуалки, `$`-транки, `MCAPI`-функции, `MCNAPI`-заглушки.

---

## 9. FAQ по твоим же вопросам

**«Какой смысл от символов, если у меня нет на них указателей?»**
Символ — это и есть указатель, только в форме строки. `symbolprovider` + symdb превращают
строку в адрес при загрузке. Указатель тебе выдает компилятор, когда ты пишешь `&Actor::getPosition`.

**«Как найти нужную секцию/книгу?»**
grep по `src/mc` + IDA Pro с PDB + чтение псевдокода и иксрефов. Готового «огромного словаря
по смыслам» нет и быть не может: Mojang не документирует внутренности. Есть имена (из символов)
и есть опыт сообщества (в виде `TypedStorage`-размеров и обёрток в хидерах).

**«Почему столько пустых структур?»**
Потому что PDB даёт имена функциям, но не layout классов. Пустая структура = «мы знаем, что
такой тип есть, но не знаем, что внутри». Заполняется постепенно, руками, с IDA.

**«Зачем тогда LeviLamina, если я всё равно открываю IDA?»**
IDA нужна, чтобы **понять**. адреса, импорты, события, команды, формы, обновление версий —
это LeviLamina. Без неё ты делаешь то же самое, но ещё и вручную поддерживаешь
17 тысяч сигнатур.

**«А если символа нет в symdb (Linux/нет PDB)?»**
Есть фоллбэки: `SignatureView` — байт-паттерн (`src/ll/api/memory/Signature.h`), и сырой адрес.
То есть ровно то, чем ты занимаешься сейчас — но как запасной вариант, а не основной.
