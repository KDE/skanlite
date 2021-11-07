set(skanlite_SRCS showimagedialog.cpp
    skanlite.cpp
    ImageViewer.cpp
    DBusInterface.h
    DBusInterface.cpp
    SaveLocation.cpp
    SkanliteImageSaver.cpp
    ImageViewer.h
    skanlite.h
    SkanliteImageSaver.h
    showimagedialog.h
    SaveLocation.h
    main.cpp)

ki18n_wrap_ui(skanlite_SRCS settings.ui SaveLocation.ui)

ecm_qt_declare_logging_category(skanlite_SRCS
  HEADER skanlite_debug.h
  IDENTIFIER SKANLITE_LOG
  CATEGORY_NAME org.kde.skanlite
)

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
