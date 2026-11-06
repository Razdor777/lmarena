# Juzdex / Solstice — Minecraft Bedrock 26.62 (GDK x64 / MSVC)
cmake_minimum_required(VERSION 3.28)
include(cmake/CPM.cmake)
project(Solstice)

# throw error if building for x86
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Solstice only supports x64 builds.")
endif ()

add_compile_definitions(__PRIVATE_DEBUG__)
add_compile_definitions(__PRIVATE_BUILD__)
add_compile_options(/utf-8)

if (PRIVATE_DEBUGBUILD)
    add_compile_definitions(__PRIVATE_DEBUG__)
    add_compile_definitions(__PRIVATE_BUILD__)
    add_compile_definitions(__DEBUG__)
    message(STATUS "Private+Debug build enabled")
endif ()

if (PRIVATE_BUILD)
    add_compile_definitions(__PRIVATE_BUILD__)
    message(STATUS "Private build enabled")
endif ()

find_program(LD_COMMAND ld)

if (NOT LD_COMMAND)
    find_program(LD_COMMAND ld HINTS "C:/msys64/mingw64/bin")
elseif (NOT LD_COMMAND)
    message(STATUS "ld found using find_program")
endif ()

execute_process(COMMAND ${LD_COMMAND} --version OUTPUT_VARIABLE LD_COMMAND_AVAILABLE)

message(STATUS "ld found at ${LD_COMMAND}")
if (NOT LD_COMMAND_AVAILABLE)
    message(FATAL_ERROR "The ld command is not available on your system. \nPlease install using `pacman -S mingw-w64-x86_64-binutils` in MSYS2, then add the bin directory to your PATH.\nAfter that, restart your IDE and CMake should be able to find the ld command.")
endif ()

# Parallel per-TU compilation instead of unity chunks so new/ported .cpp files
# never collide on anonymous-namespace helpers or global allocator overrides.
set(CMAKE_UNITY_BUILD FALSE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "$<$<CONFIG:Debug>:MultiThreadedDLL>$<$<CONFIG:Release>:MultiThreaded>")

if (MSVC)
    add_compile_options(/bigobj /MP)
endif ()

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
        NAME cryptopp-cmake
        GITHUB_REPOSITORY abdes/cryptopp-cmake
        GIT_TAG master
)

# VS 17.10+ / VS 18 stdext compatibility for cryptopp
if (TARGET cryptopp)
    target_compile_options(cryptopp PRIVATE "/FI${CMAKE_CURRENT_LIST_DIR}/cmake/msvc_stdext_compat.h")
endif ()

set(SHADER_DIR ${CMAKE_CURRENT_LIST_DIR}/src/Utils/Shaders)
set(SHADER_BIN_DIR ${CMAKE_BINARY_DIR}/SolsticeShaders)
set(HLSL_FILES_VS ${SHADER_DIR}/BlockInstanceVS.hlsl)
set(HLSL_FILES_PS ${SHADER_DIR}/DefaultPS.hlsl)
list(LENGTH HLSL_FILES_VS HLSL_FILES_VS_COUNT)
list(LENGTH HLSL_FILES_PS HLSL_FILES_PS_COUNT)
math(EXPR HLSL_FILES_COUNT "${HLSL_FILES_VS_COUNT} + ${HLSL_FILES_PS_COUNT}")

add_custom_target(SolsticeShaders ALL COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/SolsticeShaders COMMENT "Building ${HLSL_FILES_COUNT} shaders" VERBATIM)

foreach(FILE ${HLSL_FILES_VS})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    add_custom_command(TARGET SolsticeShaders
            COMMAND cmd /c fxc.exe /nologo /Evertex_main /T vs_5_0 /Zi /Fh ${SHADER_BIN_DIR}/${FILE_WE}.inc /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE} > nul
            DEPENDS ${FILE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Building shader ${FILE}"
            VERBATIM
    )
endforeach(FILE)

foreach(FILE ${HLSL_FILES_PS})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    add_custom_command(TARGET SolsticeShaders
            COMMAND cmd /c fxc.exe /nologo /Epixel_main /T ps_5_0 /Zi /Fh ${SHADER_BIN_DIR}/${FILE_WE}.inc /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE} > nul
            DEPENDS ${FILE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Building shader ${FILE}"
            VERBATIM
    )
endforeach(FILE)

include_directories(${SHADER_BIN_DIR})
find_package(OpenMP)

add_custom_target(generate_build_info
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/GenerateBuildInfo.cmake
        COMMENT "Generating build info [${CMAKE_BINARY_DIR}/build_info.h]"
)

include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 26)

add_compile_definitions(NOMINMAX)
add_compile_definitions(JM_XORSTR_DISABLE_AVX_INTRINSICS)

add_compile_definitions(ENTT_SPARSE_PAGE=2048)
add_compile_definitions(ENTT_PACKED_PAGE=128)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(__DEBUG__)
endif ()
add_compile_definitions($<$<CONFIG:Debug>:__DEBUG__>)

# Debug options
add_link_options($<$<CONFIG:Debug>:/INCREMENTAL>)
add_link_options($<$<CONFIG:Debug>:/DEBUG>)
add_compile_options($<$<CONFIG:Debug>:/Zi>)

