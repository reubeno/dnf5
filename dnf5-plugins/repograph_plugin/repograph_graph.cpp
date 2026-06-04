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
#include <unordered_map>
#include <unordered_set>
#include <utility>


namespace {


using Edge = std::pair<std::string, std::string>;

struct Providers {
    std::optional<std::string> first;
    std::optional<std::string> second;
    std::unordered_set<int> package_ids;
};

using ProviderCache = std::unordered_map<int, Providers>;


// Keep package IDs to distinguish self-satisfaction from another version or arch
// of the same name providing the dependency.
std::optional<std::string> select_provider_name(
    ProviderCache & cache,
    dnf5::repograph::GraphStats & stats,
    const libdnf5::rpm::PackageQuery & universe_query,
    const libdnf5::rpm::Package & source,
    const std::string & from_name,
    const libdnf5::rpm::Reldep & dependency) {
    auto [cached, inserted] = cache.try_emplace(dependency.get_id().id);
    auto & names = cached->second;
    if (inserted) {
        ++stats.provider_cache_misses;
        stats.has_rich_dependencies =
            stats.has_rich_dependencies || libdnf5::rpm::Reldep::is_rich_dependency(dependency.to_string());

        libdnf5::rpm::PackageQuery providers(universe_query);
        providers.filter_provides(dependency);
        for (const auto & provider : providers) {
            names.package_ids.insert(provider.get_id().id);
            auto name = provider.get_name();
            if (name.empty()) {
                continue;
            }
            // Keep two names to avoid self-edges between different builds.
            if (!names.first || name < *names.first) {
                names.second = std::move(names.first);
                names.first = std::move(name);
            } else if (name != *names.first && (!names.second || name < *names.second)) {
                names.second = std::move(name);
            }
        }
    } else {
        ++stats.provider_cache_hits;
    }
    if (names.package_ids.contains(source.get_id().id)) {
        return std::nullopt;
    }
    return names.first && *names.first == from_name ? names.second : names.first;
}


void add_dependency_edges(
    std::set<Edge> & edges,
    ProviderCache & cache,
    dnf5::repograph::GraphStats & stats,
    const libdnf5::rpm::PackageQuery & universe_query,
    const libdnf5::rpm::Package & source,
    const std::string & from_name,
    const libdnf5::rpm::ReldepList & dependencies) {
    for (const auto & dependency : dependencies) {
        if (auto provider_name = select_provider_name(cache, stats, universe_query, source, from_name, dependency)) {
            edges.emplace(from_name, *provider_name);
        }
    }
}


}  // namespace


namespace dnf5::repograph {


GraphStats emit_graph(std::ostream & out, const libdnf5::rpm::PackageSet & node_pkgs, bool include_recommends) {
    GraphStats stats;
    ProviderCache cache;
    std::set<std::string> node_names;
    for (const auto & package : node_pkgs) {
        node_names.insert(package.get_name());
    }

    std::set<Edge> edges;
    libdnf5::rpm::PackageQuery universe_query(node_pkgs);
    for (const auto & package : node_pkgs) {
        const auto package_name = package.get_name();
        add_dependency_edges(edges, cache, stats, universe_query, package, package_name, package.get_requires());
        // Include only Recommends because they are outgoing dependencies that
        // fit this graph; install_weak_deps controls whether they are included.
        if (include_recommends) {
            add_dependency_edges(edges, cache, stats, universe_query, package, package_name, package.get_recommends());
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
    return stats;
}


}  // namespace dnf5::repograph
