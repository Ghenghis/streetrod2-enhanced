cmake_minimum_required(VERSION 3.24)
project(sr2-enhanced C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Port of Street Rod 2's SR.EXE, VGA 16-colour mode (0Dh). AdLib sound (S5.4) runs on Nuked-OPL3. CGA / Tandy / Hercules are parked.
set(SR2PORT_VERSION 0.1.0)
set(SR2E_BASE "v0.1.0 b79b6e9")

find_package(SDL3 REQUIRED CONFIG)

file(GLOB_RECURSE SR_SOURCES CONFIGURE_DEPENDS src/*.c)

# Nuked-OPL3 (LGPL-2.1+, third_party/nuked-opl3/LICENSE); the AdLib chip (host.c).
add_library(nuked_opl3 STATIC third_party/nuked-opl3/opl3.c)
target_include_directories(nuked_opl3 PUBLIC third_party/nuked-opl3)

add_executable(sr2-enhanced ${SR_SOURCES})
target_include_directories(sr2-enhanced PRIVATE src)
target_link_libraries(sr2-enhanced PRIVATE SDL3::SDL3 nuked_opl3)
target_compile_definitions(sr2-enhanced PRIVATE SR2PORT_VERSION="${SR2PORT_VERSION}")
if(MSVC)
    target_compile_options(sr2-enhanced PRIVATE /W3)
else()
    # The memory model reads multi-byte values through byte pointers (see src/mem.h).
    target_compile_options(sr2-enhanced PRIVATE -Wall -Wextra -Wno-unused-parameter -fno-strict-aliasing)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(sr2-enhanced PRIVATE -Wno-format-truncation)
    endif()
endif()
if(WIN32)
    add_custom_command(TARGET sr2-enhanced POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:sr2-enhanced> $<TARGET_FILE_DIR:sr2-enhanced>
        COMMAND_EXPAND_LISTS)
    # And what those load in turn (MSYS2's SDL3.dll needs libiconv-2.dll), so the game starts outside
    # the MSYS2 shell, e.g. from the launcher.
    get_filename_component(SR2_DLL_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
    add_custom_command(TARGET sr2-enhanced POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:sr2-enhanced> "-DSEARCH=${SR2_DLL_DIR}"
                -DCMAKE_OBJDUMP=${CMAKE_OBJDUMP} -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/copy_dlls.cmake)
endif()

# ENH: the launcher is not set up yet (PLAN.md E3: adapt ../StreetRod2/sr2-enhanced/launcher, SR2E_LAUNCHER=ON|OFF).
