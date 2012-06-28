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
 * @file    nfs4_Compound.c
 * @brief   Routines used for managing the NFS4 COMPOUND functions.
 *
 * Routines used for managing the NFS4 COMPOUND functions.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#endif

#include "sal_functions.h"
#include "nfs_tools.h"

typedef struct nfs4_op_desc__
{
  char *name;
  unsigned int val;
  int (*funct) (struct nfs_argop4 *, compound_data_t *, struct nfs_resop4 *);
} nfs4_op_desc_t;

/* This array maps the operation number to the related position in
   array optab4 */
#ifndef _USE_NFS4_1
const int optab4index[] = {
     0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
     32, 33, 34, 35, 36, 37, 38, 39
};

#else
const int optab4index[] = {
     0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
     17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
     34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
     51, 52, 53, 54, 55, 56, 57, 58
};

#endif

#define POS_ILLEGAL_V40 40
#define POS_ILLEGAL_V41 59

static const nfs4_op_desc_t optab4v0[] = {
  {"OP_ACCESS", NFS4_OP_ACCESS, nfs4_op_access},
  {"OP_CLOSE", NFS4_OP_CLOSE, nfs4_op_close},
  {"OP_COMMIT", NFS4_OP_COMMIT, nfs4_op_commit},
  {"OP_CREATE", NFS4_OP_CREATE, nfs4_op_create},
  {"OP_DELEGPURGE", NFS4_OP_DELEGPURGE, nfs4_op_delegpurge},
  {"OP_DELEGRETURN", NFS4_OP_DELEGRETURN, nfs4_op_delegreturn},
  {"OP_GETATTR", NFS4_OP_GETATTR, nfs4_op_getattr},
  {"OP_GETFH", NFS4_OP_GETFH, nfs4_op_getfh},
  {"OP_LINK", NFS4_OP_LINK, nfs4_op_link},
  {"OP_LOCK", NFS4_OP_LOCK, nfs4_op_lock},
  {"OP_LOCKT", NFS4_OP_LOCKT, nfs4_op_lockt},
  {"OP_LOCKU", NFS4_OP_LOCKU, nfs4_op_locku},
  {"OP_LOOKUP", NFS4_OP_LOOKUP, nfs4_op_lookup},
  {"OP_LOOKUPP", NFS4_OP_LOOKUPP, nfs4_op_lookupp},
  {"OP_NVERIFY", NFS4_OP_NVERIFY, nfs4_op_nverify},
  {"OP_OPEN", NFS4_OP_OPEN, nfs4_op_open},
  {"OP_OPENATTR", NFS4_OP_OPENATTR, nfs4_op_openattr},
  {"OP_OPEN_CONFIRM", NFS4_OP_OPEN_CONFIRM, nfs4_op_open_confirm},
  {"OP_OPEN_DOWNGRADE", NFS4_OP_OPEN_DOWNGRADE, nfs4_op_open_downgrade},
  {"OP_PUTFH", NFS4_OP_PUTFH, nfs4_op_putfh},
  {"OP_PUTPUBFH", NFS4_OP_PUTPUBFH, nfs4_op_putpubfh},
  {"OP_PUTROOTFH", NFS4_OP_PUTROOTFH, nfs4_op_putrootfh},
  {"OP_READ", NFS4_OP_READ, nfs4_op_read},
  {"OP_READDIR", NFS4_OP_READDIR, nfs4_op_readdir},
  {"OP_READLINK", NFS4_OP_READLINK, nfs4_op_readlink},
  {"OP_REMOVE", NFS4_OP_REMOVE, nfs4_op_remove},
  {"OP_RENAME", NFS4_OP_RENAME, nfs4_op_rename},
  {"OP_RENEW", NFS4_OP_RENEW, nfs4_op_renew},
  {"OP_RESTOREFH", NFS4_OP_RESTOREFH, nfs4_op_restorefh},
  {"OP_SAVEFH", NFS4_OP_SAVEFH, nfs4_op_savefh},
  {"OP_SECINFO", NFS4_OP_SECINFO, nfs4_op_secinfo},
  {"OP_SETATTR", NFS4_OP_SETATTR, nfs4_op_setattr},
  {"OP_SETCLIENTID", NFS4_OP_SETCLIENTID, nfs4_op_setclientid},
  {"OP_SETCLIENTID_CONFIRM", NFS4_OP_SETCLIENTID_CONFIRM, nfs4_op_setclientid_confirm},
  {"OP_VERIFY", NFS4_OP_VERIFY, nfs4_op_verify},
  {"OP_WRITE", NFS4_OP_WRITE, nfs4_op_write},
  {"OP_RELEASE_LOCKOWNER", NFS4_OP_RELEASE_LOCKOWNER, nfs4_op_release_lockowner},
  {"OP_ILLEGAL", NFS4_OP_ILLEGAL, nfs4_op_illegal}
};

