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


#ifndef DNF5_PLUGINS_REPOGRAPH_PLUGIN_GRAPH_BUILDER_HPP
#define DNF5_PLUGINS_REPOGRAPH_PLUGIN_GRAPH_BUILDER_HPP


#include <libdnf5/base/transaction_package.hpp>
#include <libdnf5/rpm/package.hpp>
#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/rpm/package_set.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>


namespace dnf5::repograph {


/// Reason for an edge between two packages.
enum class EdgeKind {
    REGULAR,           ///< From `Requires` (non-pre)
    REQUIRES_PRE,      ///< From `Requires(pre)`
    RECOMMENDS,        ///< From `Recommends` (weak)
    SUGGESTS,          ///< From `Suggests` (weak)
    SUPPLEMENTED_BY,   ///< Direction-corrected from `Supplements`
    ENHANCED_BY,       ///< Direction-corrected from `Enhances`
};


std::string to_string(EdgeKind kind);


/// Policy for picking a provider among multiple satisfiers of a reldep.
enum class ProviderPolicy {
    /// In mode 2, prefer satisfiers present in the resolved node set; fall
    /// back to a deterministic tiebreak. In modes 1/3 (no Goal), falls back
    /// to BEST. Explicitly requesting SOLVER outside mode 2 is an error.
    SOLVER,
    /// First satisfier in stable id order (matches dnf4 Python behavior).
    FIRST,
    /// Repo priority, then EVR, then (name, arch, repo id, solvable id).
    BEST,
    /// Emit one edge per satisfier; mark `alt=true` when count > 1.
    ALL,
    /// Synthesize a virtual `or:<reldep>` node fanning out to all satisfiers.
    OR_NODE,
};


/// Policy for choosing both the displayed identifier of a node and how
/// (if at all) multiple package variants collapse together.
enum class NodeIdPolicy {
    NEVRA,        ///< name-epoch-version-release.arch (no merging)
    NAME,         ///< name (collapses across arches and EVRs)
};


/// Render options for dot edge labels.
enum class EdgeAnnotations {
    BOTH,     ///< "kind:reldep" per entry, comma-joined.
    RELDEP,   ///< Just the reldep string.
    KIND,     ///< Just the kind tag.
    NONE,     ///< No labels on edges.
};


/// A single reldep entry attached to an edge.
struct EdgeReldep {
    std::string reldep;
    EdgeKind kind;
    /// Set to true when this satisfier was an alternative chosen under
    /// `ProviderPolicy::ALL` (the chosen target was not unique).
    bool alt{false};
    /// Set to true when the reldep was skipped (e.g. a rich
    /// supplements/enhances). `reason` carries a human-readable explanation.
    bool skipped{false};
    std::string reason;
};


/// A node in the dependency graph. `id` is the display identifier (per
/// `NodeIdPolicy`). For merged nodes, `members` lists the underlying NEVRAs
/// so synthetic name-collision self-loops remain diagnosable.
struct Node {
    std::string id;
    std::string name;
    std::string epoch;
    std::string version;
    std::string release;
    std::string arch;
    std::string repo;
    std::string nevra;
    /// Present only when a Goal-derived `Reason` is available (mode 2).
    std::optional<std::string> reason;
    /// Underlying NEVRAs that map to this node (only populated when
    /// merging occurred).
    std::vector<std::string> members;
    /// Reldeps on this node that have no satisfier inside the closed
    /// universe. Only populated in closed-universe modes (where the
    /// caller sets `BuilderConfig::closed_universe`).
    std::vector<EdgeReldep> unresolved;
};


/// An edge between two nodes (identified by display id). Multiple
/// reldeps connecting the same (from, to) pair are aggregated.
struct Edge {
    std::string from;
    std::string to;
    std::vector<EdgeReldep> reldeps;
};


/// The complete dependency graph emitted by repograph.
struct Graph {
    /// Sorted by `id` for deterministic output.
    std::vector<Node> nodes;
    /// Sorted by (from, to) for deterministic output.
    std::vector<Edge> edges;
};


/// Inputs to the Phase 2 walker.
struct BuilderConfig {
    /// Packages whose dependencies will be walked. In modes 1 and 3 this
    /// is the same as `universe`; in mode 2 it's the solver-resolved set.
    libdnf5::rpm::PackageSet node_pkgs;
    /// Packages that may satisfy reldeps. In mode 2 this is intentionally
    /// the same as `node_pkgs` (intersection with the resolved set is how
    /// "solver" provider policy works).
    libdnf5::rpm::PackageSet universe_pkgs;
    /// Per-NEVRA reason annotations (only populated for mode 2).
    std::map<std::string, std::string> reasons;
    /// True when the builder is operating in mode 2 and Goal-based
    /// solver semantics apply (affects `ProviderPolicy::SOLVER`).
    bool have_solver_set{false};

    ProviderPolicy provider_policy{ProviderPolicy::SOLVER};
    NodeIdPolicy node_id_policy{NodeIdPolicy::NAME};
    bool include_weak_deps{true};
    bool include_reverse_weak{false};
    /// When true, the universe is closed: any reldep with zero
    /// satisfiers inside `universe_pkgs` is recorded on the source
    /// node's `unresolved` list (in addition to being silently
    /// dropped from the edge set). Set this for `--closed`,
    /// `--use-system`, and `--use-system <specs>` modes.
    bool closed_universe{false};

    explicit BuilderConfig(const libdnf5::BaseWeakPtr & base);
};


/// Build the dependency graph given the configured inputs.
///
/// Throws on internal inconsistency. Emits warnings to stderr for issues
/// that do not invalidate the overall graph (e.g. rich reverse-weak deps
/// that had to be skipped).
Graph build_graph(const BuilderConfig & cfg);


/// Compute the display id for `pkg` per the policy. Exposed for testing.
std::string display_id(const libdnf5::rpm::Package & pkg, NodeIdPolicy policy);


}  // namespace dnf5::repograph


#endif  // DNF5_PLUGINS_REPOGRAPH_PLUGIN_GRAPH_BUILDER_HPP
