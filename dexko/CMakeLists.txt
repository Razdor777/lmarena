# ============================================================================
#  Dexko — lightweight client (project-inside-a-project)
#
#  Самодостаточный под-проект: собирается отдельно от Solstice в Dexko.dll.
#  Использует общие "движковые" части родительского репо (SDK, Hook, Utils,
#  инфраструктура Features), а всё собственное живёт в dexko/src.
#
#  Сборка (из корня репо или из dexko/):
#      cmake -B dexko/build -S dexko
#      cmake --build dexko/build --config Release
#  Результат: dexko/build/Release/Dexko.dll
#
#  Скопировать проект: достаточно папки dexko/ + родительского репо рядом
#  (движок берётся по относительному пути ../src, ../include, ../resources).
# ============================================================================
cmake_minimum_required(VERSION 3.28)

# CPM лежит в родительском репо
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/CPM.cmake)

# Родительский корень (движок)
set(Dexko_PARENT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

project(Dexko)

# throw error if building for x86
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Dexko only supports x64 builds.")
endif ()

# Пarity с корневым проектом: те же дефайны, чтобы поведение движка
# байт-в-байт совпадало с Solstice-сборками.
add_compile_definitions(__PRIVATE_DEBUG__)
add_compile_definitions(__PRIVATE_BUILD__)
add_compile_definitions(__DEBUG__)
add_compile_options(/utf-8)

# ld из MSYS2 нужен только для запаковки ресурсов (см. ниже)
find_program(LD_COMMAND ld)
if (NOT LD_COMMAND)
    find_program(LD_COMMAND ld HINTS "C:/msys64/mingw64/bin")
endif ()

execute_process(COMMAND ${LD_COMMAND} --version OUTPUT_VARIABLE LD_COMMAND_AVAILABLE)
message(STATUS "ld found at ${LD_COMMAND}")
if (NOT LD_COMMAND_AVAILABLE)
    message(FATAL_ERROR "The ld command is not available on your system. \nPlease install using `pacman -S mingw-w64-x86_64-binutils` in MSYS2, then add the bin directory to your PATH.\nAfter that, restart your IDE and CMake should be able to find the ld command.")
endif ()

set(CMAKE_UNITY_BUILD TRUE)

CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        VERSION 1.9.2
)

CPMAddPackage(
        NAME magic_enum
        GITHUB_REPOSITORY Neargye/magic_enum
        VERSION 0.9.5
)

CPMAddPackage(
        NAME glm
        GITHUB_REPOSITORY g-truc/glm
        GIT_TAG 1.0.1
)

CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.11.3
)

CPMAddPackage(
        NAME DirectXTK
        GITHUB_REPOSITORY microsoft/DirectXTK
        GIT_TAG main
)

CPMAddPackage(
        NAME cryptopp-cmake
        GITHUB_REPOSITORY abdes/cryptopp-cmake
        GIT_TAG master
)

if(MSVC)
    add_compile_options(/bigobj)
endif()

find_package(OpenMP REQUIRED)

add_custom_target(generate_build_info
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/GenerateBuildInfo.cmake
        COMMENT "Generating Dexko build info [${CMAKE_BINARY_DIR}/build_info.h]"
)

include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 26)

add_compile_definitions(NOMINMAX)
add_compile_definitions(JM_XORSTR_DISABLE_AVX_INTRINSICS)

add_compile_definitions(ENTT_SPARSE_PAGE=2048)
add_compile_definitions(ENTT_PACKED_PAGE=128)

# Версия: движковый код (форк ModuleManager) использует DEXKO_VERSION,
# а SOLSTICE_VERSION определён как алиас на случай чужих строк.
set(DEXKO_VERSION "v1.0.0")
add_compile_definitions(DEXKO_VERSION="${DEXKO_VERSION}")
add_compile_definitions(SOLSTICE_VERSION="${DEXKO_VERSION}")

add_definitions(-DWIN32_LEAN_AND_MEAN)

