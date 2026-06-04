Feature: Repograph strong and weak dependency selection


Background:
  Given I use repository "repograph"
    And I use repository "dependency-chain"
    And I use repository "simple-base"


Scenario: Full graph draws Requires and Recommends but no other weak dependency edges
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
            "repograph-consumer";
            "repograph-enhancer";
            "repograph-root";
            "repograph-supplement";
            "top";
            "vagare";
            "mid" -> "bot";
            "repograph-consumer" -> "repograph-enhancer";
            "repograph-enhancer" -> "labirinto";
            "repograph-root" -> "dedalo";
            "repograph-root" -> "top";
            "repograph-supplement" -> "labirinto";
            "top" -> "mid";
            "vagare" -> "labirinto";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """


Scenario: Scoped graph includes solver-selected Supplements and their dependencies
   When I execute dnf with args "repograph repograph-root"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "bot";
            "dedalo";
            "labirinto";
            "mid";
            "repograph-root";
            "repograph-supplement";
            "top";
            "mid" -> "bot";
            "repograph-root" -> "dedalo";
            "repograph-root" -> "top";
            "repograph-supplement" -> "labirinto";
            "top" -> "mid";
        }}
        """
    And RPMDB Transaction is empty


Scenario: Disabling weak dependencies removes scoped Recommends and Supplements packages
  Given I configure dnf with
        | key               | value |
        | install_weak_deps | False |
   When I execute dnf with args "repograph repograph-root"
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


Scenario: Disabling weak dependencies in a full graph removes only Recommends edges
  Given I configure dnf with
        | key               | value |
        | install_weak_deps | False |
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
            "repograph-consumer";
            "repograph-enhancer";
            "repograph-root";
            "repograph-supplement";
            "top";
            "vagare";
            "mid" -> "bot";
            "repograph-consumer" -> "repograph-enhancer";
            "repograph-enhancer" -> "labirinto";
            "repograph-root" -> "top";
            "repograph-supplement" -> "labirinto";
            "top" -> "mid";
            "vagare" -> "labirinto";
        }}
        """
