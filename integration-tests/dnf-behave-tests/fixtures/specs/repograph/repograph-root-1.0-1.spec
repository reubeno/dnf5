Name:           repograph-root
Version:        1.0
Release:        1
Summary:        Root with strong and weak graph dependencies
License:        Public Domain
BuildArch:      noarch

Requires:       top
Provides:       repograph-choice
Recommends:     dedalo
Suggests:       vagare

%description
Uses the dependency-chain and simple-base repositories.

%package -n repograph-supplement
Summary:        Supplement with its own required dependency
Supplements:    repograph-root
Requires:       labirinto

%description -n repograph-supplement
Selected by the solver without a Requires or Recommends edge from the root.

%package -n repograph-enhancer
Summary:        Enhancement that is not automatically installed
Provides:       repograph-choice
Requires:       labirinto
Enhances:       repograph-root

%description -n repograph-enhancer
Available for the full graph, but not selected by weak dependency resolution.

%package -n repograph-consumer
Summary:        Consumer with two alternative providers
Requires:       repograph-choice

%description -n repograph-consumer
Requires a capability provided by both repograph-root and repograph-enhancer.

%install
mkdir -p %{buildroot}%{_bindir} %{buildroot}%{_datadir}/repograph
touch %{buildroot}%{_bindir}/repograph-tool %{buildroot}%{_datadir}/repograph/root.txt
chmod 755 %{buildroot}%{_bindir}/repograph-tool

%files
%{_bindir}/repograph-tool
%{_datadir}/repograph/root.txt

%files -n repograph-supplement

%files -n repograph-enhancer

%files -n repograph-consumer
