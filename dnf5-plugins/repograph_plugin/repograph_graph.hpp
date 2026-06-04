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

#ifndef DNF5_PLUGINS_REPOGRAPH_PLUGIN_REPOGRAPH_GRAPH_HPP
#define DNF5_PLUGINS_REPOGRAPH_PLUGIN_REPOGRAPH_GRAPH_HPP

#include <libdnf5/rpm/package_set.hpp>

#include <ostream>


namespace dnf5::repograph {


void emit_graph(std::ostream & out, const libdnf5::rpm::PackageSet & node_pkgs, bool include_recommends);


}  // namespace dnf5::repograph


#endif  // DNF5_PLUGINS_REPOGRAPH_PLUGIN_REPOGRAPH_GRAPH_HPP