# Release options
add_link_options($<$<CONFIG:Release>:/Zi>)
add_link_options($<$<CONFIG:Release>:/INCREMENTAL:NO>)
add_link_options($<$<CONFIG:Release>:/LTCG>)
add_link_options($<$<CONFIG:Release>:/OPT:REF>)
add_link_options($<$<CONFIG:Release>:/OPT:ICF>)
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

set(SOLSTICE_VERSION "v2.0.0-26.62")
add_compile_definitions(SOLSTICE_VERSION="${SOLSTICE_VERSION}")

add_definitions(-DWIN32_LEAN_AND_MEAN)

set(KIERO_INCLUDE_D3D11 ON)
set(KIERO_INCLUDE_D3D12 ON)
set(KIERO_BUILD_EXAMPLES OFF)

include_directories(.)
include_directories(src)
include_directories(include)
include_directories(include/minhook/include)
include_directories(include/libhat)
include_directories(include/libhat/include)
include_directories(include/libhat/include/libhat)
include_directories(include/entt)
include_directories(include/ImGui)
include_directories(include/Kiero/include)
include_directories(include/Kiero)
include_directories(include/nes)
include_directories(include/nes/include)

file(GLOB_RECURSE sourceFiles "src/*.cpp")
file(GLOB_RECURSE includeFiles "include/*.hpp")
set_source_files_properties(${sourceFiles} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
set_source_files_properties(${includeFiles} PROPERTIES CXX_SCAN_FOR_MODULES OFF)

# Resource embedding (resources/* -> .o via ld)
file(GLOB_RECURSE resource_files_list "resources/*")

function(add_resources out_var)
    set(result)
    foreach(in_f ${ARGN})
        file(RELATIVE_PATH src_f ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${in_f})
        set(out_f "${PROJECT_BINARY_DIR}/${in_f}.o")
        add_custom_command(OUTPUT ${out_f}
                COMMAND ${LD_COMMAND} -r -b binary -o ${out_f} ${src_f}
                DEPENDS ${in_f}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
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
    file(RELATIVE_PATH relative_file ${CMAKE_CURRENT_SOURCE_DIR}/resources ${file})
    list(APPEND resource_files resources/${relative_file})
endforeach()

add_resources(resource_files ${resource_files})

add_library(Solstice SHARED ${sourceFiles} ${resource_files})
add_dependencies(${PROJECT_NAME} generate_build_info SolsticeShaders)

set(RESOURCE_HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/Resources.hpp")
set(RESOURCE_SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/Resources.cpp")

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
    string(TOLOWER ${friendly_name} friendly_name)

    string(REPLACE "/" "_" resource_name ${relative_file})
    string(REPLACE "." "_" resource_name ${resource_name})
    string(REPLACE "-" "_" resource_name ${resource_name})

    set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    auto ${resource_name} = GET_RESOURCE(${resource_name});\n")
    set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    Resources.emplace(\"${friendly_name}\", ${resource_name});\n")

    if(NOT ${relative_file} MATCHES ".*\\.ttf")
        continue()
    endif()

    set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    FontHelper::Fonts.emplace(\"${friendly_name}\", ImGui::GetIO().Fonts->AddFontFromMemoryTTF(${resource_name}.data2(), ${resource_name}.size(), 20, &font_config, cyrillic_ranges));\n")
    set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}    FontHelper::Fonts.emplace(\"${friendly_name}_large\", ImGui::GetIO().Fonts->AddFontFromMemoryTTF(${resource_name}.data2(), ${resource_name}.size(), 42, &font_config, cyrillic_ranges));\n")
endforeach()

set(RESOURCE_SOURCE_CONTENT "${RESOURCE_SOURCE_CONTENT}}\n")

file(WRITE ${RESOURCE_SOURCE_FILE} "${RESOURCE_SOURCE_CONTENT}")
message(STATUS "Generated Resources.cpp")

add_subdirectory(include/minhook)
add_subdirectory(include/libhat)
add_subdirectory(include/ImGui)
add_subdirectory(include/Kiero)
add_subdirectory(include/nes)

set_property(TARGET minhook PROPERTY UNITY_BUILD FALSE)

target_include_directories(Solstice PRIVATE include/ImGui)
target_include_directories(Solstice PRIVATE include/Kiero/include)
target_include_directories(Solstice PRIVATE include/Kiero)

target_link_libraries(Solstice PRIVATE
        OpenMP::OpenMP_CXX
        spdlog
        version.lib
        nlohmann_json
        glm
        minhook
        kiero
        nuvola_event_system
        libhat
        magic_enum
        cryptopp
        d3d12.lib
        d3d11.lib
        d2d1.lib
        dxgi.lib
        dxguid.lib
        dwrite.lib
        winmm.lib
        wininet.lib
        dbghelp.lib
        comctl32.lib
        ws2_32.lib
        ole32.lib
        Mmdevapi.lib
        WindowsApp.lib
        OneCore.lib
        RuntimeObject.lib
)

set_property(TARGET Solstice nlohmann_json minhook spdlog cryptopp glm kiero nuvola_event_system magic_enum libhat
        PROPERTY MSVC_RUNTIME_LIBRARY "$<$<CONFIG:Debug>:MultiThreadedDLL>$<$<CONFIG:Release>:MultiThreaded>")

target_precompile_headers(Solstice PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:<src/pch.hpp$<ANGLE-R>>"
)
