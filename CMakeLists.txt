cmake_minimum_required(VERSION 2.8.12)

project(skanlite)

# ECM setup (Extended Cmake Modules)
find_package(ECM 0.0.9 REQUIRED NO_MODULE)
# ECM setup2 and include find same cmake module (copied from /usr/share/kde4/apps/cmake/modules/FindSane.cmake)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake-modules)

include(FeatureSummary)
include(ECMSetupVersion) # yiels ecm_setup_version
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(CheckFunctionExists)
include(KDEInstallDirs) # yields ${XDG_APPS_INSTALL_DIR}
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)
include(GenerateExportHeader)

find_package(Qt5 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Widgets)

find_package(KF5 ${KF5_VERSION} REQUIRED
    COMPONENTS # alias to find_package(KFGuiAddons ${KF5_VERSION} REQUIRED)
        GuiAddons
        DocTools # yields kdoctools_create_handbook
        Sane # will find KF5Sane
)

set(KF5_VERSION "5.0.0")

ecm_setup_version(
  ${KF5_VERSION}
  VARIABLE_PREFIX SKANLITE
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/skanlite_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/src/version.h.cmake"
)

message(STATUS "KSANE_FOUND=${KSANE_FOUND}, KSANE_INCLUDE_DIR=${KSANE_INCLUDE_DIR}")
find_package(PNG REQUIRED)

set(SKANLITE_VERSION_MAJOR "1")
set(SKANLITE_VERSION_MINOR "1")
set(SKANLITE_VERSION_STRING "${SKANLITE_VERSION_MAJOR}.${SKANLITE_VERSION_MINOR}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/version.h)

add_subdirectory(src)
add_subdirectory(doc)

