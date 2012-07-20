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
#include <FSAL/FSAL_DMLITE/fsal_handle_syscalls.h>

/*
 * DMLITE internal export
 */

struct dmlite_fsal_export {
	struct fsal_export export;
	char *mntdir;
	char *fs_spec;
	char *fstype;
	int root_fd;
	dev_t root_dev;
	struct file_handle *root_handle;
};

/* helpers to/from other DMLITE objects
 */

struct fsal_staticfsinfo_t *dmlite_staticinfo(struct fsal_module *hdl);

/* export object methods
 */

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
	if(myself->root_fd >= 0)
		close(myself->root_fd);
	if(myself->root_handle != NULL)
		free(myself->root_handle);
	if(myself->fstype != NULL)
		free(myself->fstype);
	if(myself->mntdir != NULL)
		free(myself->mntdir);
	if(myself->fs_spec != NULL)
		free(myself->fs_spec);
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
	struct dmlite_fsal_export *myself;
	struct dqblk fs_quota;
	struct stat path_stat;
	uid_t id;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int retval;

	myself = container_of(exp_hdl, struct dmlite_fsal_export, export);

        // TODO: dmlite_stat for quota (can we do this?)
	
out:
	return fsalstat(fsal_error, retval);	
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
	struct dmlite_fsal_export *myself;
	uid_t id;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int retval;

	myself = container_of(exp_hdl, struct dmlite_fsal_export, export);

        // TODO: dmlite_setquota (can we do this?)
	
