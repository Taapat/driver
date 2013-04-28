/**************************************************************
 * Copyright (C) 2010   STMicroelectronics. All Rights Reserved.
 * This file is part of the latest release of the Multicom4 project. This release 
 * is fully functional and provides all of the original MME functionality.This 
 * release  is now considered stable and ready for integration with other software 
 * components.

 * Multicom4 is a free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License as published by the Free Software Foundation 
 * version 2.

 * Multicom4 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along with Multicom4; 
 * see the file COPYING.  If not, write to the Free Software Foundation, 59 Temple Place - 
 * Suite 330, Boston, MA 02111-1307, USA.

 * Written by Multicom team at STMicroelectronics in November 2010.  
 * Contact multicom.support@st.com. 
**************************************************************/

/* 
 * 
 */ 

#include <mme.h>	/* External defines and prototypes */

#include "_mme_sys.h"	/* Internal defines and prototypes */


static
mme_transformer_reg_t *transformerLookup (const char *name)
{
  mme_transformer_reg_t *treg;

  /* First try and find a matching Registered transformer */
  list_for_each_entry(treg, &mme_state->regTrans, list)
  {
    if (!strcmp(name, treg->name))
    {
      /* Match found */
      return treg;
    }
  }

  /* Not found */
  return NULL;
}

mme_transformer_reg_t * mme_transformer_lookup (const char *name)
{
  mme_transformer_reg_t *treg;

  _ICS_OS_MUTEX_TAKE(&mme_state->lock);
  
  treg = transformerLookup(name);

  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

  return treg;
}

/* MME_RegisterTransformer()
 * Register a transformer after which instantiations may be made
 */
MME_ERROR MME_RegisterTransformer (const char *name,
				   MME_AbortCommand_t abortFunc,
				   MME_GetTransformerCapability_t getTransformerCapabilityFunc,
				   MME_InitTransformer_t initTransformerFunc,
				   MME_ProcessCommand_t processCommandFunc,
				   MME_TermTransformer_t termTransformerFunc)
{
  MME_ERROR              res;
  ICS_ERROR              err;
  mme_transformer_reg_t *treg = NULL;

  if (mme_state == NULL)
  {
    return MME_DRIVER_NOT_INITIALIZED;
  }

  if (name == NULL || strlen(name) > MME_MAX_TRANSFORMER_NAME)
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }

  MME_PRINTF(MME_DBG_MANAGER, "name '%s'\n", name);

  _ICS_OS_MUTEX_TAKE(&mme_state->lock);

  /* Check for duplicate transformer names */
  treg = transformerLookup(name);
  if (treg != NULL)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Transformer named '%s' already registerd\n",
		name);

    res = MME_INVALID_ARGUMENT;
    goto error_release;
  }
  
  /* Allocate a new transformer registration desc (variable length name) */
  _ICS_OS_ZALLOC(treg, offsetof(mme_transformer_reg_t, name) + strlen(name) + 1);
  if (treg == NULL)
  {
    res = MME_NOMEM;
    goto error_release;
  }

  /* Initialise the new desc */
  strcpy(treg->name, name);
  treg->funcs.abort                    = abortFunc;
  treg->funcs.getTransformerCapability = getTransformerCapabilityFunc;
  treg->funcs.initTransformer          = initTransformerFunc;
  treg->funcs.processCommand           = processCommandFunc;
  treg->funcs.termTransformer          = termTransformerFunc;

  INIT_LIST_HEAD(&treg->list);
  
  /* Add to list of locally registered transformers */
  list_add(&treg->list, &mme_state->regTrans);

  /* Now Register this Transformer in the Global nameserver
   * We supply our managerPort as the handle so we can then be
   * contacted directly for Transformer instantiations
   */
  err = ICS_nsrv_add(treg->name, &mme_state->managerPort, sizeof(mme_state->managerPort), 0, &treg->nhandle);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Failed to register transformer '%s' : %s (%d)\n",
		name,
		ics_err_str(err), err);

    res = MME_ICS_ERROR;
    goto error_free;
  }

  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

  return MME_SUCCESS;

error_free:
  list_del_init(&treg->list);
  _ICS_OS_FREE(treg);

error_release:
  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

error:
  MME_EPRINTF(MME_DBG_MANAGER,
	      "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}

/* MME_IsTransformerRegistered()
 * Find if the transformer is regisred.  
 */
MME_ERROR MME_IsTransformerRegistered (const char *name)
{
  MME_ERROR                   res;

  ICS_ERROR                   err;
  ICS_PORT                    mgrPort;
  ICS_SIZE                    size;

  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }
  
  MME_PRINTF(MME_DBG_MANAGER, "name '%s' \n", (name ? name : "(NULL)"));
 
  /* Validate parameters */
  if (name == NULL)
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }

  /* Lookup transformer name in the ICS nameserver */
  res = ICS_nsrv_lookup(name, ICS_NONBLOCK,_MME_TRANSFORMER_LOOKUP_TIMEOUT, &mgrPort, &size);
  if (res != ICS_SUCCESS)
  {
    /* Didn't find a matching nameserver entry */
    res = MME_UNKNOWN_TRANSFORMER;
    goto error;
  }
  return MME_SUCCESS;
error:
  MME_EPRINTF(MME_DBG_INIT,"(%s) : Failed : %s (%d)\n",
             (name ? name : "(NULL)"), MME_Error_Str(res), res);
  return res;
}

/* Remove transformer from registered list and free off memory */
static
MME_ERROR removeTransfomer (mme_transformer_reg_t *treg)
{
  ICS_ERROR err;

  /* First remove this Transformer from the Global nameserver */
  err = ICS_nsrv_remove(treg->nhandle, 0);
  MME_ASSERT(err == ICS_SUCCESS);
  
  list_del_init(&treg->list);
  _ICS_OS_FREE(treg);

  return MME_SUCCESS;
}

/* MME_DeregisterTransformer()
 * Deregister a transformer that has been registered
 */
MME_ERROR MME_DeregisterTransformer (const char *name)
{
  MME_ERROR              res = MME_INVALID_ARGUMENT;
  mme_transformer_reg_t *treg;

  if (mme_state == NULL)
  {
    return MME_DRIVER_NOT_INITIALIZED;
  }

  if (name == NULL || strlen(name) > MME_MAX_TRANSFORMER_NAME)
  {
    return MME_INVALID_ARGUMENT;
  }

  _ICS_OS_MUTEX_TAKE(&mme_state->lock);

  /* Check for a matching transformer name */
  list_for_each_entry(treg, &mme_state->regTrans, list)
  {
    if (strcmp(name, treg->name) == 0)
    {
      /* Match found - remove transformer from list and free memory */
      res = removeTransfomer(treg);
      break;
    }
  }

  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

  return res;
}

/* Deregister all locally registered transformers.
 *
 * Called during MME_Term() holding the mme_state lock
 */
void mme_register_term (void)
{
  mme_transformer_reg_t *treg, *tmp;

  /* Iterate over all local Registered transformers */
  list_for_each_entry_safe(treg, tmp, &mme_state->regTrans, list)
  {
    /* Match found - remove transformer from list and free memory */
    (void) removeTransfomer(treg);
  }

  MME_assert(list_empty(&mme_state->regTrans));

  return;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
