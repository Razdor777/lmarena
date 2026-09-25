@echo off
rem ─────────────────────────────────────────────────────────────────────────────
rem  dev.bat — быстрая сборка Solstice для итераций.
rem
rem      dev.bat            -> dev-профиль (build-dev): без /GL + /LTCG, /MP,
rem                            инкрементальная линковка. Для правок кода.
rem      dev.bat release    -> как раньше (build): полный /GL + /LTCG.
rem
rem  Configure выполняется ТОЛЬКО при первом запуске (или когда менялся
rem  CMakeLists.txt), дальше — чистая сборка без пересборки всего проекта.
rem ─────────────────────────────────────────────────────────────────────────────
chcp 65001 >nul
setlocal enabledelayedexpansion
cd /d "%~dp0"

set MODE=%1
if "%MODE%"=="" set MODE=dev

if /I "%MODE%"=="release" (
    set BUILD_DIR=build
    set CFG_ARGS=-DSOLSTICE_FAST_DEV=OFF
    set PROFILE=release, полные оптимизации
) else (
    set BUILD_DIR=build-dev
    set CFG_ARGS=-DSOLSTICE_FAST_DEV=ON
    set PROFILE=dev, быстрая итерация
)

echo [solstice] профиль: !PROFILE! ^| каталог: !BUILD_DIR!
echo [solstice] старт: %DATE% %TIME%

if not exist "!BUILD_DIR!\CMakeCache.txt" (
    echo [solstice] первый configure, нужна сеть для зависимостей...
    cmake -S . -B !BUILD_DIR! -G "Visual Studio 17 2022" -A x64 !CFG_ARGS!
    if errorlevel 1 (
        echo [solstice] configure упал
        endlocal & exit /b 1
    )
)

rem ВАЖНО: без повторного "cmake -S . -B ..." — иначе CMake перегенерирует
rem сгенерированные заголовки и MSVC пересоберёт половину проекта.
cmake --build !BUILD_DIR! --config Release --target Solstice -j
if errorlevel 1 (
    echo [solstice] сборка упала
    endlocal & exit /b 1
)

echo [solstice] готово: %DATE% %TIME%
echo [solstice] dll: !BUILD_DIR!\Release\Solstice.dll
endlocal
