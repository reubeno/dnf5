Name:           repograph-versioned
Version:        1.0
Release:        1
Summary:        Older graph root with its own dependency branch
License:        Public Domain
BuildArch:      noarch

Requires:       repograph-old-only

%description
The older version must not contribute edges when a newer version is selected.

%package -n repograph-old-only
Summary:        Dependency used only by the older graph root

%description -n repograph-old-only
Marks the dependency branch of repograph-versioned 1.0.

%files

%files -n repograph-old-only
