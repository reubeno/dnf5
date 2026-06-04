pkg_check_modules(CPPUNIT REQUIRED cppunit)
add_definitions(-DGETTEXT_DOMAIN=\"dnf5_cmd_repograph\")

file(GLOB TEST_REPOGRAPH_SOURCES *.cpp
    ${PROJECT_SOURCE_DIR}/dnf5-plugins/repograph_plugin/repograph_graph.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/dnf5-plugins/repograph_plugin)
include_directories(${PROJECT_SOURCE_DIR}/libdnf5)
include_directories(${PROJECT_SOURCE_DIR}/test/shared)

add_executable(run_tests_repograph ${TEST_REPOGRAPH_SOURCES})
target_link_libraries(run_tests_repograph PRIVATE stdc++ libdnf5 libdnf5-cli test_shared)

add_compile_definitions(TEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

add_test(NAME test_repograph COMMAND run_tests_repograph)
set_tests_properties(test_repograph PROPERTIES
    RUN_SERIAL TRUE
    ENVIRONMENT "LC_ALL=C"
)
