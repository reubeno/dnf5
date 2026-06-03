// Copyright Contributors to the DNF5 project.
// Copyright Contributors to the libdnf project.
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of libdnf: https://github.com/rpm-software-management/libdnf/
//
// Libdnf is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Libdnf is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libdnf.  If not, see <https://www.gnu.org/licenses/>.

#include "json_emitter.hpp"

#include <json-c/json.h>


namespace dnf5::repograph {


namespace {


json_object * build_node(const Node & n) {
    json_object * j = json_object_new_object();
    json_object_object_add(j, "id", json_object_new_string(n.id.c_str()));
    json_object_object_add(j, "name", json_object_new_string(n.name.c_str()));
    if (!n.epoch.empty()) {
        // libsolv may report epoch as a string of digits, or empty.
        // Emit as a string to avoid losing leading zeros and to keep
        // missing-epoch entries omitted.
        json_object_object_add(j, "epoch", json_object_new_string(n.epoch.c_str()));
    }
    json_object_object_add(j, "version", json_object_new_string(n.version.c_str()));
    json_object_object_add(j, "release", json_object_new_string(n.release.c_str()));
    json_object_object_add(j, "arch", json_object_new_string(n.arch.c_str()));
    json_object_object_add(j, "repo", json_object_new_string(n.repo.c_str()));
    json_object_object_add(j, "nevra", json_object_new_string(n.nevra.c_str()));
    if (n.reason) {
        json_object_object_add(j, "reason", json_object_new_string(n.reason->c_str()));
    }
    if (!n.members.empty()) {
        json_object * mem = json_object_new_array();
        for (const auto & m : n.members) {
            json_object_array_add(mem, json_object_new_string(m.c_str()));
        }
        json_object_object_add(j, "members", mem);
    }
    return j;
}


json_object * build_edge(const Edge & e) {
    json_object * j = json_object_new_object();
    json_object_object_add(j, "from", json_object_new_string(e.from.c_str()));
    json_object_object_add(j, "to", json_object_new_string(e.to.c_str()));
    json_object * reldeps = json_object_new_array();
    for (const auto & rd : e.reldeps) {
        json_object * jrd = json_object_new_object();
        json_object_object_add(jrd, "reldep", json_object_new_string(rd.reldep.c_str()));
        json_object_object_add(jrd, "kind", json_object_new_string(to_string(rd.kind).c_str()));
        if (rd.alt) {
            json_object_object_add(jrd, "alt", json_object_new_boolean(true));
        }
        if (rd.skipped) {
            json_object_object_add(jrd, "skipped", json_object_new_boolean(true));
            if (!rd.reason.empty()) {
                json_object_object_add(jrd, "reason", json_object_new_string(rd.reason.c_str()));
            }
        }
        json_object_array_add(reldeps, jrd);
    }
    json_object_object_add(j, "reldeps", reldeps);
    return j;
}


}  // namespace


void emit_json(
    std::ostream & out,
    const Graph & graph,
    const std::string & mode_name,
    const std::string & node_label_policy_name,
    const std::string & resolver_name) {
    json_object * root = json_object_new_object();
    json_object_object_add(root, "mode", json_object_new_string(mode_name.c_str()));
    json_object_object_add(root, "node_label_policy", json_object_new_string(node_label_policy_name.c_str()));
    json_object_object_add(root, "resolver", json_object_new_string(resolver_name.c_str()));

    json_object * nodes = json_object_new_array();
    for (const auto & n : graph.nodes) {
        json_object_array_add(nodes, build_node(n));
    }
    json_object_object_add(root, "nodes", nodes);

    json_object * edges = json_object_new_array();
    for (const auto & e : graph.edges) {
        json_object_array_add(edges, build_edge(e));
    }
    json_object_object_add(root, "edges", edges);

    out << json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY) << "\n";
    json_object_put(root);
}


}  // namespace dnf5::repograph