#ifdef _USE_NFS4_1
static const nfs4_op_desc_t optab4v1[] = {
  {"OP_ACCESS", NFS4_OP_ACCESS, nfs4_op_access},
  {"OP_CLOSE", NFS4_OP_CLOSE, nfs41_op_close},
  {"OP_COMMIT", NFS4_OP_COMMIT, nfs4_op_commit},
  {"OP_CREATE", NFS4_OP_CREATE, nfs4_op_create},
  {"OP_DELEGPURGE", NFS4_OP_DELEGPURGE, nfs4_op_delegpurge},
  {"OP_DELEGRETURN", NFS4_OP_DELEGRETURN, nfs4_op_delegreturn},
  {"OP_GETATTR", NFS4_OP_GETATTR, nfs4_op_getattr},
  {"OP_GETFH", NFS4_OP_GETFH, nfs4_op_getfh},
  {"OP_LINK", NFS4_OP_LINK, nfs4_op_link},
  {"OP_LOCK", NFS4_OP_LOCK, nfs41_op_lock},
  {"OP_LOCKT", NFS4_OP_LOCKT, nfs41_op_lockt},
  {"OP_LOCKU", NFS4_OP_LOCKU, nfs41_op_locku},
  {"OP_LOOKUP", NFS4_OP_LOOKUP, nfs4_op_lookup},
  {"OP_LOOKUPP", NFS4_OP_LOOKUPP, nfs4_op_lookupp},
  {"OP_NVERIFY", NFS4_OP_NVERIFY, nfs4_op_nverify},
  {"OP_OPEN", NFS4_OP_OPEN, nfs41_op_open},
  {"OP_OPENATTR", NFS4_OP_OPENATTR, nfs4_op_openattr},
  {"OP_OPEN_CONFIRM", NFS4_OP_OPEN_CONFIRM, nfs4_op_illegal},   /* OP_OPEN_CONFIRM is deprecated in NFSv4.1 */
  {"OP_OPEN_DOWNGRADE", NFS4_OP_OPEN_DOWNGRADE, nfs4_op_open_downgrade},
  {"OP_PUTFH", NFS4_OP_PUTFH, nfs4_op_putfh},
  {"OP_PUTPUBFH", NFS4_OP_PUTPUBFH, nfs4_op_putpubfh},
  {"OP_PUTROOTFH", NFS4_OP_PUTROOTFH, nfs4_op_putrootfh},
  {"OP_READ", NFS4_OP_READ, nfs4_op_read},
  {"OP_READDIR", NFS4_OP_READDIR, nfs4_op_readdir},
  {"OP_READLINK", NFS4_OP_READLINK, nfs4_op_readlink},
  {"OP_REMOVE", NFS4_OP_REMOVE, nfs4_op_remove},
  {"OP_RENAME", NFS4_OP_RENAME, nfs4_op_rename},
  {"OP_RENEW", NFS4_OP_RENEW, nfs4_op_renew},
  {"OP_RESTOREFH", NFS4_OP_RESTOREFH, nfs4_op_restorefh},
  {"OP_SAVEFH", NFS4_OP_SAVEFH, nfs4_op_savefh},
  {"OP_SECINFO", NFS4_OP_SECINFO, nfs4_op_secinfo},
  {"OP_SETATTR", NFS4_OP_SETATTR, nfs4_op_setattr},
  {"OP_SETCLIENTID", NFS4_OP_SETCLIENTID, nfs4_op_setclientid},
  {"OP_SETCLIENTID_CONFIRM", NFS4_OP_SETCLIENTID_CONFIRM, nfs4_op_setclientid_confirm},
  {"OP_VERIFY", NFS4_OP_VERIFY, nfs4_op_verify},
  {"OP_WRITE", NFS4_OP_WRITE, nfs4_op_write},
  {"OP_RELEASE_LOCKOWNER", NFS4_OP_RELEASE_LOCKOWNER, nfs4_op_release_lockowner},
  {"OP_BACKCHANNEL_CTL", NFS4_OP_BACKCHANNEL_CTL, nfs4_op_illegal},     /* tbd */
  {"OP_BIND_CONN_TO_SESSION", NFS4_OP_BIND_CONN_TO_SESSION, nfs4_op_illegal},   /* tbd */
  {"OP_EXCHANGE_ID", NFS4_OP_EXCHANGE_ID, nfs41_op_exchange_id},
  {"OP_CREATE_SESSION", NFS4_OP_CREATE_SESSION, nfs41_op_create_session},
  {"OP_DESTROY_SESSION", NFS4_OP_DESTROY_SESSION, nfs41_op_destroy_session},
  {"OP_FREE_STATEID", NFS4_OP_FREE_STATEID, nfs41_op_free_stateid},   
  {"OP_GET_DIR_DELEGATION", NFS4_OP_GET_DIR_DELEGATION, nfs4_op_illegal},       /* tbd */
  {"OP_GETDEVICEINFO", NFS4_OP_GETDEVICEINFO, nfs41_op_getdeviceinfo},
  {"OP_GETDEVICELIST", NFS4_OP_GETDEVICELIST, nfs41_op_getdevicelist},
  {"OP_LAYOUTCOMMIT", NFS4_OP_LAYOUTCOMMIT, nfs41_op_layoutcommit},
  {"OP_LAYOUTGET", NFS4_OP_LAYOUTGET, nfs41_op_layoutget},
  {"OP_LAYOUTRETURN", NFS4_OP_LAYOUTRETURN, nfs41_op_layoutreturn},
  {"OP_SECINFO_NO_NAME", NFS4_OP_SECINFO_NO_NAME, nfs4_op_illegal},     /* tbd */
  {"OP_SEQUENCE", NFS4_OP_SEQUENCE, nfs41_op_sequence},
  {"OP_SET_SSV", NFS4_OP_SET_SSV, nfs41_op_set_ssv},
  {"OP_TEST_STATEID", NFS4_OP_TEST_STATEID, nfs41_op_test_stateid}, 
  {"OP_WANT_DELEGATION", NFS4_OP_WANT_DELEGATION, nfs4_op_illegal},     /* tbd */
  {"OP_DESTROY_CLIENTID", NFS4_OP_DESTROY_CLIENTID, nfs4_op_illegal},   /* tbd */
  {"OP_RECLAIM_COMPLETE", NFS4_OP_RECLAIM_COMPLETE, nfs41_op_reclaim_complete},
  {"OP_ILLEGAL", NFS4_OP_ILLEGAL, nfs4_op_illegal}
};
#endif                          /* _USE_NFS4_1 */

