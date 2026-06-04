Name:           repograph-versioned
Version:        2.0
Release:        1
Summary:        Newer graph root with its own dependency branch
License:        Public Domain
BuildArch:      noarch

Requires:       repograph-new-only

%description
The latest version selects a different dependency branch from the older version.

%package -n repograph-new-only
Summary:        Dependency used only by the newer graph root

%description -n repograph-new-only
Marks the dependency branch of repograph-versioned 2.0.

%files

%files -n repograph-new-only
