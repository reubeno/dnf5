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

#include "graph_builder.hpp"

#include <libdnf5/repo/repo.hpp>
#include <libdnf5/rpm/nevra.hpp>
#include <libdnf5/rpm/reldep.hpp>
#include <libdnf5/rpm/reldep_list.hpp>
#include <libdnf5/transaction/transaction_item_reason.hpp>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <unordered_map>


namespace dnf5::repograph {


BuilderConfig::BuilderConfig(const libdnf5::BaseWeakPtr & base)
    : node_pkgs(libdnf5::rpm::PackageSet(base)), universe_pkgs(libdnf5::rpm::PackageSet(base)) {}


std::string to_string(EdgeKind kind) {
    switch (kind) {
        case EdgeKind::REGULAR:
            return "regular";
        case EdgeKind::REQUIRES_PRE:
            return "requires-pre";
        case EdgeKind::RECOMMENDS:
            return "recommends";
        case EdgeKind::SUGGESTS:
            return "suggests";
        case EdgeKind::SUPPLEMENTED_BY:
            return "supplemented-by";
        case EdgeKind::ENHANCED_BY:
            return "enhanced-by";
    }
    return "unknown";
}


std::string display_id(const libdnf5::rpm::Package & pkg, NodeIdPolicy policy) {
    switch (policy) {
        case NodeIdPolicy::NEVRA:
            return pkg.get_nevra();
        case NodeIdPolicy::NAME:
            return pkg.get_name();
    }
    return pkg.get_nevra();
}


namespace {


/// Cache satisfier lookups by reldep id. The cached set is intersected
/// with both the universe and the node set as needed.
class ProviderCache {
public:
    ProviderCache(const libdnf5::BaseWeakPtr & base, const libdnf5::rpm::PackageSet & universe)
        : base_(base), universe_(universe) {}

