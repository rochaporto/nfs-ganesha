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
 * \file    fsal_ds.c
 * \brief   DS realisation for the filesystem abstraction
 *
 * filelayout.c: DS realisation for the filesystem abstraction
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
#include "fsal_types.h"
#include "fsal_pnfs.h"
#include "pnfs_common.h"
#include "fsal_pnfs_files.h"

#define min(a,b)          \
     ({ typeof (a) _a = (a);                    \
          typeof (b) _b = (b);                  \
          _a < _b ? _a : _b; })

nfsstat4
DMLITEFSAL_DS_read(fsal_handle_t *exthandle,
                 fsal_op_context_t *extcontext,
                 const stateid4 *stateid,
                 offset4 offset,
                 count4 requested_length,
                 caddr_t buffer,
                 count4 *supplied_length,
                 fsal_boolean_t *end_of_file)
{
     /* Our format for the file handle */
     dmlitefsal_handle_t* handle = (dmlitefsal_handle_t*) exthandle;
     /* Our format for the operational context */
     dmlitefsal_op_context_t* context = (dmlitefsal_op_context_t*) extcontext;
     /* The OSD number for this machine */
     int local_OSD = 0;
     /* Width of a stripe in the file */
     uint32_t stripe_width = 0;
     /* Beginning of a block */
     uint64_t block_start = 0;
     /* Number of the stripe being read */
     uint32_t stripe = 0;
     /* Internal offset within the stripe*/
     uint32_t internal_offset = 0;
     /* The amount actually read */
     int amount_read = 0;

     // TODO: dmlite_buildlayout

     return NFS4_OK;
}

nfsstat4
DMLITEFSAL_DS_write(fsal_handle_t *exthandle,
                  fsal_op_context_t *extcontext,
                  const stateid4 *stateid,
                  offset4 offset,
                  count4 write_length,
                  caddr_t buffer,
                  stable_how4 stability_wanted,
                  count4 *written_length,
                  verifier4 *writeverf,
                  stable_how4 *stability_got)
{
     /* Our format for the file handle */
     dmlitefsal_handle_t* handle = (dmlitefsal_handle_t*) exthandle;
     /* Our format for the operational context */
     dmlitefsal_op_context_t* context = (dmlitefsal_op_context_t*) extcontext;
     /* User ID and group ID for permissions */
     int uid = FSAL_OP_CONTEXT_TO_UID(context);
     int gid = FSAL_OP_CONTEXT_TO_GID(context);
     /* The OSD number for this host */
     int local_OSD = 0;
     /* Width of a stripe in the file */
     uint32_t stripe_width = 0;
     /* Beginning of a block */
     uint64_t block_start = 0;
     /* Number of the stripe being written */
     uint32_t stripe = 0;
     /* Internal offset within the stripe*/
     uint32_t internal_offset = 0;
     /* The amount actually written */
     int32_t amount_written = 0;
     /* The adjusted write length, confined to one object */
     uint32_t adjusted_write = 0;

     // TODO: dmlite_ds_write

     return NFS4_OK;
}

nfsstat4
DMLITEFSAL_DS_commit(fsal_handle_t *exthandle,
                   fsal_op_context_t *extcontext,
                   offset4 offset,
                   count4 count,
                   verifier4 *writeverf)
{
     /* Our format for the file handle */
     dmlitefsal_handle_t* handle = (dmlitefsal_handle_t*) exthandle;
     /* Our format for the operational context */
     dmlitefsal_op_context_t* context = (dmlitefsal_op_context_t*) extcontext;
     /* Width of a stripe in the file */
     const uint32_t stripe_width = handle->data.layout.fl_stripe_unit;
     /* Error return from Ceph */
     int rc = 0;

     // TODO: dmlite_commit
     return NFS4_OK;
}
