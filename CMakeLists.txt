cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "21")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")

set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

set(SKANLITE_VERSION_STRING "${RELEASE_SERVICE_VERSION}")

project(skanlite VERSION ${RELEASE_SERVICE_VERSION})

set(REQUIRED_QT_VERSION "5.14.0")
set(KF5_MIN_VERSION "5.79.0")

# ECM setup (Extended Cmake Modules)
find_package(ECM ${KF5_MIN_VERSION} QUIET REQUIRED NO_MODULE)
# ECM setup2 and include find same cmake module (copied from /usr/share/kde4/apps/cmake/modules/FindSane.cmake)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake-modules)

include(FeatureSummary)
include(ECMSetupVersion) # yiels ecm_setup_version
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(CheckFunctionExists)
include(KDEInstallDirs) # yields ${XDG_APPS_INSTALL_DIR}
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(GenerateExportHeader)
include(ECMQtDeclareLoggingCategory)
include(ECMInstallIcons)

ecm_setup_version(
  ${SKANLITE_VERSION_STRING}
  VARIABLE_PREFIX SKANLITE
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/skanlite_version.h"
)

# Dependencies
find_package(Qt5 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Widgets)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
        CoreAddons # KAboutData
        DocTools # yields kdoctools_create_handbook
        I18n
        KIO # contains the KIOWidgets which we use in target_link_libraries
        XmlGui # KAboutApplicationDialog
)

find_package(KF5Sane ${RELEASE_SERVICE_VERSION} REQUIRED)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

# Subdirectories
add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(autotests)
add_subdirectory(tests)

ecm_install_icons(ICONS 48-apps-org.kde.skanlite.svg DESTINATION ${KDE_INSTALL_ICONDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
