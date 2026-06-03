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

``dnf5 repograph [options] [<pkg-spec>...]``


Description
===========

The ``repograph`` command emits the package **dependency graph** in
`Graphviz <https://graphviz.org/>`_ dot format or as a structured JSON
document. It is the dnf5 successor to the dnf4 ``repograph`` /
``repo-graph`` plugin, with several significant extensions:

* Multiple modes of operation: walk an entire repo (the historical
  behavior), compute a focused dependency closure from one or more
  *root* package specs, restrict the universe to an explicit set of
  packages, or analyze the dependency graph *within* the set of
  currently installed packages.
* Full per-dependency edge data distinguishing ``Requires``,
  ``Requires(pre)``, ``Recommends``, ``Suggests``, and (with
  ``--include-reverse-weak``) ``Supplements``/``Enhances``.
* Honors the dnf5 ``install_weak_deps`` configuration option (so
  ``--setopt install_weak_deps=False`` is a one-shot way to drop weak
  edges).
* JSON output suitable for downstream tooling.

The plugin acts purely as a **query / reporting** tool: it never
modifies the system.


Modes
=====

The mode is selected automatically from the combination of positional
``<pkg-spec>`` arguments, ``--use-system``, and ``--closed``:

.. list-table::
   :widths: 30 30 40
   :header-rows: 1

   * - Command line
     - Mode
     - Description
   * - ``dnf5 repograph``
     - Repo-wide
     - Walks every package in the selected available repositories. The
       resulting graph can be very large; consider passing a SPEC.
   * - ``dnf5 repograph <spec>...``
     - Targeted (solver-driven)
     - Runs the libdnf5 dependency solver to compute the transitive
       closure of the specs, then graphs the resulting set.
   * - ``dnf5 repograph --closed <spec>...``
     - Specs-closed
     - Universe is exactly the resolved SPEC set. No closure expansion.
       Edges are only drawn between SPECs that satisfy one another's
       deps. Useful for "what are the internal dependencies of this
       set of packages?"
   * - ``dnf5 repograph --use-system``
     - Closed-set
     - Roots and universe are both taken from the currently installed
       packages. Edges are drawn between packages in the installed set.
   * - ``dnf5 repograph --use-system <spec>...``
     - Targeted-closed
     - Roots come from the specs (resolved against the installed sack);
       the universe is the closed installed set. Useful for "what does
       the installed system look like, starting from these packages?"


Options
=======

Defaults are noted inline. All flags also appear in ``dnf5 repograph
--help``.

``--use-system``
    | Use installed packages as the graph universe. With no SPECs,
      the universe also defines the root set (closed-set mode). With
      SPECs, roots come from the SPECs and are resolved against the
      installed sack. Mutually exclusive with ``--closed``.

``--closed``
    | Treat the positional SPECs as the entire universe: no closure
      expansion, no solver. Requires one or more SPECs. Mutually
      exclusive with ``--use-system``. Specs are resolved against the
      available repos.

``--include-reverse-weak``
    | Also draw edges derived from ``Supplements`` and ``Enhances``
      dependencies. These are direction-corrected to read as forward
      edges labeled ``supplemented-by`` and ``enhanced-by``
      respectively. Rich reverse-weak deps (those with multi-package
      boolean conditions) are skipped with a stderr warning; the JSON
      output includes a ``"skipped":true`` marker in that case.

``--resolver=solver|first|best|all|or-node`` (default: ``solver``)
    | How to resolve which provider to draw an edge to when multiple
      packages satisfy a dep.
    | ``solver`` — for targeted mode, prefer satisfiers that the
      solver actually included in the resolved set; in other modes
      silently fall back to ``best``. Explicitly requesting
      ``solver`` outside a targeted mode is an error.
    | ``first`` — pick the first satisfier in stable id order. This
      matches the behavior of the dnf4 Python plugin.
    | ``best`` — pick the best satisfier using repository priority,
      EVR, name, arch, repo id, and solvable id as ordered tiebreaks.
    | ``all`` — emit one edge per satisfier; alternatives are marked
      ``"alt": true`` in JSON.
    | ``or-node`` — synthesize a virtual ``or:<dep>`` node that
      fans out to all satisfiers. Useful for human-readable
      visualization of disjunctive dependencies.

