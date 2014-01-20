set(skanlite_SRCS main.cpp skanlite.cpp ImageViewer.cpp KSaneImageSaver.cpp SaveLocation.cpp)

qt5_wrap_ui(skanlite_SRCS settings.ui SaveLocation.ui)

#kde4_add_app_icon(skanlite_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/devices/scanner.png")

add_executable(skanlite ${skanlite_SRCS})

target_link_libraries(skanlite
  PUBLIC
    Qt5::Core
  PRIVATE
  #  KF5::CoreAddons
  #  Other private libs
    KF5::Sane
    KF5::I18n
    KF5::PrintUtils
    KF5::Wallet
    #KF5::WidgetsAddons
    ${PNG_LIBRARY}
    KF5::KDE4Support # FIXME KF5: remove that later
)

install(TARGETS skanlite ${INSTALL_TARGETS_DEFAULT_ARGS})
message(STATUS "XDG_APPS_INSTALL_DIR=${XDG_APPS_INSTALL_DIR}")
install(PROGRAMS skanlite.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
