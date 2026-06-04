Feature: Repograph warns once about approximate rich dependency edges


Scenario: Enabled rich Recommends warn and select the active branch
  Given I use repository "dnf-ci-rich"
   When I execute dnf with args "repograph breakfast"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "breakfast";
            "water";
            "breakfast" -> "water";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        Warning: Rich dependencies are approximated in this graph; edges may omit required relationships or include inactive branches.
        """


Scenario Outline: Multiple and repeated rich dependencies produce one warning and the expected nodes
  Given I use repository "repograph-rich"
    And I use repository "dnf-ci-rich" with configuration
        | key         | value                  |
        | includepkgs | cream dill flour water |
   When I execute dnf with args "repograph <specs>"
   Then the exit code is 0
    # The approximation picks one provider per expression, not every required operand.
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "cream";
            "dill";
            "flour";
            "repograph-rich";
            "repograph-rich-duplicate";
            "water";
            "repograph-rich" -> "cream";
            "repograph-rich" -> "flour";
            "repograph-rich-duplicate" -> "cream";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        Warning: Rich dependencies are approximated in this graph; edges may omit required relationships or include inactive branches.
        """
    And RPMDB Transaction is empty

  Examples:
        | specs                                        |
        |                                              |
        | repograph-rich repograph-rich-duplicate dill  |


Scenario: Rich Requires still warn when weak dependencies are disabled
  Given I use repository "repograph-rich"
    And I use repository "dnf-ci-rich"
    And I configure dnf with
        | key               | value |
        | install_weak_deps | False |
   When I execute dnf with args "repograph repograph-rich"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "cream";
            "repograph-rich";
            "water";
            "repograph-rich" -> "cream";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        Warning: Rich dependencies are approximated in this graph; edges may omit required relationships or include inactive branches.
        """


Scenario: Disabled rich Recommends do not warn in a scoped graph
  Given I use repository "dnf-ci-rich"
    And I configure dnf with
        | key               | value |
        | install_weak_deps | False |
   When I execute dnf with args "repograph breakfast"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "breakfast";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """


Scenario: Disabled rich Recommends do not warn or remove available nodes in a full graph
  Given I use repository "dnf-ci-rich" with configuration
        | key         | value           |
        | includepkgs | breakfast water |
    And I configure dnf with
        | key               | value |
        | install_weak_deps | False |
   When I execute dnf with args "repograph"
   Then the exit code is 0
    And stdout is
        """
        digraph packages {{
            node [shape=box, style=rounded, fontname="Helvetica"];
            "breakfast";
            "water";
        }}
        """
    And stderr is
        """
        <REPOSYNC>
        """
