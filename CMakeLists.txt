cmake_minimum_required(VERSION 4.0)
project(glapd-web)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_subdirectory(external)

add_subdirectory(apps)
