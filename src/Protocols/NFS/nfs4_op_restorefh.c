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
 * \file    nfs4_op_restorefh.c
 * \brief   Routines used for managing the NFS4_OP_RESTOREFH operation (number 31).
 *
 * Routines used for managing the NFS4_OP_RESTOREFH operation.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#endif

#include "log.h"
#include "nfs4.h"
#include "nfs_core.h"
#include "cache_inode.h"
#include "cache_inode_lru.h"
#include "nfs_exports.h"
#include "nfs_proto_functions.h"
#include "nfs_tools.h"
#include "nfs_file_handle.h"

/**
 *
 * @brief The NFS4_OP_RESTOREFH operation.
 *
 * This functions handles the NFS4_OP_RESTOREFH operation in
 * NFSv4. This function can be called only from nfs4_Compound.  This
 * operation replaces the current FH with the previously saved FH.
 *
 * @param[in]     op   Arguments for nfs4_op
 * @param[in,out] data Compound request's data
 * @param[out]    resp Results for nfs4_op
 *
 * @return per RFC5661, p. 373
 *
 * @see nfs4_Compound
 *
 */

#define arg_RESTOREFH op->nfs_argop4_u.oprestorefh
#define res_RESTOREFH resp->nfs_resop4_u.oprestorefh

int nfs4_op_restorefh(struct nfs_argop4 *op,
                      compound_data_t * data, struct nfs_resop4 *resp)
{
  /* First of all, set the reply to zero to make sure it contains no
     parasite information */
  memset(resp, 0, sizeof(struct nfs_resop4));

  resp->resop = NFS4_OP_RESTOREFH;
  res_RESTOREFH.status = NFS4_OK;

  /* If there is no currentFH, teh return an error */
  if(nfs4_Is_Fh_Empty(&(data->savedFH)))
    {
      /* There is no current FH, return NFS4ERR_RESTOREFH (cg RFC3530,
         page 202) */
      res_RESTOREFH.status = NFS4ERR_RESTOREFH;
      return res_RESTOREFH.status;
    }

  /* If the filehandle is invalid */
  if(nfs4_Is_Fh_Invalid(&(data->savedFH)))
    {
      res_RESTOREFH.status = NFS4ERR_BADHANDLE;
      return res_RESTOREFH.status;
    }

  /* Tests if the Filehandle is expired (for volatile filehandle) */
  if(nfs4_Is_Fh_Expired(&(data->savedFH)))
    {
      res_RESTOREFH.status = NFS4ERR_FHEXPIRED;
      return res_RESTOREFH.status;
    }

  /* If data->exportp is null, a junction from pseudo fs was
     traversed, credp and exportp have to be updated */
  if(data->pexport == NULL)
    {
      res_RESTOREFH.status = nfs4_SetCompoundExport(data);
      if(res_RESTOREFH.status != NFS4_OK)
        {
          LogCrit(COMPONENT_NFS_V4,
                  "Error %d in nfs4_SetCompoundExport", res_RESTOREFH.status);
          return res_RESTOREFH.status;
        }
    }

  /* Copy the data from current FH to saved FH */
  memcpy(data->currentFH.nfs_fh4_val,
         data->savedFH.nfs_fh4_val,
         data->savedFH.nfs_fh4_len);

  data->currentFH.nfs_fh4_len = data->savedFH.nfs_fh4_len;

  /* If current and saved entry are identical, get no references and
     make no changes. */

  if (data->current_entry == data->saved_entry) {
      goto out;
  }

  if (data->current_entry) {
      cache_inode_put(data->current_entry);
      data->current_entry = NULL;
  }

  data->current_entry = data->saved_entry;
  data->current_filetype = data->saved_filetype;

  /* Take another reference.  As of now the filehandle is both saved
     and current and both must be counted.  Protect in case of
     pseudofs handle. */

  if (data->current_entry) {
       if (cache_inode_lru_ref(data->current_entry,
                               LRU_FLAG_NONE) != CACHE_INODE_SUCCESS) {
            resp->nfs_resop4_u.opgetfh.status = NFS4ERR_STALE;
            data->current_entry = NULL;
            return resp->nfs_resop4_u.opgetfh.status;
       }
  }

 out:

  if(isFullDebug(COMPONENT_NFS_V4))
    {
      char str[LEN_FH_STR];
      sprint_fhandle4(str, &data->currentFH);
      LogFullDebug(COMPONENT_NFS_V4,
                   "RESTORE FH: Current FH %s", str);
    }


  return NFS4_OK;
} /* nfs4_op_restorefh */

/**
 * @brief Free memory allocated for RESTOREFH result
 *
 * This function frees any memory allocated for the result of the
 * NFS4_OP_RESTOREFH operation.
 *
 * @param[in,out] resp nfs4_op results
 */
void nfs4_op_restorefh_Free(RESTOREFH4res *resp)
{
  /* Nothing to be done */
  return;
} /* nfs4_op_restorefh_Free */
