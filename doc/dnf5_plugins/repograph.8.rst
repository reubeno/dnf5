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

* Three modes of operation: walk an entire repo (the historical
  behavior), compute a focused dependency closure from one or more
  *root* package specs, or analyze the dependency graph *within* the
  set of currently installed packages.
* Full per-reldep edge annotations distinguishing regular requires,
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
``<pkg-spec>`` arguments and the ``--use-system`` option:

.. list-table::
   :widths: 25 40 35
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
   * - ``dnf5 repograph --use-system``
     - Closed-set
     - Roots and universe are both taken from the currently installed
       packages. Edges are drawn between packages in the installed set.
   * - ``dnf5 repograph --use-system <spec>...``
     - Targeted-in-closed
     - Roots come from the specs (resolved against the installed sack);
       the universe is the closed installed set. Useful for asking
       "what does the installed system look like, starting from these
       packages?"


Options
=======

``--use-system``
    | Use installed packages as the graph universe. With no SPECs,
      the universe also defines the root set (closed-set mode). With
      SPECs, roots come from the SPECs and are resolved against the
      installed sack.

``--include-reverse-weak``
    | Also draw edges derived from ``Supplements`` and ``Enhances``
      reldeps. These are direction-corrected to read as forward edges
      labeled ``supplemented-by`` and ``enhanced-by`` respectively.
      Rich reverse-weak deps (those with multi-package boolean
      conditions) are skipped with a stderr warning; the JSON output
      includes a ``"skipped":true`` marker in that case.

``--provider-policy=solver|first|best|all|or-node``
    | How to pick a target when a single reldep has multiple
      satisfiers.
    | ``solver`` *(default)* — for targeted mode, prefer satisfiers
      that the solver actually included in the resolved set; in other
      modes silently fall back to ``best``. Explicitly requesting
      ``solver`` outside a targeted mode is an error.
    | ``first`` — pick the first satisfier in stable id order. This
      matches the behavior of the dnf4 Python plugin.
    | ``best`` — pick the best satisfier using repository priority,
      EVR, name, arch, repo id, and solvable id as ordered tiebreaks.
    | ``all`` — emit one edge per satisfier; alternatives are marked
      ``"alt": true`` in JSON.
    | ``or-node`` — synthesize a virtual ``or:<reldep>`` node that
      fans out to all satisfiers. Useful for human-readable
      visualization of disjunctive dependencies.

``--node-id=nevra|name|name.arch``
    | Display identifier for graph nodes. Also controls node *merging*:
      with ``name`` or ``name.arch``, multiple package variants
      collapse into one node and their NEVRAs appear in the
      ``members`` array in JSON output.
    | ``nevra`` *(default)* — full ``name-epoch-version-release.arch``;
      no merging.
    | ``name`` — package name only; collapses across arches and EVRs.
    | ``name.arch`` — name and arch; collapses across EVRs.

``--edge-annotations=both|reldep|kind|none``
    | What text to render on dot edge labels. Does **not** affect
      JSON output, which always contains the full structured reldep
      list per edge.
    | ``both`` *(default)* — ``kind:reldep`` per entry, comma-joined.
    | ``reldep`` — just the reldep string.
    | ``kind`` — just the dependency kind tag.
    | ``none`` — no edge labels.

``--format=dot|json``
    | Output format. ``dot`` is the default.
    | If both ``--format=dot`` and ``--json`` are passed, the command
      errors out.

``--json``
    | Shortcut for ``--format=json``. This is the standard
      machine-readable output flag shared with other dnf5 commands.

``--output=FILE``
    | Write output to ``FILE`` instead of stdout.

``<pkg-spec>``
    | One or more package specs (NEVRA, glob, file path, provides) to
      use as graph roots. Optional.

Standard dnf5 options are honored, including ``--repo``,
``--enable-repo``, ``--disable-repo``, ``--setopt``,
``--installroot``, and so on. In particular,
``--setopt install_weak_deps=False`` is the recommended way to drop
``Recommends``/``Suggests`` edges from the output.


Examples
========

``dnf5 repograph bash``
    | Compute the dependency closure of ``bash`` and print it as a dot
      graph. Each node corresponds to one NEVRA in the resolved
      transaction; each edge is annotated with the reldep(s) that
      caused it and their kind.

``dnf5 repograph --json bash | jq '.nodes | length'``
    | Count nodes in the closure of ``bash``.

``dnf5 repograph --use-system | dot -Tsvg -o system.svg``
    | Render the entire installed system as an SVG dependency graph.

``dnf5 repograph --use-system bash``
    | Graph the dependency closure of ``bash`` restricted to packages
      that are already installed on this system.

``dnf5 repograph --node-id=name --setopt install_weak_deps=False httpd``
    | Compute the strong-only closure of ``httpd`` and collapse
      multiple arches/versions of each package into a single node.

``dnf5 repograph --provider-policy=or-node bash``
    | Render the closure of ``bash`` with disjunctive dependencies
      shown as virtual "or" nodes (helpful for diagnostic
      visualization).


JSON Output
===========

* ``dnf5 repograph --json [SPECS]``
* ``dnf5 repograph --format=json [SPECS]``

The command returns a single JSON object with the following top-level
fields:

- ``mode`` (string) — one of ``"repo-wide"``, ``"targeted"``,
  ``"closed-set"``, ``"targeted-closed"``.
- ``node_id_policy`` (string) — the active ``--node-id`` setting.
- ``provider_policy`` (string) — the active ``--provider-policy``
  setting.
- ``nodes`` (array) — graph nodes (see below). Sorted by ``id``.
- ``edges`` (array) — graph edges (see below). Sorted by
  ``(from, to)``.

Each **node** object contains:

- ``id`` (string) — display identifier per ``node_id_policy``.
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
  ``node_id_policy`` is ``name`` or ``name.arch`` and more than one
  underlying NEVRA collapsed into this node. Lists those NEVRAs so
  synthetic name-collision self-loops remain diagnosable.

Each **edge** object contains:

- ``from`` (string) — source node id.
- ``to`` (string) — target node id.
- ``reldeps`` (array) — one entry per reldep that contributes to this
  edge. Each entry has:

  - ``reldep`` (string) — the reldep text (e.g.
    ``libc.so.6()(64bit)`` or ``python3 >= 3.6``).
  - ``kind`` (string) — one of ``regular``, ``requires-pre``,
    ``recommends``, ``suggests``, ``supplemented-by``, ``enhanced-by``.
  - ``alt`` (boolean, optional) — ``true`` if this entry was
    selected as an alternative under ``--provider-policy=all``.
  - ``skipped`` (boolean, optional) — ``true`` if this reldep was
    skipped (currently only used for rich reverse-weak deps under
    ``--include-reverse-weak``). ``reason`` carries an explanation.

For an empty result the document is still well-formed; ``nodes`` and
``edges`` are empty arrays.
