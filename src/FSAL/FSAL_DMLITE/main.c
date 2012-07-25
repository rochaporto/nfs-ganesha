/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CERN IT/GT/DMS <it-dep-gt-dms@cern.ch> 2012
 *
 * Based on the VFS FSAL from Panasas Inc. / Jim Lieb <jlieb@panasas.com>.

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

/* main.c
 * Module core functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include <libgen.h>             /* used for 'dirname' */
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include "nlm_list.h"
#include "fsal_internal.h"
#include "dmlite_methods.h"
#include "FSAL/fsal_init.h"

/* DMLITE FSAL module private storage
 */

struct dmlite_fsal_module {	
	struct fsal_module fsal;
	struct fsal_staticfsinfo_t fs_info;
	fsal_init_info_t fsal_info;
	 /* dmlitefs_specific_initinfo_t specific_info;  placeholder */
};

const char myname[] = "DMLITE";

/* filesystem info for DMLITE */
static struct fsal_staticfsinfo_t default_posix_info = {
	.maxfilesize = 0xFFFFFFFFFFFFFFFFLL, /* (64bits) */
	.maxlink = _POSIX_LINK_MAX,
	.maxnamelen = 1024,
	.maxpathlen = 1024,
	.no_trunc = TRUE,
	.chown_restricted = TRUE,
	.case_insensitive = FALSE,
	.case_preserving = TRUE,
	.fh_expire_type = FSAL_EXPTYPE_PERSISTENT,
	.link_support = TRUE,
	.symlink_support = TRUE,
	.lock_support = TRUE,
	.lock_support_owner = FALSE,
	.lock_support_async_block = FALSE,
	.named_attr = TRUE,
	.unique_handles = TRUE,
	.lease_time = {10, 0},
	.acl_support = FSAL_ACLSUPPORT_ALLOW,
	.cansettime = TRUE,
	.homogenous = TRUE,
	.supported_attrs = DMLITE_SUPPORTED_ATTRIBUTES,
	.maxread = 0,
	.maxwrite = 0,
	.umask = 0,
	.auth_exportpath_xdev = FALSE,
	.xattr_access_rights = 0400, /* root=RW, owner=R */
	.dirs_have_sticky_bit = TRUE
};

/* private helper for export object
 */

struct fsal_staticfsinfo_t *dmlite_staticinfo(struct fsal_module *hdl)
{
	struct dmlite_fsal_module *myself;

	myself = container_of(hdl, struct dmlite_fsal_module, fsal);
	return &myself->fs_info;
}

/* Module methods
 */

/* init_config
 * must be called with a reference taken (via lookup_fsal)
 */

static fsal_status_t init_config(struct fsal_module *fsal_hdl,
				 config_file_t config_struct)
{
	struct dmlite_fsal_module *dmlite_me
		= container_of(fsal_hdl, struct dmlite_fsal_module, fsal);
	fsal_status_t fsal_status;

	dmlite_me->fs_info = default_posix_info; /* get a copy of the defaults */

        fsal_status = fsal_load_config(fsal_hdl->ops->get_name(fsal_hdl),
                                       config_struct,
                                       &dmlite_me->fsal_info,
                                       &dmlite_me->fs_info,
                                       NULL);

	if(FSAL_IS_ERROR(fsal_status))
		return fsal_status;
	/* if we have fsal specific params, do them here
	 * fsal_hdl->name is used to find the block containing the
	 * params.
	 */

	display_fsinfo(&dmlite_me->fs_info);
	LogFullDebug(COMPONENT_FSAL,
		     "Supported attributes constant = 0x%"PRIx64,
                     (uint64_t) DMLITE_SUPPORTED_ATTRIBUTES);
	LogFullDebug(COMPONENT_FSAL,
		     "Supported attributes default = 0x%"PRIx64,
		     default_posix_info.supported_attrs);
	LogDebug(COMPONENT_FSAL,
		 "FSAL INIT: Supported attributes mask = 0x%"PRIx64,
		 dmlite_me->fs_info.supported_attrs);
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* Internal DMLITE method linkage to export object
 */

fsal_status_t dmlite_create_export(struct fsal_module *fsal_hdl,
				const char *export_path,
				const char *fs_options,
				struct exportlist__ *exp_entry,
				struct fsal_module *next_fsal,
				struct fsal_export **export);

/* Module initialization.
 * Called by dlopen() to register the module
 * keep a private pointer to me in myself
 */

/* my module private storage
 */

static struct dmlite_fsal_module DMLITE;

/* linkage to the exports and handle ops initializers
 */

void dmlite_export_ops_init(struct export_ops *ops);
void dmlite_handle_ops_init(struct fsal_obj_ops *ops);

MODULE_INIT void dmlite_init(void) {
	int retval;
	struct fsal_module *myself = &DMLITE.fsal;

	retval = register_fsal(myself, myname,
			       FSAL_MAJOR_VERSION,
			       FSAL_MINOR_VERSION);
	if(retval != 0) {
		fprintf(stderr, "DMLITE module failed to register");
		return;
	}
	myself->ops->create_export = dmlite_create_export;
	myself->ops->init_config = init_config;
	dmlite_export_ops_init(myself->exp_ops);
	dmlite_handle_ops_init(myself->obj_ops);
        init_fsal_parameters(&DMLITE.fsal_info);
}

MODULE_FINI void dmlite_unload(void) {
	int retval;

	retval = unregister_fsal(&DMLITE.fsal);
	if(retval != 0) {
		fprintf(stderr, "VFS module failed to unregister");
		return;
	}
}
