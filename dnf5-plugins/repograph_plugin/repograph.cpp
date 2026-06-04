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

#include "repograph_graph.hpp"

#include <libdnf5-cli/argument_parser.hpp>
#include <libdnf5-cli/exception.hpp>
#include <libdnf5/base/goal.hpp>
#include <libdnf5/conf/const.hpp>
#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/transaction/transaction_item_reason.hpp>
#include <libdnf5/utils/bgettext/bgettext-lib.h>
#include <libdnf5/utils/bgettext/bgettext-mark-domain.h>

#include <iostream>


namespace dnf5 {

void RepographCommand::set_parent_command() {
    auto * arg_parser_parent_cmd = get_session().get_argument_parser().get_root_command();
    auto * arg_parser_this_cmd = get_argument_parser_command();
    arg_parser_parent_cmd->register_command(arg_parser_this_cmd);
}


void RepographCommand::set_argument_parser() {
    auto & parser = get_context().get_argument_parser();
    auto & cmd = *get_argument_parser_command();

    cmd.set_description(_("Output a package dependency graph in dot format"));
    cmd.set_long_description(
        _("With no arguments, outputs a dependency graph for all available packages.\n"
          "\n"
          "When package specs are provided, outputs a dependency graph for the "
          "matching packages and their dependencies. Recommended packages are included "
          "when the `install_weak_deps` option is enabled."));

    auto * specs = parser.add_new_positional_arg(
        "package-spec-NPFB", libdnf5::cli::ArgumentParser::PositionalArg::UNLIMITED, nullptr, nullptr);
    specs->set_description(_("List of package specifications to graph"));
    specs->set_parse_hook_func(
        [this](
            [[maybe_unused]] libdnf5::cli::ArgumentParser::PositionalArg * arg, int argc, const char * const argv[]) {
            for (int i = 0; i < argc; ++i) {
                pkg_specs.emplace_back(argv[i]);
            }
            return true;
        });
    cmd.register_positional_arg(specs);
}


void RepographCommand::configure() {
    auto & ctx = get_context();
    // Available repositories are needed for both the complete graph and
    // package-spec resolution. Repograph does not query the installed system.
    ctx.set_load_available_repos(Context::LoadAvailableRepos::ENABLED);
    ctx.set_load_system_repo(false);
    // Filelists are essential: many Requires reference file paths
    // (e.g. `Requires: /usr/bin/sh`) and without filelists those edges
    // silently drop out of the graph. Matches what repoclosure does
    // for the same reason.
    ctx.get_base().get_config().get_optional_metadata_types_option().add_item(
        libdnf5::Option::Priority::RUNTIME, libdnf5::METADATA_TYPE_FILELISTS);
}


void RepographCommand::run() {
    auto & base = get_context().get_base();
    const bool include_recommends = base.get_config().get_install_weak_deps_option().get_value();

    libdnf5::rpm::PackageSet node_pkgs(base);
    if (pkg_specs.empty()) {
        libdnf5::rpm::PackageQuery q(base);
        q.filter_available();
        node_pkgs = q;
        if (node_pkgs.empty()) {
            throw libdnf5::cli::CommandExitError(
                1,
                M_("No available packages were found. Check that at least one enabled repository contains packages."));
        }
    } else {
        libdnf5::Goal goal(base);
        for (const auto & spec : pkg_specs) {
            goal.add_rpm_install(spec);
        }
        auto transaction = goal.resolve();
        if (transaction.get_problems() != libdnf5::GoalProblem::NO_PROBLEM) {
            throw libdnf5::cli::GoalResolveError(transaction);
        }
        for (const auto & tspkg : transaction.get_transaction_packages()) {
            // The graph contains the packages selected for the resolved dependency closure,
            // not packages removed or replaced by the hypothetical transaction.
            if (libdnf5::transaction::transaction_item_action_is_inbound(tspkg.get_action())) {
                node_pkgs.add(tspkg.get_package());
            }
        }
    }

    repograph::emit_graph(std::cout, node_pkgs, include_recommends);

    // Report late write failures before returning success.
    if (!std::cout.flush()) {
        throw libdnf5::cli::CommandExitError(1, M_("Error writing repograph output to stdout."));
    }
}


}  // namespace dnf5