``--node-label=nevra|name`` (default: ``name``)
    | Label used to identify graph nodes. Also controls node
      **merging**.
    | ``nevra`` — full ``name-epoch-version-release.arch``; no
      merging.
    | ``name`` — package name only; multiple package variants
      (different arches or EVRs) collapse into one node. JSON output
      records the underlying NEVRAs in a ``members`` array.

``--edge-label=none|dep|kind|both`` (default: ``none``)
    | What text to render on dot edge labels. Does **not** affect
      JSON output, which always contains the full structured
      dependency list per edge.
    | ``none`` — no edge labels (keeps large graphs readable).
    | ``dep`` — just the dependency string (e.g.
      ``libc.so.6()(64bit)`` or ``python3 >= 3.6``).
    | ``kind`` — just the dependency kind tag.
    | ``both`` — ``kind:dep`` per entry.
    | Multiple entries on the same edge are rendered one per line
      (left-aligned). See ``--edge-label-limit`` to cap long labels.

``--edge-label-limit=N`` (default: ``5``)
    | When more than ``N`` dependency entries would appear on a
      single dot edge, only the first ``N - 1`` are shown and the
      rest are summarized as ``...and X more``. A value of ``0``
      disables the cap and renders every entry. Affects dot output
      only; JSON always carries the full list.

``--edge-style=plain|by-kind`` (default: ``plain``)
    | Visual styling of dot edges. With ``by-kind``, edges carrying
      only weak dependencies (``recommends``/``suggests`` and, with
      ``--include-reverse-weak``, ``supplemented-by``/``enhanced-by``)
      are rendered dashed and dimmed; edges with at least one strong
      dependency (``requires``/``requires-pre``) stay solid. Useful
      for telling weak vs strong relationships apart at a glance when
      labels are off. Affects dot output only.

``--format=dot|json`` (default: ``dot``)
    | Output format. If both ``--format=dot`` and ``--json`` are
      passed, the command errors out.

``--json``
    | Shortcut for ``--format=json``. This is the standard
      machine-readable output flag shared with other dnf5 commands.

``--output=FILE``
    | Write output to ``FILE`` instead of stdout.

``<pkg-spec>``
    | One or more package specs (NEVRA, glob, file path, provides) to
      use as graph roots. Optional in repo-wide and closed-set modes;
      required in targeted, specs-closed, and targeted-closed modes.

Standard dnf5 options are honored, including ``--repo``,
``--enable-repo``, ``--disable-repo``, ``--setopt``,
``--installroot``, and so on. In particular,
``--setopt install_weak_deps=False`` is the recommended way to drop
``Recommends``/``Suggests`` edges from the output.


Examples
========

``dnf5 repograph bash``
    | Compute the dependency closure of ``bash`` and print it as a dot
      graph. Default ``--node-label=name`` collapses arches/versions;
      default ``--edge-label=none`` keeps the graph compact.

``dnf5 repograph --closed bash glibc filesystem``
    | Draw only the internal dependency edges among the three SPECs;
      no further packages are included.

``dnf5 repograph --json bash | jq '.nodes | length'``
    | Count nodes in the closure of ``bash``.

``dnf5 repograph --use-system | dot -Tsvg -o system.svg``
    | Render the entire installed system as an SVG dependency graph.

``dnf5 repograph --use-system bash``
    | Graph the dependency closure of ``bash`` restricted to packages
      that are already installed on this system.

``dnf5 repograph --node-label=nevra --edge-label=both httpd``
    | Compute the closure of ``httpd``, render NEVRA-precise nodes,
      and annotate each edge with both the dep kind and the
      dependency string.

``dnf5 repograph --resolver=or-node bash``
    | Render the closure of ``bash`` with disjunctive dependencies
      shown as virtual "or" nodes (helpful for diagnostic
      visualization).


Rendering Recipes
=================

The plugin emits portable Graphviz dot. Different layout engines and
attributes suit different graph sizes and use cases. The plugin does
not bake any layout decisions into its output; the recipes below are
recommendations only.

**Small targeted graphs** (a handful of packages, e.g. a single SPEC
and its direct deps): the default ``dot`` engine with no tweaks is
usually fine.

