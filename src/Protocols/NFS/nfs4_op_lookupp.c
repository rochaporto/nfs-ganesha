/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright CEA/DAM/DIF  (2008)
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *                Thomas LEIBOVICI  thomas.leibovici@cea.fr
 *
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
 * ---------------------------------------
 */

/**
 * @file    nfs4_op_lookupp.c
 * @brief   Routines used for managing the NFS4 COMPOUND functions.
 *
 * Routines used for managing the NFS4 COMPOUND functions.
 *
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#endif

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include "HashData.h"
#include "HashTable.h"
#include "log.h"
#include "ganesha_rpc.h"
#include "nfs4.h"
#include "nfs_core.h"
#include "cache_inode.h"
#include "nfs_exports.h"
#include "nfs_creds.h"
#include "nfs_proto_functions.h"
#include "nfs_proto_tools.h"
#include "nfs_tools.h"
#include "nfs_file_handle.h"

/**
 * @brief NFS4_OP_LOOKUPP
 *
 * This function implements the NFS4_OP_LOOKUPP operation, which looks
 * up the parent of the supplied directory.
 *
 * @param[in]     op   Arguments for nfs4_op
 * @param[in,out] data Compound request's data
 * @param[out]    resp Results for nfs4_op
 *
 * @return per RFC5661, p. 369
 *
 */
#define arg_LOOKUPP4 op->nfs_argop4_u.oplookupp
#define res_LOOKUPP4 resp->nfs_resop4_u.oplookupp

int nfs4_op_lookupp(struct nfs_argop4 *op,
                    compound_data_t * data, struct nfs_resop4 *resp)
{
  fsal_name_t            name;
  cache_entry_t        * dir_entry = NULL;
  cache_entry_t        * file_entry = NULL;
  fsal_attrib_list_t     attrlookup;
  cache_inode_status_t   cache_status;
  int                    error = 0;

  resp->resop = NFS4_OP_LOOKUPP;
  res_LOOKUPP4.status = NFS4_OK;

  /* Do basic checks on a filehandle */
  res_LOOKUPP4.status = nfs4_sanity_check_FH(data, 0LL);
  if(res_LOOKUPP4.status != NFS4_OK)
    return res_LOOKUPP4.status;

  /* looking up for parent directory from ROOTFH return NFS4ERR_NOENT (RFC3530, page 166) */
  if(data->currentFH.nfs_fh4_len == data->rootFH.nfs_fh4_len
     && memcmp(data->currentFH.nfs_fh4_val, data->rootFH.nfs_fh4_val,
               data->currentFH.nfs_fh4_len) == 0)
    {
      /* Nothing to do, just reply with success */
      res_LOOKUPP4.status = NFS4ERR_NOENT;
      return res_LOOKUPP4.status;
    }

  /* If in pseudoFS, proceed with pseudoFS specific functions */
  if(nfs4_Is_Fh_Pseudo(&(data->currentFH)))
    return nfs4_op_lookupp_pseudo(op, data, resp);

  /* If Filehandle points to a xattr object, manage it via the xattrs specific functions */
  if(nfs4_Is_Fh_Xattr(&(data->currentFH)))
    return nfs4_op_lookupp_xattr(op, data, resp);

  /* If data->exportp is null, a junction from pseudo fs was traversed, credp and exportp have to be updated */
  if(data->pexport == NULL)
    {
      if((error = nfs4_SetCompoundExport(data)) != NFS4_OK)
        {
          res_LOOKUPP4.status = error;
          return res_LOOKUPP4.status;
        }
    }

  /* Preparying for cache_inode_lookup ".." */
  file_entry = NULL;
  dir_entry = data->current_entry;
  name = FSAL_DOT_DOT;

  /* BUGAZOMEU: Faire la gestion des cross junction traverse */
  if((file_entry
      = cache_inode_lookup(dir_entry,
                           &name,
                           &attrlookup,
                           &data->user_credentials, &cache_status)) != NULL)
    {
      /* Convert it to a file handle */
      if(!nfs4_FSALToFhandle(&data->currentFH, file_entry->obj_handle, data))
        {
          res_LOOKUPP4.status = NFS4ERR_SERVERFAULT;
          cache_inode_put(file_entry);
          return res_LOOKUPP4.status;
        }

      /* Copy this to the mounted on FH (if no junction is traversed */
      memcpy(data->mounted_on_FH.nfs_fh4_val,
             data->currentFH.nfs_fh4_val,
             data->currentFH.nfs_fh4_len);
      data->mounted_on_FH.nfs_fh4_len = data->currentFH.nfs_fh4_len;

      /* Release dir_pentry, as it is not reachable from anywhere in
         compound after this function returns.  Count on later
         operations or nfs4_Compound to clean up current_entry. */

      if (dir_entry)
        cache_inode_put(dir_entry);

      /* Keep the pointer within the compound data */
      data->current_entry = file_entry;
      data->current_filetype = file_entry->type;

      /* Return successfully */
      res_LOOKUPP4.status = NFS4_OK;
      return NFS4_OK;

    }

  /* If the part of the code is reached, then something wrong occured in the
   * lookup process, status is not HPSS_E_NOERROR and contains the code for
   * the error */

  /* For any wrong file type LOOKUPP should return NFS4ERR_NOTDIR */
  res_LOOKUPP4.status = nfs4_Errno(cache_status);

  return res_LOOKUPP4.status;
}                               /* nfs4_op_lookupp */

/**
 * @brief Free memory allocated for LOOKUPP result
 *
 * This function frees any memory allocated for the result of the
 * NFS4_OP_LOOKUPP operation.
 *
 * @param[in,out] resp nfs4_op results
 */
void nfs4_op_lookupp_Free(LOOKUPP4res *resp)
{
  /* Nothing to be done */
  return;
} /* nfs4_op_lookupp_Free */
