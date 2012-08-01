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

/* handle.c
 * VFS object (file|dir) handle object
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include <FSAL/FSAL_DMLITE/fsal_handle_syscalls.h>
#include <libgen.h>             /* used for 'dirname' */
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <mntent.h>
#include "nlm_list.h"
#include "fsal_internal.h"
#include "fsal_convert.h"
#include "dmlite_methods.h"
#include "FSAL/fsal_commonlib.h"
#include "dmlite/c/catalog.h"
#include "dmlite/c/dmlite.h"
#include "dmlite/c/inode.h"

/* helpers
 */

/* allocate_handle
 * 
 * Allocate and fill in a handle.
 */
static struct dmlite_fsal_obj_handle *allocate_handle(struct dmlite_xstat *dmlite_stat,
                                                      struct fsal_export *exp_hdl)
{
	struct dmlite_fsal_obj_handle *dmlite_priv_obj;
	fsal_status_t status;

	LogFullDebug(COMPONENT_FSAL, "allocate_handle: start");
	
	dmlite_priv_obj = malloc(sizeof(struct dmlite_fsal_obj_handle));
	
	if (dmlite_priv_obj == NULL)
		return NULL;

	/* Start with setting defaults for the internal properties */
	if(dmlite_priv_obj->obj_handle.type == REGULAR_FILE) {
		dmlite_priv_obj->u.file.fd = -1;  /* no open on this yet */
		dmlite_priv_obj->u.file.openflags = FSAL_O_CLOSED;
		dmlite_priv_obj->u.file.lock_status = 0;
	}
	
	/* Then set the dmlite specific (also private) properties */
	dmlite_priv_obj->dmlite.ino = dmlite_stat->stat.st_ino;
	strncpy(dmlite_priv_obj->dmlite.name, dmlite_stat->name, NAME_MAX);

	/* Finally set public pointer (obj_handle from fsal_api.h) properties */
	//memset(&dmlite_priv_obj, 0, (sizeof(struct dmlite_fsal_obj_handle)));
	dmlite_priv_obj->obj_handle.type = posix2fsal_type(dmlite_stat->stat.st_mode);
	dmlite_priv_obj->obj_handle.export = exp_hdl;
	dmlite_priv_obj->obj_handle.attributes.mask
		= exp_hdl->ops->fs_supported_attrs(exp_hdl);
	dmlite_priv_obj->obj_handle.attributes.supported_attributes
		= dmlite_priv_obj->obj_handle.attributes.mask;
	status = posix2fsal_attributes(&dmlite_stat->stat, &dmlite_priv_obj->obj_handle.attributes);
	if(FSAL_IS_ERROR(status))
		goto errout;
		
	/* ... and initialize the public object handle */
	if(!fsal_obj_handle_init(&dmlite_priv_obj->obj_handle, exp_hdl->fsal->obj_ops,
		exp_hdl, posix2fsal_type(dmlite_stat->stat.st_mode)))
		return dmlite_priv_obj;

	/* And we're done... cleanup */
	dmlite_priv_obj->obj_handle.ops = NULL;
	pthread_mutex_unlock(&dmlite_priv_obj->obj_handle.lock);
	pthread_mutex_destroy(&dmlite_priv_obj->obj_handle.lock);
errout:
	return NULL;
}

/* handle methods
 */

/* lookup
 * deprecated NULL parent && NULL path implies root handle
 */

static fsal_status_t lookup(struct fsal_obj_handle *parent,
							const char *path,
							struct fsal_obj_handle **handle)
{
	LogFullDebug(COMPONENT_FSAL, "lookup: start");

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* create
 * 
 * Creates a regular file and sets its attributes.
 */
static fsal_status_t create(struct fsal_obj_handle *dir_handle,
                            const char *file_name,
                            struct attrlist *file_attrs,
                            struct fsal_obj_handle **file_out_public_handle)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_fsal_obj_handle *dmlite_dir_priv_handle;
	struct dmlite_fsal_obj_handle *dmlite_file_priv_handle;
	struct dmlite_xstat dmlite_xstat_obj;
	
	LogFullDebug(COMPONENT_FSAL, "create: start");

	/* Make sure we clean it up first */
	*file_out_public_handle = NULL;
	
	/* Is the directory really a directory? */
	if( !dir_handle->ops->handle_is(dir_handle, DIRECTORY)) {
		LogCrit(COMPONENT_FSAL,
			"Given parent handle is not a directory. hdl = 0x%p", dir_handle);
		return fsalstat(ERR_FSAL_NOTDIR, 0);
	}
	
