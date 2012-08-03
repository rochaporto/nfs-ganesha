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

/* export.c
 * VFS FSAL export object
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include <libgen.h>             /* used for 'dirname' */
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <mntent.h>
#include <sys/quota.h>
#include "nlm_list.h"
#include "fsal_internal.h"
#include "fsal_convert.h"
#include "dmlite_methods.h"
#include "FSAL/fsal_commonlib.h"
#include "FSAL/fsal_config.h"

#include <dmlite/c/dmlite.h>

/* 
 * DMLITE internal export (FSAL internal private fields).
 *
 * These are fields that other objects (like handle) will need.
 */

struct dmlite_fsal_export {
	struct fsal_export export;
    struct dmlite_manager *manager;
};

/* Methods to be called from other DMLITE objects (like handle) */

struct fsal_staticfsinfo_t *dmlite_staticinfo(struct fsal_module *hdl);

struct dmlite_manager * dmlite_get_manager(struct fsal_export *export_handle) {
	struct dmlite_fsal_export *dmlite_export_priv_handle;

	dmlite_export_priv_handle = container_of(export_handle, struct dmlite_fsal_export, export);
	return dmlite_export_priv_handle->manager;
}

struct dmlite_context * dmlite_get_context(struct fsal_export *export_handle) {
	
	int retval = 0;
	struct dmlite_manager *dmlite_manager_obj;
	struct dmlite_context *dmlite_context_obj;
	struct dmlite_credentials dmlite_creds_obj;
	
	/* Get a dmlite_manager object first */
	dmlite_manager_obj = dmlite_get_manager(export_handle);
	if (dmlite_manager_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "dmlite_get_context: manager was null... this should not happen");
		goto errout;
	}
	dmlite_context_obj = dmlite_context_new(dmlite_manager_obj);
	if (dmlite_context_obj == NULL) {
		LogMajor(COMPONENT_FSAL, "dmlite_get_context: failed to create context :: %s",
			dmlite_manager_error(dmlite_manager_obj));
		goto errout;
	}
	
	/* Set user credentials 
	 * TODO: actually use credentials, and probably move this somewhere else */
	memset(&dmlite_creds_obj, 0, sizeof(dmlite_creds_obj));
	dmlite_creds_obj.mech = "ID";
	dmlite_creds_obj.client_name = "/C=CH/O=CERN/OU=GD/CN=Test user 1";
	retval = dmlite_setcredentials(dmlite_context_obj, &dmlite_creds_obj);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "dmlite_get_context: failed to set credentials :: %s",
					dmlite_error(dmlite_context_obj));
		goto errout;
	}
		
	return dmlite_context_obj;
	
errout:
	if (dmlite_context_obj != NULL)
		dmlite_context_free(dmlite_context_obj);
	return NULL;
}

/* export object methods */

static fsal_status_t release(struct fsal_export *exp_hdl)
{
	struct dmlite_fsal_export *myself;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int retval = 0;

	myself = container_of(exp_hdl, struct dmlite_fsal_export, export);

	pthread_mutex_lock(&exp_hdl->lock);
	if(exp_hdl->refs > 0 || !glist_empty(&exp_hdl->handles)) {
		LogMajor(COMPONENT_FSAL,
			 "DMLITE release: export (0x%p)busy",
			 exp_hdl);
		fsal_error = posix2fsal_error(EBUSY);
		retval = EBUSY;
		goto errout;
	}
	fsal_detach_export(exp_hdl->fsal, &exp_hdl->exports);

	myself->export.ops = NULL; /* poison myself */
	pthread_mutex_unlock(&exp_hdl->lock);

	pthread_mutex_destroy(&exp_hdl->lock);
	free(myself);  /* elvis has left the building */
	return fsalstat(fsal_error, retval);

errout:
	pthread_mutex_unlock(&exp_hdl->lock);
	return fsalstat(fsal_error, retval);
}

static fsal_status_t get_dynamic_info(struct fsal_export *exp_hdl,
					 fsal_dynamicfsinfo_t *infop)
{
	struct dmlite_fsal_export *myself;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int retval = 0;

        LogFullDebug(COMPONENT_FSAL, "get_dynamic_info: start");

	if( !infop) {
		fsal_error = ERR_FSAL_FAULT;
		goto out;
	}
	myself = container_of(exp_hdl, struct dmlite_fsal_export, export);

        // TODO: dmlite_stat export

	if(retval < 0) {
		fsal_error = posix2fsal_error(errno);
		retval = errno;
		goto out;
	}
/**
	infop->total_bytes = buffstatvfs.f_frsize * buffstatvfs.f_blocks;
	infop->free_bytes = buffstatvfs.f_frsize * buffstatvfs.f_bfree;
	infop->avail_bytes = buffstatvfs.f_frsize * buffstatvfs.f_bavail;
	infop->total_files = buffstatvfs.f_files;
	infop->free_files = buffstatvfs.f_ffree;
	infop->avail_files = buffstatvfs.f_favail;
	infop->time_delta.seconds = 1;
	infop->time_delta.nseconds = 0;
*/
out:
	return fsalstat(fsal_error, retval);	
}

