# set gettext domain for translations
set(GETTEXT_DOMAIN dnf5-plugin-repograph)
add_definitions(-DGETTEXT_DOMAIN=\"${GETTEXT_DOMAIN}\")

add_library(repograph_cmd_plugin MODULE
    dot_emitter.cpp
    graph_builder.cpp
    json_emitter.cpp
    repograph.cpp
    repograph_cmd_plugin.cpp
)

# disable the 'lib' prefix in order to create repograph_cmd_plugin.so
set_target_properties(repograph_cmd_plugin PROPERTIES PREFIX "")

target_link_libraries(repograph_cmd_plugin PRIVATE libdnf5 libdnf5-cli)
target_link_libraries(repograph_cmd_plugin PRIVATE dnf5)

install(TARGETS repograph_cmd_plugin LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/dnf5/plugins/)

add_subdirectory(po)
