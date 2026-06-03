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


#ifndef DNF5_PLUGINS_REPOGRAPH_PLUGIN_JSON_EMITTER_HPP
#define DNF5_PLUGINS_REPOGRAPH_PLUGIN_JSON_EMITTER_HPP


#include "graph_builder.hpp"

#include <ostream>
#include <string>


namespace dnf5::repograph {


/// Stream the graph as a JSON document to `out`. The top-level fields
/// `mode`, `node_id_policy`, and `provider_policy` describe how the
/// graph was produced.
void emit_json(
    std::ostream & out,
    const Graph & graph,
    const std::string & mode_name,
    const std::string & node_id_policy_name,
    const std::string & provider_policy_name);


}  // namespace dnf5::repograph


#endif  // DNF5_PLUGINS_REPOGRAPH_PLUGIN_JSON_EMITTER_HPP