static bool_t fs_supports(struct fsal_export *exp_hdl,
                          fsal_fsinfo_options_t option)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_supports(info, option);
}

static uint64_t fs_maxfilesize(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_maxfilesize(info);
}

static uint32_t fs_maxread(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_maxread(info);
}

static uint32_t fs_maxwrite(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_maxwrite(info);
}

static uint32_t fs_maxlink(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_maxlink(info);
}

static uint32_t fs_maxnamelen(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_maxnamelen(info);
}

static uint32_t fs_maxpathlen(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_maxpathlen(info);
}

static fsal_fhexptype_t fs_fh_expire_type(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_fh_expire_type(info);
}

static gsh_time_t fs_lease_time(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_lease_time(info);
}

static fsal_aclsupp_t fs_acl_support(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_acl_support(info);
}

static attrmask_t fs_supported_attrs(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_supported_attrs(info);
}

static uint32_t fs_umask(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_umask(info);
}

static uint32_t fs_xattr_access_rights(struct fsal_export *exp_hdl)
{
	struct fsal_staticfsinfo_t *info;

	info = dmlite_staticinfo(exp_hdl->fsal);
	return fsal_xattr_access_rights(info);
}

/* get_quota
 * return quotas for this export.
 * path could cross a lower mount boundary which could
 * mask lower mount values with those of the export root
 * if this is a real issue, we can scan each time with setmntent()
 * better yet, compare st_dev of the file with st_dev of root_fd.
 * on linux, can map st_dev -> /proc/partitions name -> /dev/<name>
 */

static fsal_status_t get_quota(struct fsal_export *exp_hdl,
			       const char * filepath,
			       int quota_type,
			       struct req_op_context *req_ctx,
			       fsal_quota_t *pquota)
{

	LogFullDebug(COMPONENT_FSAL, "get_quota: start");
	
	//TODO: can we implement this one?
	
	return fsalstat(ERR_FSAL_NO_ERROR, 0);	
}

/* set_quota
 * same lower mount restriction applies
 */

