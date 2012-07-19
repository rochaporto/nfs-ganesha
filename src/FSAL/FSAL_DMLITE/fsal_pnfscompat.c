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
#ifdef _PNFS
#include "fsal_pnfs.h"
#endif /* _PNFS */

#ifdef _PNFS_MDS
fsal_mdsfunctions_t fsal_dmlite_mdsfunctions = {
     .layoutget = DMLITEFSAL_layoutget,
     .layoutreturn = DMLITEFSAL_layoutreturn,
     .layoutcommit = DMLITEFSAL_layoutcommit,
     .getdeviceinfo = DMLITEFSAL_getdeviceinfo,
     .getdevicelist = DMLITEFSAL_getdevicelist
};

fsal_mdsfunctions_t FSAL_GetMDSFunctions(void)
{
     return fsal_dmlite_mdsfunctions;
}
#endif /* _PNFS_MDS */

#ifdef _PNFS_DS
fsal_dsfunctions_t fsal_dmlite_dsfunctions = {
     .DS_read = DMLITEFSAL_DS_read,
     .DS_write = DMLITEFSAL_DS_write,
     .DS_commit = DMLITEFSAL_DS_commit
};
#endif /* _PNFS_DS */

#ifdef _PNFS_DS
fsal_dsfunctions_t FSAL_GetDSFunctions(void)
{
     return fsal_dmlite_dsfunctions;
}
#endif /* _PNFS_DS */
