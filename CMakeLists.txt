cmake_minimum_required(VERSION 3.24)
project(res_packer_gui VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)

find_package(lz4 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)

configure_file (
    "${PROJECT_SOURCE_DIR}/Version.h.in"
    "${PROJECT_SOURCE_DIR}/Version.h"
)

add_subdirectory(imgui)
enable_language(RC)
add_executable(
    res_packer_gui
    main.cpp
    Paths.h
    PakTypes.h
    Packer.h
    Packer.cpp
#    Pack.h
#    Pack.cpp
    Unpacker.h
    Unpacker.cpp
    External/miniz/miniz.c
    External/miniz/miniz.h
    Version.h
#    FileBrowser/ImGuiFileBrowser.cpp
#    FileBrowser/ImGuiFileBrowser.h
#    FileBrowser/Dirent/dirent.h
    Utils.h
    Widgets.h
    Theme.h
#    External/hash-library/crc32.cpp
#    External/hash-library/crc32.h
    External/IconsFontAwesome6.h
    External/stb_image.h
    version.rc
    Gui.cpp
    Gui.h
)

target_link_libraries(res_packer_gui PRIVATE imgui imgui-win32 imgui-dx11 lz4::lz4 $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static> unofficial-sodium::sodium cereal::cereal)

set(RC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")  # Replace 'resources.rc' with the actual name of your RC file

# Add the RC file to the project
target_sources(${PROJECT_NAME} PRIVATE ${RC_FILE})

# Enable Windows-specific compiler flags for resource compilation
set(CMAKE_RC_FLAGS "/l0x409")

# Set the resource compiler command
set(RC_COMPILER "${CMAKE_RC_COMPILER}")

# Specify the resource compilation command
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${RC_COMPILER} ${RC_FILE})