/*
 * Copyright © 2012, CohortFS, LLC.
 * Author: Adam C. Emerson <aemerson@linuxbox.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/**
 * @file   export.c
 * @author Adam C. Emerson <aemerson@linuxbox.com>
 * @date   Thu Jul  5 16:37:47 2012
 *
 * @brief Implementation of FSAL export functions for Ceph
 *
 * This file implements the Ceph specific functionality for the FSAL
 * export handle.
 */

#include <limits.h>
#include <stdint.h>
#include <sys/statvfs.h>
#include <cephfs/libcephfs.h>
#include "abstract_mem.h"
#include "fsal.h"
#include "fsal_types.h"
#include "fsal_api.h"
#include "FSAL/fsal_commonlib.h"
#include "internal.h"

/**
 * @brief Clean up an export
 *
 * This function cleans up an export after the last reference is
 * released.
 *
 * @param[in,out] export The export to be released
 *
 * @retval ERR_FSAL_NO_ERROR on success.
 * @retval ERR_FSAL_BUSY if the export is in use.
 */

static fsal_status_t
release(struct fsal_export *pub_export)
{
        /* The priate, expanded export */
        struct export *export
                = container_of(pub_export, struct export, export);
        /* Return code */
        fsal_status_t status = {ERR_FSAL_INVAL, 0};

        pthread_mutex_lock(&export->export.lock);
        if((export->export.refs > 0) ||
           (!glist_empty(&export->export.handles))) {
                pthread_mutex_lock(&export->export.lock);
                status.major = ERR_FSAL_INVAL;
                return status;
        }
        fsal_detach_export(export->export.fsal, &export->export.exports);
        pthread_mutex_unlock(&export->export.lock);

        export->export.ops = NULL;
        ceph_shutdown(export->cmount);
        export->cmount = NULL;
        pthread_mutex_destroy(&export->export.lock);
        gsh_free(export);
        export = NULL;

        return status;
}

/**
 * @brief Return a handle corresponding to a path
 *
 * This function looks up the given path and supplies an FSAL object
 * handle.  Because the root path specified for the export is a Ceph
 * style root as supplied to mount -t ceph of ceph-fuse (of the form
 * host:/path), we check to see if the path begins with / and, if not,
 * skip until we find one.
 *
 * @param[in]  pub_export The export in which to look up the file
 * @param[in]  path       The path to look up
 * @param[out] pub_handle The created public FSAL handle
 *
 * @return FSAL status.
 */

static fsal_status_t
lookup_path(struct fsal_export *pub_export,
            const char *path,
            struct fsal_obj_handle **pub_handle)
{
        /* The 'private' full export handle */
        struct export *export = container_of(pub_export,
                                             struct export,
                                             export);
        /* The 'private' full object handle */
        struct handle *handle = NULL;
        /* The buffer in which to store stat info */
        struct stat st;
        /* FSAL status structure */
        fsal_status_t status = {ERR_FSAL_NO_ERROR, 0};
        /* Return code from Ceph */
        int rc = 0;
        /* Find the actual path in the supplied path */
        const char *realpath;

        if (*path != '/') {
                realpath = strchr(path, ':');
                if (realpath == NULL) {
                        status.major = ERR_FSAL_INVAL;
                        goto out;
                }
                if (*(++realpath) != '/') {
                        status.major = ERR_FSAL_INVAL;
                        goto out;
                }
        } else {
                realpath = path;
        }

        *pub_handle = NULL;

        if (strcmp(realpath, "/") == 0) {
                vinodeno_t root;
                root.ino.val = CEPH_INO_ROOT;
                root.snapid.val = CEPH_NOSNAP;
                rc = ceph_ll_getattr(export->cmount, root, &st, 0, 0);
        } else {
                rc = ceph_ll_walk(export->cmount, realpath, &st);
        }
        if (rc < 0) {
                status = ceph2fsal_error(rc);
                goto out;
        }

        rc = construct_handle(&st, export, &handle);
        if (rc < 0) {
                status = ceph2fsal_error(rc);
                goto out;
        }

        *pub_handle = &handle->handle;

out:
        return status;
}

/**
 * @brief Extract handle from buffer
 *
 * This function, in the Ceph FSAL, merely checks that the supplied
 * buffer is the appropriate size, or returns the size of the wire
 * handle if FSAL_DIGEST_SIZEOF is passed as the type.
 *
 * @param[in]     export_pub Public export
 * @param[in]     type       The type of digest this buffer represents
 * @param[in,out] fh_desc    The buffer from which to extract/buffer
 *                           containing extracted handle
 *
 * @return FSAL status.
 */

