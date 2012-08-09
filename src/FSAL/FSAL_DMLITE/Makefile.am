lib_LTLIBRARIES           = libfsaldmlite.la

libfsaldmlite_la_SOURCES = fsal_convert.c   \
                        fsal_internal.c	 \
                        fsal_internal.h  \
                        main.c		 \
			export.c	 \
			handle.c	 \
			file.c		 \
			xattrs.c         \
			dmlite_methods.h \
	                ../../include/fsal.h                     \
                        ../../include/fsal_types.h	         \
	                ../../include/err_fsal.h	         \
	                ../../include/FSAL/FSAL_DMLITE/fsal_types.h

libfsaldmlite_la_CFLAGS                = $(FSAL_CFLAGS) $(SEC_CFLAGS) @DMLITE_CFLAGS@
libfsaldmlite_la_LDFLAGS               = @DMLITE_LDFLAGS@
libfsaldmlite_la_LIBADD                = @DMLITE_LIB@

new: clean all