	/* Fetch the private handler for the given directory (from the public obj_handle) */
	dmlite_dir_priv_handle = container_of(dir_handle, struct dmlite_fsal_obj_handle, obj_handle);
	
	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(dir_handle->export);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "create: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Go ahead and create the catalog entry (using a xstat object) */
	dmlite_xstat_obj.parent = dmlite_dir_priv_handle->dmlite.ino;
	dmlite_xstat_obj.stat.st_mode = file_attrs->mode;
	dmlite_xstat_obj.stat.st_uid = file_attrs->owner;
	dmlite_xstat_obj.stat.st_gid = file_attrs->group;
	strncpy(dmlite_xstat_obj.name, file_name, NAME_MAX); 
	retval = dmlite_icreate(dmlite_context_obj, &dmlite_xstat_obj);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "create: failed to create new catalog entry :: %s",
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
		
	/* Fetch the file info back so that we can fill the handle obj */
	memset(&dmlite_xstat_obj, 0, sizeof(struct dmlite_xstat));
	retval = dmlite_statx(dmlite_context_obj, file_name, &dmlite_xstat_obj);
	if (retval != 0) {
		retval = dmlite_errno(dmlite_context_obj);
		LogMajor(COMPONENT_FSAL, "create: failed to stat new file :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}

	/* Allocate a handle object and fill it up with the stat info */
	dmlite_file_priv_handle = allocate_handle(&dmlite_xstat_obj, dir_handle->export);
	if(dmlite_file_priv_handle == NULL) {
		fsal_error = ERR_FSAL_NOMEM;
		LogMajor(COMPONENT_FSAL, "create: failed to allocate handle");
		*file_out_public_handle = NULL;
		goto errout;
	}
		
	/* And return too... */
	(*file_out_public_handle) = &(dmlite_file_priv_handle->obj_handle);

	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);

	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return fsalstat(fsal_error, retval);	
}

/* mkdir
 * 
 * Creates a new directory.
 */
static fsal_status_t makedir(struct fsal_obj_handle *parent_public_handle,
							 const char *dir_name,
							 struct attrlist *dir_attrs,
							 struct fsal_obj_handle **dir_out_public_handle)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_fsal_obj_handle *dmlite_parent_priv_handle;
	struct dmlite_fsal_obj_handle *dmlite_dir_priv_handle;
	struct dmlite_xstat dmlite_xstat_obj;
	
	LogFullDebug(COMPONENT_FSAL, "makedir: start");

	/* Make sure we clean it up first */
	*dir_out_public_handle = NULL;
	
	/* Is the directory really a directory? */
	if( !parent_public_handle->ops->handle_is(parent_public_handle, DIRECTORY)) {
		LogCrit(COMPONENT_FSAL,
			"makedir: given parent handle is not a directory. hdl = 0x%p", parent_public_handle);
		return fsalstat(ERR_FSAL_NOTDIR, 0);
	}
	