static fsal_status_t
extract_handle(struct fsal_export *export_pub,
               fsal_digesttype_t type,
               struct netbuf *fh_desc)
{
        if (type == FSAL_DIGEST_SIZEOF) {
                fh_desc->len = sizeof(struct wire_handle);
                return fsalstat(ERR_FSAL_NO_ERROR, 0);
        } else if (fh_desc->len != sizeof(struct wire_handle)) {
                return fsalstat(ERR_FSAL_SERVERFAULT, 0);
        } else {
                return fsalstat(ERR_FSAL_NO_ERROR, 0);
        }
}

/**
 * @brief Create a handle object from a wire handle
 *
 * The wire handle is given in a buffer outlined by desc, which it
 * looks like we shouldn't modify.
 *
 * @param[in]  pub_export Public export
 * @param[in]  desc       Handle buffer descriptor
 * @param[out] pub_handle The created handle
 *
 * @return FSAL status.
 */
static fsal_status_t
create_handle(struct fsal_export *pub_export,
              struct gsh_buffdesc *desc,
              struct fsal_obj_handle **pub_handle)
{
        /* Full 'private' export structure */
        struct export *export = container_of(pub_export,
                                             struct export,
                                             export);
        /* FSAL status to return */
        fsal_status_t status = {ERR_FSAL_NO_ERROR, 0};
        /* The FSAL specific portion of the handle received by the
           client */
        struct wire_handle *wire = desc->addr;
        /* Ceph return code */
        int rc = 0;
        /* Stat buffer */
        struct stat st;
        /* Handle to be created */
        struct handle *handle = NULL;

        *pub_handle = NULL;

        if (desc->len != sizeof(struct wire_handle)) {
                status.major = ERR_FSAL_INVAL;
                goto out;
        }

        rc = ceph_ll_connectable_m(export->cmount, &wire->vi,
                                   wire->parent_ino,
                                   wire->parent_hash);
        if (rc < 0) {
                return ceph2fsal_error(rc);
        }

        /* The ceph_ll_connectable_m should have populated libceph's
           cache with all this anyway */
        rc = ceph_ll_getattr(export->cmount, wire->vi, &st, 0, 0);
        if (rc < 0) {
                return ceph2fsal_error(rc);
        }

        rc = construct_handle(&st, export, &handle);
        if (rc < 0) {
                status = ceph2fsal_error(rc);
                goto out;
        }

        *pub_handle = &handle->handle;

out:
        return status;
}

/**
 * @brief Get dynamic filesystem info
 *
 * This function returns dynamic filesystem information for the given
 * export.
 *
 * @param[in]  pub_export The public export handle
 * @param[out] info       The dynamic FS information
 *
 * @return FSAL status.
 */

