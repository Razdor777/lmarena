# Dexko

Лёгкий отдельный клиент, из которого вырезано всё лишнее.

## Что осталось

Модули:
- **DeviceSpoof** — всегда включён при запуске (спуф устройств).
- **CustomCrosshair** — динамический прицел (стили, пресеты, sway).
- **HitParticles** — частицы при попадании (стили: blood/sparks/critical/hearts/fire/frost/soul/toxic).
- **Reach** — только боевое досягаемость (attack range).
- Скрытые служебные: `Interface` (тема/цвета), `Notifications` (тосты).

ClickGUI убран — всё управление через команды в чате.

## Команды

- `.reach 3.00` — установить боевое досягаемость (3.00–7.00), автовключает Reach.
- `.htp <style>` — стиль HitParticles: `.htp sparks`, `.htp fire`, `.htp hearts`, `.htp blood`, `.htp critical`, `.htp frost`, `.htp soul`, `.htp toxic`.
- `.config <load/save/list/delete/default> <name>` — конфиги.
- `.toggle <module>` — вкл/выкл модуль.
- `.crosshair <on/off>` — вкл/выкл прицел.
- `.set <module> <setting> <value>` — изменить настройку.
- `.help [command]` — справка по командам.

## Сборка

Требования: Visual Studio 2022 (x64), CMake, MSYS2 mingw-w64 (для `ld`), доступ в сеть (CPM качает зависимости).

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Dexko -- -m
```

Готовый файл: `build/Release/Dexko.dll`.

Данные/конфиги/лог: `%LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\RoamingState\Dexko\`
