add_executable(skanlite ${skanlite_SRCS})
target_sources(skanlite PRIVATE showimagedialog.cpp
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

ki18n_wrap_ui(skanlite settings.ui SaveLocation.ui)

ecm_qt_declare_logging_category(skanlite
  HEADER skanlite_debug.h
  IDENTIFIER SKANLITE_LOG
  CATEGORY_NAME org.kde.skanlite
)


target_link_libraries(skanlite
  PUBLIC
    Qt${QT_MAJOR_VERSION}::Core
  PRIVATE
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF5::Sane
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::XmlGui
    KF${KF_MAJOR_VERSION}::KIOWidgets
)

install(TARGETS skanlite ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.skanlite.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install( FILES org.kde.skanlite.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
