cmake_minimum_required(VERSION 3.16)
project(MavishGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find raylib package
find_package(raylib CONFIG REQUIRED)

# Add executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Copy resources folder to build directory (if it exists)
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
    )
endif()
