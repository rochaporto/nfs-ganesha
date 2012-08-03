/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CERN IT/GT/DMS <it-dep-gt-dms@cern.ch> 2012
 *
 * Based on the VFS FSAL from Panasas Inc. / Jim Lieb <jlieb@panasas.com>.
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *                Thomas LEIBOVICI  thomas.leibovici@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * ------------- 
 */

/* file.c
 * File I/O methods for VFS module
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include "fsal.h"
#include "fsal_internal.h"
#include "FSAL/access_check.h"
#include "fsal_convert.h"
#include <unistd.h>
#include <fcntl.h>
#include "dmlite_methods.h"
#include "FSAL/fsal_commonlib.h"

/** 
 * dmlite_open
 * 
 * Called with appropriate locks taken at the cache inode level.
 */
fsal_status_t dmlite_open(struct fsal_obj_handle *public_handle,
		       			  fsal_openflags_t openflags)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	struct dmlite_fd *fd;
	
	LogFullDebug(COMPONENT_FSAL, "dmlite_open: start");
	
	/* Fetch the private handler for the given file (from the public obj_handle) */
	dmlite_priv_handle = container_of(public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);

	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(public_handle->export);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "dmlite_open: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	assert(dmlite_priv_handle->u.file.fd == -1 
		&& dmlite_priv_handle->u.file.openflags == FSAL_O_CLOSED);

	// TODO: do a dmlite_get here (using the file inode)
	
	if(fd < 0) {
		fsal_error = posix2fsal_error(errno);
		retval = errno;
		goto errout;
	}
	dmlite_priv_handle->u.file.fd = fd;
	dmlite_priv_handle->u.file.openflags = openflags;
	dmlite_priv_handle->u.file.lock_status = 0; /* no locks on new files */

errout:
	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
		
	return fsalstat(fsal_error, retval);	
}

/* 
 * dmlite_status
 * 
 * Let the caller peek into the file's open/close state.
 */

fsal_openflags_t dmlite_status(struct fsal_obj_handle *public_handle)
{
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;

	dmlite_priv_handle = container_of(public_handle, struct dmlite_fsal_obj_handle, obj_handle);
	
	return dmlite_priv_handle->u.file.openflags;
}

/* 
 * dmlite_read
 * 
 * Concurrency (locks) is managed in cache_inode_*.
 */
fsal_status_t dmlite_read(struct fsal_obj_handle *public_handle,
		       			  uint64_t offset,
                       	  size_t buffer_size,
                       	  void *buffer,
		       			  size_t *read_amount,
		       			  bool_t *end_of_file)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	ssize_t nb_read;

	/* Fetch the private handler for the given file (from the public obj_handle) */
	dmlite_priv_handle = container_of(public_handle, struct dmlite_fsal_obj_handle, obj_handle);

	assert(dmlite_priv_handle->u.file.fd >= 0 
		&& dmlite_priv_handle->u.file.openflags != FSAL_O_CLOSED);

	// TODO: do the actual read here
	//nb_read = pread(myself->u.file.fd, buffer, buffer_size, offset);

	/* Check for errors */
	if(offset == -1 || nb_read == -1) {
		retval = errno;
		fsal_error = posix2fsal_error(retval);
		goto errout;
	}
	
	/* And we're done ... */
	*end_of_file = nb_read == 0 ? TRUE : FALSE;
	*read_amount = nb_read;
	
errout:		
	return fsalstat(fsal_error, retval);	
}

/* 
 * dmlite_write
 * 
 * Concurrency (locks) is managed in cache_inode_*.
 */