err:
	return fsalstat(fsal_error, retval);	
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
	struct file_handle *hdl;
	size_t fh_size;

	/* sanity checks */
	if( !fh_desc || !fh_desc->buf)
		return fsalstat(ERR_FSAL_FAULT, 0);

	hdl = (struct file_handle *)fh_desc->buf;
	fh_size = dmlite_sizeof_handle(hdl);
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
	struct dmlite_fsal_export *myself;
	FILE *fp;
	struct mntent *p_mnt;
	size_t pathlen, outlen = 0;
	char mntdir[MAXPATHLEN];  /* there has got to be a better way... */
	char fs_spec[MAXPATHLEN];
	char type[MAXNAMLEN];
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;

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

	myself = malloc(sizeof(struct dmlite_fsal_export));
	if(myself == NULL) {
		LogMajor(COMPONENT_FSAL,
			 "dmlite_fsal_create: out of memory for object");
		return fsalstat(posix2fsal_error(errno), errno);
	}
	memset(myself, 0, sizeof(struct dmlite_fsal_export));
	myself->root_fd = -1;

        fsal_export_init(&myself->export, fsal_hdl->exp_ops, exp_entry);

	/* lock myself before attaching to the fsal.
	 * keep myself locked until done with creating myself.
	 */

	pthread_mutex_lock(&myself->export.lock);
	retval = fsal_attach_export(fsal_hdl, &myself->export.exports);
	if(retval != 0)
		goto errout; /* seriously bad */
	myself->export.fsal = fsal_hdl;

	/* start looking for the mount point */
	fp = setmntent(MOUNTED, "r");
	if(fp == NULL) {
		retval = errno;
		LogCrit(COMPONENT_FSAL,
			"Error %d in setmntent(%s): %s", retval, MOUNTED,
			strerror(retval));
		fsal_error = posix2fsal_error(retval);
		goto errout;
	}
	while((p_mnt = getmntent(fp)) != NULL) {
		if(p_mnt->mnt_dir != NULL) {
			pathlen = strlen(p_mnt->mnt_dir);
			if(pathlen > outlen) {
				if(strcmp(p_mnt->mnt_dir, "/") == 0) {
					outlen = pathlen;
					strncpy(mntdir,
						p_mnt->mnt_dir, MAXPATHLEN);
					strncpy(type,
						p_mnt->mnt_type, MAXNAMLEN);
					strncpy(fs_spec,
						p_mnt->mnt_fsname, MAXPATHLEN);
				} else if((strncmp(export_path,
						  p_mnt->mnt_dir,
						  pathlen) == 0) &&
					  ((export_path[pathlen] == '/') ||
					   (export_path[pathlen] == '\0'))) {
					if(strcasecmp(p_mnt->mnt_type, "xfs") == 0) {
						LogDebug(COMPONENT_FSAL,
							 "Mount (%s) is XFS, skipping",
							 p_mnt->mnt_dir);
						continue;
					}
					outlen = pathlen;
					strncpy(mntdir,
						p_mnt->mnt_dir, MAXPATHLEN);
					strncpy(type,
						p_mnt->mnt_type, MAXNAMLEN);
					strncpy(fs_spec,
						p_mnt->mnt_fsname, MAXPATHLEN);
				}
			}
		}
	}
	endmntent(fp);
	if(outlen <= 0) {
		LogCrit(COMPONENT_FSAL,
			"No mount entry matches '%s' in %s",
			export_path, MOUNTED);
		fsal_error = ERR_FSAL_NOENT;
		goto errout;
        }
	myself->root_fd = open(mntdir,  O_RDONLY|O_DIRECTORY);
	if(myself->root_fd < 0) {
		LogMajor(COMPONENT_FSAL,
			 "Could not open VFS mount point %s: rc = %d",
			 mntdir, errno);
		fsal_error = posix2fsal_error(errno);
		retval = errno;
		goto errout;
	} else {
		struct stat root_stat;
		int mnt_id = 0;
		struct file_handle *fh = alloca(sizeof(struct file_handle)
					       + MAX_HANDLE_SZ);

		memset(fh, 0, sizeof(struct file_handle) + MAX_HANDLE_SZ);
		fh->handle_bytes = MAX_HANDLE_SZ;
		retval = fstat(myself->root_fd, &root_stat);
		if(retval < 0) {
			LogMajor(COMPONENT_FSAL,
				 "fstat: root_path: %s, fd=%d, errno=(%d) %s",
				 mntdir, myself->root_fd, errno, strerror(errno));
			fsal_error = posix2fsal_error(errno);
			retval = errno;
			goto errout;
		}
		myself->root_dev = root_stat.st_dev;
		retval = name_to_handle_at(myself->root_fd, "", fh,
					   &mnt_id, AT_EMPTY_PATH);
		if(retval != 0) {
			LogMajor(COMPONENT_FSAL,
				 "name_to_handle: root_path: %s, root_fd=%d, errno=(%d) %s",
				 mntdir, myself->root_fd, errno, strerror(errno));
			fsal_error = posix2fsal_error(errno);
			retval = errno;
			goto errout;
		}
		myself->root_handle = malloc(sizeof(struct file_handle) + fh->handle_bytes);
		if(myself->root_handle == NULL) {
			LogMajor(COMPONENT_FSAL,
				 "memory for root handle, errno=(%d) %s",
				 errno, strerror(errno));
			fsal_error = posix2fsal_error(errno);
			retval = errno;
			goto errout;
		}
		memcpy(myself->root_handle, fh, sizeof(struct file_handle) + fh->handle_bytes);
	}
	myself->fstype = strdup(type);
	myself->fs_spec = strdup(fs_spec);
	myself->mntdir = strdup(mntdir);
	*export = &myself->export;
	pthread_mutex_unlock(&myself->export.lock);
	return fsalstat(ERR_FSAL_NO_ERROR, 0);

errout:
	if(myself->root_fd >= 0)
		close(myself->root_fd);
	if(myself->root_handle != NULL)
		free(myself->root_handle);
	if(myself->fstype != NULL)
		free(myself->fstype);
	if(myself->mntdir != NULL)
		free(myself->mntdir);
	if(myself->fs_spec != NULL)
		free(myself->fs_spec);
	myself->export.ops = NULL; /* poison myself */
	pthread_mutex_unlock(&myself->export.lock);
	pthread_mutex_destroy(&myself->export.lock);
	free(myself);  /* elvis has left the building */
	return fsalstat(fsal_error, retval);
}


