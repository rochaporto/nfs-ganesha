/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) 2012, CERN IT/GT/DMS <it-dep-gt-dms@cern.ch>
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
 * ---------------------------------------
 */

/**
 * \file    fsal_mds.c
 * \brief   MDS realisation for the filesystem abstraction
 *
 * fsal_mds.c: MDS realisation for the filesystem abstraction
 *             Obviously, all of these functions should dispatch
 *             on type if more than one layout type is supported.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"
#include "nfsv41.h"
#include <fcntl.h>
#include "HashTable.h"
#include <pthread.h>
#include <stdint.h>
#include "fsal_types.h"
#include "fsal_pnfs.h"
#include "pnfs_common.h"
#include "fsal_pnfs_files.h"

const size_t BIGGEST_PATTERN = 1024; /* Linux supports a stripe
                                        pattern with no more than 4096
                                        stripes, but for now we stick
                                        to 1024 to keep them da_addrs
                                        from being too gigantic. */



nfsstat4
DMLITEFSAL_layoutget(fsal_handle_t *exthandle,
                   fsal_op_context_t *extcontext,
                   XDR *loc_body,
                   const struct fsal_layoutget_arg *arg,
                   struct fsal_layoutget_res *res)
{
     /* The FSAL handle as defined for the CEPH FSAL */
     dmlitefsal_handle_t* handle = (dmlitefsal_handle_t*) exthandle;
     /* The FSAL operation context as defined for the CEPH FSAL */
     dmlitefsal_op_context_t* context = (dmlitefsal_op_context_t*) extcontext;
     /* Width of each stripe on the file */
     uint32_t stripe_width = 0;
     /* Utility parameter */
     nfl_util4 util = 0;
     /* The last byte that can be accessed through pNFS */
     uint64_t last_possible_byte = 0;
     /* The deviceid for this layout */
     struct pnfs_deviceid deviceid = {0, 0};
     /* NFS Status */
     nfsstat4 nfs_status = 0;

     /* We support only LAYOUT4_NFSV4_1_FILES layouts */

     if (arg->type != LAYOUT4_NFSV4_1_FILES) {
          LogCrit(COMPONENT_PNFS,
                  "Unsupported layout type: %x",
                  arg->type);
          return NFS4ERR_UNKNOWN_LAYOUTTYPE;
     }

     // dmlite_layoutget

     return NFS4_OK;
}

nfsstat4
DMLITEFSAL_layoutreturn(fsal_handle_t* handle,
                      fsal_op_context_t* context,
                      XDR *lrf_body,
                      const struct fsal_layoutreturn_arg *arg)

{
     /* Sanity check on type */
     if (arg->lo_type != LAYOUT4_NFSV4_1_FILES) {
          LogCrit(COMPONENT_PNFS,
                  "Unsupported layout type: %x",
                  arg->lo_type);
          return NFS4ERR_UNKNOWN_LAYOUTTYPE;
     }

     /* Since we no longer store DS addresses, we no longer have
        anything to free.  Later on we should unravel the Ceph client
        a bit more and coordinate with the Ceph MDS's notion of read
        and write pins, but that isn't germane until we have
        LAYOUTRECALL. */

     return NFS4_OK;
}

nfsstat4
DMLITEFSAL_layoutcommit(fsal_handle_t *exthandle,
                      fsal_op_context_t *extcontext,
                      XDR *lou_body,
                      const struct fsal_layoutcommit_arg *arg,
                      struct fsal_layoutcommit_res *res)
{
     /* Filehandle for calls */
     dmlitefsal_handle_t* handle = (dmlitefsal_handle_t*) exthandle;
     /* Operation context */
     dmlitefsal_op_context_t* context = (dmlitefsal_op_context_t*) extcontext;
     /* User ID and group ID for permissions */
     int uid = FSAL_OP_CONTEXT_TO_UID(context);
     int gid = FSAL_OP_CONTEXT_TO_GID(context);
     /* Old stat, so we don't truncate file or reverse time */
     struct stat stold;
     /* new stat to set time and size */
     struct stat stnew;
     /* Mask to determine exactly what gets set */
     int attrmask = 0;

     /* Sanity check on type */
     if (arg->type != LAYOUT4_NFSV4_1_FILES) {
          LogCrit(COMPONENT_PNFS,
                  "Unsupported layout type: %x",
                  arg->type);
          return NFS4ERR_UNKNOWN_LAYOUTTYPE;
     }

     // TODO: dmlite_layoutcommit
     return NFS4_OK;
}

nfsstat4
DMLITEFSAL_getdeviceinfo(fsal_op_context_t *extcontext,
                       XDR* da_addr_body,
                       layouttype4 type,
                       const struct pnfs_deviceid *deviceid)
{
     /* Operation context */
     dmlitefsal_op_context_t* context = (dmlitefsal_op_context_t*) extcontext;
     /* Minimal information needed to get layout info */
     vinodeno_t vinode;
     /* Currently, all layouts have the same number of stripes */
     uint32_t stripes = BIGGEST_PATTERN;
     /* Index for iterating over stripes */
     size_t stripe  = 0;
     /* Index for iterating over OSDs */
     size_t osd = 0;
     /* NFSv4 status code */
     nfsstat4 nfs_status = 0;

     /* Sanity check on type */
     if (type != LAYOUT4_NFSV4_1_FILES) {
          LogCrit(COMPONENT_PNFS,
                  "Unsupported layout type: %x",
                  type);
          return NFS4ERR_UNKNOWN_LAYOUTTYPE;
     }

     // TODO: dmlite_getdeviceinfo
     return NFS4_OK;
}

nfsstat4
DMLITEFSAL_getdevicelist(fsal_handle_t *handle,
                       fsal_op_context_t *context,
                       const struct fsal_getdevicelist_arg *arg,
                       struct fsal_getdevicelist_res *res)
{
     /* Sanity check on type */
     if (arg->type != LAYOUT4_NFSV4_1_FILES) {
          LogCrit(COMPONENT_PNFS,
                  "Unsupported layout type: %x",
                  arg->type);
          return NFS4ERR_UNKNOWN_LAYOUTTYPE;
     }

     /* We have neither the ability nor the desire to return all valid
        deviceids, so we do nothing successfully. */

     res->count = 0;
     res->eof = TRUE;

     return NFS4_OK;
}
