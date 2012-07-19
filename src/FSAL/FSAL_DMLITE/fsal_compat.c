/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) 2012, CERN IT/GT/DMS <it-dep-gt-dms@cern.ch>
 *
 * Some portions Copyright CEA/DAM/DIF  (2008)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_types.h"
#include "fsal_glue.h"
#include "fsal_internal.h"
#include "FSAL/common_methods.h"

fsal_functions_t fsal_dmlite_functions = {
  .fsal_access = DMLITEFSAL_access,
  .fsal_getattrs = DMLITEFSAL_getattrs,
  .fsal_setattrs = DMLITEFSAL_setattrs,
  .fsal_buildexportcontext = DMLITEFSAL_BuildExportContext,
  .fsal_cleanupexportcontext = DMLITEFSAL_CleanUpExportContext,
  .fsal_initclientcontext = COMMON_InitClientContext,
  .fsal_getclientcontext = COMMON_GetClientContext,
  .fsal_create = DMLITEFSAL_create,
  .fsal_mkdir = DMLITEFSAL_mkdir,
  .fsal_link = DMLITEFSAL_link,
  .fsal_mknode = DMLITEFSAL_mknode,
  .fsal_opendir = DMLITEFSAL_opendir,
  .fsal_readdir = DMLITEFSAL_readdir,
  .fsal_closedir = DMLITEFSAL_closedir,
  .fsal_open_by_name = DMLITEFSAL_open_by_name,
  .fsal_open = DMLITEFSAL_open,
  .fsal_read = DMLITEFSAL_read,
  .fsal_write = DMLITEFSAL_write,
  .fsal_commit = DMLITEFSAL_commit,
  .fsal_close = DMLITEFSAL_close,
  .fsal_open_by_fileid = COMMON_open_by_fileid,
  .fsal_close_by_fileid = COMMON_close_by_fileid,
  .fsal_dynamic_fsinfo = DMLITEFSAL_dynamic_fsinfo,
  .fsal_init = DMLITEFSAL_Init,
  .fsal_terminate = DMLITEFSAL_terminate,
  .fsal_test_access = DMLITEFSAL_test_access,
  .fsal_setattr_access = COMMON_setattr_access_notsupp,
  .fsal_rename_access = COMMON_rename_access,
  .fsal_create_access = COMMON_create_access,
  .fsal_unlink_access = COMMON_unlink_access,
  .fsal_link_access = COMMON_link_access,
  .fsal_merge_attrs = COMMON_merge_attrs,
  .fsal_lookup = DMLITEFSAL_lookup,
  .fsal_lookuppath = DMLITEFSAL_lookupPath,
  .fsal_lookupjunction = DMLITEFSAL_lookupJunction,
  .fsal_cleanobjectresources = COMMON_CleanObjectResources,
  .fsal_set_quota = COMMON_set_quota_noquota,
  .fsal_get_quota = COMMON_get_quota_noquota,
  .fsal_check_quota = COMMON_check_quota,
  .fsal_rcp = DMLITEFSAL_rcp,
  .fsal_rename = DMLITEFSAL_rename,
  .fsal_get_stats = DMLITEFSAL_get_stats,
  .fsal_readlink = DMLITEFSAL_readlink,
  .fsal_symlink = DMLITEFSAL_symlink,
  .fsal_handlecmp = DMLITEFSAL_handlecmp,
  .fsal_handle_to_hashindex = DMLITEFSAL_Handle_to_HashIndex,
  .fsal_handle_to_rbtindex = DMLITEFSAL_Handle_to_RBTIndex,
  .fsal_handle_to_hash_both = NULL,
  .fsal_digesthandle = DMLITEFSAL_DigestHandle,
  .fsal_expandhandle = DMLITEFSAL_ExpandHandle,
  .fsal_setdefault_fsal_parameter = DMLITEFSAL_SetDefault_FSAL_parameter,
  .fsal_setdefault_fs_common_parameter
  = DMLITEFSAL_SetDefault_FS_common_parameter,
  .fsal_setdefault_fs_specific_parameter
  = DMLITEFSAL_SetDefault_FS_specific_parameter,
  .fsal_load_fsal_parameter_from_conf
  = DMLITEFSAL_load_FSAL_parameter_from_conf,
  .fsal_load_fs_common_parameter_from_conf
  = DMLITEFSAL_load_FS_common_parameter_from_conf,
  .fsal_load_fs_specific_parameter_from_conf
  = DMLITEFSAL_load_FS_specific_parameter_from_conf,
  .fsal_truncate = DMLITEFSAL_truncate,
  .fsal_unlink = DMLITEFSAL_unlink,
  .fsal_getfsname = DMLITEFSAL_GetFSName,
  .fsal_getxattrattrs = DMLITEFSAL_GetXAttrAttrs,
  .fsal_listxattrs = DMLITEFSAL_ListXAttrs,
  .fsal_getxattrvaluebyid = DMLITEFSAL_GetXAttrValueById,
  .fsal_getxattridbyname = DMLITEFSAL_GetXAttrIdByName,
  .fsal_getxattrvaluebyname = DMLITEFSAL_GetXAttrValueByName,
  .fsal_setxattrvalue = DMLITEFSAL_SetXAttrValue,
  .fsal_setxattrvaluebyid = DMLITEFSAL_SetXAttrValueById,
  .fsal_removexattrbyid = DMLITEFSAL_RemoveXAttrById,
  .fsal_removexattrbyname = DMLITEFSAL_RemoveXAttrByName,
  .fsal_getextattrs = DMLITEFSAL_getextattrs,
  .fsal_getfileno = DMLITEFSAL_GetFileno,
  .fsal_share_op = COMMON_share_op_notsupp
};

fsal_const_t fsal_dmlite_consts = {
  .fsal_handle_t_size = sizeof(dmlitefsal_handle_t),
  .fsal_op_context_t_size = sizeof(dmlitefsal_op_context_t),
  .fsal_export_context_t_size = sizeof(dmlitefsal_export_context_t),
  .fsal_file_t_size = sizeof(dmlitefsal_file_t),
  .fsal_cookie_t_size = sizeof(dmlitefsal_cookie_t),
  .fsal_cred_t_size = sizeof(struct user_credentials),
  .fs_specific_initinfo_t_size = sizeof(dmlitefs_specific_initinfo_t),
  .fsal_dir_t_size = sizeof(dmlitefsal_dir_t)
};

fsal_functions_t FSAL_GetFunctions(void)
{
  return fsal_dmlite_functions;
} /* FSAL_GetFunctions */

fsal_const_t FSAL_GetConsts(void)
{
  return fsal_dmlite_consts;
}                               /* FSAL_GetConsts */
