cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "08")
set (RELEASE_SERVICE_VERSION_MICRO "0")

set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

set(SKANLITE_VERSION_STRING "${RELEASE_SERVICE_VERSION}")

project(skanlite VERSION ${RELEASE_SERVICE_VERSION})

set(REQUIRED_QT_VERSION "5.15.0")
set(KF_MIN_VERSION "5.90.0")

# ECM setup (Extended Cmake Modules)
find_package(ECM ${KF_MIN_VERSION} QUIET REQUIRED NO_MODULE)
# ECM setup2 and include find same cmake module (copied from /usr/share/kde4/apps/cmake/modules/FindSane.cmake)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake-modules)

include(KDEInstallDirs) # yields ${XDG_APPS_INSTALL_DIR}
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FeatureSummary)
include(ECMSetupVersion) # yiels ecm_setup_version
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(CheckFunctionExists)
include(GenerateExportHeader)
include(ECMQtDeclareLoggingCategory)
include(ECMInstallIcons)

if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
    set(KF_MIN_VERSION "5.240.0")
    set(KF_MAJOR_VERSION "6")
else()
    set(KF_MAJOR_VERSION "5")
endif()

ecm_setup_version(
  ${SKANLITE_VERSION_STRING}
  VARIABLE_PREFIX SKANLITE
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/skanlite_version.h"
)

# Dependencies
find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Widgets)

find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS
        CoreAddons # KAboutData
        I18n
        KIO # contains the KIOWidgets which we use in target_link_libraries
        XmlGui # KAboutApplicationDialog
)

find_package(KF${KF_MAJOR_VERSION}DocTools ${KF_MIN_VERSION})  # yields kdoctools_create_handbook
set_package_properties(KF${KF_MAJOR_VERSION}DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)

if (QT_MAJOR_VERSION STREQUAL "6")
    find_package(KSaneWidgets6 "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}" REQUIRED)
else()
    find_package(KF5Sane "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}" REQUIRED)
endif()

add_definitions(
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050F00
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
    -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055800
    -DKF_DEPRECATED_WARNINGS_SINCE=0x060000
)

# Subdirectories
add_subdirectory(src)

ki18n_install(po)
if (KF${KF_MAJOR_VERSION}DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()

if(BUILD_TESTING)
   add_subdirectory(autotests)
   add_subdirectory(tests)
endif()
ecm_install_icons(ICONS sc-apps-org.kde.skanlite.svg DESTINATION ${KDE_INSTALL_ICONDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
