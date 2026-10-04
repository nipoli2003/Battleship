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
    GIT_SHALLOW TRUE
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
    GIT_SHALLOW TRUE
)
set(SOCKPP_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_STATIC ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sockpp)

# ---------------------------------------------------------
# Dependencies: Glaze
# ---------------------------------------------------------
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

# target_link_libraries(${PROJECT_NAME} PRIVATE glaze::glaze)

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
target_link_libraries(battleship_core PUBLIC glaze::glaze)

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
add_executable(battleship_server
    src/relay_main.cpp
    src/network/Server.cpp
)

target_include_directories(battleship_server PRIVATE src)
target_link_libraries(battleship_server PRIVATE sockpp-static pthread glaze::glaze)

# ---------------------------------------------------------
# Unit Testing
# ---------------------------------------------------------
enable_testing()
add_subdirectory(tests)