#ifdef _USE_NFS4_1
nfs4_op_desc_t *optabvers[] =
    { (nfs4_op_desc_t *) optab4v0, (nfs4_op_desc_t *) optab4v1 };
#else
nfs4_op_desc_t *optabvers[] = { (nfs4_op_desc_t *) optab4v0 };
#endif

/**
 * @brief nfs4_COMPOUND: The NFS PROC4 COMPOUND
 *
 * Implements the NFS PROC4 COMPOUND.  This routine processes the
 * content of the nfsv4 operation list and composes the result.  On
 * this aspect it is a little similar to a dispatch routine.
 * Operation and functions necessary to process them are defined in
 * the optab4 array.
 *
 *
 *  @param[in]  arg        Generic nfs arguments
 *  @param[in]  exportlist The full export list
 *  @param[in]  contex     Context for the FSAL
 *  @param[in]  worker     Worker thread data
 *  @param[in]  req        NFSv4 request structure
 *  @param[out] res        NFSv4 reply structure
 *
 *  @see nfs4_op_<*> functions
 *  @see nfs4_GetPseudoFs
 *
 */

int nfs4_Compound(nfs_arg_t *arg,
                  exportlist_t *export,
                  struct user_cred *creds,
                  nfs_worker_data_t *worker,
                  struct svc_req *req,
                  nfs_res_t *res)
{
  unsigned int i = 0;
  int status = NFS4_OK;
  struct nfs_resop4 resop;
  compound_data_t data;
  int opindex;
  #define TAGLEN 64
  char tagstr[TAGLEN + 1 + 5];

  /* A "local" #define to avoid typo with nfs (too) long structure names */
#define COMPOUND4_ARRAY arg->arg_compound4.argarray
#define COMPOUND4_MINOR arg->arg_compound4.minorversion

#ifdef _USE_NFS4_1
  if(COMPOUND4_MINOR > 1)
#else
  if(COMPOUND4_MINOR != 0)
#endif
    {
      LogCrit(COMPONENT_NFS_V4,
              "Bad Minor Version %d",
              COMPOUND4_MINOR);

      res->res_compound4.status = NFS4ERR_MINOR_VERS_MISMATCH;
      res->res_compound4.resarray.resarray_len = 0;
      return NFS_REQ_OK;
    }

  /* Check for empty COMPOUND request */
  if(COMPOUND4_ARRAY.argarray_len == 0)
    {
      LogMajor(COMPONENT_NFS_V4,
               "An empty COMPOUND (no operation in it) was received");

      res->res_compound4.status = NFS4_OK;
      res->res_compound4.resarray.resarray_len = 0;
      return NFS_REQ_OK;
    }

  /* Check if this export supports NFSv4 */
  if ((export->options & EXPORT_OPTION_NFSV4) == 0)
   {
      LogMajor(COMPONENT_NFS_V4,
               "The export(id=%u) does not support NFSv4... rejecting it",
               export->id);
      res->res_compound4.status = NFS4ERR_PERM ;
      res->res_compound4.resarray.resarray_val[0].nfs_resop4_u.opaccess.status = NFS4ERR_PERM ;
      return NFS_REQ_OK ;
   }

  /* Check for too long request */
  if(COMPOUND4_ARRAY.argarray_len > 30)
    {
      LogMajor(COMPONENT_NFS_V4,
               "A COMPOUND with too many operations (%d) was received",
               COMPOUND4_ARRAY.argarray_len);

      res->res_compound4.status = NFS4ERR_RESOURCE;
      res->res_compound4.resarray.resarray_len = 0;
      return NFS_REQ_OK;
    }

  /* Initialisation of the compound request internal's data */
  memset(&data, 0, sizeof(data));

  /* Minor version related stuff */
  data.minorversion = COMPOUND4_MINOR;
  /**
   * @todo BUGAZOMEU: Reminder: Stats on NFSv4 operations are to be
   * set here
   */

  data.pfullexportlist = export; /* Full export list is provided in
                                    input */
  data.user_credentials = *creds;     /* Get the client user's credentials */
  data.pworker = worker;
  data.pseudofs = nfs4_GetPseudoFs();
  data.reqp = req;

  strcpy(data.MntPath, "/");

  /* Building the client credential field */
  if(nfs_rpc_req2client_cred(req, &(data.credential)) == -1)
    return NFS_REQ_DROP;        /* Malformed credential */

  /* Keeping the same tag as in the arguments */
  memcpy(&(res->res_compound4.tag), &(arg->arg_compound4.tag),
         sizeof(arg->arg_compound4.tag));

  if(utf8dup(&(res->res_compound4.tag), &(arg->arg_compound4.tag)) == -1)
    {
      LogCrit(COMPONENT_NFS_V4, "Unable to duplicate tag into response");
      return NFS_REQ_DROP;
    }

  /* Allocating the reply nfs_resop4 */
  if((res->res_compound4.resarray.resarray_val =
      gsh_calloc((COMPOUND4_ARRAY.argarray_len),
                 sizeof(struct nfs_resop4))) == NULL)
    {
      return NFS_REQ_DROP;
    }

  if(isDebug(COMPONENT_NFS_V4) && res->res_compound4.tag.utf8string_len > 0)
    {
      sprintf(tagstr, " TAG=");
      utf82str(tagstr+5, TAGLEN, &(res->res_compound4.tag));
    }
  else
    {
      tagstr[0] = '\0';
    }

  /* Managing the operation list */
  LogDebug(COMPONENT_NFS_V4,
           "COMPOUND: There are %d operations%s",
           COMPOUND4_ARRAY.argarray_len, tagstr);

#ifdef _USE_NFS4_1
  /* Manage error NFS4ERR_NOT_ONLY_OP */
  if(COMPOUND4_ARRAY.argarray_len > 1)
    {
      /* If not prepended ny OP4_SEQUENCE, OP4_EXCHANGE_ID should be
       * the only request in the compound see 18.35.3. and test EID8
       * for details */
      if(optabvers[1][optab4index[COMPOUND4_ARRAY.argarray_val[0].argop]].val
         == NFS4_OP_EXCHANGE_ID)
        {
          status = NFS4ERR_NOT_ONLY_OP;
          res->res_compound4.resarray.resarray_val[0].nfs_resop4_u
            .opexchange_id.eir_status = status;
          res->res_compound4.status = status;

          return NFS_REQ_OK;
        }
    }
#endif

  res->res_compound4.resarray.resarray_len = COMPOUND4_ARRAY.argarray_len;
  for(i = 0; i < COMPOUND4_ARRAY.argarray_len; i++)
    {
      /* Use optab4index to reference the operation */
#ifdef _USE_NFS4_1
      data.oppos = i;           /* Useful to check if OP_SEQUENCE is used as the first operation */

      if(COMPOUND4_MINOR == 1)
        {
          if(data.psession != NULL)
            {
              if(data.psession->fore_channel_attrs.ca_maxoperations == i)
                {
                  status = NFS4ERR_TOO_MANY_OPS;
                  res->res_compound4.resarray.resarray_val[i].nfs_resop4_u.opaccess.
                      status = status;
                  res->res_compound4.resarray.resarray_val[i].resop =
                      COMPOUND4_ARRAY.argarray_val[i].argop;
                  res->res_compound4.status = status;
                  break;        /* stop loop */
                }
            }
        }

      /* if( COMPOUND4_MINOR == 1 ) */
      if((COMPOUND4_ARRAY.argarray_val[i].argop <= NFS4_OP_RELEASE_LOCKOWNER
          && COMPOUND4_MINOR == 0)
         || (COMPOUND4_ARRAY.argarray_val[i].argop <= NFS4_OP_RECLAIM_COMPLETE
             && COMPOUND4_MINOR == 1))
#else
      if(COMPOUND4_ARRAY.argarray_val[i].argop <= NFS4_OP_RELEASE_LOCKOWNER)
#endif
        opindex = optab4index[COMPOUND4_ARRAY.argarray_val[i].argop];
      else
       {
         /* Set optindex to op_illegal */
#ifdef _USE_NFS4_1
         opindex = (COMPOUND4_MINOR==0)?optab4index[POS_ILLEGAL_V40]:optab4index[POS_ILLEGAL_V41];  
#else
         opindex = optab4index[POS_ILLEGAL_V40];
#endif
         LogMajor( COMPONENT_NFS_V4, "Client is using Illegal operation #%u", COMPOUND4_ARRAY.argarray_val[i].argop ) ;
       }

      LogDebug(COMPONENT_NFS_V4,
               "Request %d is %d = %s, entry %d in the op array%s",
               i,
               optabvers[COMPOUND4_MINOR][opindex].val,
               optabvers[COMPOUND4_MINOR][opindex].name,
               opindex,
               tagstr);

      memset(&res, 0, sizeof(res));
      status = (optabvers[COMPOUND4_MINOR][opindex].funct)(
           &(COMPOUND4_ARRAY.argarray_val[i]),
           &data,
           &resop);

      memcpy(&(res->res_compound4.resarray.resarray_val[i]),
             &resop, sizeof(resop));

      LogCompoundFH(&data);

      /* All the operation, like NFS4_OP_ACESS, have a first replyied
         field called .status */
      res->res_compound4.resarray.resarray_val[i].nfs_resop4_u.opaccess
        .status = status;

      if(status != NFS4_OK)
        {
          /* An error occured, we do not manage the other requests in
             the COMPOUND, this may be a regular behaviour */
          LogDebug(COMPONENT_NFS_V4,
                   "Status of %s in position %d = %s%s",
                   optabvers[COMPOUND4_MINOR][opindex].name,
                   i,
                   nfsstat4_to_str(status),
                   tagstr);

          res->res_compound4.resarray.resarray_len = i + 1;

          break;
        }
#ifdef _USE_NFS4_1
      /* Check Req size */

      /* NFS_V4.1 specific stuff */
      if(data.use_drc)
        {
          /* Replay cache, only true for SEQUENCE or CREATE_SESSION w/o SEQUENCE.
           * Since will only be set in those cases, no need to check operation or anything.
           */
          LogFullDebug(COMPONENT_SESSIONS,
                       "Use session replay cache %p",
                       data.pcached_res);

          /* Free the reply allocated above */
          gsh_free(res->res_compound4.resarray.resarray_val);

          /* Copy the reply from the cache */
          res->res_compound4_extended = *data.pcached_res;
          status = ((COMPOUND4res *) data.pcached_res)->status;
          break;    /* Exit the for loop */
        }
#endif
    }                           /* for */

  /* Complete the reply, in particular, tell where you stopped if unsuccessfull COMPOUD */
  res->res_compound4.status = status;

#ifdef _USE_NFS4_1
  /* Manage session's DRC: keep NFS4.1 replay for later use, but don't save a
   * replayed result again.
   */
  if(data.pcached_res != NULL && !data.use_drc)
    {
      /* Pointer has been set by nfs41_op_sequence and points to slot to cache
       * result in.
       */
      LogFullDebug(COMPONENT_SESSIONS,
                   "Save result in session replay cache %p sizeof nfs_res_t=%d",
                   data.pcached_res,
                   (int) sizeof(nfs_res_t));

      /* Indicate to nfs4_Compound_Free that this reply is cached. */
      res->res_compound4_extended.res_cached = TRUE;

      /* If the cache is already in use, free it. */
      if(data.pcached_res->res_cached)
        {
          data.pcached_res->res_cached = FALSE;
          nfs4_Compound_Free((nfs_res_t *)data.pcached_res);
        }

      /* Save the result in the cache. */
      *data.pcached_res = res->res_compound4_extended;
    }

  /* If we have reserved a lease, update it and release it */
  if(data.preserved_clientid != NULL)
    {
      /* Update and release lease */
      P(data.preserved_clientid->cid_mutex);

      update_lease(data.preserved_clientid);

      V(data.preserved_clientid->cid_mutex);
    }
#endif

  if(status != NFS4_OK)
    LogDebug(COMPONENT_NFS_V4,
             "End status = %s lastindex = %d%s",
             nfsstat4_to_str(status), i, tagstr);

  compound_data_Free(&data);

  return NFS_REQ_OK;
}                               /* nfs4_Compound */

