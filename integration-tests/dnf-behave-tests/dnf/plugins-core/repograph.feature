Feature: Repograph available packages and solver-selected roots


Scenario: Full graph includes isolated available names and collapses package versions
  Given I use repository "dependency-chain"
    And I use repository "simple-base"
   When I execute dnf with args "repograph"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "bot";
            "dedalo";
            "labirinto";
            "mid";
            "top";
            "vagare";
            "mid" -> "bot";
            "top" -> "mid";
            "vagare" -> "labirinto";
        }}
        """
    And stdout contains "}}\n\Z"
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty


Scenario Outline: Canonical command and compatibility alias resolve a root and its transitive dependencies
  Given I use repository "dependency-chain"
    And I use repository "simple-base"
   When I execute dnf with args "<command> <spec>"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "bot";
            "mid";
            "top";
            "mid" -> "bot";
            "top" -> "mid";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty

  Examples:
        | command    | spec                 |
        | repograph  | top                  |
        | repograph  | top-1-1.fc29.x86_64   |
        | repo-graph | top                  |


Scenario Outline: Package specs complete available names and versions for both command names
  Given I use repository "repograph-versions"
    And I successfully execute dnf with args "makecache"
    # Completion uses cached metadata and requires --complete as the first argument.
   When I execute "{context.dnf.dnf_command} --complete=6 dnf5 --installroot={context.dnf.installroot} --releasever=29 --setopt=cachedir={context.dnf.installroot}/var/cache/dnf --no-plugins <command> repograph-ver"
   Then the exit code is 0
    And stdout is
        """
        repograph-versioned
        repograph-versioned-0:1.0-1.noarch
        repograph-versioned-0:1.0-1.src
        repograph-versioned-0:2.0-1.noarch
        repograph-versioned-0:2.0-1.src
        """
    And stderr is empty

  Examples:
        | command    |
        | repograph  |
        | repo-graph |


Scenario Outline: Scoped graph includes only the dependency branch of the selected version
  Given I use repository "repograph-versions"
   When I execute dnf with args "repograph <spec>"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "repograph-<branch>-only";
            "repograph-versioned";
            "repograph-versioned" -> "repograph-<branch>-only";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty

  Examples:
        | spec                              | branch |
        | repograph-versioned               | new    |
        | repograph-versioned-1.0-1.noarch   | old    |


Scenario Outline: Installation policy can replace the requested root with its obsoleter
  Given I use repository "dnf-ci-obsoletes" with configuration
        | key         | value                       |
        | includepkgs | PackageB PackageB-Obsoleter |
    And I configure dnf with
        | key       | value       |
        | obsoletes | <obsoletes> |
    # This is an installation selection, not a literal graph of the requested package.
    # With obsoletes enabled, even the requested name can disappear from the graph.
   When I execute dnf with args "repograph PackageB"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "<selected>";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty

  Examples:
        | obsoletes | selected           |
        | True      | PackageB-Obsoleter |
        | False     | PackageB           |


Scenario: Best policy rejects a broken newest root until fallback to an older version is allowed
  Given I use repository "repograph-versions" with configuration
        | key         | value              |
        | excludepkgs | repograph-new-only |
    And I configure dnf with
        | key  | value |
        | best | True  |
    # The older root is installable, but an installation goal with best=True
    # refuses to fall back just to produce a graph.
   When I execute dnf with args "repograph repograph-versioned"
   Then the exit code is 1
    And stdout is empty
    And stderr contains "requires repograph-new-only"
    And stderr contains "cannot install the best candidate for the job"
    And RPMDB Transaction is empty
   When I execute dnf with args "repograph --setopt=best=False repograph-versioned"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "repograph-old-only";
            "repograph-versioned";
            "repograph-versioned" -> "repograph-old-only";
        }}
        """
    And RPMDB Transaction is empty


Scenario: Full graph merges dependency branches from all versions under one package name
  Given I use repository "repograph-versions"
   When I execute dnf with args "repograph"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "repograph-new-only";
            "repograph-old-only";
            "repograph-versioned";
            "repograph-versioned" -> "repograph-new-only";
            "repograph-versioned" -> "repograph-old-only";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty


Scenario: Full graph ignores version locks but scoped selection respects them
  Given I use repository "repograph-versions"
    And I create file "/etc/dnf/versionlock.toml" with
        """
        version = "1.0"
        [[packages]]
        name = "repograph-versioned"
        [[packages.conditions]]
        key = "evr"
        comparator = "="
        value = "0:1.0-1"
        """
   When I execute dnf with args "repograph"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "repograph-new-only";
            "repograph-old-only";
            "repograph-versioned";
            "repograph-versioned" -> "repograph-new-only";
            "repograph-versioned" -> "repograph-old-only";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty
   When I execute dnf with args "repograph repograph-versioned"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "repograph-old-only";
            "repograph-versioned";
            "repograph-versioned" -> "repograph-old-only";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty


