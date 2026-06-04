..
    Copyright Contributors to the DNF5 project.
    Copyright Contributors to the libdnf project.
    SPDX-License-Identifier: GPL-2.0-or-later

    This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

    Libdnf is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Libdnf is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdnf.  If not, see <https://www.gnu.org/licenses/>.

.. _repograph_plugin_ref-label:

##################
 Repograph Command
##################

Synopsis
========

``dnf5 repograph [<package-spec-NPFB>...]``


Description
===========

The ``repograph`` command outputs a package dependency graph in dot
format to standard output.

With no package specs, the graph contains every available package from
the enabled repositories.

When one or more ``<package-spec-NPFB>`` arguments are specified, the
graph contains the matching packages and their transitive dependencies.
The package specs and their dependencies are resolved against all
available packages. If the requested dependency closure cannot be
resolved, the command reports the solver problems and does not output a
partial graph.

Nodes are package names (multiple arches or versions of the same name
collapse to one node). Edges point from a package to one provider of
each of its runtime ``Requires``. For a virtual dependency, the edge
points to the lexicographically first provider package name in the
graph. This makes provider selection deterministic when multiple
packages provide the same dependency.

The graph includes ``Requires`` and, when the ``install_weak_deps``
configuration option is enabled (the default), ``Recommends``. It does
not include ``Suggests`` or reverse dependencies (``Supplements`` and
``Enhances``). Pass ``--setopt=install_weak_deps=false`` to exclude
``Recommends``.


Examples
========

``dnf5 repograph > all.dot``
    | Emit the full available-repo graph and save it to a file.

``dnf5 repograph bash > bash.dot``
    | Emit just the closure of ``bash`` and save it.

``dnf5 repograph httpd --setopt=install_weak_deps=false > httpd.dot``
    | Emit the closure of ``httpd`` without weak dependencies.


See Also
========

* :manpage:`dnf5(8)`, DNF5 Command Reference
* :manpage:`dnf5-specs(7)`, Patterns specification
