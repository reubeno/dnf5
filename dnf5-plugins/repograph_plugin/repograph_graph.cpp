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

#include "repograph_graph.hpp"

#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/rpm/reldep.hpp>

#include <iomanip>
#include <optional>
#include <set>


namespace {


using Edge = std::pair<std::string, std::string>;


std::optional<std::string> select_provider_name(
    const libdnf5::rpm::PackageQuery & universe_query,
    const std::string & from_name,
    const libdnf5::rpm::Reldep & dependency) {
    libdnf5::rpm::PackageQuery providers(universe_query);
    providers.filter_provides(dependency);

    std::optional<std::string> selected_name;
    for (const auto & provider : providers) {
        auto provider_name = provider.get_name();
        if (provider_name.empty() || provider_name == from_name) {
            continue;
        }
        // DNF4 also selected one provider per dependency (provider[0]).
        // Select the lexicographically first non-self package name here so
        // the single-provider policy does not depend on query iteration order.
        if (!selected_name || provider_name < *selected_name) {
            selected_name = provider_name;
        }
    }
    return selected_name;
}


void add_dependency_edges(
    std::set<Edge> & edges,
    const libdnf5::rpm::PackageQuery & universe_query,
    const std::string & from_name,
    const libdnf5::rpm::ReldepList & dependencies) {
    for (const auto & dependency : dependencies) {
        if (auto provider_name = select_provider_name(universe_query, from_name, dependency)) {
            edges.emplace(from_name, *provider_name);
        }
    }
}


}  // namespace


namespace dnf5::repograph {


void emit_graph(std::ostream & out, const libdnf5::rpm::PackageSet & node_pkgs, bool include_recommends) {
    std::set<std::string> node_names;
    for (const auto & package : node_pkgs) {
        node_names.insert(package.get_name());
    }

    std::set<Edge> edges;
    libdnf5::rpm::PackageQuery universe_query(node_pkgs);
    for (const auto & package : node_pkgs) {
        const auto package_name = package.get_name();
        add_dependency_edges(edges, universe_query, package_name, package.get_requires());
        // Include only Recommends because they are outgoing dependencies that
        // fit this graph; install_weak_deps controls whether they are included.
        if (include_recommends) {
            add_dependency_edges(edges, universe_query, package_name, package.get_recommends());
        }
    }

    out << "digraph packages {\n";
    out << "    node [shape=box, style=rounded, fontname=\"Helvetica\"];\n";
    for (const auto & name : node_names) {
        out << "    " << std::quoted(name) << ";\n";
    }
    for (const auto & [from, to] : edges) {
        out << "    " << std::quoted(from) << " -> " << std::quoted(to) << ";\n";
    }
    out << "}\n";
}


}  // namespace dnf5::repograph
