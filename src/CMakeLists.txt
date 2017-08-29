
include_directories (
    ${PROJECT_BINARY_DIR}
    ${LIBPINYIN_INCLUDE_DIRS}
    ${FCITX4_FCITX_UTILS_INCLUDE_DIRS}
    ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
    ${FCITX4_FCITX_INCLUDE_DIRS}
    )

set(FCITX_DPINPUT_SOURCES dpinput.cpp)

fcitx_add_addon_full(dpinput
    SOURCES ${FCITX_DPINPUT_SOURCES}
    IM_CONFIG dpinput.conf
    LINK_LIBS ${LIBPINYIN_LIBRARIES})

install(FILES pinyin.txt DESTINATION /usr/share/fcitx/dpinput/)

