cmake_minimum_required(VERSION 3.13)
add_library("config" STATIC "config.cpp")
set_property(TARGET "config" PROPERTY CXX_STANDARD 17)
target_include_directories("config" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries("config" PUBLIC "jsonio" "compose")