::

    dnf5 repograph bash | dot -Tsvg -o bash.svg

**Medium graphs** (50–200 packages, e.g. a container image manifest
under ``--closed``): the default ``dot`` engine still works well, but
benefits from generous rank/node separation and rounded boxes for a
**wide, browseable** layout. Pan/zoom in your viewer:

::

    dnf5 repograph --closed --edge-style=by-kind $(cat nevras.txt) \
      | sed '1a\
            graph [rankdir=TB,ranksep=2.5,nodesep=0.8,concentrate=true];\
            node [fontsize=14,shape=box,style=rounded];' \
      | dot -Tsvg -o image.svg

**Large graphs** (hundreds to thousands of packages, e.g.
repo-wide): hierarchical layouts blow up; switch to a force-directed
engine such as ``sfdp`` or ``neato``:

::

    dnf5 repograph --use-system | sfdp -Tsvg -o system.svg

For very large outputs, prefer ``--format=json`` and post-process with
a graph library (e.g. ``networkx``) rather than rendering directly.

**Telling weak deps apart** without re-introducing labels: combine
``--edge-style=by-kind`` (weak edges dashed/dimmed) with the default
``--edge-label=none``. The structural information is preserved in the
visual style.


JSON Output
===========

* ``dnf5 repograph --json [SPECS]``
* ``dnf5 repograph --format=json [SPECS]``

The command returns a single JSON object with the following top-level
fields:

- ``mode`` (string) — one of ``"repo-wide"``, ``"targeted"``,
  ``"specs-closed"``, ``"closed-set"``, ``"targeted-closed"``.
- ``node_label_policy`` (string) — the active ``--node-label``
  setting.
- ``resolver`` (string) — the active ``--resolver`` setting.
- ``nodes`` (array) — graph nodes (see below). Sorted by ``id``.
- ``edges`` (array) — graph edges (see below). Sorted by
  ``(from, to)``.

Each **node** object contains:

- ``id`` (string) — display identifier per ``node_label_policy``.
- ``name`` (string) — RPM name.
- ``epoch`` (string, optional) — RPM epoch (omitted when empty).
- ``version`` (string)
- ``release`` (string)
- ``arch`` (string)
- ``repo`` (string) — originating repository id.
- ``nevra`` (string) — fully qualified NEVRA of the package this node
  was created from (one of several when merged).
- ``reason`` (string, optional) — only present in ``targeted`` mode;
  one of the libdnf5 transaction-item-reason values (e.g. ``User``,
  ``Dependency``, ``WeakDependency``).
- ``members`` (array of strings, optional) — only present when
  ``node_label_policy`` is ``name`` and more than one underlying
  NEVRA collapsed into this node. Lists those NEVRAs so synthetic
  name-collision self-loops remain diagnosable.

- ``unresolved`` (array, optional) — only present (and only in
  closed-universe modes: ``--closed``, ``--use-system``, and
  ``--use-system <specs>``) when this node has dependencies with no
  satisfier inside the closed universe. Each entry has the same
  ``dep`` and ``kind`` shape as in ``edges[].deps``. The plugin
  also prints a single-line summary to stderr when any unresolved
  deps were detected. (Exit code is unchanged.)

Each **edge** object contains:

- ``from`` (string) — source node id.
- ``to`` (string) — target node id.
- ``deps`` (array) — one entry per dependency that contributes to
  this edge. Each entry has:

  - ``dep`` (string) — the dependency text (e.g.
    ``libc.so.6()(64bit)`` or ``python3 >= 3.6``).
  - ``kind`` (string) — one of ``requires``, ``requires-pre``,
    ``recommends``, ``suggests``, ``supplemented-by``, ``enhanced-by``.
  - ``alt`` (boolean, optional) — ``true`` if this entry was
    selected as an alternative under ``--resolver=all``.
  - ``skipped`` (boolean, optional) — ``true`` if this dependency
    was skipped (currently only used for rich reverse-weak deps
    under ``--include-reverse-weak``). ``reason`` carries an
    explanation.

For an empty result the document is still well-formed; ``nodes`` and
``edges`` are empty arrays.
