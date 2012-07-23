
Name: 		ganesha
Version: 	1.4.0
Release: 	1%{?dist}
Summary: 	NFS Server running in user space with large cache
License: 	LGPLv3
Group: 		Applications/System
Url:		http://nfs-ganesha.sourceforge.net
Source0:	http://downloads.sourceforge.net/nfs-ganesha/%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	bison
BuildRequires:	dmlite-devel
BuildRequires:	flex
BuildRequires:	gcc
BuildRequires:	git
BuildRequires:	krb5-devel
BuildRequires:	libgssglue-devel
BuildRequires:	libtool
BuildRequires:	m4
BuildRequires:	pkgconfig

Requires:	libtirpc-libs

%description
Ganesha is a NFS Server running in user space with a large cache.
It comes with various back-end modules to support different file systems and
name-spaces.

%package doc
Summary: 	Documentation for Ganesha
Group:		Applications/System

%description doc
This packages provides documentation on the ganesha server

%package libntirpc
Summary:	TI-RPC variant packaged with Ganesha
Group:		Applications/System

%description libntirpc
A variant of TI-RPC with enhanced mt-safety, bi-directional operation, and
other enhancements (in progress).

%package libntirpc-devel
Summary:	Headers for the TI-RPC variant packaged with Ganesha
Group:		Applications/System

%description libntirpc-devel
This package provides development files and headers for the variant of 
TI-RPC for Ganesha.

%package fsal-vfs
Summary: 	Ganesha VFS filesystem plugin
Group:		Applications/System
Requires:	%{name}-common = %{version}-%{release} 
Requires:	libattr

%description fsal-vfs
This package provides the VFS filesystem plugin for Ganesha.

%package fsal-dmlite
Summary: 	The NFS-GANESHA server compiled for use with DMLITE
Group:		Applications/System
Requires:	%{name}-common = %{version}-%{release} 

%description fsal-dmlite
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various back-end modules to support different file systems and
name-spaces. This package provides support for NFS-GANESHA used on top of VFS.
This feature requires a kernel higher than 2.6.39

%prep
%setup -q -n %{name}-%{version}

%build
cd src
./tirpc.sh
aclocal
autoreconf -f
libtoolize
%configure \
	--disable-nfsidmap \
	--disable-fsal-proxy \
	--enable-fsal-dmlite \
	--enable-fsal-vfs

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
mkdir -p %{buildroot}%{_sysconfdir}/ganesha
mkdir -p %{buildroot}%{_sysconfdir}/init.d
mkdir -p %{buildroot}%{_sysconfdir}/sysconfig
mkdir -p %{buildroot}%{_sysconfdir}/logrotate.d
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_datadir}
mkdir -p %{buildroot}%{_initrddir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_libdir}/pkgconfig
mkdir -p %{buildroot}%{_sbindir}

cd src
make install DESTDIR=%{buildroot} sysconfdir=%{_sysconfdir}/%{name}

cd ..
install -m 644 dist/ganesha.fedora.logrotate %{buildroot}%{_sysconfdir}/logrotate.d/%{name}
install -m 755 dist/ganesha.fedora.init %{buildroot}%{_initrddir}/%{name}
install -m 755 dist/ganesha.fedora.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/%{name}

# packaging corrections
mv %{buildroot}%{_bindir}/ganesha.nfsd %{buildroot}%{_sbindir}
mv %{buildroot}%{_bindir}/ganestat.pl %{buildroot}%{_sbindir}

# cleanup of unpackaged files
rm %{buildroot}%{_libdir}/libfsal*.*a
rm %{buildroot}%{_usr}/lib/libntirpc*.*a

%clean
rm -rf %{buildroot}

%preun
if [ "$1" = "0" ]; then
    /sbin/service %{name} stop >/dev/null 2>&1
    /sbin/chkconfig --del %{name} > /dev/null 2>&1 || 
fi

%post
/sbin/chkconfig --add %{name} > /dev/null 2>&1 || :

%post libntirpc -p /sbin/ldconfig

%postun libntirpc -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/%{name}
%{_initrddir}/%{name}
%{_sbindir}/%{name}.nfsd
%{_sbindir}/ganestat.pl
%config(noreplace) %{_sysconfdir}/logrotate.d/%{name}
%config(noreplace) %{_sysconfdir}/sysconfig/%{name}
%config(noreplace) %{_sysconfdir}/%{name}/hosts.ganesha
%config(noreplace) %{_sysconfdir}/%{name}/snmp.conf
%config(noreplace) %{_sysconfdir}/%{name}/uidgid_mapfile.ganesha

