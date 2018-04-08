Name: crystalspace
Summary: 2D/3D application Software Development Kit (SDK)
Version: 2.1
Release: 3%{?dist}
License: LGPL with exceptions
URL: http://www.crystalspace3d.org
Source0: %{name}-src-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#
# Note: Developers and users on RHEL/CentOS need EPEL available to meet the
# build and runtime requirements. See: https://fedoraproject.org/wiki/EPEL
#

# Required build packages.
BuildRequires: alsa-lib-devel
BuildRequires: assimp-devel
BuildRequires: bullet-devel
BuildRequires: freetype-devel
BuildRequires: jam
BuildRequires: lcms-devel
BuildRequires: lib3ds-devel
BuildRequires: libX11-devel
BuildRequires: libjpeg-turbo-devel
BuildRequires: libmng-devel
BuildRequires: libogg-devel
BuildRequires: libpng-devel
BuildRequires: libtheora-devel
BuildRequires: libvorbis-devel
BuildRequires: mesa-libEGL-devel
BuildRequires: mesa-libGL-devel
BuildRequires: openal-soft-devel
BuildRequires: speex-devel
BuildRequires: swig
BuildRequires: zlib-devel

# Exclude these architectures from build/packaging.
ExcludeArch: s390 s390x

%description -n %{name}
A modular, configurable, and extensible rendering engine SDK supporting OpenGL
and advanced features such as shaders and lighting systems, physics, 3D
sounds, animation blending, foliage, terrain, virtual file system, physical
peripherals and more.

#
# Note: We do not need to define/declare a default debuginfo package.
#
# We further down do have to define what we wish to go into the default
# debuginfo package.
#

# Development package.
%package -n %{name}-devel
Summary: Development files for Crystal Space SDK.
Provides: %{name}-devel = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
%description -n %{name}-devel
Development files for Crystal Space SDK.

# Data package.
%package -n %{name}-data
Summary: Data files for Crystal Space SDK.
Provides: %{name}-data = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
%description -n %{name}-data
Data files for Crystal Space SDK.

# Documentation package.
%package -n %{name}-doc
Summary: Documentation for Crystal Space SDK.
Provides: %{name}-doc = %{version}-%{release}
%description -n %{name}-doc
Documentation (manual and public API reference) for Crystal Space SDK.

# Applications package.
%package -n %{name}-apps
Summary: Applications for Crystal Space SDK.
Provides: %{name}-apps = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
Requires: %{name}-data = %{version}-%{release}
%description -n %{name}-apps
Applications for Crystal Space SDK.

# Utilities package.
%package -n %{name}-utils
Summary: Utilities for Crystal Space SDK.
Provides: %{name}-utils = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
%description -n %{name}-utils
Utilities for Crystal Space SDK.

# Applications debuginfo package.
%package -n %{name}-apps-debuginfo
Summary: Debug information for applications package.
Provides: %{name}-apps-debuginfo = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
Requires: %{name}-debuginfo = %{version}-%{release}
Requires: %{name}-apps = %{version}-%{release}
%description -n %{name}-apps-debuginfo
Debug information for applications package.

# Preperation and Setup elements.
%prep
%setup -q

# Configure and build elements.
%build
%configure --disable-make-emulation
jam -q %{?_smp_mflags}

# Installation elements.
%install
rm -rf %{buildroot}
DESTDIR=%{buildroot} jam install

# Post install elements.
%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

# Files for 'crystalspace' package.
%files -n %{name}
%defattr(-,root,root)
%exclude %{_libdir}/*.dbg
%exclude %{_libdir}/%{name}-%{version}/*.dbg
%{_libdir}/*.so
%{_libdir}/%{name}-%{version}/*.so
%{_sysconfdir}/%{name}-%{version}/*

# Files for 'crystalspace-devel' package.
%files -n %{name}-devel
%defattr(-,root,root)
%{_bindir}/cs-config*
%{_datadir}/%{name}-%{version}/bindings/*
%{_datadir}/%{name}-%{version}/build/*
%{_includedir}/%{name}-%{version}/*

# Files for 'crystalspace-data' package.
%files -n %{name}-data
%defattr(-,root,root)
%{_datadir}/%{name}-%{version}/data/*

# Files for 'crystalspace-doc' package.
%files -n %{name}-doc
%defattr(-,root,root)
%{_defaultdocdir}/%{name}-%{version}/*

# Files for 'crystalspace-apps' package.
%files -n %{name}-apps
%defattr(-,root,root)
%exclude %{_bindir}/cs-config*
%exclude %{_bindir}/*.dbg
%{_bindir}/*

# Files for 'crystalspace-utils' package.
%files -n %{name}-utils
%defattr(-,root,root)
%{_datadir}/%{name}-%{version}/conversion/*

# Files for 'crystalspace-debuginfo' package.
%files -n %{name}-debuginfo
%defattr(-,root,root)
%{_libdir}/*.dbg
%{_libdir}/%{name}-%{version}/*.dbg

# Files for 'crystalspace-apps-debuginfo' package.
%files -n %{name}-apps-debuginfo
%defattr(-,root,root)
%{_bindir}/*.dbg

#
# Changelog entries begin below. Newest goes at the top.
#
%changelog
* Fri May 16 2014 Phil Wyett <aura.yoda@gmail.com> - 2.1-3
- Disable makefile emulation layer creation at configure time.
- Add '-p' to 'sbin/ldconfig' call in 'post' and 'postun'.

* Sat Apr 26 2014 Phil Wyett <aura.yoda@gmail.com> - 2.1-2
- Enable bullet-devel in build requires.
- Add lib3ds-devel to build requires.

* Wed Apr 23 2014 Phil Wyett <aura.yoda@gmail.com> - 2.1-1
- Spec file creation/example entry.
