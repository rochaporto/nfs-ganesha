AM_CFLAGS                     = $(FSAL_CFLAGS) $(SEC_CFLAGS)

if BUILD_SHARED_FSAL

lib_LTLIBRARIES = libfsalsnmp.la
libfsalsnmp_la_LDFLAGS = -version-number @LIBVERSION@
libfsalsnmp_la_LIBADD = ../../SemN/libSemN.la  $(FSAL_LDFLAGS)

else
noinst_LTLIBRARIES          = libfsalsnmp.la
endif



libfsalsnmp_la_SOURCES = fsal_access.c  fsal_context.c  fsal_dirs.c    fsal_fsinfo.c    fsal_rcp.c     fsal_compat.c   fsal_truncate.c \
			 fsal_attrs.c   fsal_convert.c  fsal_errors.c  fsal_init.c      fsal_lookup.c     fsal_rename.c  fsal_symlinks.c  fsal_unlink.c   \
			 fsal_common.c  fsal_create.c   fsal_fileop.c  fsal_internal.c  fsal_stats.c   fsal_tools.c     fsal_xattrs.c   \
                         fsal_local_op.c \
			 fsal_common.h  fsal_convert.h  fsal_internal.h	\
		         ../../include/fsal.h ../../include/fsal_types.h ../../include/FSAL/FSAL_SNMP/fsal_types.h                                        \
                         ../../include/err_fsal.h

new: clean all

doc:
	doxygen ./doxygen.conf
	rep=`grep OUTPUT_DIRECTORY doxygen.conf | grep share  | awk -F '=' '{print $$2;}'` ; cd $$rep/latex ; make

