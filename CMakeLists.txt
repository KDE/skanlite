cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "12")
set (RELEASE_SERVICE_VERSION_MICRO "2")

set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

set(SKANLITE_VERSION_STRING "${RELEASE_SERVICE_VERSION}")

project(skanlite VERSION ${RELEASE_SERVICE_VERSION})

set(REQUIRED_QT_VERSION "6.4.0")
set(KF_MIN_VERSION "6.0.0")

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
include(ECMDeprecationSettings)
include(KDEClangFormat)
include(KDEGitCommitHooks)

ecm_setup_version(
  ${SKANLITE_VERSION_STRING}
  VARIABLE_PREFIX SKANLITE
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/skanlite_version.h"
)

# Dependencies
find_package(Qt6 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Widgets)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
        CoreAddons # KAboutData
        I18n
        KIO # contains the KIOWidgets which we use in target_link_libraries
        XmlGui # KAboutApplicationDialog
        Crash
)

find_package(KF6DocTools ${KF_MIN_VERSION})  # yields kdoctools_create_handbook
set_package_properties(KF6DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)

find_package(KSaneWidgets6 "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}" REQUIRED)

ecm_set_disabled_deprecation_versions(QT 6.7.2
    KF 6.7.0
)


# Subdirectories
add_subdirectory(src)

ki18n_install(po)
if (KF6DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()

if(BUILD_TESTING)
   add_subdirectory(autotests)
   add_subdirectory(tests)
endif()
ecm_install_icons(ICONS sc-apps-org.kde.skanlite.svg DESTINATION ${KDE_INSTALL_ICONDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