static fsal_status_t set_quota(struct fsal_export *exp_hdl,
			       const char *filepath,
			       int quota_type,
			       struct req_op_context *req_ctx,
			       fsal_quota_t * pquota,
			       fsal_quota_t * presquota)
{
	LogFullDebug(COMPONENT_FSAL, "set_quota: start");
	
	//TODO: can we implement this one?
	
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* extract a file handle from a buffer.
 * do verification checks and flag any and all suspicious bits.
 * Return an updated fh_desc into whatever was passed.  The most
 * common behavior, done here is to just reset the length.  There
 * is the option to also adjust the start pointer.
 */

static fsal_status_t extract_handle(struct fsal_export *exp_hdl,
				    				fsal_digesttype_t in_type,
				    				struct netbuf *fh_desc)
{
	struct dmlite_handle *dmlite_handle_obj;
	size_t fh_size;

	LogFullDebug(COMPONENT_FSAL, "extract_handle: start");

	/* sanity checks */
	if( !fh_desc || !fh_desc->buf)
		return fsalstat(ERR_FSAL_FAULT, 0);

	/* Fetch our internal dmlite_handle from the buffer */
	dmlite_handle_obj = (struct dmlite_handle *)fh_desc->buf;
	fh_size = sizeof(struct dmlite_handle);
	
	if(in_type == FSAL_DIGEST_NFSV2) {
		if(fh_desc->len < fh_size) {
			LogMajor(COMPONENT_FSAL,
				 "V2 size too small for handle.  should be %lu, got %u",
				 fh_size, fh_desc->len);
			return fsalstat(ERR_FSAL_SERVERFAULT, 0);
		}
	} else if(in_type != FSAL_DIGEST_SIZEOF && fh_desc->len != fh_size) {
		LogMajor(COMPONENT_FSAL,
			 "Size mismatch for handle.  should be %lu, got %u",
			 fh_size, fh_desc->len);
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}
	fh_desc->len = fh_size;  /* pass back the actual size */
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* dmlite_export_ops_init
 * overwrite vector entries with the methods that we support
 */

void dmlite_export_ops_init(struct export_ops *ops)
{
	ops->release = release;
	ops->lookup_path = dmlite_lookup_path;
	ops->extract_handle = extract_handle;
	ops->create_handle = dmlite_create_handle;
	ops->get_fs_dynamic_info = get_dynamic_info;
	ops->fs_supports = fs_supports;
	ops->fs_maxfilesize = fs_maxfilesize;
	ops->fs_maxread = fs_maxread;
	ops->fs_maxwrite = fs_maxwrite;
	ops->fs_maxlink = fs_maxlink;
	ops->fs_maxnamelen = fs_maxnamelen;
	ops->fs_maxpathlen = fs_maxpathlen;
	ops->fs_fh_expire_type = fs_fh_expire_type;
	ops->fs_lease_time = fs_lease_time;
	ops->fs_acl_support = fs_acl_support;
	ops->fs_supported_attrs = fs_supported_attrs;
	ops->fs_umask = fs_umask;
	ops->fs_xattr_access_rights = fs_xattr_access_rights;
	ops->get_quota = get_quota;
	ops->set_quota = set_quota;
}

/* create_export
 * Create an export point and return a handle to it to be kept
 * in the export list.
 * First lookup the fsal, then create the export and then put the fsal back.
 * returns the export with one reference taken.
 */

fsal_status_t dmlite_create_export(struct fsal_module *fsal_hdl,
								   const char *export_path,
								   const char *fs_options,
								   struct exportlist__ *exp_entry,
								   struct fsal_module *next_fsal,
								   struct fsal_export **export)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct dmlite_fsal_export *dmlite_export_obj;
	struct dmlite_manager *dmlite_manager_obj;

	LogFullDebug(COMPONENT_FSAL, "dmlite_create_export: start :: %s :: %s", 
		export_path, fs_options);

	/* Validation first */
	*export = NULL; /* poison it first */
	if(export_path == NULL
	   || strlen(export_path) == 0
	   || strlen(export_path) > MAXPATHLEN) {
		LogMajor(COMPONENT_FSAL,
			 "dmlite_create_export: export path empty or too big");
		return fsalstat(ERR_FSAL_INVAL, 0);
	}
	if(next_fsal != NULL) {
		LogCrit(COMPONENT_FSAL,
			"This module is not stackable");
		return fsalstat(ERR_FSAL_INVAL, 0);
	}
	dmlite_export_obj = malloc(sizeof(struct dmlite_fsal_export));
	if(dmlite_export_obj == NULL) {
		LogMajor(COMPONENT_FSAL,
			 "dmlite_fsal_create: out of memory for object");
		return fsalstat(posix2fsal_error(errno), errno);
	}
	memset(dmlite_export_obj, 0, sizeof(struct dmlite_fsal_export));

	/* Generic export initialization method (not our own) */
	fsal_export_init(&dmlite_export_obj->export, fsal_hdl->exp_ops, exp_entry);

	/* Now we lock and get to business of creating the dmlite export */
	pthread_mutex_lock(&dmlite_export_obj->export.lock);
	// attach ourselfs to the list of exports (generic method)
	retval = fsal_attach_export(fsal_hdl, &dmlite_export_obj->export.exports);
	if(retval != 0) {
		LogMajor(COMPONENT_FSAL, "dmlite_create_export: failed to attach FSAL");
		return fsalstat(ERR_FSAL_FAULT, 0);
	}
	dmlite_export_obj->export.fsal = fsal_hdl;

	/* Initialize the dmlite manager object and load configuration */
	dmlite_manager_obj = dmlite_manager_new();
	retval = dmlite_manager_load_configuration(dmlite_manager_obj, "/etc/dmlite.conf");
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "dmlite_create_export: %s", 
			dmlite_manager_error(dmlite_manager_obj));
		fsal_error = ERR_FSAL_FAULT;
		goto errout;
	}

	/* Store the manager in the export object (we'll need it in later calls) */
	dmlite_export_obj->manager = dmlite_manager_obj;

	/* And that's it... we can wrap up */
	*export = &dmlite_export_obj->export;
	pthread_mutex_unlock(&dmlite_export_obj->export.lock);

	LogFullDebug(COMPONENT_FSAL, "dmlite_create_export: end");

	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if(dmlite_export_obj->manager != NULL)
		dmlite_manager_free(dmlite_export_obj->manager);
	dmlite_export_obj->export.ops = NULL; /* poison myself */
	pthread_mutex_unlock(&dmlite_export_obj->export.lock);
	pthread_mutex_destroy(&dmlite_export_obj->export.lock);
	free(dmlite_export_obj);  /* elvis has left the building */

	return fsalstat(fsal_error, retval);
}

