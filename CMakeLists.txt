cmake_minimum_required(VERSION 3.28)
project(D_bus)
set(CMAKE_CXX_STANDARD 20)
# Добавляем директории
include_directories(src include)

# Устанавливаем директорию, в которй будут хранится бинарники
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# добавляем бинарник service
add_executable(
        service
        src/start_service.cpp
        include/service.h
        src/service.cpp
)

#Добавляем бинарник application
add_executable(
        application
        src/start_app.cpp
        include/application.h
        src/application.cpp

)

# скачиваем библеотеку sdbud-c++
include(FetchContent)
FetchContent_Declare(
        sdbus-cpp
        GIT_REPOSITORY https://github.com/Kistler-Group/sdbus-cpp
        GIT_TAG v2.1.0
)

# Делаем библеотку доступной в проекте
FetchContent_MakeAvailable(sdbus-cpp)
target_link_libraries(service PRIVATE sdbus-c++)
target_link_libraries(application PRIVATE sdbus-c++)
