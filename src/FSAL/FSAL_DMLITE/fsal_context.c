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

/**
 *
 * \file    fsal_creds.c
 * \brief   FSAL credentials handling functions.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include <pwd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/**
 * @defgroup FSALCredFunctions Credential handling functions.
 *
 * Those functions handle security contexts (credentials).
 *
 * @{
 */

/**
 * Parse FS specific option string
 * to build the export entry option.
 */
fsal_status_t DMLITEFSAL_BuildExportContext(
                       fsal_export_context_t * export_context,
                       fsal_path_t * export_path,
                       char *fs_specific_options)
{
    dmlitefsal_export_context_t* dmlite_export_context =
        (dmlitefsal_export_context_t*) export_context;
    char *argv[2];
    int argc=1;
    int rc;

  // TODO: dmlite_init, dmlite_config

  /* Save pointer to fsal_staticfsinfo_t in export context */
  export_context->fe_static_fs_info = &global_fs_info;

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_BuildExportContext);
}

/**
 * FSAL_CleanUpExportContext :
 * this will clean up and state in an export that was created during
 * the BuildExportContext phase.  For many FSALs this may be a noop.
 *
 * \param p_export_context (in, gpfsfsal_export_context_t)
 */

fsal_status_t DMLITEFSAL_CleanUpExportContext(
                              fsal_export_context_t *export_context)
{
  dmlitefsal_export_context_t* dmlite_export_context =
    (dmlitefsal_export_context_t*) export_context;

  // TODO: dmlite_cleanup

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_CleanUpExportContext);
}

/* @} */

