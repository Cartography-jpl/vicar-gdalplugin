Summary: This is a plugin for GDAL to support VICAR files
Name: vicar-gdalplugin
Version: 1.12
Release: 1.el%{rhel}
License: Copyright 2021 California Institute of Technology ALL RIGHTS RESERVED
Group: Applications/Engineering
Vendor: California Institute of Technology
URL: http://www-mipl.jpl.nasa.gov/external/vicar.html
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: gdal-afids boost-afids openjpeg20 vicar-rtl
Requires: gdal-afids openjpeg20 vicar-rtl
Prefix: /opt/afids_support

%description

This is a plugin that gives support in GDAL for VICAR files. Note that you
need to set GDAL_DRIVER_PATH environment variable to point to where this
is installed for GDAL to use this.

%prep
%setup -q

%build
./configure --prefix=/opt/afids_support 
make %_smp_mflags 

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT/opt/afids_support/lib/gdalplugins/gdal_vicar.la
rm -f $RPM_BUILD_ROOT/opt/afids_support/lib/gdalplugins/gdal_vicar.a

%clean
rm -rf $RPM_BUILD_ROOT
%files
%defattr(-,root,root,-)
%doc
/opt/afids_support/lib/gdalplugins/gdal_vicar.so

%changelog
* Tue Apr  4 2023 Smyth <smyth@macsmyth> - 1.12-1.el%{rhel}
- Change location of tempary file used in processing geotiff labels. We had this
  in the current directory, but it should actually be in /tmp.

* Wed Mar  3 2021 Smyth <smyth@macsmyth> - 1.11-1.el%{rhel}
- Update to work with GDAL 3.2.1. Dependency on vicar_rtl was added back in,
  this was removed by Walt with the hope that the plugin would be part of GDAL.
  But this isn't going to happen, so no reason to have a second build of all
  the vicar rtl stuff.

* Wed Sep 25 2019 Smyth <smyth@macsmyth> - 1.10-1.el%{rhel}
- Substantial rework of vicar_gdalplugin. Shouldn't be any user visible
  changes, rework was all internal to simplify the code. and remove
  dependency on vicar_rtl.

* Fri Jul 20 2018 Smyth <smyth@macsmyth> - 1.08-2.el%{rhel}
- Rebuild after changes to vicar_rtl

* Tue Jun  5 2018 Smyth <smyth@macsmyth> - 1.08-1.el%{rhel}
- Add proper support for pixel as point vs pixel as area

* Fri Oct  7 2016 Mike M Smyth <smyth@pistol> - 1.07-2.el%{rhel}
- Rebuild with new python

* Tue May 17 2016 Mike M Smyth <smyth@pistol> - 1.07-1.el%{rhel}
- Fix the permissions of VicarDataset, which is needed to correctly
  write out VICAR format using gdalwarp and other GDAL tools.

* Mon Feb 29 2016 Mike M Smyth <smyth@pistol> - 1.06-1.el%{rhel}
- Add a few geotiff tags we simply missed before, and also add all the
  double geotiff tags which we had completely left out. This is needed
  to support Mars coordinate systems.

* Wed Jan 20 2016 Mike M Smyth <smyth@pistol> - 1.05-7.el%{rhel}
- Rebuild with gdal changes.

* Thu Dec 17 2015 Mike M Smyth <smyth@pistol> - 1.05-6
- Rebuild

* Wed Nov 25 2015 Mike M Smyth <smyth@pistol> - 1.05-3
- Update to version 1.59.0 of boost

* Wed Dec 10 2014 Mike M Smyth <smyth@pistol> - 1.05-3
- Rebuild using the new version boost we built for AFIDS. No actually
  changes to the plugin.

* Mon Jul  7 2014 Mike M Smyth <smyth@pistol> - 1.05-2
- Rebuild using the latest 1.11.0 version of GDAL.

* Thu May 29 2014 Mike M Smyth <smyth@pistol> - 1.05-1
- Add support for multiple band VICAR files

* Fri May  9 2014 Mike M Smyth <smyth@pistol> - 1.04-1
- Add support for putting TRE information into the geotiff property.

* Fri May  9 2014 Mike M Smyth <smyth@pistol> - 1.03-1
- Update to pass all geotiff properties items through if they aren't 
  RPC or geotiff. This is because AFIDS has historically shoved all kind
  of things into the geotiff property that have nothing to do with with 
  geotiff.

* Thu Jun  6 2013 Mike M Smyth <smyth@pistol> - 1.02-1
- Minor change to Makefile, and update to latest automake

* Thu Dec  6 2012 Mike M Smyth <smyth@pistol> - gdalplugin-1
- Initial build.