static fsal_status_t
get_fs_dynamic_info(struct fsal_export *pub_export,
                    fsal_dynamicfsinfo_t *info)
{
        /* Full 'private' export */
        struct export* export
                = container_of(pub_export, struct export, export);
        /* Return value from Ceph calls */
        int rc = 0;
        /* Filesystem stat */
        struct statvfs vfs_st;
        /* The root of whatever filesystem this is */
        vinodeno_t root;

        root.ino.val = CEPH_INO_ROOT;
        root.snapid.val = CEPH_NOSNAP;

        rc = ceph_ll_statfs(export->cmount, root, &vfs_st);

        if (rc < 0) {
                return ceph2fsal_error(rc);
        }

        memset(info, 0, sizeof(fsal_dynamicfsinfo_t));
        info->total_bytes = vfs_st.f_frsize * vfs_st.f_blocks;
        info->free_bytes = vfs_st.f_frsize * vfs_st.f_bfree;
        info->avail_bytes = vfs_st.f_frsize * vfs_st.f_bavail;
        info->total_files = vfs_st.f_files;
        info->free_files = vfs_st.f_ffree;
        info->avail_files = vfs_st.f_favail;
        info->time_delta.seconds = 1;
        info->time_delta.nseconds = 0;

        return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/**
 * @brief Query the FSAL's capabilities
 *
 * This function queries the capabilities of an FSAL export.
 *
 * @param[in] pub_export The public export handle
 * @param[in] option     The option to check
 *
 * @retval TRUE if the option is supported.
 * @retval FALSE if the option is unsupported (or unknown).
 */

static bool_t
fs_supports(struct fsal_export *pub_export,
            fsal_fsinfo_options_t option)
{
        switch (option) {
        case no_trunc:
                return TRUE;

        case chown_restricted:
                return TRUE;

        case case_insensitive:
                return FALSE;


        case case_preserving:
                return TRUE;

        case link_support:
                return TRUE;

        case symlink_support:
                return TRUE;

        case lock_support:
                return FALSE;

        case lock_support_owner:
                return FALSE;

        case lock_support_async_block:
                return FALSE;

        case named_attr:
                return FALSE;

        case unique_handles:
                return TRUE;

        case cansettime:
                return TRUE;

        case homogenous:
                return TRUE;

        case auth_exportpath_xdev:
                return FALSE;

        case dirs_have_sticky_bit:
                return TRUE;

        case accesscheck_support:
                return FALSE;

        case share_support:
                return FALSE;

        case share_support_owner:
                return FALSE;

        case pnfs_supported:
                return FALSE;
        }

        return FALSE;
}

/**
 * @brief Return the longest file supported
 *
 * This function returns the length of the longest file supported.
 *
 * @param[in] pub_export The public export
 *
 * @return UINT64_MAX.
 */

static uint64_t
fs_maxfilesize(struct fsal_export *pub_export)
{
        return UINT64_MAX;
}

/**
 * @brief Return the longest read supported
 *
 * This function returns the length of the longest read supported.
 *
 * @param[in] pub_export The public export
 *
 * @return 4 mebibytes.
 */

static uint32_t
fs_maxread(struct fsal_export *pub_export)
{
        return 0x400000;
}

/**
 * @brief Return the longest write supported
 *
 * This function returns the length of the longest write supported.
 *
 * @param[in] pub_export The public export
 *
 * @return 4 mebibytes.
 */

static uint32_t
fs_maxwrite(struct fsal_export *pub_export)
{
        return 0x400000;
}

/**
 * @brief Return the maximum number of hard links to a file
 *
 * This function returns the maximum number of hard links supported to
 * any file.
 *
 * @param[in] pub_export The public export
 *
 * @return 1024.
 */

static uint32_t
fs_maxlink(struct fsal_export *pub_export)
{
        /* Ceph does not like hard links.  See the anchor table
           design.  We should fix this, but have to do it in the Ceph
           core. */
        return 1024;
}

/**
 * @brief Return the maximum size of a Ceph filename
 *
 * This function returns the maximum filename length.
 *
 * @param[in] pub_export The public export
 *
 * @return UINT32_MAX.
 */

static uint32_t
fs_maxnamelen(struct fsal_export *pub_export)
{
        /* Ceph actually supports filenames of unlimited length, at
           least according to the protocol docs.  We may wish to
           constrain this later. */
        return UINT32_MAX;
}

/**
 * @brief Return the maximum length of a Ceph path
 *
 * This function returns the maximum path length.
 *
 * @param[in] pub_export The public export
 *
 * @return UINT32_MAX.
 */

static uint32_t
fs_maxpathlen(struct fsal_export *pub_export)
{
        /* Similarly unlimited int he protocol */
        return UINT32_MAX;
}

/**
 * @brief Return the FH expiry type
 *
 * This function returns the FH expiry type.
 *
 * @param[in] pub_export The public export
 *
 * @return FSAL_EXPTYPE_PERSISTENT
 */

static fsal_fhexptype_t
fs_fh_expire_type(struct fsal_export *pub_export)
{
        return FSAL_EXPTYPE_PERSISTENT;
}

/**
 * @brief Return the lease time
 *
 * This function returns the lease time.
 *
 * @param[in] pub_export The public export
 *
 * @return five minutes.
 */

static gsh_time_t
fs_lease_time(struct fsal_export *pub_export)
{
        gsh_time_t lease = {300, 0};

        return lease;
}

/**
 * @brief Return ACL support
 *
 * This function returns the export's ACL support.
 *
 * @param[in] pub_export The public export
 *
 * @return FSAL_ACLSUPPORT_DENY.
 */

static fsal_aclsupp_t
fs_acl_support(struct fsal_export *pub_export)
{
        return FSAL_ACLSUPPORT_DENY;
}

/**
 * @brief Return the attributes supported by this FSAL
 *
 * This function returns the mask of attributes this FSAL can support.
 *
 * @param[in] pub_export The public export
 *
 * @return supported_attributes as defined in internal.c.
 */

static attrmask_t
fs_supported_attrs(struct fsal_export *exp_hdl)
{
        return supported_attributes;
}

/**
 * @brief Return the mode under which the FSAL will create files
 *
 * This function returns the default mode on any new file created.
 *
 * @param[in] pub_export The public export
 *
 * @return 0600.
 */

static uint32_t
fs_umask(struct fsal_export *pub_export)
{
        return 0600;
}

/**
 * @brief Return the mode for extended attributes
 *
 * This function returns the access mode applied to extended
 * attributes.  This seems a bit dubious
 *
 * @param[in] pub_export The public export
 *
 * @return 0644.
 */

static uint32_t
fs_xattr_access_rights(struct fsal_export *pub_export)
{
        return 0644;
}

/**
 * @brief Set operations for exports
 *
 * This function overrides operations that we've implemented, leaving
 * the rest for the default.
 *
 * @param[in,out] ops Operations vector
 */

void
export_ops_init(struct export_ops *ops)
{
        ops->release = release;
        ops->lookup_path = lookup_path;
        ops->extract_handle = extract_handle;
        ops->create_handle = create_handle;
        ops->get_fs_dynamic_info = get_fs_dynamic_info;
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
}

