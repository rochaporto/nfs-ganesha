AM_CFLAGS                     = $(FSAL_CFLAGS) $(SEC_CFLAGS)

#noinst_LTLIBRARIES          = libfsaldmlite.la
lib_LTLIBRARIES          = libfsaldmlite.la

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
	                ../../include/FSAL/FSAL_DMLITE/fsal_types.h \
	                ../../include/FSAL/FSAL_DMLITE/fsal_handle_syscalls.h

new: clean all

