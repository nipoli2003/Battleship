cmake_minimum_required(VERSION 3.20)
project(Battleship VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Headless core library (Model, Rules, Engine)
file(GLOB_RECURSE MODEL_SOURCES "src/model/*.cpp")
file(GLOB_RECURSE CONTROLLER_SOURCES "src/controller/*.cpp")

add_library(battleship_core STATIC
    ${MODEL_SOURCES}
    ${CONTROLLER_SOURCES}
)
target_include_directories(battleship_core PUBLIC include)

# Main game binary
file(GLOB_RECURSE VIEW_SOURCES "src/view/*.cpp")
file(GLOB_RECURSE NET_SOURCES "src/network/*.cpp")

add_executable(battleship
    src/main.cpp
    ${VIEW_SOURCES}
    ${NET_SOURCES}
)
target_include_directories(battleship PRIVATE include)
target_link_libraries(battleship PRIVATE battleship_core)

# Platform graphics linkage (if using Raylib)
if(APPLE)
    target_link_libraries(battleship PRIVATE 
        "-framework OpenGL" 
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreVideo"
    )
endif()

# Unit testing
enable_testing()
add_subdirectory(tests)