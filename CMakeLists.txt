cmake_minimum_required(VERSION 2.8.12)

project(skanlite)

# ECM setup (Extended Cmake Modules)
find_package(ECM 1.2.0 REQUIRED NO_MODULE)
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

find_package(Qt5 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Widgets)

# be not so strict with the poor developer who just wants to use strings
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

set(KF5_VERSION "5.1.0")

ecm_setup_version(
  ${KF5_VERSION}
  VARIABLE_PREFIX SKANLITE
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/skanlite_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/src/version.h.cmake"
)

# Dependencies
set(REQUIRED_QT_VERSION "5.2.0")

# Required Qt5 components to build this framework
find_package(Qt5 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core)
find_package(PNG REQUIRED)

include_directories(${PNG_INCLUDE_DIRS})

set(SKANLITE_VERSION_STRING "2.0.0")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/version.h)

find_package(KF5 ${KF5_VERSION} REQUIRED
    COMPONENTS # alias to find_package(KFGuiAddons ${KF5_VERSION} REQUIRED)
        CoreAddons # KAboutData
        I18n
        XmlGui # KAboutApplicationDialog
        KIO # contains the KIOWidgets which we use in target_link_libraries
        DocTools # yields kdoctools_create_handbook
        Sane # will find KF5Sane
        TextWidgets
)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

# Subdirectories
add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(autotests)
add_subdirectory(tests)
