set(skanlite_SRCS main.cpp skanlite.cpp ImageViewer.cpp showimagedialog.cpp SkanliteImageSaver.cpp SaveLocation.cpp DBusInterface.cpp)

ki18n_wrap_ui(skanlite_SRCS settings.ui SaveLocation.ui)

ecm_qt_declare_logging_category(skanlite_SRCS
  HEADER skanlite_debug.h
  IDENTIFIER SKANLITE_LOG
  CATEGORY_NAME org.kde.skanlite
)

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
)

install(TARGETS skanlite ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.skanlite.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES org.kde.skanlite.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
