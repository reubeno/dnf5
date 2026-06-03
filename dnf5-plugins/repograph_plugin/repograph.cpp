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

#include "repograph.hpp"

#include "dot_emitter.hpp"
#include "json_emitter.hpp"

#include <libdnf5-cli/argument_parser.hpp>
#include <libdnf5-cli/exception.hpp>
#include <libdnf5/base/goal.hpp>
#include <libdnf5/conf/const.hpp>
#include <libdnf5/conf/option_enum.hpp>
#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/transaction/transaction_item_reason.hpp>
#include <libdnf5/utils/bgettext/bgettext-mark-domain.h>
#include <libdnf5/utils/format.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>


namespace dnf5 {


namespace {


constexpr const char * RESOLVER_SOLVER = "solver";
constexpr const char * RESOLVER_FIRST = "first";
constexpr const char * RESOLVER_BEST = "best";
constexpr const char * RESOLVER_ALL = "all";
constexpr const char * RESOLVER_OR_NODE = "or-node";

constexpr const char * NODE_LABEL_NEVRA = "nevra";
constexpr const char * NODE_LABEL_NAME = "name";

constexpr const char * EDGE_LABEL_NONE = "none";
constexpr const char * EDGE_LABEL_DEP = "dep";
constexpr const char * EDGE_LABEL_KIND = "kind";
constexpr const char * EDGE_LABEL_BOTH = "both";

constexpr const char * EDGE_STYLE_PLAIN = "plain";
constexpr const char * EDGE_STYLE_BY_KIND = "by-kind";

constexpr const char * FORMAT_DOT = "dot";
constexpr const char * FORMAT_JSON = "json";


std::string mode_to_string(RepographMode mode) {
    switch (mode) {
        case RepographMode::REPO_WIDE:
            return "repo-wide";
        case RepographMode::TARGETED:
            return "targeted";
        case RepographMode::SPECS_CLOSED:
            return "specs-closed";
        case RepographMode::CLOSED_SET:
            return "closed-set";
        case RepographMode::TARGETED_CLOSED:
            return "targeted-closed";
    }
    return "unknown";
}


repograph::ProviderPolicy parse_resolver(const std::string & s) {
    if (s == RESOLVER_SOLVER)
        return repograph::ProviderPolicy::SOLVER;
    if (s == RESOLVER_FIRST)
        return repograph::ProviderPolicy::FIRST;
    if (s == RESOLVER_BEST)
        return repograph::ProviderPolicy::BEST;
    if (s == RESOLVER_ALL)
        return repograph::ProviderPolicy::ALL;
    if (s == RESOLVER_OR_NODE)
        return repograph::ProviderPolicy::OR_NODE;
    throw std::runtime_error("invalid resolver: " + s);
}


repograph::NodeIdPolicy parse_node_label(const std::string & s) {
    if (s == NODE_LABEL_NEVRA)
        return repograph::NodeIdPolicy::NEVRA;
    if (s == NODE_LABEL_NAME)
        return repograph::NodeIdPolicy::NAME;
    throw std::runtime_error("invalid node-label: " + s);
}


repograph::EdgeAnnotations parse_edge_label(const std::string & s) {
    if (s == EDGE_LABEL_BOTH)
        return repograph::EdgeAnnotations::BOTH;
    if (s == EDGE_LABEL_DEP)
        return repograph::EdgeAnnotations::RELDEP;
    if (s == EDGE_LABEL_KIND)
        return repograph::EdgeAnnotations::KIND;
    if (s == EDGE_LABEL_NONE)
        return repograph::EdgeAnnotations::NONE;
    throw std::runtime_error("invalid edge-label: " + s);
}


repograph::EdgeStyle parse_edge_style(const std::string & s) {
    if (s == EDGE_STYLE_PLAIN)
        return repograph::EdgeStyle::PLAIN;
    if (s == EDGE_STYLE_BY_KIND)
        return repograph::EdgeStyle::BY_KIND;
    throw std::runtime_error("invalid edge-style: " + s);
}


}  // namespace


void RepographCommand::set_parent_command() {
    auto * arg_parser_parent_cmd = get_session().get_argument_parser().get_root_command();
    auto * arg_parser_this_cmd = get_argument_parser_command();
    arg_parser_parent_cmd->register_command(arg_parser_this_cmd);
}


void RepographCommand::set_argument_parser() {
    auto & ctx = get_context();
    auto & parser = ctx.get_argument_parser();
    auto & cmd = *get_argument_parser_command();

    cmd.set_description(_("Emit the package dependency graph in dot or JSON format"));

    auto * specs = parser.add_new_positional_arg(
        "specs", libdnf5::cli::ArgumentParser::PositionalArg::UNLIMITED, nullptr, nullptr);
    specs->set_description(_("Root package specs whose dependency closure will be graphed"));
    specs->set_parse_hook_func(
        [this](
            [[maybe_unused]] libdnf5::cli::ArgumentParser::PositionalArg * arg, int argc, const char * const argv[]) {
            for (int i = 0; i < argc; ++i) {
                pkg_specs.emplace_back(argv[i]);
            }
            return true;
        });
    cmd.register_positional_arg(specs);

    use_system_option =
        dynamic_cast<libdnf5::OptionBool *>(parser.add_init_value(std::make_unique<libdnf5::OptionBool>(false)));
    auto * use_system_arg = parser.add_new_named_arg("use-system");
    use_system_arg->set_long_name("use-system");
    use_system_arg->set_description(
        _("Use installed packages as the graph universe (and as roots when no SPEC is given)"));
    use_system_arg->set_const_value("true");
    use_system_arg->link_value(use_system_option);
    cmd.register_named_arg(use_system_arg);

    closed_option =
        dynamic_cast<libdnf5::OptionBool *>(parser.add_init_value(std::make_unique<libdnf5::OptionBool>(false)));
    auto * closed_arg = parser.add_new_named_arg("closed");
    closed_arg->set_long_name("closed");
    closed_arg->set_description(
        _("Treat the positional SPECs as the entire universe: no dependency closure expansion. "
          "Requires SPECs; cannot be combined with --use-system."));
    closed_arg->set_const_value("true");
    closed_arg->link_value(closed_option);
    cmd.register_named_arg(closed_arg);

    include_reverse_weak_option =
        dynamic_cast<libdnf5::OptionBool *>(parser.add_init_value(std::make_unique<libdnf5::OptionBool>(false)));
    auto * include_reverse_weak_arg = parser.add_new_named_arg("include-reverse-weak");
    include_reverse_weak_arg->set_long_name("include-reverse-weak");
    include_reverse_weak_arg->set_description(
        _("Also draw edges derived from Supplements/Enhances (direction-corrected)"));
    include_reverse_weak_arg->set_const_value("true");
    include_reverse_weak_arg->link_value(include_reverse_weak_option);
    cmd.register_named_arg(include_reverse_weak_arg);

    resolver_option =
        dynamic_cast<libdnf5::OptionEnum *>(parser.add_init_value(
            std::make_unique<libdnf5::OptionEnum>(
                RESOLVER_SOLVER,
                std::vector<std::string>{
                    RESOLVER_SOLVER,
                    RESOLVER_FIRST,
                    RESOLVER_BEST,
                    RESOLVER_ALL,
                    RESOLVER_OR_NODE})));
    auto * resolver_arg = parser.add_new_named_arg("resolver");
    resolver_arg->set_long_name("resolver");
    resolver_arg->set_description(
        _("How to resolve which provider to draw an edge to when multiple packages satisfy a dep "
          "(default: solver)"));
    resolver_arg->set_has_value(true);
    resolver_arg->set_arg_value_help("solver|first|best|all|or-node");
    resolver_arg->link_value(resolver_option);
    cmd.register_named_arg(resolver_arg);

    node_label_option = dynamic_cast<libdnf5::OptionEnum *>(parser.add_init_value(
        std::make_unique<libdnf5::OptionEnum>(
            NODE_LABEL_NAME, std::vector<std::string>{NODE_LABEL_NEVRA, NODE_LABEL_NAME})));
    auto * node_label_arg = parser.add_new_named_arg("node-label");
    node_label_arg->set_long_name("node-label");
    node_label_arg->set_description(
        _("Label used to identify graph nodes (also controls node merging; default: name)"));
    node_label_arg->set_has_value(true);
    node_label_arg->set_arg_value_help("nevra|name");
    node_label_arg->link_value(node_label_option);
    cmd.register_named_arg(node_label_arg);

    edge_label_option = dynamic_cast<libdnf5::OptionEnum *>(parser.add_init_value(
        std::make_unique<libdnf5::OptionEnum>(
            EDGE_LABEL_NONE,
            std::vector<std::string>{EDGE_LABEL_NONE, EDGE_LABEL_DEP, EDGE_LABEL_KIND, EDGE_LABEL_BOTH})));
    auto * edge_label_arg = parser.add_new_named_arg("edge-label");
    edge_label_arg->set_long_name("edge-label");
    edge_label_arg->set_description(
        _("What to render on dot edge labels (default: none; JSON always contains the full dependency list)"));
    edge_label_arg->set_has_value(true);
    edge_label_arg->set_arg_value_help("none|dep|kind|both");
    edge_label_arg->link_value(edge_label_option);
    cmd.register_named_arg(edge_label_arg);

    edge_label_limit_option = dynamic_cast<libdnf5::OptionNumber<std::int32_t> *>(parser.add_init_value(
        std::make_unique<libdnf5::OptionNumber<std::int32_t>>(5, 0, INT32_MAX)));
    auto * edge_label_limit_arg = parser.add_new_named_arg("edge-label-limit");
    edge_label_limit_arg->set_long_name("edge-label-limit");
    edge_label_limit_arg->set_description(
        _("Maximum number of dependency entries to render per dot edge label; further entries are summarized as "
          "\"...and N more\" (default: 5; 0 = unlimited)"));
    edge_label_limit_arg->set_has_value(true);
    edge_label_limit_arg->set_arg_value_help("N");
    edge_label_limit_arg->link_value(edge_label_limit_option);
    cmd.register_named_arg(edge_label_limit_arg);

    edge_style_option = dynamic_cast<libdnf5::OptionEnum *>(parser.add_init_value(
        std::make_unique<libdnf5::OptionEnum>(
            EDGE_STYLE_PLAIN, std::vector<std::string>{EDGE_STYLE_PLAIN, EDGE_STYLE_BY_KIND})));
    auto * edge_style_arg = parser.add_new_named_arg("edge-style");
    edge_style_arg->set_long_name("edge-style");
    edge_style_arg->set_description(
        _("Visual styling of dot edges (default: plain). With \"by-kind\", edges carrying only weak dependencies "
          "(recommends/suggests/supplemented-by/enhanced-by) are dashed and dimmed; strong edges stay solid."));
    edge_style_arg->set_has_value(true);
    edge_style_arg->set_arg_value_help("plain|by-kind");
    edge_style_arg->link_value(edge_style_option);
    cmd.register_named_arg(edge_style_arg);

    format_option = dynamic_cast<libdnf5::OptionEnum *>(parser.add_init_value(
        std::make_unique<libdnf5::OptionEnum>(FORMAT_DOT, std::vector<std::string>{FORMAT_DOT, FORMAT_JSON})));
    auto * format_arg = parser.add_new_named_arg("format");
    format_arg->set_long_name("format");
    format_arg->set_description(_("Output format (default: dot)"));
    format_arg->set_has_value(true);
    format_arg->set_arg_value_help("dot|json");
    format_arg->link_value(format_option);
    cmd.register_named_arg(format_arg);

    output_option = dynamic_cast<libdnf5::OptionPath *>(parser.add_init_value(
        std::make_unique<libdnf5::OptionPath>("")));
    auto * output_arg = parser.add_new_named_arg("output");
    output_arg->set_long_name("output");
    output_arg->set_description(_("Write output to FILE instead of stdout"));
    output_arg->set_has_value(true);
    output_arg->set_arg_value_help("FILE");
    output_arg->link_value(output_option);
    cmd.register_named_arg(output_arg);

    create_json_option(*this);
}


void RepographCommand::configure() {
    auto & ctx = get_context();

    // ---- Mode selection.
    bool use_system = use_system_option->get_value();
    bool closed = closed_option->get_value();
    bool has_specs = !pkg_specs.empty();
    if (closed && use_system) {
        throw libdnf5::cli::CommandExitError(
            1, M_("--closed cannot be combined with --use-system; pick one universe"));
    }
    if (closed && !has_specs) {
        throw libdnf5::cli::CommandExitError(1, M_("--closed requires one or more positional SPECs"));
    }
    if (closed) {
        mode = RepographMode::SPECS_CLOSED;
    } else if (use_system && has_specs) {
        mode = RepographMode::TARGETED_CLOSED;
    } else if (use_system) {
        mode = RepographMode::CLOSED_SET;
    } else if (has_specs) {
        mode = RepographMode::TARGETED;
    } else {
        mode = RepographMode::REPO_WIDE;
    }

    // ---- Output format precedence.
    // Explicit --format wins. Else --json switches to JSON. Else dot.
    bool format_explicit = format_option->get_priority() > libdnf5::Option::Priority::DEFAULT;
    bool json_requested = ctx.get_json_output_requested();
    if (format_explicit) {
        const std::string & fmt = format_option->get_value();
        if (json_requested && fmt == FORMAT_DOT) {
            throw libdnf5::cli::CommandExitError(
                1, M_("Conflicting output options: --format=dot together with --json"));
        }
        format = (fmt == FORMAT_JSON) ? RepographFormat::JSON : RepographFormat::DOT;
    } else if (json_requested) {
        format = RepographFormat::JSON;
    } else {
        format = RepographFormat::DOT;
    }

    // ---- Other policy options.
    resolver = parse_resolver(resolver_option->get_value());
    node_label_policy = parse_node_label(node_label_option->get_value());
    edge_label = parse_edge_label(edge_label_option->get_value());
    edge_label_limit = static_cast<size_t>(edge_label_limit_option->get_value());
    edge_style = parse_edge_style(edge_style_option->get_value());
    include_reverse_weak = include_reverse_weak_option->get_value();
    include_weak_deps = ctx.get_base().get_config().get_install_weak_deps_option().get_value();
    output_path = output_option->get_value();

    // ---- Reject incompatible resolver choices.
    bool resolver_explicit = resolver_option->get_priority() > libdnf5::Option::Priority::DEFAULT;
    bool have_solver_set = (mode == RepographMode::TARGETED || mode == RepographMode::TARGETED_CLOSED);
    if (resolver == repograph::ProviderPolicy::SOLVER && !have_solver_set && resolver_explicit) {
        throw libdnf5::cli::CommandExitError(
            1, M_("--resolver=solver is only meaningful when SPECs are given (targeted modes)"));
    }

    // ---- Repository loading.
    // We always want available repos loaded (for spec resolution and
    // for repo-wide / open-universe walks). When --use-system is set we
    // also load the installed sack.
    ctx.set_load_available_repos(Context::LoadAvailableRepos::ENABLED);
    if (mode == RepographMode::CLOSED_SET || mode == RepographMode::TARGETED_CLOSED) {
        ctx.set_load_system_repo(true);
    } else {
        // CRITICAL: for targeted mode we must NOT load the installed
        // system, otherwise already-installed roots produce empty
        // transactions. (Rubber-duck found this; see plan.md.)
        ctx.set_load_system_repo(false);
    }
    // File-level requires are common; loading filelists is essential for
    // accurate provider lookup (mirrors what repoclosure does).
    ctx.get_base().get_config().get_optional_metadata_types_option().add_item(
        libdnf5::Option::Priority::RUNTIME, libdnf5::METADATA_TYPE_FILELISTS);
}


namespace {


/// Read packages from the appropriate sack into a `PackageSet`, applying
/// `filter_installed` / repo filters as needed.
libdnf5::rpm::PackageSet build_set(libdnf5::Base & base, bool installed_only) {
    libdnf5::rpm::PackageQuery q(base);
    if (installed_only) {
        q.filter_installed();
    } else {
        // PackageQuery default already covers everything; restrict to
        // available so we don't accidentally include the system-rpmdb
        // entries when both sacks happen to be loaded.
        libdnf5::rpm::PackageQuery available(base);
        available.filter_installed();
        q -= available;
    }
    libdnf5::rpm::PackageSet set(base);
    for (const auto & p : q) {
        set.add(p);
    }
    return set;
}


std::string reason_to_string_safe(libdnf5::transaction::TransactionItemReason r) {
    return libdnf5::transaction::transaction_item_reason_to_string(r);
}


}  // namespace


void RepographCommand::run() {
    auto & ctx = get_context();
    auto & base = ctx.get_base();

    repograph::BuilderConfig cfg(base.get_weak_ptr());
    cfg.provider_policy = resolver;
    cfg.node_id_policy = node_label_policy;
    cfg.include_weak_deps = include_weak_deps;
    cfg.include_reverse_weak = include_reverse_weak;
    cfg.closed_universe = (mode == RepographMode::SPECS_CLOSED || mode == RepographMode::CLOSED_SET ||
                           mode == RepographMode::TARGETED_CLOSED);

    switch (mode) {
        case RepographMode::REPO_WIDE: {
            cfg.universe_pkgs = build_set(base, /*installed_only=*/false);
            cfg.node_pkgs = cfg.universe_pkgs;
            cfg.have_solver_set = false;
            if (cfg.node_pkgs.size() > 5000) {
                std::cerr << libdnf5::utils::sformat(
                                 _("repograph: warning: repo-wide graph has {} packages; consider passing one or more "
                                   "SPECs to scope the graph"),
                                 cfg.node_pkgs.size())
                          << std::endl;
            }
            break;
        }
        case RepographMode::CLOSED_SET: {
            cfg.universe_pkgs = build_set(base, /*installed_only=*/true);
            cfg.node_pkgs = cfg.universe_pkgs;
            cfg.have_solver_set = false;
            break;
        }
        case RepographMode::SPECS_CLOSED: {
            // Universe = the resolved spec set itself (no closure expansion,
            // no solver run). Edges are only drawn between SPECs that
            // happen to satisfy one another's deps.
            libdnf5::rpm::PackageQuery available_q(base);
            // Restrict to available (not installed) packages, mirroring
            // build_set(installed_only=false).
            libdnf5::rpm::PackageQuery installed_q(base);
            installed_q.filter_installed();
            available_q -= installed_q;
            libdnf5::rpm::PackageSet resolved(base);
            libdnf5::ResolveSpecSettings settings;
            settings.set_with_nevra(true);
            settings.set_with_provides(true);
            settings.set_with_filenames(true);
            bool any_resolved = false;
            for (const auto & spec : pkg_specs) {
                libdnf5::rpm::PackageQuery candidates(available_q);
                auto nevra_pair = candidates.resolve_pkg_spec(spec, settings, true);
                if (!nevra_pair.first) {
                    std::cerr << libdnf5::utils::sformat(
                                     _("repograph: no match for spec in available repos: {}"), spec)
                              << std::endl;
                    continue;
                }
                for (const auto & p : candidates) {
                    resolved.add(p);
                }
                any_resolved = true;
            }
            if (!any_resolved) {
                throw libdnf5::cli::CommandExitError(1, M_("Failed to resolve any package specifications."));
            }
            cfg.universe_pkgs = resolved;
            cfg.node_pkgs = resolved;
            cfg.have_solver_set = false;
            break;
        }
        case RepographMode::TARGETED: {
            // Universe = available; node set = solver-resolved.
            cfg.universe_pkgs = build_set(base, /*installed_only=*/false);
            libdnf5::Goal goal(base);
            for (const auto & spec : pkg_specs) {
                goal.add_rpm_install(spec);
            }
            auto transaction = goal.resolve();
            if (transaction.get_problems() != libdnf5::GoalProblem::NO_PROBLEM) {
                throw libdnf5::cli::GoalResolveError(transaction);
            }
            libdnf5::rpm::PackageSet resolved(base);
            for (const auto & tspkg : transaction.get_transaction_packages()) {
                if (libdnf5::transaction::transaction_item_action_is_inbound(tspkg.get_action())) {
                    auto pkg = tspkg.get_package();
                    resolved.add(pkg);
                    cfg.reasons[pkg.get_nevra()] = reason_to_string_safe(tspkg.get_reason());
                }
            }
            cfg.node_pkgs = resolved;
            // For SOLVER policy: limit universe to the resolved set so
            // satisfier lookups intersect with the consistent install.
            cfg.universe_pkgs = resolved;
            cfg.have_solver_set = true;
            break;
        }
        case RepographMode::TARGETED_CLOSED: {
            // Universe = installed; resolve roots against installed sack.
            cfg.universe_pkgs = build_set(base, /*installed_only=*/true);
            libdnf5::rpm::PackageQuery installed(base);
            installed.filter_installed();
            libdnf5::rpm::PackageSet roots(base);
            libdnf5::ResolveSpecSettings settings;
            settings.set_with_nevra(true);
            settings.set_with_provides(true);
            settings.set_with_filenames(true);
            for (const auto & spec : pkg_specs) {
                libdnf5::rpm::PackageQuery candidates(installed);
                auto nevra_pair = candidates.resolve_pkg_spec(spec, settings, true);
                if (!nevra_pair.first) {
                    std::cerr << libdnf5::utils::sformat(
                                     _("repograph: no match for spec in installed system: {}"), spec)
                              << std::endl;
                    continue;
                }
                for (const auto & p : candidates) {
                    roots.add(p);
                }
            }
            // Compute transitive closure within the installed set by
            // doing a naive BFS over reldeps (no solver runs, since the
            // universe is fully pinned).
            libdnf5::rpm::PackageSet visited(base);
            std::vector<libdnf5::rpm::Package> frontier;
            for (const auto & p : roots) {
                visited.add(p);
                frontier.push_back(p);
            }
            while (!frontier.empty()) {
                std::vector<libdnf5::rpm::Package> next;
                for (const auto & p : frontier) {
                    auto walk_one = [&](libdnf5::rpm::ReldepList deps) {
                        for (const auto & rd : deps) {
                            libdnf5::rpm::PackageQuery q(base);
                            q.filter_provides(rd);
                            for (const auto & dep_pkg : q) {
                                if (!cfg.universe_pkgs.contains(dep_pkg)) {
                                    continue;
                                }
                                if (visited.contains(dep_pkg)) {
                                    continue;
                                }
                                visited.add(dep_pkg);
                                next.push_back(dep_pkg);
                            }
                        }
                    };
                    walk_one(p.get_regular_requires());
                    walk_one(p.get_requires_pre());
                    if (include_weak_deps) {
                        walk_one(p.get_recommends());
                        walk_one(p.get_suggests());
                    }
                }
                frontier = std::move(next);
            }
            cfg.node_pkgs = visited;
            cfg.have_solver_set = false;
            break;
        }
    }

    auto graph = repograph::build_graph(cfg);

    // Surface unresolved-dep summary on stderr for closed-universe modes.
    // The graph itself is still emitted; this is informational. JSON
    // output also includes per-node "unresolved" arrays.
    if (cfg.closed_universe) {
        size_t total_unresolved = 0;
        size_t nodes_with_unresolved = 0;
        for (const auto & n : graph.nodes) {
            if (!n.unresolved.empty()) {
                ++nodes_with_unresolved;
                total_unresolved += n.unresolved.size();
            }
        }
        if (total_unresolved > 0) {
            std::cerr << libdnf5::utils::sformat(
                             _("repograph: warning: {} unresolved dependencies on {} packages had no satisfier in "
                               "the closed universe (see per-node \"unresolved\" arrays in JSON output)"),
                             total_unresolved,
                             nodes_with_unresolved)
                      << std::endl;
        }
    }

    std::ofstream file_stream;
    std::ostream * out = &std::cout;
    if (!output_path.empty()) {
        file_stream.open(output_path);
        if (!file_stream) {
            throw libdnf5::cli::CommandExitError(
                1, M_("Failed to open output file: {}"), output_path);
        }
        out = &file_stream;
    }

    if (format == RepographFormat::JSON) {
        repograph::emit_json(
            *out,
            graph,
            mode_to_string(mode),
            node_label_option->get_value(),
            resolver_option->get_value());
    } else {
        repograph::emit_dot(*out, graph, edge_label, edge_label_limit, edge_style);
    }
}


}  // namespace dnf5
