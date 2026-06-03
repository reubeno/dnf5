pkg_check_modules(CPPUNIT REQUIRED cppunit)
add_definitions(-DGETTEXT_DOMAIN=\"dnf5_cmd_repograph\")

# use any sources found under the current directory
file(GLOB TEST_REPOGRAPH_SOURCES *.cpp
    ${PROJECT_SOURCE_DIR}/dnf5-plugins/repograph_plugin/repograph.cpp
    ${PROJECT_SOURCE_DIR}/dnf5-plugins/repograph_plugin/graph_builder.cpp
    ${PROJECT_SOURCE_DIR}/dnf5-plugins/repograph_plugin/dot_emitter.cpp
    ${PROJECT_SOURCE_DIR}/dnf5-plugins/repograph_plugin/json_emitter.cpp
    ${PROJECT_SOURCE_DIR}/dnf5/context.cpp
    ${PROJECT_SOURCE_DIR}/dnf5/plugins.cpp
    ${PROJECT_SOURCE_DIR}/dnf5/download_callbacks.cpp
    ${PROJECT_SOURCE_DIR}/dnf5/library.cpp
    ${PROJECT_SOURCE_DIR}/dnf5/version.cpp
    ${PROJECT_SOURCE_DIR}/dnf5/shared_options.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/dnf5-plugins/repograph_plugin)
include_directories(${PROJECT_SOURCE_DIR}/libdnf5)
include_directories(${PROJECT_SOURCE_DIR}/dnf5)
include_directories(${PROJECT_SOURCE_DIR}/dnf5/include)

include(sdbus_cpp)

pkg_check_modules(RPM REQUIRED rpm>=4.19.0)

pkg_check_modules(JSONC REQUIRED json-c)

add_executable(run_tests_repograph ${TEST_REPOGRAPH_SOURCES})
target_link_libraries(run_tests_repograph PRIVATE common_obj stdc++ libdnf5 libdnf5-cli test_shared ${SDBUS_CPP_LIBRARIES} ${RPM_LIBRARIES} ${JSONC_LIBRARIES})

add_test(NAME test_repograph COMMAND run_tests_repograph)
set_tests_properties(test_repograph PROPERTIES
    RUN_SERIAL TRUE
    ENVIRONMENT "LC_ALL=C"
)
