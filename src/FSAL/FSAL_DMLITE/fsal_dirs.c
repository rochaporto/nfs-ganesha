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
 * \file    fsal_dirs.c
 * \brief   Directory browsing operations.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"
#include <string.h>

/**
 * FSAL_opendir :
 *     Opens a directory for reading its content.
 *
 * \param exthandle (input)
 *         the handle of the directory to be opened.
 * \param extcontext (input)
 *         Permission context for the operation (user, export context...).
 * \param extdescriptor (output)
 *         pointer to an allocated structure that will receive
 *         directory stream informations, on successfull completion.
 * \param attributes (optional output)
 *         On successfull completion,the structure pointed
 *         by dir_attributes receives the new directory attributes.
 *         Can be NULL.
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error)
 *        - ERR_FSAL_ACCESS       (user does not have read permission on directory)
 *        - ERR_FSAL_STALE        (exthandle does not address an existing object)
 *        - ERR_FSAL_FAULT        (a NULL pointer was passed as mandatory argument)
 *        - Other error codes can be returned :
 *          ERR_FSAL_IO, ...
 */
fsal_status_t DMLITEFSAL_opendir(fsal_handle_t * exthandle,
                               fsal_op_context_t * extcontext,
                               fsal_dir_t * extdescriptor,
                               fsal_attrib_list_t * dir_attributes)
{
  dmlitefsal_handle_t* handle = (dmlitefsal_handle_t*) exthandle;
  dmlitefsal_op_context_t* context = (dmlitefsal_op_context_t*) extcontext;
  dmlitefsal_dir_t* descriptor = (dmlitefsal_dir_t*) extdescriptor;
  fsal_status_t status;
  int rc;
  int uid = FSAL_OP_CONTEXT_TO_UID(context);
  int gid = FSAL_OP_CONTEXT_TO_GID(context);
  struct dmlite_dir_result *dh; // TODO: dmlite_dir

  /* sanity checks
   * note : dir_attributes is optional.
   */
  if(!handle || !context || !descriptor)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_opendir);

  TakeTokenFSCall();

  // TODO: dmlite_opendir

  ReleaseTokenFSCall();

  if (rc < 0)
    Return(posix2fsal_error(rc), 0, INDEX_FSAL_opendir);

  descriptor->dh = dh;
  descriptor->vi = VINODE(handle);
  descriptor->ctx = *context;

  if(dir_attributes)
    {
      status = DMLITEFSAL_getattrs(exthandle, extcontext, dir_attributes);

      if(FSAL_IS_ERROR(status))
        {
          FSAL_CLEAR_MASK(dir_attributes->asked_attributes);
          FSAL_SET_MASK(dir_attributes->asked_attributes, FSAL_ATTR_RDATTR_ERR);
        }
    }

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_opendir);
}

/**
 * FSAL_readdir :
 *     Read the entries of an opened directory.
 *
 * \param descriptor (input):
 *        Pointer to the directory descriptor filled by FSAL_opendir.
 * \param start_position (input):
 *        Cookie that indicates the first object to be read during
 *        this readdir operation.
 *        This should be :
 *        - FSAL_READDIR_FROM_BEGINNING for reading the content
 *          of the directory from the beginning.
 *        - The end_position parameter returned by the previous
 *          call to FSAL_readdir.
 * \param get_attr_mask (input)
 *        Specify the set of attributes to be retrieved for directory entries.
 * \param buffersize (input)
 *        The size (in bytes) of the buffer where
 *        the direntries are to be stored.
 * \param dirents (output)
 *        Adress of the buffer where the direntries are to be stored.
 * \param end_position (output)
 *        Cookie that indicates the current position in the directory.
 * \param count (output)
 *        Pointer to the number of entries read during the call.
 * \param end_of_dir (output)
 *        Pointer to a boolean that indicates if the end of dir
 *        has been reached during the call.
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error)
 *        - ERR_FSAL_FAULT        (a NULL pointer was passed as mandatory argument) 
 *        - Other error codes can be returned :
 *          ERR_FSAL_IO, ...
 */
fsal_status_t DMLITEFSAL_readdir(fsal_dir_t *extdescriptor,
                               fsal_cookie_t extstart,
                               fsal_attrib_mask_t attrmask,
                               fsal_mdsize_t buffersize,
                               fsal_dirent_t *dirents,
                               fsal_cookie_t *extend,
                               fsal_count_t *count,
                               fsal_boolean_t *end_of_dir)
{
  int rc = 0;
  fsal_status_t status;
  struct dirent de;
  dmlitefsal_dir_t* descriptor = (dmlitefsal_dir_t*) extdescriptor;

  loff_t start = ((dmlitefsal_cookie_t*) extstart.data)->cookie;
  loff_t* end = &((dmlitefsal_cookie_t*) extend->data)->cookie;
  unsigned int max_entries = buffersize / sizeof(fsal_dirent_t);

  /* sanity checks */

  if(!descriptor || !dirents || !end || !count || !end_of_dir)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_readdir);

  *end_of_dir = FALSE;
  *count = 0;

  TakeTokenFSCall();

  // TODO: dmlite_readdir

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_readdir);
}

/**
 * FSAL_closedir :
 * Free the resources allocated for reading directory entries.
 *
 * \param dir_descriptor (input):
 *        Pointer to a directory descriptor filled by FSAL_opendir.
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error)
 *        - ERR_FSAL_FAULT        (a NULL pointer was passed as mandatory argument)
 *        - Other error codes can be returned :
 *          ERR_FSAL_IO, ...
 */
fsal_status_t DMLITEFSAL_closedir(fsal_dir_t * extdescriptor)
{
  dmlitefsal_dir_t* descriptor = (dmlitefsal_dir_t*) extdescriptor;
  int rc = 0;

  /* sanity checks */
  if(!descriptor)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_closedir);

  TakeTokenFSCall();

  // TODO: dmlite_closedir

  ReleaseTokenFSCall();

  if (rc < 0)
    Return(posix2fsal_error(rc), 0, INDEX_FSAL_closedir);

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_closedir);
}
