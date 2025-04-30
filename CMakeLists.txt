cmake_minimum_required(VERSION 3.31)
project(D_bus)

set(CMAKE_CXX_STANDARD 20)

add_executable(D_bus main.cpp
        test.cpp
)

include(FetchContent)
# download sdbus-cpp library
FetchContent_Declare(
        sdbus-cpp
        GIT_REPOSITORY https://github.com/Kistler-Group/sdbus-cpp
        GIT_TAG v2.1.0
)
# make the library available in the project
FetchContent_MakeAvailable(sdbus-cpp)
# link library to the executable
target_link_libraries(D_bus PRIVATE sdbus-c++)