/**
 *
 * @brief Free the result for one NFS4_OP
 *
 * This function frees any memory allocated for the result of an NFSv4
 * operation.
 *
 * @param[in,out] res The result to be freed
 *
 */
void nfs4_Compound_FreeOne(nfs_resop4 *res)
{
  switch (res->resop)
    {
      case NFS4_OP_ACCESS:
        nfs4_op_access_Free(&(res->nfs_resop4_u.opaccess));
        break;

      case NFS4_OP_CLOSE:
        nfs4_op_close_Free(&(res->nfs_resop4_u.opclose));
        break;

      case NFS4_OP_COMMIT:
        nfs4_op_commit_Free(&(res->nfs_resop4_u.opcommit));
        break;

      case NFS4_OP_CREATE:
        nfs4_op_create_Free(&(res->nfs_resop4_u.opcreate));
        break;

      case NFS4_OP_DELEGPURGE:
        nfs4_op_delegpurge_Free(&(res->nfs_resop4_u.opdelegpurge));
        break;

      case NFS4_OP_DELEGRETURN:
        nfs4_op_delegreturn_Free(&(res->nfs_resop4_u.opdelegreturn));
        break;

      case NFS4_OP_GETATTR:
        nfs4_op_getattr_Free(&(res->nfs_resop4_u.opgetattr));
        break;

      case NFS4_OP_GETFH:
        nfs4_op_getfh_Free(&(res->nfs_resop4_u.opgetfh));
        break;

      case NFS4_OP_LINK:
        nfs4_op_link_Free(&(res->nfs_resop4_u.oplink));
        break;

      case NFS4_OP_LOCK:
        nfs4_op_lock_Free(&(res->nfs_resop4_u.oplock));
        break;

      case NFS4_OP_LOCKT:
        nfs4_op_lockt_Free(&(res->nfs_resop4_u.oplockt));
        break;

      case NFS4_OP_LOCKU:
        nfs4_op_locku_Free(&(res->nfs_resop4_u.oplocku));
        break;

      case NFS4_OP_LOOKUP:
        nfs4_op_lookup_Free(&(res->nfs_resop4_u.oplookup));
        break;

      case NFS4_OP_LOOKUPP:
        nfs4_op_lookupp_Free(&(res->nfs_resop4_u.oplookupp));
        break;

      case NFS4_OP_NVERIFY:
        nfs4_op_nverify_Free(&(res->nfs_resop4_u.opnverify));
        break;

      case NFS4_OP_OPEN:
        nfs4_op_open_Free(&(res->nfs_resop4_u.opopen));
        break;

      case NFS4_OP_OPENATTR:
        nfs4_op_openattr_Free(&(res->nfs_resop4_u.opopenattr));
        break;

      case NFS4_OP_OPEN_CONFIRM:
        nfs4_op_open_confirm_Free(&(res->nfs_resop4_u.opopen_confirm));
        break;

      case NFS4_OP_OPEN_DOWNGRADE:
        nfs4_op_open_downgrade_Free(&(res->nfs_resop4_u.opopen_downgrade));
        break;

      case NFS4_OP_PUTFH:
        nfs4_op_putfh_Free(&(res->nfs_resop4_u.opputfh));
        break;

      case NFS4_OP_PUTPUBFH:
        nfs4_op_putpubfh_Free(&(res->nfs_resop4_u.opputpubfh));
        break;

      case NFS4_OP_PUTROOTFH:
        nfs4_op_putrootfh_Free(&(res->nfs_resop4_u.opputrootfh));
        break;

      case NFS4_OP_READ:
        nfs4_op_read_Free(&(res->nfs_resop4_u.opread));
        break;

      case NFS4_OP_READDIR:
        nfs4_op_readdir_Free(&(res->nfs_resop4_u.opreaddir));
        break;

      case NFS4_OP_READLINK:
        nfs4_op_readlink_Free(&(res->nfs_resop4_u.opreadlink));
        break;

      case NFS4_OP_REMOVE:
        nfs4_op_remove_Free(&(res->nfs_resop4_u.opremove));
        break;

      case NFS4_OP_RENAME:
        nfs4_op_rename_Free(&(res->nfs_resop4_u.oprename));
        break;

      case NFS4_OP_RENEW:
        nfs4_op_renew_Free(&(res->nfs_resop4_u.oprenew));
        break;

      case NFS4_OP_RESTOREFH:
        nfs4_op_restorefh_Free(&(res->nfs_resop4_u.oprestorefh));
        break;

      case NFS4_OP_SAVEFH:
        nfs4_op_savefh_Free(&(res->nfs_resop4_u.opsavefh));
        break;

      case NFS4_OP_SECINFO:
        nfs4_op_secinfo_Free(&(res->nfs_resop4_u.opsecinfo));
        break;

      case NFS4_OP_SETATTR:
        nfs4_op_setattr_Free(&(res->nfs_resop4_u.opsetattr));
        break;

      case NFS4_OP_SETCLIENTID:
        nfs4_op_setclientid_Free(&(res->nfs_resop4_u.opsetclientid));
        break;

      case NFS4_OP_SETCLIENTID_CONFIRM:
        nfs4_op_setclientid_confirm_Free(&(res->nfs_resop4_u
                                           .opsetclientid_confirm));
        break;

      case NFS4_OP_VERIFY:
        nfs4_op_verify_Free(&(res->nfs_resop4_u.opverify));
        break;

      case NFS4_OP_WRITE:
        nfs4_op_write_Free(&(res->nfs_resop4_u.opwrite));
        break;

      case NFS4_OP_RELEASE_LOCKOWNER:
        nfs4_op_release_lockowner_Free(&(res->nfs_resop4_u
                                         .oprelease_lockowner));
        break;

#ifdef _USE_NFS4_1
      case NFS4_OP_EXCHANGE_ID:
        nfs41_op_exchange_id_Free(&(res->nfs_resop4_u.opexchange_id));
        break;

      case NFS4_OP_CREATE_SESSION:
        nfs41_op_create_session_Free(&(res->nfs_resop4_u.opcreate_session));
        break;

      case NFS4_OP_SEQUENCE:
        nfs41_op_sequence_Free(&(res->nfs_resop4_u.opsequence));
        break;

      case NFS4_OP_GETDEVICEINFO:
        nfs41_op_getdeviceinfo_Free(&(res->nfs_resop4_u.opgetdeviceinfo));
        break;

      case NFS4_OP_GETDEVICELIST:
        nfs41_op_getdevicelist_Free(&(res->nfs_resop4_u.opgetdevicelist));
        break;

      case NFS4_OP_TEST_STATEID:
        nfs41_op_test_stateid_Free(&(res->nfs_resop4_u.optest_stateid));
        break;

      case NFS4_OP_FREE_STATEID:
        nfs41_op_free_stateid_Free(&(res->nfs_resop4_u.opfree_stateid));
        break;

      case NFS4_OP_BACKCHANNEL_CTL:
      case NFS4_OP_BIND_CONN_TO_SESSION:
      case NFS4_OP_DESTROY_SESSION:
      case NFS4_OP_GET_DIR_DELEGATION:
      case NFS4_OP_LAYOUTCOMMIT:
      case NFS4_OP_LAYOUTGET:
      case NFS4_OP_LAYOUTRETURN:
      case NFS4_OP_SECINFO_NO_NAME:
      case NFS4_OP_SET_SSV:
      case NFS4_OP_WANT_DELEGATION:
      case NFS4_OP_DESTROY_CLIENTID:
      case NFS4_OP_RECLAIM_COMPLETE:
        nfs41_op_reclaim_complete_Free(&(res->nfs_resop4_u.opreclaim_complete));
        break;
#endif

      case NFS4_OP_ILLEGAL:
        nfs4_op_illegal_Free(&(res->nfs_resop4_u.opillegal));
        break;
    }                       /* switch */
}

