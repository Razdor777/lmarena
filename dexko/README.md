# Dexko

Отдельный лёгкий клиент для **Minecraft Bedrock 1.21.44** (Windows x64).
Собран из минимального набора исходников: всё, что не используется, вырезано —
компилируется только то, что реально работает в игре, поэтому `.dll` получается
заметно меньше обычной сборки.

## Что внутри

Модули:

- **DeviceSpoof** — спуф устройства, включается автоматически при запуске.
- **Reach** — боевое досягаемость (attack range) + режим *Full Override*,
  который снимает «зажим» игры на ~7 блоках.
- **CustomCrosshair** — динамический прицел (стили, пресеты, sway).
- **HitParticles** — частицы при попадании: 8 стилей
  (`blood, sparks, critical, hearts, fire, frost, soul, toxic`).
- **Interface** — тема, шрифт, оформление (скрытая служебная).

ClickGUI в клиенте нет — всё управление идёт через команды в чате.

## Команды

Досягаемость:

- `.reach` — текущее значение и режим.
- `.reach 5` — поставить досягаемость и включить модуль.
- `.reach 10 full` — поставить 10 **и** включить Full Override (без него игра
  срезает всё, что выше ~7 блоков — это её собственный кламп).
- `.reach full` / `.reach min` — переключить только режим.

HitParticles:

- `.htp` — текущий стиль, список стилей, справка.
- `.htp hearts` — поставить стиль (можно `heart`, `h`, `4` — сокращения и номера).
- `.htp test` / `.htp test fire` — показать эффект прямо перед собой, не трогая
  сохранённый стиль.
- `.htp list` — список стилей с номерами.
- `.htp color` / `.htp color style` / `.htp color #ff3355` — откуда частицы берут
  цвет (палитра стиля / тема клиента / свой цвет).
- `.htp on|off|toggle` — состояние модуля.

Остальное:

- `.config <load|save|list|delete|default> [name]` — конфиги.
- `.toggle <module>` — вкл/выкл модуль.
- `.set <module> <setting> <value>` — изменить настройку.
- `.crosshair <on|off>` — прицел.
- `.help [command]` — справка.

Каждое изменение стиля частиц сразу показывает превью впереди игрока — так видно,
что команда действительно сработала (раньше результат был виден только при ударе).

## Данные

Логи, конфиги и настройки: `%LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\RoamingState\Dexko\`
(`Configs\`, `preferences.json`, `dexko.log`).

Кнопка выгрузки клиента из игры — **END**.

## Сборка

Требования: Visual Studio 2022 (x64), CMake, MSYS2 mingw-w64 (нужен `ld`),
доступ в сеть (CPM скачивает зависимости).

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Dexko -- -m
```

Готовый файл: `build/Release/Dexko.dll`.

## Размер

Ничего лишнего в сборке не осталось.

Вырезано из `src/` (этих файлов больше нет):

- мёртвые утилиты: `AudioUtils`, `SoundUtils`, `ItemTextures`, `ItemUtils`,
  `BlockUtils`, `DataStore`, `CommandUtils`, `CubemapGenerator`, `PngWriter`,
  `ClientPack`, `MessageTemplate`, `HttpRequest`, `OAuthUtils`, `HWUtils`,
  `RegUtils`, `NotifyUtils`, `PacketUtils`;
- неиспользуемые события (20+ заголовков: растровые, хук-события без
  слушателей, `WindowResize`, `Notify`, `PacketIn` и т. д.);
- неиспользуемые хуки: перехват входящих пакетов (его событие никто не слушал);
- пустые файлы-заглушки (`JSON.cpp`, `Options.cpp`, `Resource.cpp`,
  `SHA256.cpp` — всё, что в них было, объявлено в заголовках);
- мёртвые функции: форматирование текста в `ChatUtils` (6 штук), работа с
  крипто-строками и хешами в `StringUtils` (18 штук), загрузка текстур из
  ресурсов в `D3DHook`;
- неиспользуемые пакеты (`SetPlayerGameTypePacket`, `UpdateBlockPacket`,
  `ModalFormResponsePacket`).

В сборку больше не компилируются (файлы на месте, но исключены из CMake):
`imgui_demo.cpp`, `imgui_impl_dx12.cpp`, `imgui_toggle*.cpp` — ни одна из этих
функций в клиенте не вызывается.

Плюс:

- библиотека DirectXTK больше не скачивается и не линкуется (не использовалась
  ни в одном файле), убраны неиспользуемые импорт-библиотеки (winmm, ws2_32,
  comctl32, Mmdevapi);
- из ресурсов убраны шрифты Tenacity-Icons и Nurik и файл `skinblinker.txt`
  (30 КБ, ни один код их не читал);
- в Release включены `/OPT:REF` и `/OPT:ICF` — лишний код не попадает в `.dll`
  даже косвенно.
