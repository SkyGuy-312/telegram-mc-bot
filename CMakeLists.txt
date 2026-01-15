cmake_minimum_required(VERSION 3.15)

project(telegram-mc-bot LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages via vcpkg
find_package(TgBot REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add executable
add_executable(mcbot
    src/main.cpp
    src/ProcessController.cpp
    src/ProcessController.hpp
)

# Link libraries
target_link_libraries(mcbot PRIVATE TgBot::TgBot nlohmann_json::nlohmann_json)

# Copy config file to output directory
configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
