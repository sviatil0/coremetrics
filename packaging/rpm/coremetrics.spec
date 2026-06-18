# RPM spec for source builds of CoreMetrics on Fedora / RHEL / openSUSE.
#
# This file targets `rpmbuild` from a SRPM. The .github/workflows/release.yml
# pipeline produces a *binary* RPM via `fpm` instead, so it does not consume
# this spec; this spec is the artifact distros (Fedora COPR, openSUSE OBS)
# pull from when they want to build from source.
#
# TODO: when submitting upstream to Fedora COPR, add a %changelog entry
# for each tag and verify that `rpmlint` passes on the build host.

Name:           coremetrics
Version:        0.2.18
Release:        1%{?dist}
Summary:        Cross-platform desktop system metrics monitor (CPU / RAM / GPU / processes)

License:        LGPL-2.1-only
URL:            https://github.com/sviatil0/coremetrics
Source0:        https://github.com/sviatil0/coremetrics/archive/refs/tags/v%{version}.tar.gz#/%{name}-%{version}.tar.gz

BuildRequires:  gcc-c++ >= 13
BuildRequires:  make
BuildRequires:  pkgconfig
BuildRequires:  SDL3-devel
BuildRequires:  SDL3_ttf-devel
BuildRequires:  SDL3_image-devel
BuildRequires:  freetype-devel
BuildRequires:  harfbuzz-devel

Requires:       SDL3
Requires:       SDL3_ttf
Requires:       SDL3_image

%description
CoreMetrics is a small C++23 system monitor built directly on raw SDL3 pixel
surfaces. It shows live CPU, RAM, GPU, and per-process resource usage with an
optional rolling sparklines view and an htop-style kill flow on the Processes
tab. No Qt, no Electron, no JavaScript runtime; every widget is hand-rasterized
by code in this repository. Three native metrics backends (Linux /proc, macOS
Mach + IOKit, Windows PDH) are selected at compile time so the same source
tree builds the same UI on each platform.

%prep
%autosetup -n %{name}-%{version}

%build
mkdir -p obj bin
make bin/coremetrics \
    CXXFLAGS="-std=c++23 -Wall -O2 -I ./include $(pkg-config --cflags sdl3 sdl3-ttf sdl3-image)" \
    LDFLAGS="$(pkg-config --libs sdl3 sdl3-ttf sdl3-image)"

%install
install -D -m 0755 bin/coremetrics %{buildroot}%{_bindir}/coremetrics
install -D -m 0644 base.xml %{buildroot}%{_datadir}/coremetrics/base.xml
mkdir -p %{buildroot}%{_datadir}/coremetrics/assets
cp -r assets/* %{buildroot}%{_datadir}/coremetrics/assets/
install -D -m 0644 man/coremetrics.1 %{buildroot}%{_mandir}/man1/coremetrics.1 || true

%files
%license LICENSE
%doc README.md CONTRIBUTING.md
%{_bindir}/coremetrics
%{_datadir}/coremetrics/
%{_mandir}/man1/coremetrics.1*

%changelog
* Sun Nov 16 2025 Sviatoslav Oleksiienko <soleksiienko1@gmail.com> - 0.2.18-1
- Initial RPM packaging. See git log for upstream changes.
