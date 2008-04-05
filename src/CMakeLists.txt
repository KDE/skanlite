project(skanlite)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../cmake/modules)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include_directories (${KDE4_INCLUDES})
find_package(KSane REQUIRED)

set(skanlite_SRCS main.cpp skanlite.cpp)

kde4_add_ui_files(skanlite_SRCS settings.ui)

kde4_add_executable(skanlite ${skanlite_SRCS})

target_link_libraries(skanlite ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KSANE_LIBRARY})

install(TARGETS skanlite DESTINATION ${BIN_INSTALL_DIR})
install( FILES skanlite.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )