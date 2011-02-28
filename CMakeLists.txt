cmake_minimum_required(VERSION 2.6)

project(skanlite)


find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include_directories (${KDE4_INCLUDES})

find_package(KSane REQUIRED)
find_package(PNG REQUIRED)

add_subdirectory(src)
add_subdirectory(doc)