Scenario Outline: Glob and multiple named roots include only the selected closures
  Given I use repository "dependency-chain"
    And I use repository "simple-base"
   When I execute dnf with args "repograph <specs>"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "dedalo";
            "labirinto";
            "vagare";
            "vagare" -> "labirinto";
        }}
        """
    And RPMDB Transaction is empty

  Examples:
        | specs                  |
        | '[dv]*'                |
        | vagare dedalo          |
        | dedalo vagare dedalo   |


Scenario Outline: File and binary root specs resolve their owner and its dependency closure
  Given I use repository "repograph"
    And I use repository "dependency-chain"
    And I use repository "simple-base"
    And I configure dnf with
        | key               | value |
        | install_weak_deps | False |
    And I set config option "optional_metadata_types" to ","
   When I execute dnf with args "repograph <spec>"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "bot";
            "mid";
            "repograph-root";
            "top";
            "mid" -> "bot";
            "repograph-root" -> "top";
            "top" -> "mid";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty

  Examples:
        | spec                         |
        | /usr/share/repograph/root.txt |
        | repograph-tool               |


Scenario: A virtual spec selects one provider and its dependency branch rather than every match
  Given I use repository "repograph"
    And I use repository "dependency-chain"
    And I use repository "simple-base"
    And I configure dnf with
        | key               | value |
        | install_weak_deps | False |
   When I execute dnf with args "repograph repograph-choice"
   Then the exit code is 0
    # Either provider is valid, but its complete branch must appear without the other provider.
    And stdout contains "\Adigraph packages {{\n    node \[shape=box, style=rounded, fontname="Helvetica"\];\n(    "bot";\n    "mid";\n    "repograph-root";\n    "top";\n    "mid" -> "bot";\n    "repograph-root" -> "top";\n    "top" -> "mid";\n|    "labirinto";\n    "repograph-enhancer";\n    "repograph-enhancer" -> "labirinto";\n)}}\n\Z"
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty


Scenario Outline: Scoped edges choose the first provider name even when both alternatives are selected
  Given I use repository "repograph"
    And I use repository "dependency-chain"
    And I use repository "simple-base"
    And I configure dnf with
        | key               | value |
        | install_weak_deps | False |
   When I execute dnf with args "repograph <specs>"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "bot";
            "labirinto";
            "mid";
            "repograph-consumer";
            "repograph-enhancer";
            "repograph-root";
            "top";
            "mid" -> "bot";
            "repograph-consumer" -> "repograph-enhancer";
            "repograph-enhancer" -> "labirinto";
            "repograph-root" -> "top";
            "top" -> "mid";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty

  Examples:
        | specs                                                 |
        | repograph-consumer repograph-root repograph-enhancer   |
        | repograph-enhancer repograph-root repograph-consumer   |


Scenario Outline: Installed dependencies do not change full or scoped graphs
  Given I use repository "dependency-chain"
    And I use repository "simple-base"
    And I successfully execute dnf with args "install --nogpgcheck bot mid dedalo"
    And I drop repository "simple-base"
   When I execute dnf with args "repograph <specs>"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "bot";
            "mid";
            "top";
            "mid" -> "bot";
            "top" -> "mid";
        }}
        """
    And RPMDB Transaction is empty

  Examples:
        | specs |
        |       |
        | top   |


Scenario: An installed-only package cannot satisfy an explicit root
  Given I use repository "simple-base"
    And I successfully execute dnf with args "install --nogpgcheck dedalo"
    And I drop repository "simple-base"
    And I use repository "dependency-chain"
   When I execute dnf with args "repograph dedalo"
   Then the exit code is 1
    And stdout is empty
    And stderr contains "No match for argument: dedalo"
    And RPMDB Transaction is empty


Scenario: An installed-only transitive dependency cannot satisfy an available root
  Given I use repository "dependency-chain"
    And I successfully execute dnf with args "install --nogpgcheck bot"
    And I configure repository "dependency-chain" with
        | key         | value   |
        | includepkgs | top mid |
   When I execute dnf with args "repograph top"
   Then the exit code is 1
    And stdout is empty
    And stderr contains "requires bot, but none of the providers can be installed"
    And stderr contains "package bot-1-1\.fc29\.x86_64 from dependency-chain is filtered out by exclude filtering"
    And RPMDB Transaction is empty


