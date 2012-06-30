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
 * @file nfs4_op_readlink.c
 * @brief Routines used for managing the NFS4 COMPOUND functions.
 *
 * Routines used for managing the NFS4 COMPOUND functions.
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
#include "nfs_proto_functions.h"
#include "nfs_proto_tools.h"
#include "nfs_tools.h"
#include "nfs_file_handle.h"

/**
 * @brief The NFS4_OP_READLINK operation.
 *
 * This function implements the NFS4_OP_READLINK operation.
 *
 * @param[in]     op   Arguments for nfs4_op
 * @param[in,out] data Compound request's data
 * @param[out]    resp Results for nfs4_op
 *
 * @return per RFC5661, p. 372
 *
 * @see nfs4_Compound
 *
 */

#define arg_READLINK4 op->nfs_argop4_u.opreadlink
#define res_READLINK4 resp->nfs_resop4_u.opreadlink

int nfs4_op_readlink(struct nfs_argop4 *op,
                     compound_data_t *data,
                     struct nfs_resop4 *resp)
{
  cache_inode_status_t cache_status;
  fsal_path_t          symlink_path;

  resp->resop = NFS4_OP_READLINK;
  res_READLINK4.status = NFS4_OK;

  /*
   * Do basic checks on a filehandle You can readlink only on a link
   * ...
   */
  res_READLINK4.status = nfs4_sanity_check_FH(data, SYMBOLIC_LINK);
  if(res_READLINK4.status != NFS4_OK)
    return res_READLINK4.status;

  /* Using cache_inode_readlink */
  if(cache_inode_readlink(data->current_entry,
                          &symlink_path,
                          data->req_ctx->creds,
			  &cache_status) == CACHE_INODE_SUCCESS)
    {
      /* Alloc read link */

      if((res_READLINK4.READLINK4res_u.resok4.link.utf8string_val =
          gsh_malloc(symlink_path.len)) == NULL)
        {
          res_READLINK4.status = NFS4ERR_INVAL;
          return res_READLINK4.status;
        }

      /* convert the fsal path to a utf8 string */
      if(str2utf8(symlink_path.path, &res_READLINK4.READLINK4res_u.resok4.link)
         == -1)
        {
          res_READLINK4.status = NFS4ERR_INVAL;
          return res_READLINK4.status;
        }

      res_READLINK4.status = NFS4_OK;
      return res_READLINK4.status;
    }

  res_READLINK4.status = nfs4_Errno(cache_status);
  return res_READLINK4.status;
} /* nfs4_op_readlink */

/**
 * @brief Free memory allocated for READLINK result
 *
 * This function frees the memory allocated for the resutl of the
 * NFS4_OP_READLINK operation.
 *
 * @param[in,out] resp nfs4_op results
*/
void nfs4_op_readlink_Free(READLINK4res * resp)
{
  if(resp->status == NFS4_OK && resp->READLINK4res_u.resok4.link
     .utf8string_len > 0)
    gsh_free(resp->READLINK4res_u.resok4.link.utf8string_val);
  return;
} /* nfs4_op_readlink_Free */
