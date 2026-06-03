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

#include "dot_emitter.hpp"

#include <string>


namespace dnf5::repograph {


namespace {


/// Quote a string for use as a dot identifier or attribute value.
std::string quote(const std::string & s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (char c : s) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}


std::string render_edge_label(const Edge & edge, EdgeAnnotations annotations) {
    if (annotations == EdgeAnnotations::NONE || edge.reldeps.empty()) {
        return {};
    }
    std::string label;
    bool first = true;
    for (const auto & rd : edge.reldeps) {
        if (!first) {
            label.append(", ");
        }
        first = false;
        switch (annotations) {
            case EdgeAnnotations::BOTH:
                label.append(to_string(rd.kind));
                label.append(":");
                label.append(rd.reldep);
                break;
            case EdgeAnnotations::RELDEP:
                label.append(rd.reldep);
                break;
            case EdgeAnnotations::KIND:
                label.append(to_string(rd.kind));
                break;
            case EdgeAnnotations::NONE:
                break;
        }
    }
    return label;
}


}  // namespace


void emit_dot(std::ostream & out, const Graph & graph, EdgeAnnotations annotations) {
    out << "digraph packages {\n";
    for (const auto & n : graph.nodes) {
        out << "    " << quote(n.id) << ";\n";
    }
    for (const auto & e : graph.edges) {
        out << "    " << quote(e.from) << " -> " << quote(e.to);
        std::string label = render_edge_label(e, annotations);
        if (!label.empty()) {
            out << " [label=" << quote(label) << "]";
        }
        out << ";\n";
    }
    out << "}\n";
}


}  // namespace dnf5::repograph
