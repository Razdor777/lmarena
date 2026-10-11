# Dexko

Отдельный лёгкий клиент под Minecraft Bedrock (Windows/UWP). Управление только
через команды в чате: графического меню нет вообще.

## Модули

- **DeviceSpoof** — спуф устройства/подписи, включён по умолчанию.
- **Reach** — боевая и блочная дистанция (`.reach`).
- **CustomCrosshair** — динамический прицел (`.crosshair on|off`).
- **HitParticles** — частицы при попадании (`.htp`).
- Служебный **Interface** — тема, цвета, шрифт, arraylist.

## Команды

- `.reach <1.00-20.00>` — боевая дистанция, автовключает Reach.
  - `.reach force <on|off>` — экспериментально: не «обрезать» значение движка, а
    подставлять своё. Только так работают значения выше ~7 блоков.
  - `.reach reset` — вернуть 3.00.
  - `.reach` — текущее состояние.
- `.htp <стиль|номер>` — стиль частиц: `blood, sparks, critical, hearts, fire,
  frost, soul, toxic` (или `.htp 3`).
  - `.htp test` — выпустить вспышку перед собой, чтобы сразу увидеть стиль.
  - `.htp list` — список стилей, `.htp` — текущее состояние.
- `.crosshair <on|off>` — прицел.
- `.config <load|save|list|delete|default> <имя>` — конфиги.
- `.toggle <модуль> <on|off>` — вкл/выкл модуль.
- `.set <модуль> <настройка> <значение>` — изменить настройку.
- `.help [команда]` — справка.

## Сборка

Требования: Visual Studio 2022 (x64), CMake, MSYS2 mingw-w64 (для `ld`), доступ в
сеть (CPM качает зависимости).

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Dexko -- -m
```

Готовый файл: `build/Release/Dexko.dll`.

Данные/конфиги/лог: `%LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\RoamingState\Dexko\`
