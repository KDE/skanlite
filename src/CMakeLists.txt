include_directories(${KSANE_INCLUDE_DIR})

set(skanlite_SRCS main.cpp skanlite.cpp ImageViewer.cpp KSaneImageSaver.cpp SaveLocation.cpp)

kde4_add_ui_files(skanlite_SRCS settings.ui SaveLocation.ui)

kde4_add_app_icon(skanlite_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/devices/scanner.png")

kde4_add_executable(skanlite ${skanlite_SRCS})

target_link_libraries(skanlite ${KDE4_KIO_LIBS} ${KSANE_LIBRARY} ${PNG_LIBRARY})

install(TARGETS skanlite ${INSTALL_TARGETS_DEFAULT_ARGS})
install( PROGRAMS skanlite.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