/**
 *
 * @brief Free the result for NFS4PROC_COMPOUND
 *
 * This function frees the result for one NFS4PROC_COMPOUND.
 *
 * @param[in] res The result
 *
 */
void nfs4_Compound_Free(nfs_res_t *res)
{
  unsigned int     i = 0;
  log_components_t component = COMPONENT_NFS_V4;

  if(isFullDebug(COMPONENT_SESSIONS))
    component = COMPONENT_SESSIONS;

  if(res->res_compound4_extended.res_cached)
    {
      LogFullDebug(component,
                   "Skipping free of NFS4 result %p",
                   res);
      return;
    }

  LogFullDebug(component,
               "nfs4_Compound_Free %p (resarraylen=%i)",
               res,
               res->res_compound4.resarray.resarray_len);

  for(i = 0; i < res->res_compound4.resarray.resarray_len; i++) {
      nfs_resop4 *val = &res->res_compound4.resarray.resarray_val[i];
      if (val) {
          /* !val is an error case, but it can occur, so avoid
           * indirect on NULL */
          nfs4_Compound_FreeOne(val);
      }
  }

  gsh_free(res->res_compound4.resarray.resarray_val);
  free_utf8(&res->res_compound4.tag);

  return;
}                               /* nfs4_Compound_Free */

