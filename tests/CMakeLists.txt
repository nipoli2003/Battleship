include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(unit_tests
    test_app.cpp
    test_board.cpp
    test_player_name.cpp
    test_admiral_names.cpp
)

add_executable(integration_tests
    test_connection.cpp
)

target_link_libraries(unit_tests PRIVATE
    battleship_app
    battleship_server_lib
    GTest::gtest_main
)

target_link_libraries(integration_tests PRIVATE
    battleship_app
    battleship_server_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)