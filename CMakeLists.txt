cmake_minimum_required(VERSION 2.8.6)
project(fcitx-dpinput)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(Fcitx REQUIRED)
pkg_check_modules(LIBPINYIN REQUIRED libpinyin glib-2.0)

_fcitx_add_uninstall_target()

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_CXX_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")

string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" FCITX4_MAJOR_VERSION "${FCITX4_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+.*" "\\1" FCITX4_MINOR_VERSION "${FCITX4_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" FCITX4_PATCH_VERSION "${FCITX4_VERSION}")

configure_file(config.h.in config.h)

add_subdirectory(src)