	/* Fetch the private handler for the given parent (from the public obj_handle) */
	dmlite_parent_priv_handle = container_of(parent_public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
	
	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(parent_public_handle->export);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "makedir: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Go ahead and create the catalog entry */
	dmlite_xstat_obj.parent = dmlite_parent_priv_handle->dmlite.ino;
	dmlite_xstat_obj.stat.st_mode = dir_attrs->mode;
	dmlite_xstat_obj.stat.st_uid = dir_attrs->owner;
	dmlite_xstat_obj.stat.st_gid = dir_attrs->group;
	strncpy(dmlite_xstat_obj.name, dir_name, NAME_MAX); 
	retval = dmlite_icreate(dmlite_context_obj, &dmlite_xstat_obj);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "makedir: failed to create new directory entry :: %s",
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Fetch the directory info back so that we can fill the handle obj */
	memset(&dmlite_xstat_obj, 0, sizeof(struct dmlite_xstat));
	retval = dmlite_istatx_by_name(dmlite_context_obj, 
		dmlite_parent_priv_handle->dmlite.ino, dir_name, &dmlite_xstat_obj);
	if (retval != 0) {
		retval = dmlite_errno(dmlite_context_obj);
		LogMajor(COMPONENT_FSAL, "create: failed to stat new directory :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}

	/* Allocate a handle object and fill it up with the stat info */
	dmlite_dir_priv_handle = allocate_handle(&dmlite_xstat_obj, parent_public_handle->export);
	if(dmlite_dir_priv_handle == NULL) {
		fsal_error = ERR_FSAL_NOMEM;
		LogMajor(COMPONENT_FSAL, "create: failed to allocate handle");
		*dir_out_public_handle = NULL;
		goto errout;
	}
		
	/* And return too... */
	(*dir_out_public_handle) = &(dmlite_dir_priv_handle->obj_handle);

	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);

	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return fsalstat(fsal_error, retval);	
	
}

/**
 * We do not support this one...
 */
static fsal_status_t makenode(struct fsal_obj_handle *dir_hdl,
                              const char *name,
                              object_file_type_t nodetype,  /* IN */
                              fsal_dev_t *dev,  /* IN */
                              struct attrlist *attrib,
                              struct fsal_obj_handle **handle)
{
	LogFullDebug(COMPONENT_FSAL, "makenode: not supported... should not be called!");

	return fsalstat(ERR_FSAL_NO_ERROR, 0);	
}

/** makesymlink
 * 
 * Creates a new symlink in the catalog.
 */
static fsal_status_t makesymlink(struct fsal_obj_handle *parent_public_handle,
                                 const char *file_name,
                                 const char *link_path,
                                 struct attrlist *link_attrs,
                                 struct fsal_obj_handle **out_link_public_handle)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_fsal_obj_handle *dmlite_parent_priv_handle;
	struct dmlite_fsal_obj_handle *dmlite_link_priv_handle;
	struct dmlite_xstat dmlite_xstat_obj;
	
	LogFullDebug(COMPONENT_FSAL, "makesymlink: start");

	/* Make sure we clean it up first */
	*out_link_public_handle = NULL;
	
	/* Is the directory really a directory? */
	if( !parent_public_handle->ops->handle_is(parent_public_handle, DIRECTORY)) {
		LogCrit(COMPONENT_FSAL,
			"makesymlink: given parent handle is not a directory. hdl = 0x%p", parent_public_handle);
		return fsalstat(ERR_FSAL_NOTDIR, 0);
	}
	
	/* Fetch the private handler for the given directory (from the public obj_handle) */
	dmlite_parent_priv_handle = container_of(parent_public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
	
	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(parent_public_handle->export);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "makesymlink: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
		
	/* We start by creating the new entry */
	dmlite_xstat_obj.parent = dmlite_parent_priv_handle->dmlite.ino;
	strncpy(dmlite_xstat_obj.name, file_name, NAME_MAX);
	dmlite_xstat_obj.stat.st_mode = link_attrs->mode;
	dmlite_xstat_obj.stat.st_uid = link_attrs->owner;
	dmlite_xstat_obj.stat.st_gid = link_attrs->group;
	retval = dmlite_icreate(dmlite_context_obj, &dmlite_xstat_obj);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "makesymlink: failed to create new entry :: %s",
			dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;	
	}
	
	/* Fetch the entry info back so that we get the inode */
	memset(&dmlite_xstat_obj, 0, sizeof(struct dmlite_xstat));
	retval = dmlite_istatx_by_name(dmlite_context_obj, 
		dmlite_parent_priv_handle->dmlite.ino, file_name, &dmlite_xstat_obj);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "makesymlink: failed to stat new file :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}

	/* Fill in the link information for the entry we just created */
	retval = dmlite_isymlink(dmlite_context_obj, dmlite_xstat_obj.stat.st_ino, link_path);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "makesymlink: failed to add link info :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Allocate a handle object and fill it up with the stat info */
	// TODO: shouldn't we give link info to allocate_handle??
	dmlite_link_priv_handle = allocate_handle(&dmlite_xstat_obj, 
		parent_public_handle->export);
	if(dmlite_link_priv_handle == NULL) {
		fsal_error = ERR_FSAL_NOMEM;
		LogMajor(COMPONENT_FSAL, "create: failed to allocate handle");
		*out_link_public_handle = NULL;
		goto errout;
	}
		
	/* And return too... */
	(*out_link_public_handle) = &(dmlite_link_priv_handle->obj_handle);

	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);

	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return fsalstat(fsal_error, retval);
}

