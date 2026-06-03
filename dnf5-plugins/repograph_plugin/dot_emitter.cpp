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


std::string render_entry(const EdgeReldep & rd, EdgeAnnotations annotations) {
    switch (annotations) {
        case EdgeAnnotations::BOTH:
            return to_string(rd.kind) + ":" + rd.reldep;
        case EdgeAnnotations::RELDEP:
            return rd.reldep;
        case EdgeAnnotations::KIND:
            return to_string(rd.kind);
        case EdgeAnnotations::NONE:
            return {};
    }
    return {};
}


/// Escape characters that Graphviz dot interprets inside a quoted label.
/// In particular `\` introduces escapes (`\l`, `\n`, `\r`, `\N`, etc.),
/// so each literal backslash and double-quote in user-supplied text
/// must be escaped. We deliberately *do not* escape `\l` sequences we
/// add ourselves for line breaks; this function is only applied to
/// individual entry strings.
std::string escape_label_text(const std::string & s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}


std::string render_edge_label(const Edge & edge, EdgeAnnotations annotations, size_t limit) {
    if (annotations == EdgeAnnotations::NONE || edge.reldeps.empty()) {
        return {};
    }
    const size_t total = edge.reldeps.size();
    const bool capped = limit > 0 && total > limit;
    // When capping, reserve the last visible slot for the "...and N more" line.
    const size_t to_render = capped ? limit - 1 : total;

    std::string label;
    for (size_t i = 0; i < to_render; ++i) {
        label.append(escape_label_text(render_entry(edge.reldeps[i], annotations)));
        // Graphviz `\l` is a left-aligned newline; using it for every
        // line (including the last) keeps the whole block flush-left.
        label.append("\\l");
    }
    if (capped) {
        label.append("\\l...and ");
        label.append(std::to_string(total - to_render));
        label.append(" more\\l");
    }
    return label;
}


bool is_strong_kind(EdgeKind k) {
    return k == EdgeKind::REQUIRES || k == EdgeKind::REQUIRES_PRE;
}


/// Edge style attributes (e.g. `style=dashed,color="#888"`) appropriate
/// for the dominant kind of `edge`. An edge with at least one strong
/// dependency is rendered as strong (solid); otherwise it's weak-only
/// and gets dashed-gray.
std::string render_edge_style(const Edge & edge) {
    for (const auto & rd : edge.reldeps) {
        if (is_strong_kind(rd.kind)) {
            return {};  // default solid
        }
    }
    return "style=dashed,color=\"#888888\"";
}


}  // namespace


void emit_dot(
    std::ostream & out,
    const Graph & graph,
    EdgeAnnotations annotations,
    size_t edge_label_limit,
    EdgeStyle style) {
    out << "digraph packages {\n";
    for (const auto & n : graph.nodes) {
        out << "    " << quote(n.id) << ";\n";
    }
    for (const auto & e : graph.edges) {
        out << "    " << quote(e.from) << " -> " << quote(e.to);

        std::string label = render_edge_label(e, annotations, edge_label_limit);
        std::string style_attrs = (style == EdgeStyle::BY_KIND) ? render_edge_style(e) : std::string{};

        if (!label.empty() || !style_attrs.empty()) {
            out << " [";
            bool need_comma = false;
            if (!label.empty()) {
                out << "label=\"" << label << "\"";
                need_comma = true;
            }
            if (!style_attrs.empty()) {
                if (need_comma) {
                    out << ",";
                }
                out << style_attrs;
            }
            out << "]";
        }
        out << ";\n";
    }
    out << "}\n";
}


}  // namespace dnf5::repograph
