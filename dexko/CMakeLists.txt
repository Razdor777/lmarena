# ============================================================================
#  Dexko — автономный лёгкий клиент (Dexko.dll)
#
#  Папка dexko/ полностью самодостаточна: собственные исходники, собственные
#  include-библиотеки и ресурсы. Внешние зависимости (spdlog, magic_enum,
#  glm, nlohmann_json) качаются CPM при первой конфигурации.
#
#  Сборка:
#      cmake -B dexko/build -S dexko
#      cmake --build dexko/build --config Release
#  Результат: dexko/build/Release/Dexko.dll
#
#  Перенос проекта: достаточно папки dexko/ целиком.
# ============================================================================
cmake_minimum_required(VERSION 3.28)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/CPM.cmake)

project(Dexko)

# throw error if building for x86
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Dexko only supports x64 builds.")
endif ()

add_compile_definitions(__PRIVATE_DEBUG__)
add_compile_definitions(__PRIVATE_BUILD__)
add_compile_definitions(__DEBUG__)
add_compile_options(/utf-8)

# ld из MSYS2 нужен для запаковки ресурсов в COFF-объекты
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

# cryptopp нужен не только Auth: StringUtils (md5/aes/base64) тоже его использует.
# Отключаем сборку тестов cryptest — иначе они падают на линковке: cryptopp.lib
# собирается с /MT (наш MSVC_RUNTIME_LIBRARY), а cryptest — с /MD по умолчанию.
set(BUILD_TESTING OFF CACHE BOOL "Disable dependency tests (cryptest etc.)" FORCE)
CPMAddPackage(
        NAME cryptopp-cmake
        GITHUB_REPOSITORY abdes/cryptopp-cmake
        GIT_TAG master
)

if(MSVC)
    add_compile_options(/bigobj)
endif()

find_package(OpenMP)

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

set(DEXKO_VERSION "v1.0.0")
add_compile_definitions(DEXKO_VERSION="${DEXKO_VERSION}")
# Движковый код (форки) местами ссылается на SOLSTICE_VERSION — алиас.
add_compile_definitions(SOLSTICE_VERSION="${DEXKO_VERSION}")

add_definitions(-DWIN32_LEAN_AND_MEAN)

# ---------------------------------------------------------------------------
# Debug / Release флаги
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
# Include paths. Корень проекта — в списке специально: часть движковых
# заголовков использует пути вида <include/libhat/include/libhat.hpp>.
# ---------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/minhook/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/libhat)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/entt)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/libhat/libhat)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/ImGui)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/Kiero)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/nes)

# ---------------------------------------------------------------------------
# Исходники
# ---------------------------------------------------------------------------
file(GLOB_RECURSE sourceFiles "src/*.cpp")
file(GLOB_RECURSE includeFiles "src/*.hpp")
set_source_files_properties(${sourceFiles} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
set_source_files_properties(${includeFiles} PROPERTIES CXX_SCAN_FOR_MODULES OFF)

# Resources.cpp генерируется ниже и добавляется к сборке явно
set(RESOURCE_HEADER_FILE "${CMAKE_CURRENT_LIST_DIR}/src/Utils/Resources.hpp")
set(RESOURCE_SOURCE_FILE "${CMAKE_CURRENT_LIST_DIR}/src/Utils/Resources.cpp")

# ---------------------------------------------------------------------------
# Ресурсы: запаковка ld-ом в COFF-объекты + генерация Resources.hpp/cpp
# ---------------------------------------------------------------------------
file(GLOB_RECURSE resource_files_list "resources/*")

function(add_resources out_var)
    set(result)
    foreach(in_f ${ARGN})
        # in_f приходит относительным ("resources/..."); ld и MSBuild ломаются
        # на путях с пробелами, поэтому всё берём в кавычки
        file(RELATIVE_PATH src_f ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${in_f})
        set(out_f "${PROJECT_BINARY_DIR}/${in_f}.o")
        add_custom_command(OUTPUT "${out_f}"
                COMMAND ${LD_COMMAND} -r -b binary -o "${out_f}" "${src_f}"
                DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${in_f}"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Building object ${out_f}"
                VERBATIM
        )
        list(APPEND result "${out_f}")
        message(STATUS "Embedding resource: ${in_f}")
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

set(resource_files "")
foreach(file ${resource_files_list})
    # GLOB возвращает абсолютные пути — превращаем в относительные, иначе
    # ${PROJECT_BINARY_DIR}/${file}.o даёт невалидный путь "build\C:\..."
    file(RELATIVE_PATH rel_file ${CMAKE_CURRENT_SOURCE_DIR} ${file})
    list(APPEND resource_files ${rel_file})
endforeach()

add_resources(resource_files ${resource_files})

add_library(Dexko SHARED ${sourceFiles} ${resource_files})
add_dependencies(Dexko generate_build_info)

set(RESOURCE_HEADER_CONTENT "// This file is generated by CMake. Do not edit it manually.\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}#pragma once\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}#include <string>\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}#include <unordered_map>\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}#include \"Resource.hpp\"\n")
set(RESOURCE_HEADER_CONTENT "${RESOURCE_HEADER_CONTENT}\n")

foreach(file ${resource_files_list})
    file(RELATIVE_PATH relative_file ${CMAKE_CURRENT_SOURCE_DIR}/resources ${file})
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
    file(RELATIVE_PATH relative_file ${CMAKE_CURRENT_SOURCE_DIR}/resources ${file})

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

target_sources(Dexko PRIVATE ${RESOURCE_SOURCE_FILE})

# ---------------------------------------------------------------------------
# Статические библиотеки
# ---------------------------------------------------------------------------
add_subdirectory(include/minhook)
add_subdirectory(include/libhat)
add_subdirectory(include/ImGui)
add_subdirectory(include/Kiero)
add_subdirectory(include/nes)

set_property(TARGET minhook PROPERTY UNITY_BUILD FALSE)

target_include_directories(Dexko PRIVATE include/ImGui)
target_include_directories(Dexko PRIVATE include/Kiero)

target_link_libraries(Dexko PRIVATE OpenMP::OpenMP_CXX spdlog version.lib cryptopp nlohmann_json glm minhook kiero nuvola_event_system libhat dbghelp.lib winmm.lib magic_enum d3d12.lib d3d11.lib d2d1.lib dxgi.lib dxguid.lib dwrite.lib comctl32.lib ws2_32.lib ole32.lib Mmdevapi.lib RuntimeObject.lib)

set_property(TARGET Dexko nlohmann_json minhook spdlog cryptopp glm kiero nuvola_event_system magic_enum libhat PROPERTY MSVC_RUNTIME_LIBRARY "$<$<CONFIG:Debug>:MultiThreadedDLL>$<$<CONFIG:Release>:MultiThreaded>")

target_precompile_headers(Dexko PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp
)