static fsal_status_t readsymlink(struct fsal_obj_handle *link_public_handle,
                                 char *link_content,
                                 size_t *link_len,
                                 bool_t refresh)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_fsal_obj_handle *dmlite_link_priv_handle = NULL;
	char link_buff[NAME_MAX];
	
	LogFullDebug(COMPONENT_FSAL, "readsymlink: start");
	
	/* Make sure it's a symlink we're handling */
	if(link_public_handle->type != SYMBOLIC_LINK) {
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Fetch the private handler for the given entry (from the public obj_handle) */
	dmlite_link_priv_handle = container_of(link_public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
		
	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(link_public_handle->export);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "readsymlink: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}

	/* Read in the symlink information for the given inode */
	retval = dmlite_ireadlink(dmlite_context_obj, dmlite_link_priv_handle->dmlite.ino,
		link_buff, NAME_MAX);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "makesymlink: failed to read link info :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Fill in the output values */
	*link_len = NAME_MAX;
	strncpy(link_content, link_buff, NAME_MAX);
	
	// TODO: We need to update the internal link info in the priv handler?
	// We could use those fields instead of the local buff we use above
		
	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
		
	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return fsalstat(fsal_error, retval);
}

/**
 * linkfile
 * 
 * Hard link a file... not supported.
 */
static fsal_status_t linkfile(struct fsal_obj_handle *obj_hdl,
			      struct fsal_obj_handle *destdir_hdl,
			      const char *name)
{
	LogFullDebug(COMPONENT_FSAL, "linkfile: not supported... should not be called!");

	return fsalstat(ERR_FSAL_NO_ERROR, 0);	
}

/**
 * read_dirents
 * read the directory and call through the callback function for
 * each entry.
 * @param dir_hdl [IN] the directory to read
 * @param entry_cnt [IN] limit of entries. 0 implies no limit
 * @param whence [IN] where to start (next)
 * @param dir_state [IN] pass thru of state to callback
 * @param cb [IN] callback function
 * @param eof [OUT] eof marker TRUE == end of dir
 */

static fsal_status_t read_dirents(struct fsal_obj_handle *dir_public_handle,
				  				  uint32_t entry_count,
				  				  struct fsal_cookie *whence,
				  				  void *dir_state,
				  				  fsal_status_t (*cb)(
				  				  	const char *name,
					  				unsigned int dtype,
				   					struct fsal_obj_handle *dir_public_handle,
					  				void *dir_state,
					  				struct fsal_cookie *cookie),
                                  	bool_t *eof
                                  )
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_obj_handle *dmlite_dir_priv_handle;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_idir *dir_fd;
	struct dmlite_xstat *dir_entry_xstat;
	fsal_status_t status;
	struct fsal_cookie *entry_cookie;

	LogFullDebug(COMPONENT_FSAL, "read_dirents: start");

	/* Initial checks and allocations */
	// TODO: handle whence here... it might have the offset if we need it in the future
	entry_cookie = alloca(sizeof(struct fsal_cookie) + sizeof(off_t));
	
	/* Fetch the private handler for the given directory (from the public obj_handle) */
	dmlite_dir_priv_handle = container_of(dir_public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);

	/* Open the directory */
	dir_fd = dmlite_iopendir(dmlite_context_obj, dmlite_dir_priv_handle->dmlite.ino);
	if (dir_fd == NULL) {
		LogMajor(COMPONENT_FSAL, "read_dirents: failed to open dir :: %s",
			dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	// TODO: do we need to seek?
	
	/* Read the directory, entry by entry */
	dir_entry_xstat = dmlite_ireaddirx(dmlite_context_obj, dir_fd);
	// TODO: we need to handle eventual errors here
	while (dir_entry_xstat != NULL) {
		//TODO: at least we need to handle the cookie here
		
		/* Callback to caller */
		status = cb(dir_entry_xstat->name, dir_entry_xstat->stat.st_mode,
			dir_public_handle, dir_state, entry_cookie);
		
		/* Read the next entry */	
		dir_entry_xstat = dmlite_ireaddirx(dmlite_context_obj, dir_fd);
	}
	
	/* Close the directory when we're done */
	retval = dmlite_iclosedir(dmlite_context_obj, dir_fd);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "read_dirents: failed to close dir :: %s",
			dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}

errout:
	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
		
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/**
 * renamefile
 * 
 * Renames a file.
 */
static fsal_status_t renamefile(struct fsal_obj_handle *old_parent_public_handle,
								const char *old_name,
								struct fsal_obj_handle *new_parent_public_handle,
								const char *new_name)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_fsal_obj_handle *dmlite_new_parent_priv_handle;
	struct dmlite_fsal_obj_handle *dmlite_old_parent_priv_handle;
	struct dmlite_xstat dmlite_xstat_obj;
	
	LogFullDebug(COMPONENT_FSAL, "renamefile: start");

	/* Fetch the private handlers for both old and new directories */
	dmlite_old_parent_priv_handle = container_of(old_parent_public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
	dmlite_new_parent_priv_handle = container_of(new_parent_public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
	
	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(old_parent_public_handle->export);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "renamefile: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Fetch the current information about the file (we need the inode) */
	memset(&dmlite_xstat_obj, 0, sizeof(struct dmlite_xstat));
	retval = dmlite_istatx_by_name(dmlite_context_obj, 
		dmlite_old_parent_priv_handle->dmlite.ino, old_name, &dmlite_xstat_obj);
	if (retval != 0) {
		retval = dmlite_errno(dmlite_context_obj);
		LogMajor(COMPONENT_FSAL, "renamefile: failed to stat file :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* We first rename the file */
	retval = dmlite_irename(dmlite_context_obj, dmlite_xstat_obj.stat.st_ino, new_name);
	if (retval != 0) {
		retval = dmlite_errno(dmlite_context_obj);
		LogMajor(COMPONENT_FSAL, "renamefile: failed to rename file :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* ... and then we move it to the new directory */
	retval = dmlite_imove(dmlite_context_obj, dmlite_xstat_obj.stat.st_ino, 
		dmlite_new_parent_priv_handle->dmlite.ino);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "renamefile: failed to move file :: %s",
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
		
	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return fsalstat(fsal_error, retval);	
}

/* FIXME: attributes are now merged into fsal_obj_handle.  This
 * spreads everywhere these methods are used.  eventually deprecate
 * everywhere except where we explicitly want to to refresh them.
 * NOTE: this is done under protection of the attributes rwlock in the
 * cache entry.
 * 
 * TODO: is this really used?
 */

static fsal_status_t getattrs(struct fsal_obj_handle *obj_hdl,
                              struct attrlist *obj_attr)
{
	struct dmlite_fsal_obj_handle *myself;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int retval = 0;
	
	LogFullDebug(COMPONENT_FSAL, "getattrs: start");
	
	myself = container_of(obj_hdl, struct dmlite_fsal_obj_handle, obj_handle);
	//mntfd = dmlite_get_root_fd(obj_hdl->export);
	/**if(obj_hdl->type == SOCKET_FILE) {
		fd = open_by_handle_at(mntfd,
				       myself->u.sock.sock_dir,
				       (O_PATH|O_NOACCESS));
		if(fd < 0) {
			goto errout;
		}
		retval = fstatat(fd,
				 myself->u.sock.sock_name,
				 &stat,
				 AT_SYMLINK_NOFOLLOW);
		if(retval < 0) {
			goto errout;
		}
	} else {
		if(obj_hdl->type == SYMBOLIC_LINK)
			open_flags |= O_PATH;
		else if(obj_hdl->type == FIFO_FILE)
			open_flags |= O_NONBLOCK;
		fd = open_by_handle_at(mntfd, myself->handle, open_flags);
		if(fd < 0) {
			goto errout;
		}
		retval = fstatat(fd,
				 "",
				 &stat,
				 (AT_SYMLINK_NOFOLLOW|AT_EMPTY_PATH));
		if(retval < 0) {
			goto errout;
		}
	}*/

	/* convert attributes *//**
	obj_hdl->attributes.mask = obj_attr->mask;
	st = posix2fsal_attributes(&stat, &obj_hdl->attributes);
	if(FSAL_IS_ERROR(st)) {
		FSAL_CLEAR_MASK(obj_attr->mask);
		FSAL_SET_MASK(obj_attr->mask,
			      ATTR_RDATTR_ERR);
		fsal_error = st.major;  retval = st.minor;
		goto out;
	}
	memcpy(obj_attr, &obj_hdl->attributes, sizeof(struct attrlist));
	goto out;

errout:
        retval = errno;
        if(retval == ENOENT)
                fsal_error = ERR_FSAL_STALE;
        else
                fsal_error = posix2fsal_error(retval);
out:
	if(fd >= 0)
		close(fd);*/
	return fsalstat(fsal_error, retval);	
}

/*
 * NOTE: this is done under protection of the attributes rwlock in the cache entry.
 */

static fsal_status_t setattrs(struct fsal_obj_handle *public_handle,
			      			  struct attrlist *attrs)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_xstat dmlite_xstat_obj;
	
	LogFullDebug(COMPONENT_FSAL, "setattrs: start");

	/* We need to apply the umaks if mode is set */
	if (FSAL_TEST_MASK(attrs->mask, ATTR_MODE)) {
		attrs->mode
			&= ~public_handle->export->ops->fs_umask(public_handle->export);
	}
	
	/* Fetch the private handler of the file */
	dmlite_priv_handle = container_of(public_handle,
		struct dmlite_fsal_obj_handle, obj_handle);

	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(public_handle->export);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "setattrs: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Fetch the current information about the file */
	memset(&dmlite_xstat_obj, 0, sizeof(struct dmlite_xstat));
	retval = dmlite_istatx(dmlite_context_obj, dmlite_priv_handle->dmlite.ino, &dmlite_xstat_obj);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "setattrs: failed to stat file :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* chmod: call dmlite_isetmode */
	if(FSAL_TEST_MASK(attrs->mask, ATTR_MODE) && public_handle->type != SYMBOLIC_LINK) {
		retval = dmlite_isetmode(dmlite_context_obj, dmlite_priv_handle->dmlite.ino, 
			-1, -1, fsal2unix_mode(attrs->mode), -1, NULL);
		if (retval != 0) {
			LogMajor(COMPONENT_FSAL, "setattrs: failed to set mode :: %s",
					dmlite_error(dmlite_context_obj));
			fsal_error = ERR_FSAL_FAULT;
			goto errout;
		}
	}
	/* chown: call dmlite_isetmode **/
	if(FSAL_TEST_MASK(attrs->mask,
			  ATTR_OWNER | ATTR_GROUP)) {
		uid_t user = FSAL_TEST_MASK(attrs->mask, ATTR_OWNER) ? (int)attrs->owner : -1;
		gid_t group = FSAL_TEST_MASK(attrs->mask, ATTR_GROUP) ? (int)attrs->group : -1;
		retval = dmlite_isetmode(dmlite_context_obj, dmlite_priv_handle->dmlite.ino,
			user, group, -1, -1, NULL);
		if (retval != 0) {
			LogMajor(COMPONENT_FSAL, "setattrs: failed to change owner/group :: %s",
					dmlite_error(dmlite_context_obj));
			fsal_error = ERR_FSAL_FAULT;
			goto errout;
		}		
	}
	/* utime: we do this one with dmlite_iutime */
	if(FSAL_TEST_MASK(attrs->mask, ATTR_ATIME | ATTR_MTIME)) {
		//TODO: implement this... dmlite expects a utimbuf
	}

	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
		
	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return fsalstat(fsal_error, retval);
}

/* compare
 * compare two handles.
 * return TRUE for equal, FALSE for anything else
 */
static bool_t compare(struct fsal_obj_handle *public_handle,
                      struct fsal_obj_handle *other_public_handle)
{
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	struct dmlite_fsal_obj_handle *dmlite_other_priv_handle;

	LogFullDebug(COMPONENT_FSAL, "compare: start");

	/* Basic consistency */
	if(!other_public_handle)
		return FALSE;
		
	/* Fetch the private handlers for both entries */
	dmlite_priv_handle = container_of(public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
	dmlite_other_priv_handle = container_of(other_public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
	
	/* Compare both public and private members of the handles */
	if((public_handle->type != other_public_handle->type) ||
	   (dmlite_priv_handle->dmlite.ino != dmlite_other_priv_handle->dmlite.ino))
		return FALSE;
		
	return TRUE;
}

/* file_truncate
 * truncate a file to the size specified.
 * size should really be off_t...
 */

static fsal_status_t file_truncate(struct fsal_obj_handle *obj_hdl,
				   uint64_t length)
{
	LogFullDebug(COMPONENT_FSAL, "file_truncate: not supported... should not be called!");

	return fsalstat(ERR_FSAL_NO_ERROR, 0);	
}

/* file_unlink
 * unlink the named file in the directory
 */
static fsal_status_t file_unlink(struct fsal_obj_handle *parent_public_handle,
				 				 const char *name)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_obj_handle *dmlite_parent_priv_handle;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_xstat dmlite_xstat_obj;

	LogFullDebug(COMPONENT_FSAL, "file_unlink: start");

	/* Fetch the private handler of the file */
	dmlite_parent_priv_handle = container_of(parent_public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
	
	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(parent_public_handle->export);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "file_unlink: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Fetch the current information about the file */
	memset(&dmlite_xstat_obj, 0, sizeof(struct dmlite_xstat));
	retval = dmlite_istatx(dmlite_context_obj, 
		dmlite_parent_priv_handle->dmlite.ino, &dmlite_xstat_obj);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "file_unlink: failed to stat file :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Unlink using the inode from above */
	retval = dmlite_iunlink(dmlite_context_obj, dmlite_xstat_obj.stat.st_ino);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "file_unlink: failed to stat file :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	// TODO: we could make use of unlink_by_name to do it with one single call
	
	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
		
	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return fsalstat(fsal_error, retval);
}

/* handle_digest
 * fill in the opaque f/s file handle part.
 * we zero the buffer to length first.  This MAY already be done above
 * at which point, remove memset here because the caller is zeroing
 * the whole struct.
 */
static fsal_status_t handle_digest(struct fsal_obj_handle *public_handle,
                                   fsal_digesttype_t output_type,
                                   struct gsh_buffdesc *fh_desc)
{
	uint32_t ino32;
	uint64_t ino64;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	int handle_size;
	
	LogFullDebug(COMPONENT_FSAL, "handle_digest: start");

	/* Some basic checks */
    if( !fh_desc)
		return fsalstat(ERR_FSAL_FAULT, 0);
		
	dmlite_priv_handle = container_of(public_handle, struct dmlite_fsal_obj_handle, obj_handle);

	switch(output_type) {
	case FSAL_DIGEST_NFSV2:
	case FSAL_DIGEST_NFSV3:
	case FSAL_DIGEST_NFSV4:
		handle_size = sizeof(public_handle);
		if(fh_desc->len < handle_size)
			goto errout;
		memcpy(fh_desc->addr, public_handle, handle_size);
		break;
	case FSAL_DIGEST_FILEID2:
		handle_size = FSAL_DIGEST_SIZE_FILEID2;
		if(fh_desc->len < handle_size)
			goto errout;
		memcpy(fh_desc->addr, &dmlite_priv_handle->dmlite.ino, handle_size);
		break;
	case FSAL_DIGEST_FILEID3:
		handle_size = FSAL_DIGEST_SIZE_FILEID3;
		if(fh_desc->len < handle_size)
			goto errout;
		memcpy(&ino32, &dmlite_priv_handle->dmlite.ino, sizeof(ino32));
		ino64 = ino32;
		memcpy(fh_desc->addr, &ino64, handle_size);
		break;
	case FSAL_DIGEST_FILEID4:
		handle_size = FSAL_DIGEST_SIZE_FILEID4;
		if(fh_desc->len < handle_size)
			goto errout;
		memcpy(&ino32, &dmlite_priv_handle->dmlite.ino, sizeof(ino32));
		ino64 = ino32;
		memcpy(fh_desc->addr, &ino64, handle_size);
		break;
	default:
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}
	fh_desc->len = handle_size;
	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	LogMajor(COMPONENT_FSAL, "handle_digest: space too small for handle. need %lu, have %lu",
		 handle_size, fh_desc->len);
	return fsalstat(ERR_FSAL_TOOSMALL, 0);
}

/**
 * handle_to_key
 * return a handle descriptor into the handle in this object handle
 * @TODO reminder.  make sure things like hash keys don't point here
 * after the handle is released.
 */

static void handle_to_key(struct fsal_obj_handle *public_handle,
                          struct gsh_buffdesc *fh_desc)
{
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;

	LogFullDebug(COMPONENT_FSAL, "handle_to_key: start");

	dmlite_priv_handle = container_of(public_handle, 
		struct dmlite_fsal_obj_handle, obj_handle);
	memcpy(fh_desc->addr, &dmlite_priv_handle->dmlite, sizeof(dmlite_priv_handle->dmlite)); 
	fh_desc->len = sizeof(dmlite_priv_handle->dmlite);
}

/*
 * release
 * release our export first so they know we are gone
 */

static fsal_status_t release(struct fsal_obj_handle *obj_hdl)
{
	struct fsal_export *exp = obj_hdl->export;
	struct dmlite_fsal_obj_handle *myself;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int retval = 0;

	LogFullDebug(COMPONENT_FSAL, "release: start");

	myself = container_of(obj_hdl, struct dmlite_fsal_obj_handle, obj_handle);
	pthread_mutex_lock(&obj_hdl->lock);
	obj_hdl->refs--;  /* subtract the reference when we were created */
	if(obj_hdl->refs != 0 || (obj_hdl->type == REGULAR_FILE
				  && (myself->u.file.lock_status != 0
				      || myself->u.file.fd >=0
				      || myself->u.file.openflags != FSAL_O_CLOSED))) {
		pthread_mutex_unlock(&obj_hdl->lock);
		retval = obj_hdl->refs > 0 ? EBUSY : EINVAL;
		LogCrit(COMPONENT_FSAL,
			"Tried to release busy handle, "
			"hdl = 0x%p->refs = %d, fd = %d, openflags = 0x%x, lock = %d",
			obj_hdl, obj_hdl->refs,
			myself->u.file.fd, myself->u.file.openflags,
			myself->u.file.lock_status);
		return fsalstat(posix2fsal_error(retval), retval);
	}
	fsal_detach_handle(exp, &obj_hdl->handles);
	pthread_mutex_unlock(&obj_hdl->lock);
	pthread_mutex_destroy(&obj_hdl->lock);
	myself->obj_handle.ops = NULL; /*poison myself */
	myself->obj_handle.export = NULL;
	if(obj_hdl->type == SYMBOLIC_LINK) {
		if(myself->u.symlink.link_content != NULL)
			free(myself->u.symlink.link_content);
	} else if(obj_hdl->type == SOCKET_FILE) {
		if(myself->u.sock.sock_name != NULL)
			free(myself->u.sock.sock_name);
		if(myself->u.sock.sock_dir != NULL)
			free(myself->u.sock.sock_dir);
	}
	free(myself);
	return fsalstat(fsal_error, 0);
}

void dmlite_handle_ops_init(struct fsal_obj_ops *ops)
{
	ops->release = release;
	ops->lookup = lookup;
	ops->readdir = read_dirents;
	ops->create = create;
	ops->mkdir = makedir;
	ops->mknode = makenode;
	ops->symlink = makesymlink;
	ops->readlink = readsymlink;
	ops->test_access = fsal_test_access;
	ops->getattrs = getattrs;
	ops->setattrs = setattrs;
	ops->link = linkfile;
	ops->rename = renamefile;
	ops->unlink = file_unlink;
	ops->truncate = file_truncate;
	ops->open = dmlite_open;
	ops->status = dmlite_status;
	ops->read = dmlite_read;
	ops->write = dmlite_write;
	ops->commit = dmlite_commit;
	ops->lock_op = dmlite_lock_op;
	ops->close = dmlite_close;
	ops->lru_cleanup = dmlite_lru_cleanup;
	ops->compare = compare;
	ops->handle_digest = handle_digest;
	ops->handle_to_key = handle_to_key;
}

/* 
 * export methods that create object handles
 */
fsal_status_t dmlite_lookup_path(struct fsal_export *export_handle,
			         			 const char *path,
			         			 struct fsal_obj_handle **out_handle)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_fsal_obj_handle *dmlite_priv_handle;
	struct dmlite_xstat dmlite_xstat_obj;

	LogFullDebug(COMPONENT_FSAL, "dmlite_lookup_path: start :: %s", path);

	/* Get a dmlite_context object */
	dmlite_context_obj = dmlite_get_context(export_handle);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "file_unlink: failed to create context");
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}
	
	/* Fetch the stat information for the request path */
	memset(&dmlite_xstat_obj, 0, sizeof(struct dmlite_xstat));
	if(strcmp(path, "/") == 0) { // root file handle
		retval = dmlite_statx(dmlite_context_obj, "/", &dmlite_xstat_obj);
	} else { // some other handle
		retval = dmlite_statx(dmlite_context_obj, path, &dmlite_xstat_obj);
	}
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "dmlite_lookup_path: failed to lookup :: %s", 
					dmlite_error(dmlite_context_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}

	/* Allocate a handle object and fill it up with the stat info */
	dmlite_priv_handle = allocate_handle(&dmlite_xstat_obj, export_handle);
	if(dmlite_priv_handle == NULL) {
		fsal_error = ERR_FSAL_NOMEM;
		LogMajor(COMPONENT_FSAL, "dmlite_lookup_path: failed to allocate handle");
		*out_handle = NULL; /* poison it */
		goto errout;
	}

	/* And return too... */
	(*out_handle) = &(dmlite_priv_handle->obj_handle);

	/* And we're done... do the final cleanup and return */
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);

	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return fsalstat(fsal_error, retval);	
}

/* create_handle
 * Does what original FSAL_ExpandHandle did (sort of)
 * returns a ref counted handle to be later used in cache_inode etc.
 * NOTE! you must release this thing when done with it!
 * BEWARE! Thanks to some holes in the *AT syscalls implementation,
 * we cannot get an fd on an AF_UNIX socket.  Sorry, it just doesn't...
 * we could if we had the handle of the dir it is in, but this method
 * is for getting handles off the wire for cache entries that have LRU'd.
 * Ideas and/or clever hacks are welcome...
 */

fsal_status_t dmlite_create_handle(struct fsal_export *export_handle,
								   struct gsh_buffdesc *hdl_desc,
								   struct fsal_obj_handle **out_handle)
{
	LogFullDebug(COMPONENT_FSAL, "dmlite_create_handle: start");

	// TODO: do we need this one?

	return fsalstat(ERR_FSAL_NO_ERROR, 0);	
}

