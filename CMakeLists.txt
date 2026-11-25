cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.2 CACHE STRING "" FORCE)

project(Battleship VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------
# Dependencies
# ---------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    sockpp
    GIT_REPOSITORY https://github.com/fpagliughi/sockpp.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

set(SOCKPP_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_STATIC ON  CACHE BOOL "" FORCE)

FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v3.6.0  # Pin to a known stable release
    GIT_SHALLOW TRUE
)

# Asio has no top-level CMakeLists.txt, so populate manually.
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
endif()

FetchContent_MakeAvailable(raylib sockpp glaze)

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

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(battleship_core PRIVATE -fbracket-depth=1024)
endif()

# Force C++20 and block transitive C++23 elevation
set_target_properties(battleship_core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# ---------------------------------------------------------
# Main Game Executable (GUI + Networking client)
# ---------------------------------------------------------
file(GLOB_RECURSE VIEW_SOURCES "src/view/*.cpp")
file(GLOB_RECURSE APP_SOURCES  "src/app/*.cpp")

add_library(battleship_app STATIC
    src/network/Client.cpp
    ${VIEW_SOURCES}
    ${APP_SOURCES}
)

target_include_directories(battleship_app PRIVATE
    src
    ${raylib_SOURCE_DIR}/src
)

target_link_libraries(battleship_app PUBLIC battleship_core raylib sockpp-static pthread)

if(APPLE)
    target_link_libraries(battleship_app PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

add_executable(battleship src/main.cpp)
target_link_libraries(battleship PRIVATE battleship_app)

# ---------------------------------------------------------
# Relay Server Executable
# ---------------------------------------------------------
add_library(battleship_server_lib STATIC
    src/network/Server.cpp
    src/network/GameSession.cpp
)

target_include_directories(battleship_server_lib PUBLIC
    src
    ${asio_SOURCE_DIR}/asio/include
)
target_compile_definitions(battleship_server_lib PUBLIC ASIO_STANDALONE)
target_link_libraries(battleship_server_lib PUBLIC battleship_core sockpp-static pthread)

add_executable(battleship_server src/main_server.cpp)
target_link_libraries(battleship_server PRIVATE battleship_server_lib)

# ---------------------------------------------------------
# Unit Testing
# ---------------------------------------------------------
enable_testing()
add_subdirectory(tests)