/**
 * @brief Free a compound data structure
 *
 * This function frees one compound data structure.
 *
 * @param[in,out] data The compound_data_t to be freed
 *
 */
void compound_data_Free(compound_data_t *data)
{
  /* Release refcounted cache entries */
  if (data->current_entry)
      cache_inode_put(data->current_entry);

  if (data->saved_entry)
      cache_inode_put(data->saved_entry);

  if(data->currentFH.nfs_fh4_val != NULL)
    gsh_free(data->currentFH.nfs_fh4_val);

  if(data->rootFH.nfs_fh4_val != NULL)
    gsh_free(data->rootFH.nfs_fh4_val);

  if(data->publicFH.nfs_fh4_val != NULL)
    gsh_free(data->publicFH.nfs_fh4_val);

  if(data->savedFH.nfs_fh4_val != NULL)
    gsh_free(data->savedFH.nfs_fh4_val);

  if(data->mounted_on_FH.nfs_fh4_val != NULL)
    gsh_free(data->mounted_on_FH.nfs_fh4_val);

}                               /* compound_data_Free */

/**
 *
 * @brief Copy the result for one NFS4_OP
 *
 * This function copies the result structure for a single NFSv4
 * operation.
 *
 * @param[out] res_dst Buffer to which to copy the result
 * @param[in]  res_src The result to copy
 *
 */
