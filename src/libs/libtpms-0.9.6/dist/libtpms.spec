# --- libtpm rpm-spec ---

%define name      libtpms
%define version   0.9.6
%define release   1

# Valid crypto subsystems are 'freebl' and 'openssl'
%if "%{?crypto_subsystem}" == ""
%define crypto_subsystem openssl
%endif

# Valid build types are 'production' or 'debug'
%define build_type  production

Summary: Library providing Trusted Platform Module (TPM) functionality
Name:           %{name}
Version:        %{version}
Release:        %{release}%{?dist}
License:        BSD
Group:          Development/Libraries
Url:            http://github.com/stefanberger/libtpms
Source:         libtpms-%{version}.tar.gz
Provides:       libtpms-%{crypto_subsystem} = %{version}-%{release}

%if "%{crypto_subsystem}" == "openssl"
BuildRequires:  openssl-devel
%else
BuildRequires:  nss-devel >= 3.12.9-2
BuildRequires:  nss-softokn-freebl-devel >= 3.12.9-2
%if 0%{?rhel} > 6 || 0%{?fedora} >= 13
BuildRequires:  nss-softokn-freebl-static >= 3.12.9-2
%endif
BuildRequires:  nss-softokn-devel >= 3.12.9-2, gmp-devel
%endif
BuildRequires:  pkgconfig gawk sed
BuildRequires:  automake autoconf libtool bash coreutils gcc-c++

%if "%{crypto_subsystem}" == "openssl"
Requires:       openssl
%else
Requires:       nss-softokn-freebl >= 3.12.9-2, nss-softokn >= 3.12.9-2
%endif
Requires:       gmp

%description
A library providing TPM functionality for VMs. Targeted for integration
into Qemu.

%package        devel
Summary:        Include files for libtpms
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description   devel
Libtpms header files and documentation.

%files
%defattr(-, root, root, -)
%{_libdir}/%{name}.so.%{version}
%{_libdir}/%{name}.so.0
%doc LICENSE README CHANGES

%files devel
%defattr(-, root, root, -)