# ---------------------------------------------------------------------------
# Debug / Release флаги — 1:1 с корневым CMakeLists
# ---------------------------------------------------------------------------
add_link_options($<$<CONFIG:Debug>:/INCREMENTAL>)
add_link_options($<$<CONFIG:Debug>:/DEBUG>)
add_compile_options($<$<CONFIG:Debug>:/Zi>)

add_link_options($<$<CONFIG:Release>:/Zi>)
add_link_options($<$<CONFIG:Release>:/INCREMENTAL:NO>)
add_link_options($<$<CONFIG:Release>:/LTCG>)
add_link_options($<$<CONFIG:Release>:/DEBUG>)
add_compile_options($<$<CONFIG:Release>:/O2>)
add_compile_options($<$<CONFIG:Release>:/Oi>)
add_compile_options($<$<CONFIG:Release>:/GL>)
add_compile_options($<$<CONFIG:Release>:/Gy>)
add_compile_options($<$<CONFIG:Release>:/Gw>)
add_compile_options($<$<CONFIG:Release>:/Gm->)
add_compile_options($<$<CONFIG:Release>:/GF>)
add_compile_options($<$<CONFIG:Release>:/fp:fast>)
add_compile_options($<$<CONFIG:Release>:/GR->)
add_compile_options($<$<CONFIG:Release>:/Gd>)
add_compile_options($<$<CONFIG:Release>:/MT>)
add_compile_options($<$<CONFIG:Release>:/Ob2>)
add_compile_options($<$<CONFIG:Release>:/Ot>)
add_compile_options($<$<CONFIG:Release>:/Oy->)

# ---------------------------------------------------------------------------
# Include paths. ПОРЯДОК ВАЖЕН: dexko/src стоит первым, поэтому движковые
# файлы, делающие #include <Solstice.hpp>, получают Dexko-версию заголовка.
# ---------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
include_directories(${Dexko_PARENT_DIR})
include_directories(${Dexko_PARENT_DIR}/src)
include_directories(${Dexko_PARENT_DIR}/include)
include_directories(${Dexko_PARENT_DIR}/include/minhook/include)
include_directories(${Dexko_PARENT_DIR}/include/libhat)
include_directories(${Dexko_PARENT_DIR}/include/entt)
include_directories(${Dexko_PARENT_DIR}/include/libhat/libhat)
include_directories(${Dexko_PARENT_DIR}/include/ImGui)
include_directories(${Dexko_PARENT_DIR}/include/Kiero)
include_directories(${Dexko_PARENT_DIR}/include/nes)