void nfs4_Compound_CopyResOne(nfs_resop4 *res_dst,
                              nfs_resop4 *res_src)
{
  /* Copy base data structure */
  memcpy(res_dst, res_src, sizeof(*res_dst));

  /* Do deep copy where necessary */
  switch(res_src->resop)
    {
      case NFS4_OP_ACCESS:
        break;

      case NFS4_OP_CLOSE:
        nfs4_op_close_CopyRes(&(res_dst->nfs_resop4_u.opclose),
                              &(res_src->nfs_resop4_u.opclose));
        return;

      case NFS4_OP_COMMIT:
      case NFS4_OP_CREATE:
      case NFS4_OP_DELEGPURGE:
      case NFS4_OP_DELEGRETURN:
      case NFS4_OP_GETATTR:
      case NFS4_OP_GETFH:
      case NFS4_OP_LINK:
        break;

      case NFS4_OP_LOCK:
        nfs4_op_lock_CopyRes(&(res_dst->nfs_resop4_u.oplock),
                             &(res_src->nfs_resop4_u.oplock));
        return;

      case NFS4_OP_LOCKT:
        break;

      case NFS4_OP_LOCKU:
        nfs4_op_locku_CopyRes(&(res_dst->nfs_resop4_u.oplocku),
                              &(res_src->nfs_resop4_u.oplocku));
        return;

      case NFS4_OP_LOOKUP:
      case NFS4_OP_LOOKUPP:
      case NFS4_OP_NVERIFY:
        break;

      case NFS4_OP_OPEN:
        nfs4_op_open_CopyRes(&(res_dst->nfs_resop4_u.opopen),
                             &(res_src->nfs_resop4_u.opopen));
        return;

      case NFS4_OP_OPENATTR:
        break;

      case NFS4_OP_OPEN_CONFIRM:
        nfs4_op_open_confirm_CopyRes(&(res_dst->nfs_resop4_u.opopen_confirm),
                                     &(res_src->nfs_resop4_u.opopen_confirm));
        return;

      case NFS4_OP_OPEN_DOWNGRADE:
        nfs4_op_open_downgrade_CopyRes(&(res_dst->nfs_resop4_u
                                         .opopen_downgrade),
                                       &(res_src->nfs_resop4_u
                                         .opopen_downgrade));
        return;

      case NFS4_OP_PUTFH:
      case NFS4_OP_PUTPUBFH:
      case NFS4_OP_PUTROOTFH:
      case NFS4_OP_READ:
      case NFS4_OP_READDIR:
      case NFS4_OP_READLINK:
      case NFS4_OP_REMOVE:
      case NFS4_OP_RENAME:
      case NFS4_OP_RENEW:
      case NFS4_OP_RESTOREFH:
      case NFS4_OP_SAVEFH:
      case NFS4_OP_SECINFO:
      case NFS4_OP_SETATTR:
      case NFS4_OP_SETCLIENTID:
      case NFS4_OP_SETCLIENTID_CONFIRM:
      case NFS4_OP_VERIFY:
      case NFS4_OP_WRITE:
      case NFS4_OP_RELEASE_LOCKOWNER:
        break;

#ifdef _USE_NFS4_1
      case NFS4_OP_EXCHANGE_ID:
      case NFS4_OP_CREATE_SESSION:
      case NFS4_OP_SEQUENCE:
      case NFS4_OP_GETDEVICEINFO:
      case NFS4_OP_GETDEVICELIST:
      case NFS4_OP_BACKCHANNEL_CTL:
      case NFS4_OP_BIND_CONN_TO_SESSION:
      case NFS4_OP_DESTROY_SESSION:
      case NFS4_OP_FREE_STATEID:
      case NFS4_OP_GET_DIR_DELEGATION:
      case NFS4_OP_LAYOUTCOMMIT:
      case NFS4_OP_LAYOUTGET:
      case NFS4_OP_LAYOUTRETURN:
      case NFS4_OP_SECINFO_NO_NAME:
      case NFS4_OP_SET_SSV:
      case NFS4_OP_TEST_STATEID:
      case NFS4_OP_WANT_DELEGATION:
      case NFS4_OP_DESTROY_CLIENTID:
      case NFS4_OP_RECLAIM_COMPLETE:
        break;
#endif

      case NFS4_OP_ILLEGAL:
        break;
    }                       /* switch */

  LogFatal(COMPONENT_NFS_V4,
           "nfs4_Compound_CopyResOne not implemented for %d",
           res_src->resop);
}

