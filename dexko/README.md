# Dexko

Лёгкий клиент с минимальным набором функций: боевой reach, частицы при ударе, прицел и спуф устройства.

## Модули

- **DeviceSpoof** — подмена идентификаторов устройства (включён при запуске).
- **CustomCrosshair** — настраиваемый динамический прицел.
- **HitParticles** — частицы при попадании (стили: blood, sparks, critical, hearts, fire, frost, soul, toxic).
- **Reach** — увеличение дальности атаки.
- **Interface** — служебный модуль: цвета и тема интерфейса.

Управление — через команды в чате.

## Команды

- `.reach <value>` (alias `.r`) — установить дальность атаки, например `.reach 4.5`; включает Reach. Без аргумента показывает текущее значение.
- `.htp <style>` (alias `.hitparticles`) — стиль HitParticles: `.htp sparks`, `.htp fire`, `.htp hearts`, `.htp blood`, `.htp critical`, `.htp frost`, `.htp soul`, `.htp toxic`.
- `.crosshair` (alias `.ch`) — настройки прицела.
- `.config <load/save/list/delete/default> <name>` (alias `.c`) — конфиги.
- `.toggle <module>` (alias `.t`) — вкл/выкл модуль.
- `.set <module> <setting> <value>` (alias `.s`) — изменить настройку.
- `.help [command]` (alias `.?`) — справка по командам.

## Сборка

Требования: Visual Studio 2022 (x64), CMake, MSYS2 mingw-w64 (для `ld`), доступ в сеть (CPM качает зависимости).

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Dexko -- -m
```

Готовый файл: `build/Release/Dexko.dll`.

## Данные

Конфиги и настройки хранятся в `%LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\RoamingState\Dexko\`.