Scenario Outline: The inherited repo option exclusively selects an enabled or disabled repository
  Given I use repository "dependency-chain" with configuration
        | key     | value     |
        | enabled | <enabled> |
    And I use repository "simple-base"
   When I execute dnf with args "repograph --repo=dependency-chain"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "bot";
            "mid";
            "top";
            "mid" -> "bot";
            "top" -> "mid";
        }}
        """

  Examples:
        | enabled |
        | 1       |
        | 0       |


Scenario: An enabled empty repository produces a valid empty graph
  Given I use repository "repograph-empty"
   When I execute dnf with args "repograph"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty


Scenario: A failure to flush graph output is reported without changing installed packages
  Given I use repository "dependency-chain"
    And I successfully execute dnf with args "install --nogpgcheck bot"
   When I execute dnf with args "repograph top > /dev/full"
   Then the exit code is 1
    And stdout is empty
    And stderr contains "Error writing repograph output to stdout\."
    And RPMDB Transaction is empty


Scenario: No enabled repositories leave stdout empty
   When I execute dnf with args "repograph"
   Then the exit code is 1
    And stdout is empty
    And stderr contains "No repositories are enabled\."


Scenario Outline: Default resolution failures leave stdout empty
  Given I use repository "dnf-ci-install-remove"
   When I execute dnf with args "repograph <specs>"
   Then the exit code is 1
    And stdout is empty
    And stderr contains "<diagnostic>"
    And RPMDB Transaction is empty

  Examples:
        | specs                           | diagnostic              |
        | tea repograph-missing           | No match for argument   |
        | mate                            | nothing provides water  |
        | coffee chockolate               | cannot install both     |


Scenario: A broken package remains an available node when no roots are specified
  Given I use repository "dnf-ci-install-remove" with configuration
        | key         | value      |
        | includepkgs | mate water |
   When I execute dnf with args "repograph"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "mate";
            "water";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And RPMDB Transaction is empty


Scenario: Configured skip_unavailable keeps valid roots and reports missing roots on stderr
  Given I use repository "dnf-ci-install-remove"
    And I configure dnf with
        | key              | value |
        | skip_unavailable | True  |
   When I execute dnf with args "repograph tea repograph-missing"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "tea";
            "water";
            "tea" -> "water";
        }}
        """
    And stderr contains "No match for argument: repograph-missing"
    And RPMDB Transaction is empty


Scenario: Configured skip_unavailable permits an empty graph
  Given I use repository "dnf-ci-install-remove"
    And I configure dnf with
        | key              | value |
        | skip_unavailable | True  |
   When I execute dnf with args "repograph repograph-missing"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
        }}
        """
    And stderr contains "No match for argument: repograph-missing"
    And RPMDB Transaction is empty


Scenario: Configured skip_broken keeps a resolvable subset
  Given I use repository "dnf-ci-install-remove"
    And I configure dnf with
        | key         | value |
        | skip_broken | True  |
   When I execute dnf with args "repograph tea mate"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "tea";
            "water";
            "tea" -> "water";
        }}
        """
    And stderr contains "nothing provides water >= 2"
    And RPMDB Transaction is empty


Scenario: Configured skip_broken permits an empty graph and reports available diagnostics
  Given I use repository "dnf-ci-install-remove"
    And I configure dnf with
        | key         | value |
        | skip_broken | True  |
   When I execute dnf with args "repograph mate"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
        }}
        """
    And stderr contains "nothing provides water >= 2"
    And RPMDB Transaction is empty


Scenario: Available diagnostics stay on stderr when unavailable and broken roots are skipped
  Given I use repository "dnf-ci-install-remove"
    And I configure dnf with
        | key              | value |
        | skip_broken      | True  |
        | skip_unavailable | True  |
   When I execute dnf with args "repograph tea mate repograph-missing"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "tea";
            "water";
            "tea" -> "water";
        }}
        """
    And stderr contains "No match for argument: repograph-missing"
    And RPMDB Transaction is empty


Scenario Outline: File-owned dependencies are available after a primary-only cache was created
  Given I use repository "fileprovides"
    And I set config option "optional_metadata_types" to ","
    # Like fileprovides.feature, this initializes the provides cache without filelists.
    And I successfully execute dnf with args "repoquery --whatprovides htop"
    And file "/var/cache/dnf/fileprovides-*/solv/fileprovides.solv" exists
    And file "/var/cache/dnf/fileprovides-*/repodata/*filelists*" does not exist
   When I execute dnf with args "repograph <specs>"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "agua";
            "planta";
            "planta" -> "agua";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
    And file "/var/cache/dnf/fileprovides-*/repodata/*filelists*" exists
    And RPMDB Transaction is empty
  Given I successfully execute dnf with args "install --nogpgcheck agua planta"
   When I execute dnf with args "repograph <specs>"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "agua";
            "planta";
            "planta" -> "agua";
        }}
        """
    And RPMDB Transaction is empty

  Examples:
        | specs  |
        |        |
        | planta |