    /// Return packages from the universe that provide `reldep`. Cached.
    const std::vector<libdnf5::rpm::Package> & lookup(const libdnf5::rpm::Reldep & reldep) {
        int rid = reldep.get_id().id;
        auto it = cache_.find(rid);
        if (it != cache_.end()) {
            return it->second;
        }
        libdnf5::rpm::PackageQuery q(base_);
        q.filter_provides(reldep);
        std::vector<libdnf5::rpm::Package> result;
        for (const auto & pkg : q) {
            if (universe_.contains(pkg)) {
                result.push_back(pkg);
            }
        }
        // Deterministic order for caches consumers that fall back to
        // FIRST/BEST/ALL semantics.
        std::sort(result.begin(), result.end(), [](const libdnf5::rpm::Package & a, const libdnf5::rpm::Package & b) {
            return a.get_id().id < b.get_id().id;
        });
        auto [ins_it, _ins] = cache_.emplace(rid, std::move(result));
        return ins_it->second;
    }

private:
    libdnf5::BaseWeakPtr base_;
    const libdnf5::rpm::PackageSet & universe_;
    std::unordered_map<int, std::vector<libdnf5::rpm::Package>> cache_;
};


/// Comparator implementing the BEST provider policy:
/// repo priority (lower numerically = higher priority), then nevra
/// (rpm comparison), then a stable id tiebreak.
int priority_of(const libdnf5::rpm::Package & p) {
    try {
        return p.get_repo()->get_priority();
    } catch (...) {
        return 99;
    }
}


bool best_less(const libdnf5::rpm::Package & a, const libdnf5::rpm::Package & b) {
    int pa = priority_of(a);
    int pb = priority_of(b);
    if (pa != pb) {
        return pa < pb;
    }
    // cmp_nevra returns true when a < b in NEVRA order. We want the
    // "best" (highest) first, so larger NEVRA sorts smaller for our
    // purposes — invert the comparison.
    if (a.get_name() != b.get_name()) {
        return a.get_name() < b.get_name();
    }
    if (a.get_arch() != b.get_arch()) {
        return a.get_arch() < b.get_arch();
    }
    // Same name+arch: prefer newer EVR (cmp_nevra checks the full tuple
    // including name/arch which are equal here).
    if (libdnf5::rpm::cmp_nevra(a, b)) {
        return false;
    }
    if (libdnf5::rpm::cmp_nevra(b, a)) {
        return true;
    }
    if (a.get_repo_id() != b.get_repo_id()) {
        return a.get_repo_id() < b.get_repo_id();
    }
    return a.get_id().id < b.get_id().id;
}


/// Pick providers given the configured policy. Returns the list of
/// chosen targets and a flag indicating whether each is an "alternative"
/// (only relevant under ProviderPolicy::ALL).
std::vector<std::pair<libdnf5::rpm::Package, bool>> select_providers(
    const std::vector<libdnf5::rpm::Package> & candidates_in_universe,
    const libdnf5::rpm::PackageSet & node_pkgs,
    ProviderPolicy policy,
    bool have_solver_set,
    const libdnf5::rpm::Package & from_pkg) {
    std::vector<std::pair<libdnf5::rpm::Package, bool>> chosen;
    if (candidates_in_universe.empty()) {
        return chosen;
    }

    if (policy == ProviderPolicy::SOLVER && have_solver_set) {
        // Filter to satisfiers that are members of the resolved set.
        std::vector<libdnf5::rpm::Package> in_set;
        for (const auto & p : candidates_in_universe) {
            if (node_pkgs.contains(p)) {
                in_set.push_back(p);
            }
        }
        if (!in_set.empty()) {
            // If multiple satisfiers ended up in the resolved set
            // (rare but possible — e.g. for file deps), apply a
            // deterministic tiebreak so output is stable.
            std::sort(in_set.begin(), in_set.end(), best_less);
            // Prefer self-provider if present (matches solver semantics
            // where a package satisfies its own dep via self-provides).
            for (const auto & p : in_set) {
                if (p == from_pkg) {
                    chosen.emplace_back(p, false);
                    return chosen;
                }
            }
            chosen.emplace_back(in_set.front(), false);
            return chosen;
        }
        // No solver-set member satisfies it — fall through to BEST
        // semantics so the edge is not lost.
    }

    if (policy == ProviderPolicy::FIRST) {
        chosen.emplace_back(candidates_in_universe.front(), false);
        return chosen;
    }

    if (policy == ProviderPolicy::ALL) {
        // De-dup by package id and emit all.
        std::vector<libdnf5::rpm::Package> sorted(candidates_in_universe);
        std::sort(sorted.begin(), sorted.end(), best_less);
        bool first = true;
        for (const auto & p : sorted) {
            chosen.emplace_back(p, !first);
            first = false;
        }
        return chosen;
    }

    if (policy == ProviderPolicy::OR_NODE) {
        // OR-node generation happens at a higher layer (we'd need to
        // synthesize a virtual node). For now, return all candidates
        // so the builder can detect this and synthesize.
        std::vector<libdnf5::rpm::Package> sorted(candidates_in_universe);
        std::sort(sorted.begin(), sorted.end(), best_less);
        for (const auto & p : sorted) {
            chosen.emplace_back(p, false);
        }
        return chosen;
    }

    // BEST (also the SOLVER fallback when no Goal context).
    std::vector<libdnf5::rpm::Package> sorted(candidates_in_universe);
    std::sort(sorted.begin(), sorted.end(), best_less);
    chosen.emplace_back(sorted.front(), false);
    return chosen;
}


void add_reldep_to_edge(
    std::map<std::pair<std::string, std::string>, Edge> & edge_map,
    const std::string & from_id,
    const std::string & to_id,
    const std::string & reldep_str,
    EdgeKind kind,
    bool alt) {
    auto key = std::make_pair(from_id, to_id);
    auto it = edge_map.find(key);
    if (it == edge_map.end()) {
        Edge e;
        e.from = from_id;
        e.to = to_id;
        it = edge_map.emplace(key, std::move(e)).first;
    }
    EdgeReldep er;
    er.reldep = reldep_str;
    er.kind = kind;
    er.alt = alt;
    // De-duplicate identical reldep entries on the same edge.
    for (const auto & existing : it->second.reldeps) {
        if (existing.reldep == er.reldep && existing.kind == er.kind && existing.alt == er.alt) {
            return;
        }
    }
    it->second.reldeps.push_back(std::move(er));
}


/// Walk one forward-dep bucket for a single source package.
void walk_forward(
    const libdnf5::rpm::Package & pkg,
    const std::string & from_id,
    const libdnf5::rpm::ReldepList & deps,
    EdgeKind kind,
    ProviderCache & cache,
    const BuilderConfig & cfg,
    std::map<std::pair<std::string, std::string>, Edge> & edge_map) {
    for (const auto & rd : deps) {
        const auto & candidates = cache.lookup(rd);
        auto providers = select_providers(candidates, cfg.node_pkgs, cfg.provider_policy, cfg.have_solver_set, pkg);
        std::string rd_str = rd.to_string();

        if (cfg.provider_policy == ProviderPolicy::OR_NODE && providers.size() > 1) {
            // Synthesize "or:<reldep>" virtual node.
            std::string or_id = "or:" + rd_str;
            add_reldep_to_edge(edge_map, from_id, or_id, rd_str, kind, false);
            for (const auto & [prov, _alt] : providers) {
                std::string to_id = display_id(prov, cfg.node_id_policy);
                add_reldep_to_edge(edge_map, or_id, to_id, rd_str, kind, false);
            }
        } else {
            for (const auto & [prov, alt] : providers) {
                std::string to_id = display_id(prov, cfg.node_id_policy);
                add_reldep_to_edge(edge_map, from_id, to_id, rd_str, kind, alt);
            }
        }
    }
}


/// Reverse-weak walk: for each node, look up packages in the universe
/// that supplement/enhance *this* node. Draw direction-corrected
/// forward edges from this node to the supplementing/enhancing pkg.
/// Rich reverse-weak reldeps are skipped with a logged warning because
/// resolving the condition reliably requires solver-level reasoning
/// that we don't reproduce here.
void walk_reverse_weak(
    const libdnf5::rpm::Package & pkg,
    const std::string & from_id,
    EdgeKind kind,
    const libdnf5::BaseWeakPtr & base,
    const BuilderConfig & cfg,
    std::map<std::pair<std::string, std::string>, Edge> & edge_map) {
    libdnf5::rpm::PackageSet single(base);
    single.add(pkg);
    libdnf5::rpm::PackageQuery q(base);
    if (kind == EdgeKind::SUPPLEMENTED_BY) {
        q.filter_supplements(single);
    } else {
        q.filter_enhances(single);
    }
    for (const auto & supplier : q) {
        if (!cfg.universe_pkgs.contains(supplier)) {
            continue;
        }
        // Detect rich reverse-weak reldeps that mention more than this
        // package. We inspect the supplier's own supplements/enhances
        // list; any rich entry that *would* otherwise be matched against
        // this node is treated as ambiguous and skipped.
        const auto & sup_list =
            (kind == EdgeKind::SUPPLEMENTED_BY) ? supplier.get_supplements() : supplier.get_enhances();
        bool rich_seen = false;
        std::string rd_str;
        for (const auto & rd : sup_list) {
            std::string s = rd.to_string();
            if (libdnf5::rpm::Reldep::is_rich_dependency(s)) {
                rich_seen = true;
                rd_str = s;
                break;
            }
            // Fallback: pick the first non-rich one as the canonical
            // annotation (we can't easily know which one matched the
            // current node without re-running provider resolution).
            if (rd_str.empty()) {
                rd_str = s;
            }
        }
        if (rich_seen) {
            std::fprintf(
                stderr,
                "repograph: warning: skipping rich %s reldep on %s -> %s (\"%s\")\n",
                to_string(kind).c_str(),
                supplier.get_nevra().c_str(),
                pkg.get_nevra().c_str(),
                rd_str.c_str());
            continue;
        }
        std::string to_id = display_id(supplier, cfg.node_id_policy);
        add_reldep_to_edge(edge_map, from_id, to_id, rd_str, kind, false);
    }
}


std::optional<std::string> reason_string_for(
    const std::string & nevra, const std::map<std::string, std::string> & reasons) {
    auto it = reasons.find(nevra);
    if (it == reasons.end()) {
        return std::nullopt;
    }
    return it->second;
}


}  // namespace


Graph build_graph(const BuilderConfig & cfg) {
    if (cfg.provider_policy == ProviderPolicy::SOLVER && !cfg.have_solver_set) {
        // The caller is responsible for ensuring SOLVER is requested
        // only when explicitly compatible. The CLI surface guards
        // against explicit user requests; this branch only fires when
        // the default is unchanged and we silently switch to BEST.
        // Construct a mutable copy with the fallback.
        BuilderConfig adjusted = cfg;
        adjusted.provider_policy = ProviderPolicy::BEST;
        return build_graph(adjusted);
    }

    // We can pull the base from any package in the universe set.
    if (cfg.universe_pkgs.empty() && cfg.node_pkgs.empty()) {
        return {};
    }

    libdnf5::BaseWeakPtr base = cfg.universe_pkgs.empty()
                                    ? (*cfg.node_pkgs.begin()).get_base()
                                    : (*cfg.universe_pkgs.begin()).get_base();

    ProviderCache cache(base, cfg.universe_pkgs);

    std::map<std::string, Node> node_map;
    std::map<std::pair<std::string, std::string>, Edge> edge_map;

    for (const auto & pkg : cfg.node_pkgs) {
        std::string id = display_id(pkg, cfg.node_id_policy);
        auto node_it = node_map.find(id);
        if (node_it == node_map.end()) {
            Node n;
            n.id = id;
            n.name = pkg.get_name();
            n.epoch = pkg.get_epoch();
            n.version = pkg.get_version();
            n.release = pkg.get_release();
            n.arch = pkg.get_arch();
            n.repo = pkg.get_repo_id();
            n.nevra = pkg.get_nevra();
            n.reason = reason_string_for(pkg.get_nevra(), cfg.reasons);
            node_it = node_map.emplace(id, std::move(n)).first;
        } else if (cfg.node_id_policy != NodeIdPolicy::NEVRA) {
            // Track member NEVRAs for merged nodes.
            if (node_it->second.members.empty()) {
                node_it->second.members.push_back(node_it->second.nevra);
            }
            node_it->second.members.push_back(pkg.get_nevra());
        }

        walk_forward(pkg, id, pkg.get_regular_requires(), EdgeKind::REGULAR, cache, cfg, edge_map);
        walk_forward(pkg, id, pkg.get_requires_pre(), EdgeKind::REQUIRES_PRE, cache, cfg, edge_map);
        if (cfg.include_weak_deps) {
            walk_forward(pkg, id, pkg.get_recommends(), EdgeKind::RECOMMENDS, cache, cfg, edge_map);
            walk_forward(pkg, id, pkg.get_suggests(), EdgeKind::SUGGESTS, cache, cfg, edge_map);
        }
        if (cfg.include_reverse_weak) {
            walk_reverse_weak(pkg, id, EdgeKind::SUPPLEMENTED_BY, base, cfg, edge_map);
            walk_reverse_weak(pkg, id, EdgeKind::ENHANCED_BY, base, cfg, edge_map);
        }
    }

    Graph g;
    g.nodes.reserve(node_map.size());
    for (auto & [_id, n] : node_map) {
        // Deterministic ordering of members.
        if (!n.members.empty()) {
            std::sort(n.members.begin(), n.members.end());
            n.members.erase(std::unique(n.members.begin(), n.members.end()), n.members.end());
        }
        g.nodes.push_back(std::move(n));
    }
    std::sort(g.nodes.begin(), g.nodes.end(), [](const Node & a, const Node & b) { return a.id < b.id; });

    g.edges.reserve(edge_map.size());
    for (auto & [_key, e] : edge_map) {
        // Deterministic ordering of reldeps within an edge.
        std::sort(e.reldeps.begin(), e.reldeps.end(), [](const EdgeReldep & a, const EdgeReldep & b) {
            if (a.kind != b.kind) {
                return to_string(a.kind) < to_string(b.kind);
            }
            return a.reldep < b.reldep;
        });
        g.edges.push_back(std::move(e));
    }
    std::sort(g.edges.begin(), g.edges.end(), [](const Edge & a, const Edge & b) {
        if (a.from != b.from) {
            return a.from < b.from;
        }
        return a.to < b.to;
    });

    return g;
}


}  // namespace dnf5::repograph
