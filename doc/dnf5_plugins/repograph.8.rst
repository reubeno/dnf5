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

``repo-graph`` is a compatibility alias for ``repograph``.


Description
===========

The ``repograph`` command outputs a package dependency graph in dot
format to standard output.

With no package specs, the graph contains every available package from
the enabled repositories, subject to normal package exclusions. Version
locks are ignored, as with other non-transactional queries. Enabled
repositories containing no available packages produce an empty graph.

When one or more ``<package-spec-NPFB>`` arguments are specified, the
dependency solver selects a hypothetical installation from available
packages, without using installed packages. The graph contains that
selection, including dependencies, not every matching version or
provider. Version locks apply to this selection. No packages are installed.

Using an installation goal has consequences that may be undesirable when
inspecting a particular package. With ``obsoletes`` enabled, a bare-name
request can select an obsoleting replacement instead, so the requested
name may be absent from the graph. Set ``--setopt=obsoletes=false`` to
disable that replacement policy. With ``best=true``, an uninstallable
newest version can cause resolution to fail even when an older version
is installable. Set ``--setopt=best=false`` to allow selection of an older
version and its dependency branch. These policies affect scoped
selection, not the no-spec repository graph.

The ``skip_unavailable`` and ``skip_broken`` configuration options are
honored and default to false. When enabled, requests may be skipped and
the graph may be empty. Available skip diagnostics are printed to
standard error. Unresolved solver errors produce no graph.

Nodes are package names (multiple arches or versions of the same name
collapse to one node). Dependencies satisfied by the requiring package
itself are omitted, even when other providers are available. This is
checked per package before merging names. For each remaining ``Requires``,
an edge points to the lexicographically first provider package name in
the graph other than the source name. Dependencies with no such provider
are omitted. These representative edges are selected independently of
the solver's installation reasons.

The graph includes ``Requires`` edges and, by default, ``Recommends``
edges. Use ``--setopt=install_weak_deps=false`` to omit ``Recommends`` and
prevent weak dependencies, including supplementing packages, from being
selected. ``Suggests``, ``Supplements``, and ``Enhances`` edges are never
emitted. In no-spec mode, this option does not change which nodes are
included.

A known limitation is that edges for RPM rich dependencies are
approximate and may omit required relationships or include inactive
conditional branches.


Examples
========

``dnf5 repograph > all.dot``
    | Emit the full available-repo graph and save it to a file.

``dnf5 repograph bash > bash.dot``
    | Emit the dependency graph for a hypothetical installation of ``bash`` and save it.

``dnf5 repograph httpd --setopt=install_weak_deps=false > httpd.dot``
    | Emit the dependency graph for a hypothetical installation of ``httpd`` without weak dependencies.

``dnf5 --repo=rawhide repograph > rawhide.dot``
    | Emit the available-package graph using only the rawhide repository.


See Also
========

* :manpage:`dnf5(8)`, DNF5 Command Reference
* :manpage:`dnf5-specs(7)`, Patterns specification
