set(skanlite_SRCS main.cpp skanlite.cpp ImageViewer.cpp KSaneImageSaver.cpp SaveLocation.cpp)

ki18n_wrap_ui(skanlite_SRCS settings.ui SaveLocation.ui)

#kde4_add_app_icon(skanlite_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/devices/scanner.png")

add_executable(skanlite ${skanlite_SRCS})

target_link_libraries(skanlite
  PUBLIC
    Qt5::Core
  PRIVATE
    KF5::CoreAddons
    KF5::Sane
    KF5::I18n
    KF5::XmlGui
    KF5::KIOWidgets
    ${PNG_LIBRARY}
)

install(TARGETS skanlite ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS skanlite.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
