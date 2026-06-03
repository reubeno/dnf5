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


#ifndef DNF5_PLUGINS_REPOGRAPH_PLUGIN_REPOGRAPH_HPP
#define DNF5_PLUGINS_REPOGRAPH_PLUGIN_REPOGRAPH_HPP


#include "graph_builder.hpp"

#include <dnf5/context.hpp>
#include <dnf5/shared_options.hpp>
#include <libdnf5-cli/session.hpp>
#include <libdnf5/conf/option_bool.hpp>
#include <libdnf5/conf/option_enum.hpp>
#include <libdnf5/conf/option_number.hpp>
#include <libdnf5/conf/option_path.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace dnf5 {


/// What the user is asking the plugin to do.
enum class RepographMode {
    /// Walk every package in the selected available repos.
    REPO_WIDE,
    /// Roots from positional specs, universe from available repos (open
    /// closure via the solver).
    TARGETED,
    /// Roots from positional specs, universe is exactly the resolved
    /// spec set (no closure expansion). Selected via `--closed`.
    SPECS_CLOSED,
    /// Roots and universe from the installed system (closed).
    CLOSED_SET,
    /// Roots from positional specs, universe from the installed system
    /// (closed).
    TARGETED_CLOSED,
};


/// Output formats supported by repograph.
enum class RepographFormat {
    DOT,
    JSON,
};


class RepographCommand : public Command {
public:
    explicit RepographCommand(Context & context) : Command(context, "repograph") {}

    void set_parent_command() override;
    void set_argument_parser() override;
    void configure() override;
    void run() override;

private:
    // ---- Resolved/derived state computed in configure() and consumed by run().
    RepographMode mode{RepographMode::REPO_WIDE};
    RepographFormat format{RepographFormat::DOT};
    repograph::ProviderPolicy resolver{repograph::ProviderPolicy::SOLVER};
    repograph::NodeIdPolicy node_label_policy{repograph::NodeIdPolicy::NAME};
    repograph::EdgeAnnotations edge_label{repograph::EdgeAnnotations::NONE};
    size_t edge_label_limit{5};
    bool include_reverse_weak{false};
    bool include_weak_deps{true};
    std::string output_path;

    // ---- Raw CLI input.
    std::vector<std::string> pkg_specs;
    libdnf5::OptionBool * use_system_option{nullptr};
    libdnf5::OptionBool * closed_option{nullptr};
    libdnf5::OptionBool * include_reverse_weak_option{nullptr};
    libdnf5::OptionEnum * resolver_option{nullptr};
    libdnf5::OptionEnum * node_label_option{nullptr};
    libdnf5::OptionEnum * edge_label_option{nullptr};
    libdnf5::OptionNumber<std::int32_t> * edge_label_limit_option{nullptr};
    libdnf5::OptionEnum * format_option{nullptr};
    libdnf5::OptionPath * output_option{nullptr};
};


}  // namespace dnf5


#endif  // DNF5_PLUGINS_REPOGRAPH_PLUGIN_REPOGRAPH_HPP