%files doc
%defattr(-,root,root,-)
%doc src/LICENSE.txt src/Docs/nfs-ganesha-userguide.pdf src/Docs/nfs-ganesha-adminguide.pdf src/Docs/nfs-ganeshell-userguide.pdf src/Docs/nfs-ganesha-ganestat.pdf src/Docs/using_ganeshell.pdf src/Docs/nfs-ganesha-convert_fh.pdf src/Docs/using_*_fsal.pdf src/Docs/ganesha_snmp_adm.pdf

%files libntirpc
%defattr(-,root,root,-)
%{_usr}/lib/libntirpc.so.*
%{_usr}/lib/pkgconfig/libntirpc.pc

%files libntirpc-devel
%defattr(-,root,root,-)
%{_includedir}/m4/.dummy
%{_includedir}/autogen.sh
%{_includedir}/clnt_internal.h
%{_includedir}/rpc_com.h
%{_includedir}/rpc_ctx.h
%{_includedir}/svc_internal.h
%{_includedir}/svc_xprt.h
%{_includedir}/tirpc/getpeereid.h
%{_includedir}/tirpc/misc/event.h
%{_includedir}/tirpc/misc/opr_queue.h
%{_includedir}/tirpc/misc/queue.h
%{_includedir}/tirpc/misc/rbtree.h
%{_includedir}/tirpc/misc/rbtree_x.h
%{_includedir}/tirpc/namespace.h
%{_includedir}/tirpc/netconfig.h
%{_includedir}/tirpc/reentrant.h
%{_includedir}/tirpc/rpc/auth.h
%{_includedir}/tirpc/rpc/auth_des.h
%{_includedir}/tirpc/rpc/auth_gss.h
%{_includedir}/tirpc/rpc/auth_inline.h
%{_includedir}/tirpc/rpc/auth_kerb.h
%{_includedir}/tirpc/rpc/auth_unix.h
%{_includedir}/tirpc/rpc/clnt.h
%{_includedir}/tirpc/rpc/clnt_soc.h
%{_includedir}/tirpc/rpc/clnt_stat.h
%{_includedir}/tirpc/rpc/des.h
%{_includedir}/tirpc/rpc/des_crypt.h
%{_includedir}/tirpc/rpc/nettype.h
%{_includedir}/tirpc/rpc/pmap_clnt.h
%{_includedir}/tirpc/rpc/pmap_prot.h
%{_includedir}/tirpc/rpc/pmap_rmt.h
%{_includedir}/tirpc/rpc/raw.h
%{_includedir}/tirpc/rpc/rpc.h
%{_includedir}/tirpc/rpc/rpc_com.h
%{_includedir}/tirpc/rpc/rpc_msg.h
%{_includedir}/tirpc/rpc/rpcb_clnt.h
%{_includedir}/tirpc/rpc/rpcb_prot.h
%{_includedir}/tirpc/rpc/rpcb_prot.x
%{_includedir}/tirpc/rpc/rpcent.h
%{_includedir}/tirpc/rpc/svc.h
%{_includedir}/tirpc/rpc/svc_auth.h
%{_includedir}/tirpc/rpc/svc_dg.h
%{_includedir}/tirpc/rpc/svc_dplx.h
%{_includedir}/tirpc/rpc/svc_rqst.h
%{_includedir}/tirpc/rpc/svc_soc.h
%{_includedir}/tirpc/rpc/types.h
%{_includedir}/tirpc/rpc/xdr.h
%{_includedir}/tirpc/rpc/xdr_inline.h
%{_includedir}/tirpc/rpcsvc/crypt.h
%{_includedir}/tirpc/rpcsvc/crypt.x
%{_includedir}/tirpc/un-namespace.h
%{_includedir}/vc_lock.h
%{_usr}/lib/libntirpc.so

%files fsal-vfs
%defattr(-,root,root,-)
%{_sysconfdir}/%{name}/vfs.ganesha.*conf
%{_libdir}/libfsalvfs.so*

%files fsal-dmlite
%defattr(-,root,root,-)
%{_libdir}/libfsaldmlite.so*

%changelog
