project(MsiQuery)

set(MsiQuery_FILES
    MsiQuery.hpp
    MsiUtil.hpp
    Main.cpp
)

add_definitions(-DUNICODE)

add_executable(MsiQuery ${MsiQuery_FILES} README.md)