%{_libdir}/%{name}.so
%dir %{_includedir}/%{name}
%attr(644, root, root) %{_libdir}/pkgconfig/*.pc
%attr(644, root, root) %{_includedir}/%{name}/*.h
%attr(644, root, root) %{_mandir}/man3/*

%prep
%setup -q

%build

%if "%{crypto_subsystem}" == "openssl"
%define _with_openssl --with-openssl
%endif

%if "%{build_type}" == "debug"
%define _enable_debug --enable-debug
%endif

%if "%{build_type}" == "debug"
CFLAGS=-O0
%endif
./autogen.sh \
        --with-tpm2 \
        --disable-static \
        --prefix=/usr \
        --libdir=%{_libdir} \
        %{?_with_openssl} \
        %{?_enable_debug}

make %{?_smp_mflags}

%check
make check

%install
install -d -m 0755 $RPM_BUILD_ROOT%{_libdir}
install -d -m 0755 $RPM_BUILD_ROOT%{_includedir}/libtpms
install -d -m 0755 $RPM_BUILD_ROOT%{_mandir}/man3

make %{?_smp_mflags} install DESTDIR=${RPM_BUILD_ROOT}

rm -f $RPM_BUILD_ROOT%{_libdir}/libtpms.la

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%changelog
* Tue Feb 28 2023 Stefan Berger - 0.9.6-1
- tpm2: Check size of buffer before accessing it (CVE-2023-1017 & -1018)

* Fri Jul 01 2022 Stefan Berger - 0.9.5-1
- Release of version 0.9.5

* Mon Apr 25 2022 Stefan Berger - 0.9.4-1
- Release of version 0.9.4

* Mon Mar 07 2022 Stefan Berger - 0.9.3-1
- Release of version 0.9.3

* Thu Jan 06 2022 Stefan Berger - 0.9.2-1
- Release of version 0.9.2

* Wed Nov 24 2021 Stefan Berger - 0.9.1-1
- Release of version 0.9.1

* Wed Sep 29 2021 Stefan Berger - 0.9.0-1
- Release of version 0.9.0 (rev. 164)

* Wed Feb 24 2021 Stefan Berger - 0.8.0-1
- Release of version 0.8.0 (rev. 162)

* Fri Jul 19 2019 Stefan Berger - 0.7.0-1
- Release of version 0.7.0 (rev. 150)

* Mon Jan 14 2018 Stefan Berger - 0.6.0-1
- Release of version 0.6.0 with TPM 2.0 support

* Mon Jun 30 2014 Stefan Berger - 0.5.2-1
- Updated to version 0.5.2
- coverity fixes
- fixes for ARM64 using __aarch64__

* Sat Jun 07 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.5.1-20.1
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Sat Aug 03 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.5.1-19
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Mon Mar 25 2013 Stefan Berger - 0.5.1-18
- Ran autoreconf for support of aarch64
- Checking for __arm64__ in code

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.5.1-17
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.5.1-16
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Fri Feb 17 2012 Peter Robinson <pbrobinson@fedoraproject.org> - 0.5.1-15
- Add dist tag as required by package guidelines

* Fri Jan 27 2012 Stefan Berger - 0.5.1-14
- fix gcc-4.7 compilation problem

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.5.1-13
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Tue Dec 20 2011 Dan Horák <dan[at]danny.cz> - 0.5.1-12
- fix build on secondary arches

* Wed Nov 2 2011 Stefan Berger - 0.5.1-11
- added (lib)gmp as runtime dependency

* Sat Oct 8 2011 Stefan Berger - 0.5.1-10
- internal fixes; callback fixes

* Tue Aug 30 2011 Stefan Berger - 0.5.1-9
- new directory structure and build process

* Tue Jul 12 2011 Stefan Berger - 0.5.1-8
- added pkgconfig as build dependency
- enabling __powerpc__ build following Bz 728220

* Wed May 25 2011 Stefan Berger - 0.5.1-7
- increasing NVRAM area space to have enough room for certificates

* Wed May 25 2011 Stefan Berger - 0.5.1-6
- adding libtpms.pc pkg-config file

* Wed Apr 13 2011 Stefan Berger - 0.5.1-5
- adding BuildRequires for nss-softokn-freebl-static
- several libtpms-internal changes around state serialization and 
  deserialization
- fixes to libtpms makefile (makefile-libtpms)
- adding build_type to generate a debug or production build
- need nss-devel to have nss-config

* Tue Mar 08 2011 Stefan Berger - 0.5.1-4
- small fixes to libtpms makefile

* Fri Feb 25 2011 Stefan Berger - 0.5.1-3
- removing release from tar ball name
- Use {?_smp_mflags} for make rather than hardcoding it
- Fixing post and postun scripts; removing the scripts for devel package
- Fixing usage of defattr
- Adding version information into the changelog headers and spaces between the changelog entries
- Adding LICENSE, README and CHANGELOG file into tar ball and main rpm
- Removing clean section
- removed command to clean the build root
- adding library version to the libraries required for building and during
  runtime
- Extended Requires in devel package with {?_isa}

* Fri Feb 18 2011 Stefan Berger - 0.5.1-2
- make rpmlint happy by replacing tabs with spaces
- providing a valid URL for the tgz file
- release is now 2 -> 0.5.1-2

* Mon Jan 17 2011 Stefan Berger - 0.5.1-1
- Update version to 0.5.1

* Fri Jan 14 2011 Stefan Berger - 0.5.0-1
- Changes following Fedora review comments

* Thu Dec 2 2010 Stefan Berger
- Small tweaks after reading the FedoreCore packaging requirements

* Tue Nov 16 2010 Stefan Berger
- Created initial version of rpm spec files
- Version of library is now 0.5.0
- Debuginfo rpm is built but empty -- seems to be a known problem
  Check https://bugzilla.redhat.com/show_bug.cgi?id=209316