# ---------------------------------------------------------------------------
# Исходники.
#   Движок: SDK + Hook + Utils целиком, инфраструктура Features (без модулей
#   Solstice и без лишних команд).
#   Dexko:  всё из dexko/src (свои модули, команды, точка входа).
# ---------------------------------------------------------------------------
file(GLOB_RECURSE engine_sdk    ${Dexko_PARENT_DIR}/src/SDK/*.cpp)
file(GLOB_RECURSE engine_hook   ${Dexko_PARENT_DIR}/src/Hook/*.cpp)
file(GLOB_RECURSE engine_utils  ${Dexko_PARENT_DIR}/src/Utils/*.cpp)

# FileUtils.cpp заменяется dexko-форком (папка Dexko вместо Juzdex)
list(FILTER engine_utils EXCLUDE REGEX ".*/src/Utils/FileUtils\\.cpp$")

set(engine_features
        ${Dexko_PARENT_DIR}/src/Features/FeatureManager.cpp
        ${Dexko_PARENT_DIR}/src/Features/Configs/ConfigManager.cpp
        ${Dexko_PARENT_DIR}/src/Features/Configs/PreferenceManager.cpp
        ${Dexko_PARENT_DIR}/src/Features/Modules/Module.cpp
        ${Dexko_PARENT_DIR}/src/Features/Modules/Setting.cpp
        ${Dexko_PARENT_DIR}/src/Features/Command/Command.cpp
        ${Dexko_PARENT_DIR}/src/Features/Command/Commands/ConfigCommand.cpp
        ${Dexko_PARENT_DIR}/src/Features/Command/Commands/HelpCommand.cpp
        ${Dexko_PARENT_DIR}/src/Features/Command/Commands/ToggleCommand.cpp
        # инфраструктурные модули, на которые опирается движок
        ${Dexko_PARENT_DIR}/src/Features/Modules/Visual/Interface.cpp   # цвета темы (ColorUtils)
        ${Dexko_PARENT_DIR}/src/Features/Modules/Misc/AntiBot.cpp       # фильтр ботов (ActorUtils)
)

file(GLOB_RECURSE dexko_sources ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

# Resources.cpp генерируется ниже по ходу конфигурации — добавляем явно,
# чтобы не зависеть от повторного GLOB при переконфигурации.
set(dexko_generated_src
        ${Dexko_PARENT_DIR}/src/Utils/Resources.cpp
)

set(sourceFiles
        ${engine_sdk}
        ${engine_hook}
        ${engine_utils}
        ${engine_features}
        ${dexko_sources}
        ${dexko_generated_src}
)

file(GLOB_RECURSE includeFiles ${Dexko_PARENT_DIR}/include/*.hpp)
set_source_files_properties(${sourceFiles} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
set_source_files_properties(${includeFiles} PROPERTIES CXX_SCAN_FOR_MODULES OFF)

# ---------------------------------------------------------------------------
# Ресурсы: используем ресурсы родительского репо (шрифты и т.п.).
# Генерация Resources.hpp/cpp пишется в движковый src/Utils (как у Solstice),
# содержимое идентично, поэтому оба проекта могут жить в одном дереве.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE resource_files_list ${Dexko_PARENT_DIR}/resources/*)

function(add_resources out_var)
    set(result)
    foreach(in_f ${ARGN})
        # in_f приходит как "resources/<относительный путь>" от корня репо,
        # как и в корневом CMake — от этого зависят имена символов от ld
        file(RELATIVE_PATH src_f ${Dexko_PARENT_DIR} ${Dexko_PARENT_DIR}/${in_f})
        set(out_f "${PROJECT_BINARY_DIR}/${in_f}.o")
        add_custom_command(OUTPUT ${out_f}
                COMMAND ${LD_COMMAND} -r -b binary -o ${out_f} ${src_f}
                DEPENDS ${Dexko_PARENT_DIR}/${in_f}
                WORKING_DIRECTORY ${Dexko_PARENT_DIR}
                COMMENT "Building object ${out_f}"
                VERBATIM
        )
        list(APPEND result ${out_f})
        message(STATUS "Embedding resource: ${in_f}")
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

set(resource_files "")
foreach(file ${resource_files_list})
    file(RELATIVE_PATH relative_file ${Dexko_PARENT_DIR}/resources ${file})
    list(APPEND resource_files resources/${relative_file})
endforeach()

add_resources(resource_files ${resource_files})

add_library(Dexko SHARED ${sourceFiles} ${resource_files})
add_dependencies(Dexko generate_build_info)

set(RESOURCE_HEADER_FILE "${Dexko_PARENT_DIR}/src/Utils/Resources.hpp")
set(RESOURCE_SOURCE_FILE "${Dexko_PARENT_DIR}/src/Utils/Resources.cpp")

set(RESOURCE_HEADER_CONTENT "// This file is generated by CMake. Do not edit it manually.\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}#pragma once\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}#include <string>\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}#include <unordered_map>\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}#include \"Resource.hpp\"\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}\n")

foreach(file ${resource_files_list})
    file(RELATIVE_PATH relative_file ${Dexko_PARENT_DIR}/resources ${file})
    string(REPLACE "/" "_" resource_name ${relative_file})
    string(REPLACE "." "_" resource_name ${resource_name})
    string(REPLACE "-" "_" resource_name ${resource_name})
    set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}LOAD_RESOURCE(${resource_name});\n")
endforeach()

set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}class ResourceLoader {\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}public:\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}    static inline std::unordered_map<std::string, Resource> Resources;\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}    static void loadResources();\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}};\n")

file(WRITE ${RESOURCE_HEADER_FILE} "${RESOURCE_HEADER_CONTENT}")
message(STATUS "Generated Resources.hpp")

set(RESOURCE_SOURCE_CONTENT "// This file is generated by CMake. Do not edit it manually.\n")
set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}#include \"Resources.hpp\"\n")
set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}#include \"FontHelper.hpp\"\n")
set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}\n")
set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}void ResourceLoader::loadResources() {\n")
set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    ImFontConfig font_config;\n")
set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    font_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_NoHinting;\n")
set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    font_config.FontDataOwnedByAtlas = false;\n")
set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    const ImWchar* cyrillic_ranges = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();\n")

foreach(file ${resource_files_list})
    file(RELATIVE_PATH relative_file ${Dexko_PARENT_DIR}/resources ${file})

    string(REGEX REPLACE "(.*)(/)(.*)(\\.)(.*)" "\\3" friendly_name ${relative_file})
    string(REPLACE "-" "_" friendly_name ${friendly_name})
    string(REPLACE "." "_" resource_name ${relative_file})
    string(REPLACE "-" "_" resource_name ${resource_name})

    set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    auto ${resource_name} = GET_RESOURCE(${resource_name});\n")
    set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    Resources.emplace(\"${friendly_name}\", ${resource_name});\n")

    if(NOT ${relative_file} MATCHES ".*\\.ttf")
        continue()
    endif()

    set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    FontHelper::Fonts.emplace(\"${friendly_name}\", ImGui::GetIO().Fonts->AddFontFromMemoryTTF(${resource_name}.data2(), ${resource_name}.size(), 20, &font_config, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic()));\n")
    set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    FontHelper::Fonts.emplace(\"${friendly_name}_large\", ImGui::GetIO().Fonts->AddFontFromMemoryTTF(${resource_name}.data2(), ${resource_name}.size(), 42, &font_config, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic()));\n")
endforeach()

set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}}\n")

file(WRITE ${RESOURCE_SOURCE_FILE} "${RESOURCE_SOURCE_CONTENT}")
message(STATUS "Generated Resources.cpp")

# ---------------------------------------------------------------------------
# Статические библиотеки движка
# ---------------------------------------------------------------------------
add_subdirectory(${Dexko_PARENT_DIR}/include/minhook minhook)
add_subdirectory(${Dexko_PARENT_DIR}/include/libhat libhat)
add_subdirectory(${Dexko_PARENT_DIR}/include/ImGui ImGui)
add_subdirectory(${Dexko_PARENT_DIR}/include/Kiero kiero)
add_subdirectory(${Dexko_PARENT_DIR}/include/nes nuvola_event_system)

set_property(TARGET DirectXTK PROPERTY UNITY_BUILD FALSE)
set_property(TARGET minhook PROPERTY UNITY_BUILD FALSE)

target_include_directories(Dexko PRIVATE ${Dexko_PARENT_DIR}/include/ImGui)
target_include_directories(Dexko PRIVATE ${Dexko_PARENT_DIR}/include/Kiero)

target_link_libraries(Dexko PRIVATE OpenMP::OpenMP_CXX DirectXTK spdlog version.lib nlohmann_json glm minhook kiero nuvola_event_system libhat dbghelp.lib winmm.lib magic_enum d3d12.lib d3d11.lib d2d1.lib dxgi.lib dxguid.lib dwrite.lib comctl32.lib ws2_32.lib cryptopp ole32.lib Mmdevapi.lib RuntimeObject.lib)

set_property(TARGET Dexko nlohmann_json minhook spdlog cryptopp DirectXTK glm kiero nuvola_event_system magic_enum libhat PROPERTY MSVC_RUNTIME_LIBRARY "$<$<CONFIG:Debug>:MultiThreadedDLL>$<$<CONFIG:Release>:MultiThreaded>")

target_precompile_headers(Dexko PRIVATE
        ${Dexko_PARENT_DIR}/src/pch.hpp
)
