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
# Dependencies: Sockpp
# ---------------------------------------------------------
FetchContent_Declare(
    sockpp
    GIT_REPOSITORY https://github.com/fpagliughi/sockpp.git
    GIT_TAG master
)
set(SOCKPP_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_STATIC ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sockpp)

# ---------------------------------------------------------
# Headless Core Library (Model + Controller, shared with tests)
# ---------------------------------------------------------
file(GLOB_RECURSE MODEL_SOURCES "src/model/*.cpp")
file(GLOB_RECURSE CONTROLLER_SOURCES "src/controller/*.cpp")

add_library(battleship_core STATIC
    ${MODEL_SOURCES}
    ${CONTROLLER_SOURCES}
)
target_include_directories(battleship_core PUBLIC src)

# ---------------------------------------------------------
# Main Game Executable (GUI + View)
# ---------------------------------------------------------
file(GLOB_RECURSE VIEW_SOURCES "src/view/*.cpp")

add_executable(battleship
    src/main.cpp
    ${VIEW_SOURCES}
)

target_include_directories(battleship PRIVATE
    src
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
# Relay Server Executable (without raylib + battleship_core)
# ---------------------------------------------------------
add_executable(battleship_relay
    src/relay_main.cpp
    src/network/RelayServer.cpp
)

target_include_directories(battleship_relay PRIVATE src)
target_link_libraries(battleship_relay PRIVATE sockpp-static pthread)

# ---------------------------------------------------------
# Unit Testing
# ---------------------------------------------------------
enable_testing()
add_subdirectory(tests)