/**
 *
 * @brief Copy the result for NFS4PROC_COMPOUND
 *
 * This function copies a single COMPOUND result.
 *
 * @param[out] res_dst Buffer to which to copy the result
 * @param[in]  res_src Result to copy
 *
 */
void nfs4_Compound_CopyRes(nfs_res_t *res_dst,
                           nfs_res_t *res_src)
{
  unsigned int i = 0;

  LogFullDebug(COMPONENT_NFS_V4,
               "nfs4_Compound_CopyRes of %p to %p (resarraylen : %i)",
               res_src, res_dst,
               res_src->res_compound4.resarray.resarray_len);

  for(i = 0; i < res_src->res_compound4.resarray.resarray_len; i++)
    nfs4_Compound_CopyResOne(&res_dst->res_compound4.resarray.resarray_val[i],
                             &res_src->res_compound4.resarray.resarray_val[i]);
}

/**
 *
 * @brief Updates operation statistics for a COMPOUND
 *
 * This function updates the NFSv4 operation specific statistics for a
 * COMPOUND4 request (either v4.0 or v4.1).
 *
 * @param[in]      arg      Argument for the COMPOUND4 request
 * @param[out]     res      Result for the COMPOUND4 request
 * @param[in,out]  stat_req Worker's structure for the NFSv4 stats
 *
 * @retval -1 if failed.
 * @retval 0 on success.
 *
 */

int nfs4_op_stat_update(nfs_arg_t *arg,
                        nfs_res_t *res,
                        nfs_request_stat_t *stat_req)
{
  int i = 0;

  switch (COMPOUND4_MINOR)
    {
    case 0:
      for(i = 0; i < res->res_compound4.resarray.resarray_len; i++)
        {
          stat_req->nb_nfs40_op += 1;
          stat_req->stat_op_nfs40[res->res_compound4.resarray
                                  .resarray_val[i].resop].total += 1;

          /* All operations's reply structures start with their
             status, whatever the name of this field */
          if(res->res_compound4.resarray.resarray_val[i].nfs_resop4_u
             .opaccess.status == NFS4_OK)
            stat_req->stat_op_nfs40[res->res_compound4.resarray
                                    .resarray_val[i].resop].success += 1;
          else
            stat_req->stat_op_nfs40[res->res_compound4.resarray
                                    .resarray_val[i].resop].failed += 1;
        }
      break;

#ifdef _USE_NFS4_1
    case 1:
      for(i = 0; i < res->res_compound4.resarray.resarray_len; i++)
        {
          stat_req->nb_nfs41_op += 1;
          stat_req->stat_op_nfs41[res->res_compound4.resarray
                                  .resarray_val[i].resop].total += 1;

          /* All operations's reply structures start with their
             status, whatever the name of this field */
          if(res->res_compound4.resarray.resarray_val[i].nfs_resop4_u
             .opaccess.status == NFS4_OK)
            stat_req->stat_op_nfs41[res->res_compound4.resarray
                                     .resarray_val[i].resop].success += 1;
          else
            stat_req->stat_op_nfs41[res->res_compound4.resarray
                                    .resarray_val[i].resop].failed += 1;
        }

      break;
#endif

    default:
      /* Bad parameter */
      return -1;
    }
  return 0;
} /* nfs4_op_stat_update */
