cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(Battleship VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------
# Dependencies: Raylib
# ---------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# ---------------------------------------------------------
# Headless Core Library (Model + Controller, shared with tests)
# ---------------------------------------------------------
file(GLOB_RECURSE MODEL_SOURCES "src/model/*.cpp")
file(GLOB_RECURSE CONTROLLER_SOURCES "src/controller/*.cpp")

add_library(battleship_core STATIC
    ${MODEL_SOURCES}
    ${CONTROLLER_SOURCES}
)
target_include_directories(battleship_core PUBLIC include)

# ---------------------------------------------------------
# Main Game Executable (GUI + View)
# ---------------------------------------------------------
file(GLOB_RECURSE VIEW_SOURCES "src/view/*.cpp")

add_executable(battleship
    src/main.cpp
    ${VIEW_SOURCES}
)

target_include_directories(battleship PRIVATE 
    include
    ${raylib_SOURCE_DIR}/src
)

target_link_libraries(battleship PRIVATE battleship_core raylib)

if(APPLE)
    target_link_libraries(battleship PRIVATE 
        "-framework OpenGL" 
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreVideo"
    )
endif()

# ---------------------------------------------------------
# Unit Testing
# ---------------------------------------------------------
enable_testing()
add_subdirectory(tests)