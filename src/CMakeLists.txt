set(skanlite_SRCS main.cpp skanlite.cpp ImageViewer.cpp showimagedialog.cpp KSaneImageSaver.cpp SaveLocation.cpp DBusInterface.cpp)

ki18n_wrap_ui(skanlite_SRCS settings.ui SaveLocation.ui)

find_package(Qt5Gui REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

add_executable(skanlite ${skanlite_SRCS})

target_link_libraries(skanlite
  PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::PrintSupport
  PRIVATE
    KF5::CoreAddons
    KF5::Sane
    KF5::I18n
    KF5::XmlGui
    KF5::KIOWidgets
    ${PNG_LIBRARY}
)

install(TARGETS skanlite ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.skanlite.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES org.kde.skanlite.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
