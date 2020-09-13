cmake_minimum_required(VERSION 3.17)

project(MsiQuery)

set(MsiQuery_FILES
    MsiQuery.hpp
    MsiUtil.hpp
    Main.cpp
)

add_definitions(-DUNICODE)

add_executable(MsiQuery ${MsiQuery_FILES} README.md)

# Doesn't seem to work
#set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT "MsiQuery")