fsal_status_t dmlite_write(struct fsal_obj_handle *public_handle,
						   uint64_t offset,
						   size_t buffer_size,
						   void *buffer,
						   size_t *write_amount)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	ssize_t nb_written;

	/* Fetch the private handler for the given file (from the public obj_handle) */
	dmlite_priv_handle = container_of(public_handle, struct dmlite_fsal_obj_handle, obj_handle);

	assert(dmlite_priv_handle->u.file.fd >= 0 
		&& dmlite_priv_handle->u.file.openflags != FSAL_O_CLOSED);

	// TODO: do the actual write here
	//nb_written = pwrite(dmlite_priv_handle->u.file.fd, buffer, buffer_size, offset);

	/* Check for errors */
	if(offset == -1 || nb_written == -1) {
		retval = errno;
		fsal_error = posix2fsal_error(retval);
		goto errout;
	}
	
	/* And we're done ... */
	*write_amount = nb_written;
	
errout:		
	return fsalstat(fsal_error, retval);	
}

/* 
 * dmlite_commit
 * 
 * Commit a file range to storage. For right now, fsync will have to do.
 */
fsal_status_t dmlite_commit(struct fsal_obj_handle *public_handle,
			 				off_t offset,
			 				size_t len)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	
	/* Fetch the private handler for the given file (from the public obj_handle) */
	dmlite_priv_handle = container_of(public_handle, struct dmlite_fsal_obj_handle, obj_handle);

	assert(dmlite_priv_handle->u.file.fd >= 0 
		&& dmlite_priv_handle->u.file.openflags != FSAL_O_CLOSED);

	// TODO: not sure what to do here
	
	return fsalstat(fsal_error, retval);
}

/* 
 * dmlite_lock_op
 * 
 * We do not support this.
 */
fsal_status_t dmlite_lock_op(struct fsal_obj_handle *public_handle,
			  				 void * p_owner,
			  				 fsal_lock_op_t lock_op,
			  				 fsal_lock_param_t   request_lock,
			  				 fsal_lock_param_t * conflicting_lock)
{
	return fsalstat(ERR_FSAL_NO_ERROR, 0);	
}

/* 
 * dmlite_close
 * 
 * Close the file if it is still open.
 */

fsal_status_t dmlite_close(struct fsal_obj_handle *public_handle)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	
	/* Fetch the private handler for the given file (from the public obj_handle) */
	dmlite_priv_handle = container_of(public_handle, struct dmlite_fsal_obj_handle, obj_handle);
	
	assert(dmlite_priv_handle->u.file.fd >= 0
	       && dmlite_priv_handle->u.file.openflags != FSAL_O_CLOSED
	       && !dmlite_priv_handle->u.file.lock_status);

	// TODO: close the file
	
	/* Check for errors */
	if(retval < 0) {
		retval = errno;
		fsal_error = posix2fsal_error(retval);
	}
	
	/* And we're done ... reset the values in the private handle */
	dmlite_priv_handle->u.file.fd = -1;
	dmlite_priv_handle->u.file.lock_status = 0;
	dmlite_priv_handle->u.file.openflags = FSAL_O_CLOSED;
	
	return fsalstat(fsal_error, retval);	
}

/* 
 * dmlite_lru_cleanup
 * 
 * Free non-essential resources at the request of cache inode's
 * LRU processing identifying this handle as stale enough for resource
 * trimming.
 */
fsal_status_t dmlite_lru_cleanup(struct fsal_obj_handle *public_handle,
			      				 lru_actions_t requests)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	
	/* Fetch the private handler for the given file (from the public obj_handle) */
	dmlite_priv_handle = container_of(public_handle, struct dmlite_fsal_obj_handle, obj_handle);
	
	if(dmlite_priv_handle->u.file.fd >= 0 && !(dmlite_priv_handle->u.file.lock_status)) {
		// TODO: close the file as it is still open
		
		/* And now reset the internal values */
		dmlite_priv_handle->u.file.fd = -1;
		dmlite_priv_handle->u.file.lock_status = 0;
		dmlite_priv_handle->u.file.openflags = FSAL_O_CLOSED;
	}
	
	/* Check for errors */
	if(retval == -1) {
		retval = errno;
		fsal_error = posix2fsal_error(retval);
	}
	
	return fsalstat(fsal_error, retval);	
}
