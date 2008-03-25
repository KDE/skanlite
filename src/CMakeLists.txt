project(glimpse)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include_directories (${KDE4_INCLUDES})
find_package(Ksane REQUIRED)

set(glimpse_SRCS main.cpp glimpse.cpp)

kde4_add_ui_files(glimpse_SRCS settings.ui)

kde4_add_executable(glimpse ${glimpse_SRCS})

target_link_libraries(glimpse ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KSANE_LIBRARY})

install(TARGETS glimpse DESTINATION ${BIN_INSTALL_DIR})
install( FILES glimpse.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )