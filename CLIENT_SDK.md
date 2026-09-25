# Клиентский SDK (заголовки LeviLamina) — готовые раскладки структур

Из песочницы доступен `codeload.github.com`, поэтому исходники LeviLamina нужных версий
берутся **напрямую**, без ручной пересылки zip'ов:

```bash
git clone --depth 1 --branch v1.9.9 --filter=blob:none --sparse https://github.com/LiteLDev/LeviLamina /tmp/ll199
cd /tmp/ll199 && git sparse-checkout set src-client     # ~5000 заголовков клиента, 25 МБ
```

Почему это ценно: заголовки сгенерированы **из PDB игры**, и каждый член класса описан как
`::ll::TypedStorage<выравнивание, размер, тип> имя;` — то есть размер и тип каждого поля известны
точно, а из порядка полей восстанавливаются смещения.

## Соответствие версий (из `docs/main/contents/versions.md`)

| LeviLamina | Клиент | Сервер |
|---|---|---|
| 1.9.x | **1.21.132.1** | 1.21.132.3 |
| 1.8.x | **1.21.124.2** | 1.21.124.2 |
| 26.10.x / 26.20.x / 26.32.x / 26.40.x / 26.51.x | 26.10.4 / 26.20.4 / 26.32.2 / 26.40.5 / 26.51.1 | 26.10.4 … 26.51.1 |

Для 1.21.44 клиентских заголовков нет ни у одного тега — ближайшие это 1.21.124 (v1.8.x)
и 1.21.132 (v1.9.x).

## Инструмент

```bash
python tools/ll_layouts.py /tmp/ll199/src-client --cache /tmp/ll.pkl --stats
# классов всего: 4687
#   раскладка посчитана полностью: 1526
#   частично (нет базы/неизвестный тип): 494
#   не посчитано (пустые/без полей): 2667

python tools/ll_layouts.py /tmp/ll199/src-client --cache /tmp/ll.pkl --list "Fog|Renderer" --limit 20
python tools/ll_layouts.py /tmp/ll199/src-client --cache /tmp/ll.pkl --class GameRenderer --show
python tools/ll_layouts.py /tmp/ll199/src-client --cache /tmp/ll.pkl --json out.json --filter "Fog|Screen"
python tools/ll_layouts.py /tmp/ll199/src-client --cache /tmp/ll.pkl --header sdk.hpp --filter "FogSetting"
```

`partial=True` означает, что базовый класс не найден среди заголовков (в сборке их неполный набор,
например нет `LevelListener`) — у таких классов смещения сдвинуты и использовать их нельзя
без ручной проверки. Полностью посчитанные классы (1526) безопасны.

Готовая выгрузка по ключевым классам: `tools/generated/client_1_21_132_layouts.json`.

## Что уже точно известно для 1.21.132 (полностью посчитанные)

```
FogDistanceSetting  size 0x1C
   +0x00 mce::Color mColor (16)     +0x10 float mStart
   +0x14 float mEnd                 +0x18 DistanceType mType (int)

FogSetting          size 0x48
   +0x00 FogDistanceSetting mDistanceSetting (0x1C)
   +0x1C FogTransitionSetting mTransitionSetting (0x2C)

FogTransitionSetting size 0x2C
   +0x00 FogDistanceSetting (0x1C)  +0x1C float  +0x20 float  +0x24 float  +0x28 float

FogVolumetricDensitySetting    size 0x10  (float, float, float, bool)
FogVolumetricCoefficientSetting size 0x20 (два mce::Color)

FogDefinition       size 0x2D8
   +0x00 HashedString mIdentifier (48)
   +0x30 std::optional<FogSetting> mDistanceAirSetting (76)
   +0x7C ...Weather  +0xC8 ...Water  +0x114 ...Lava  +0x160 ...LavaResistance  +0x1AC ...PowderSnow
   далее optional<FogVolumetricDensitySetting> (20) x5, optional<FogVolumetricCoefficientSetting> (36) x3,
   optional<FogVolumetricHenyeyGreensteinGSetting> (8) x2

ScreenContext       size 0x110   (frameBufferObject, viewport, guiData, clock, tessellator, ...)
GameRenderer        size 0x500
LevelRenderer       size 0x9E8   (PARTIAL — нет баз LevelListener/AppPlatformListener)
ClientInstance      size 0xB70   (PARTIAL)
LocalPlayer         size 0x4E8   (PARTIAL)
```

Как это соотносится с текущим кодом: жёсткие смещения из `AmbienceHook.cpp` (`a3 + 104` для цвета
тумана, `a3 + 120` для «дистанции») попадают внутрь `optional<FogSetting>` в `FogDefinition`
(`+0x30..0x7C` — значение, `+0x78` — флаг engaged), то есть писали не в поля тумана, а в
`mTransitionSetting` и в служебный флаг `std::optional`. Настоящие поля — `mColor` (+0x00),
`mStart` (+0x10), `mEnd` (+0x14) внутри `FogDistanceSetting`.

## Ограничения

1. Данные — для **1.21.132**, а не для 1.21.44. Для текущей версии нужен её exe/PDB или RE.
2. Набор классов неполный (нет части базовых классов) — для 1526 классов раскладка полная,
   остальные требуют ручной проверки.
3. Заголовки дают **раскладку**, но не адреса функций: для хуков по-прежнему нужны
   сигнатуры (`tools/sig_migrate.py`) и/или символы (`tools/pdb_dump.py`).
