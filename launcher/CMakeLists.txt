# The game's launcher, "Street Rod 2 Enhanced": the game folder, graphics and sound, the game's own
# switches, the window and the Enhanced picture and controls, then sr2-enhanced.
#
#   cmake -S launcher -B launcher/build -G Ninja && cmake --build launcher/build
#
# or, from the main project (SR2E_LAUNCHER, built by default when wxWidgets is found), next to sr2-enhanced.exe.
# Needs wxWidgets 3.2 (MSYS2: mingw-w64-x86_64-wxwidgets3.2-msw; Debian and Ubuntu: libwxgtk3.2-dev).
cmake_minimum_required(VERSION 3.24)
project(sr2elauncher LANGUAGES CXX)
if(WIN32)
    enable_language(RC)
endif()

# MSYS2 and wxWidgets' own CMake builds install a package config; distributions
# such as Debian only ship wx-config, which CMake's FindwxWidgets uses.
find_package(wxWidgets 3.2 CONFIG QUIET COMPONENTS core base)
if(wxWidgets_FOUND AND TARGET wx::wxcore)
    set(SR2EL_WX_LIBRARIES wx::wxcore wx::wxbase)
else()
    find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})
    set(SR2EL_WX_LIBRARIES ${wxWidgets_LIBRARIES})
endif()

add_executable(sr2elauncher WIN32
    app.cpp
    game.cpp
    icon.cpp
    launcher.cpp
    settings.cpp
)
set_target_properties(sr2elauncher PROPERTIES OUTPUT_NAME "Street Rod 2 Enhanced" CXX_EXTENSIONS OFF)
target_compile_features(sr2elauncher PRIVATE cxx_std_17)
target_link_libraries(sr2elauncher PRIVATE ${SR2EL_WX_LIBRARIES})
# Built from the main project: beside sr2-enhanced.exe, where it looks for it.
if(NOT PROJECT_IS_TOP_LEVEL)
    set_target_properties(sr2elauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if(WIN32)
    target_sources(sr2elauncher PRIVATE app.rc)
    target_link_libraries(sr2elauncher PRIVATE comctl32)
    if(MINGW)
        # The C++ runtime inside the exe, so the build needs fewer DLLs beside it.
        target_link_options(sr2elauncher PRIVATE -static-libgcc -static-libstdc++ $<$<CONFIG:Release>:-s>)
    endif()
    # The wxWidgets DLLs and everything they load, found where the compiler's own DLLs are.
    get_filename_component(SR2EL_DLL_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET sr2elauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:sr2elauncher> "-DSEARCH=${SR2EL_DLL_DIR}"
                -DCMAKE_OBJDUMP=${CMAKE_OBJDUMP} -P ${CMAKE_CURRENT_SOURCE_DIR}/../tools/copy_dlls.cmake)
endif()

if(MSVC)
    target_compile_options(sr2elauncher PRIVATE /W4 /utf-8)
else()
    target_compile_options(sr2elauncher PRIVATE -Wall -Wextra)
endif()
