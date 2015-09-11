%define majorminor  1.0
%define gstreamer   gstreamer

%define gst_minver   0.11.0

Name: 		%{gstreamer}-editing-services
Version: 	1.5.90
Release: 	1.gst
Summary: 	GStreamer helper library for editing applications

%define 	majorminor	1.0

Group: 		Applications/Multimedia
License: 	LGPL
URL:		http://gstreamer.freedesktop.org/
Vendor:         GStreamer Backpackers Team <package@gstreamer.freedesktop.org>
Source:         http://gstreamer.freedesktop.org/src/gst-editing-services/gst-editing-services-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:         %{gstreamer} >= %{gst_minver}
BuildRequires:    %{gstreamer}-devel >= %{gst_minver}

%description
GStreamer is a streaming media framework, based on graphs of filters which
operate on media data. Applications using this library can do anything
from real-time sound processing to playing videos, and just about anything
else media-related.  Its plugin-based architecture means that new data
types or processing capabilities can be added simply by installing new
plug-ins.
%prep
%setup -q -n gst-editing-services-%{version}

%build
%configure

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT

%makeinstall
                                                                                
# Clean out files that should not be part of the rpm.
rm -f $RPM_BUILD_ROOT%{_libdir}/gstreamer-%{majorminor}/*.la
rm -f $RPM_BUILD_ROOT%{_libdir}/gstreamer-%{majorminor}/*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/*.la

%clean
rm -rf $RPM_BUILD_ROOT

