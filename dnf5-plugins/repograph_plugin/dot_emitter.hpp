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


#ifndef DNF5_PLUGINS_REPOGRAPH_PLUGIN_DOT_EMITTER_HPP
#define DNF5_PLUGINS_REPOGRAPH_PLUGIN_DOT_EMITTER_HPP


#include "graph_builder.hpp"

#include <ostream>


namespace dnf5::repograph {


/// Stream the graph as a Graphviz dot document to `out`.
///
/// Edge labels are rendered multi-line (one dependency per line,
/// left-aligned). When the total number of entries on an edge exceeds
/// `edge_label_limit`, only the first `edge_label_limit - 1` are shown
/// followed by a summary line ("...and N more"). A value of 0 disables
/// the limit and renders every entry.
///
/// When `style` is `BY_KIND`, edges carrying only weak dependencies
/// (recommends/suggests/supplemented-by/enhanced-by) are rendered with
/// a dashed gray style; edges containing at least one strong dependency
/// (requires/requires-pre) get the default solid style.
void emit_dot(
    std::ostream & out,
    const Graph & graph,
    EdgeAnnotations annotations,
    size_t edge_label_limit = 0,
    EdgeStyle style = EdgeStyle::PLAIN);


}  // namespace dnf5::repograph


#endif  // DNF5_PLUGINS_REPOGRAPH_PLUGIN_DOT_EMITTER_HPP
