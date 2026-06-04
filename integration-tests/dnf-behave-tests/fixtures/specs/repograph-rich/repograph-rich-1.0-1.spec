Name:           repograph-rich
Version:        1.0
Release:        1
Summary:        Multiple rich dependencies for graph warning aggregation
License:        Public Domain
BuildArch:      noarch

Requires:       (water and cream)
Requires:       (flour if dill)
Recommends:     (cream if water)

%description
Uses the dnf-ci-rich repository to exercise solver-selected rich dependencies.

%package duplicate
Summary:        Repeated rich dependency in another package
Requires:       (water and cream)

%description duplicate
Repeats a rich dependency from the main package.

%files

%files duplicate
