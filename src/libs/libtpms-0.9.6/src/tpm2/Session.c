/********************************************************************************/
/*										*/
/*			    Manage the session context counter 			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Session.c $		*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2016 - 2020				*/
/*										*/
/********************************************************************************/


/* 8.9.2 Includes, Defines, and Local Variables */
#define SESSION_C
#include "Tpm.h"

/* 8.9.3	File Scope Function -- ContextIdSetOldest() */

static void
ContextIdSetOldest(
		   void
		   )
{
    CONTEXT_SLOT    lowBits;
    CONTEXT_SLOT    entry;
    CONTEXT_SLOT    smallest = CONTEXT_SLOT_MASKED(~0);	// libtpms changed
    UINT32  i;
    pAssert(s_ContextSlotMask == 0xff || s_ContextSlotMask == 0xffff); // libtpms added
    // Set oldestSaveContext to a value indicating none assigned
    s_oldestSavedSession = MAX_ACTIVE_SESSIONS + 1;
    lowBits = CONTEXT_SLOT_MASKED(gr.contextCounter);	// libtpms changed
    for(i = 0; i < MAX_ACTIVE_SESSIONS; i++)
	{
	    entry = gr.contextArray[i];
	    // only look at entries that are saved contexts
	    if(entry > MAX_LOADED_SESSIONS)
		{
		    // Use a less than or equal in case the oldest
		    // is brand new (= lowBits-1) and equal to our initial
		    // value for smallest.
		    if(CONTEXT_SLOT_MASKED(entry - lowBits) <= smallest)	// libtpms changed
			{
			    smallest = CONTEXT_SLOT_MASKED(entry - lowBits);	// libtpms changed
			    s_oldestSavedSession = i;
			}
		}
	}
    // When we finish, either the s_oldestSavedSession still has its initial
    // value, or it has the index of the oldest saved context.
}
/* 8.9.4 Startup Function -- SessionStartup() */
/* This function initializes the session subsystem on TPM2_Startup(). */
BOOL
SessionStartup(
	       STARTUP_TYPE     type
	       )
{
    UINT32               i;
    // Initialize session slots.  At startup, all the in-memory session slots
    // are cleared and marked as not occupied
    for(i = 0; i < MAX_LOADED_SESSIONS; i++)
	s_sessions[i].occupied = FALSE;   // session slot is not occupied
    // The free session slots the number of maximum allowed loaded sessions
    s_freeSessionSlots = MAX_LOADED_SESSIONS;
    // Initialize context ID data.  On a ST_SAVE or hibernate sequence, it will
    // scan the saved array of session context counts, and clear any entry that
    // references a session that was in memory during the state save since that
    // memory was not preserved over the ST_SAVE.
    if(type == SU_RESUME || type == SU_RESTART)
	{
	    // On ST_SAVE we preserve the contexts that were saved but not the ones
	    // in memory
	    for(i = 0; i < MAX_ACTIVE_SESSIONS; i++)
		{
		    // If the array value is unused or references a loaded session then
		    // that loaded session context is lost and the array entry is
		    // reclaimed.
		    if(gr.contextArray[i] <= MAX_LOADED_SESSIONS)
			gr.contextArray[i] = 0;
		}
	    // Find the oldest session in context ID data and set it in
	    // s_oldestSavedSession
	    ContextIdSetOldest();
	}
    else
	{
	    // For STARTUP_CLEAR, clear out the contextArray
	    for(i = 0; i < MAX_ACTIVE_SESSIONS; i++)
		gr.contextArray[i] = 0;
	    // reset the context counter
	    gr.contextCounter = MAX_LOADED_SESSIONS + 1;
	    // Initialize oldest saved session
	    s_oldestSavedSession = MAX_ACTIVE_SESSIONS + 1;

	    // Initialize the context slot mask for UINT16
	    s_ContextSlotMask = 0xffff;	// libtpms added
	}
    return TRUE;
}
/* 8.9.5 Access Functions */
/* 8.9.5.1 SessionIsLoaded() */
/* This function test a session handle references a loaded session.  The handle must have previously
   been checked to make sure that it is a valid handle for an authorization session. */
/* NOTE: A PWAP authorization does not have a session. */
/* Return Values Meaning */
/* TRUE if session is loaded */
/* FALSE if it is not loaded */
BOOL
SessionIsLoaded(
		TPM_HANDLE       handle         // IN: session handle
		)
{
    pAssert(HandleGetType(handle) == TPM_HT_POLICY_SESSION
	    || HandleGetType(handle) == TPM_HT_HMAC_SESSION);
    handle = handle & HR_HANDLE_MASK;
    // if out of range of possible active session, or not assigned to a loaded
    // session return false
    if(handle >= MAX_ACTIVE_SESSIONS
       || gr.contextArray[handle] == 0
       || gr.contextArray[handle] > MAX_LOADED_SESSIONS)
	return FALSE;
    return TRUE;
}
/* 8.9.5.2 SessionIsSaved() */
/* This function test a session handle references a saved session.  The handle must have previously
   been checked to make sure that it is a valid handle for an authorization session. */
/* NOTE: A password authorization does not have a session. */
/* This function requires that the handle be a valid session handle. */
/* Return Values Meaning */
/* TRUE if session is saved */
/* FALSE if it is not saved */
BOOL
SessionIsSaved(
	       TPM_HANDLE       handle         // IN: session handle
	       )
{
    pAssert(HandleGetType(handle) == TPM_HT_POLICY_SESSION
	    || HandleGetType(handle) == TPM_HT_HMAC_SESSION);
    handle = handle & HR_HANDLE_MASK;
    // if out of range of possible active session, or not assigned, or
    // assigned to a loaded session, return false
    if(handle >= MAX_ACTIVE_SESSIONS
       || gr.contextArray[handle] == 0
       || gr.contextArray[handle] <= MAX_LOADED_SESSIONS
       )
	return FALSE;
    return TRUE;
}
/* 8.9.5.3 SequenceNumberForSavedContextIsValid() */
BOOL
SequenceNumberForSavedContextIsValid(
				      TPMS_CONTEXT    *context        // IN: pointer to a context
				      // structure to be validated
				      )
{
#define MAX_CONTEXT_GAP ((UINT64)(CONTEXT_SLOT_MASKED(~0) + 1)) /* libtpms changed */
    pAssert(s_ContextSlotMask == 0xff || s_ContextSlotMask == 0xffff); // libtpms added

    TPM_HANDLE           handle = context->savedHandle & HR_HANDLE_MASK;
    if(// Handle must be with the range of active sessions
       handle >= MAX_ACTIVE_SESSIONS
       // the array entry must be for a saved context
       || gr.contextArray[handle] <= MAX_LOADED_SESSIONS
       // the array entry must agree with the sequence number
       || gr.contextArray[handle] != CONTEXT_SLOT_MASKED(context->sequence) // libtpms changed
       // the provided sequence number has to be less than the current counter
       || context->sequence > gr.contextCounter
       // but not so much that it could not be a valid sequence number
       || gr.contextCounter - context->sequence > MAX_CONTEXT_GAP)
	return FALSE;
    return TRUE;
}
/* 8.9.5.4 SessionPCRValueIsCurrent() */
/* This function is used to check if PCR values have been updated since the last time they were
   checked in a policy session. */
/* This function requires the session is loaded. */
/* Return Values Meaning */
/* TRUE if PCR value is current */
/* FALSE if PCR value is not current */
BOOL
SessionPCRValueIsCurrent(
			 SESSION         *session        // IN: session structure
			 )
{
    if(session->pcrCounter != 0
       && session->pcrCounter != gr.pcrCounter
       )
	return FALSE;
    else
	return TRUE;
}
/* 8.9.5.5 SessionGet() */
/* This function returns a pointer to the session object associated with a session handle. */
/* The function requires that the session is loaded. */
SESSION *
SessionGet(
	   TPM_HANDLE       handle         // IN: session handle
	   )
{
    size_t          slotIndex;
    CONTEXT_SLOT    sessionIndex;
    pAssert(HandleGetType(handle) == TPM_HT_POLICY_SESSION
	    || HandleGetType(handle) == TPM_HT_HMAC_SESSION
	    );
    slotIndex = handle & HR_HANDLE_MASK;
    pAssert(slotIndex < MAX_ACTIVE_SESSIONS);
    // get the contents of the session array.  Because session is loaded, we
    // should always get a valid sessionIndex
    sessionIndex = gr.contextArray[slotIndex] - 1;
    pAssert(sessionIndex < MAX_LOADED_SESSIONS);
    return &s_sessions[sessionIndex].session;
}
/* 8.9.6 Utility Functions */
/* 8.9.6.1 ContextIdSessionCreate() */
/* This function is called when a session is created.  It will check to see if the current gap would
   prevent a context from being saved.  If so it will return TPM_RC_CONTEXT_GAP.  Otherwise, it will
   try to find an open slot in contextArray, set contextArray to the slot. This routine requires
   that the caller has determined the session array index for the session. */
/* TPM_RC_CONTEXT_GAP can't assign a new contextID until the oldest saved session context is
   recycled */
/* TPM_RC_SESSION_HANDLE there is no slot available in the context array for tracking of this
   session context */
static TPM_RC
ContextIdSessionCreate(
		       TPM_HANDLE      *handle, /* OUT: receives the assigned handle. This will be
						   an index that must be adjusted by the caller
						   according to the type of the session created */
		       UINT32           sessionIndex   /* IN: The session context array entry that
							  will be occupied by the created session */
		       )
{
    pAssert(sessionIndex < MAX_LOADED_SESSIONS);
    // check to see if creating the context is safe
    // Is this going to be an assignment for the last session context
    // array entry?  If so, then there will be no room to recycle the
    // oldest context if needed.  If the gap is not at maximum, then
    // it will be possible to save a context if it becomes necessary.
    if(s_oldestSavedSession < MAX_ACTIVE_SESSIONS
       && s_freeSessionSlots == 1)
	{
	    // See if the gap is at maximum
	    // The current value of the contextCounter will be assigned to the next
	    // saved context. If the value to be assigned would make the same as an
	    // existing context, then we can't use it because of the ambiguity it would
	    // create.
	    if(CONTEXT_SLOT_MASKED(gr.contextCounter) // libtpms changed
	       == gr.contextArray[s_oldestSavedSession])
		return TPM_RC_CONTEXT_GAP;
	}
    // Find an unoccupied entry in the contextArray
    for(*handle = 0; *handle < MAX_ACTIVE_SESSIONS; (*handle)++)
	{
	    if(gr.contextArray[*handle] == 0)
		{
		    // indicate that the session associated with this handle
		    // references a loaded session
		    gr.contextArray[*handle] = CONTEXT_SLOT_MASKED(sessionIndex + 1); // libtpms changed
		    return TPM_RC_SUCCESS;
		}
	}
    return TPM_RC_SESSION_HANDLES;
}
/* 8.9.6.2 SessionCreate() */
/* This function does the detailed work for starting an authorization session. This is done in a
   support routine rather than in the action code because the session management may differ in
   implementations.  This implementation uses a fixed memory allocation to hold sessions and a fixed
   allocation to hold the contextID for the saved contexts. */
/* Error Returns Meaning */
/* TPM_RC_CONTEXT_GAP need to recycle sessions */
/* TPM_RC_SESSION_HANDLE active session space is full */
/* TPM_RC_SESSION_MEMORY loaded session space is full */
TPM_RC
SessionCreate(
	      TPM_SE           sessionType,   // IN: the session type
	      TPMI_ALG_HASH    authHash,      // IN: the hash algorithm
	      TPM2B_NONCE     *nonceCaller,   // IN: initial nonceCaller
	      TPMT_SYM_DEF    *symmetric,     // IN: the symmetric algorithm
	      TPMI_DH_ENTITY   bind,          // IN: the bind object
	      TPM2B_DATA      *seed,          // IN: seed data
	      TPM_HANDLE      *sessionHandle, // OUT: the session handle
	      TPM2B_NONCE     *nonceTpm       // OUT: the session nonce
	      )
{
    TPM_RC               result = TPM_RC_SUCCESS;
    CONTEXT_SLOT         slotIndex;
    SESSION             *session = NULL;
    pAssert(sessionType == TPM_SE_HMAC
	    || sessionType == TPM_SE_POLICY
	    || sessionType == TPM_SE_TRIAL);
    // If there are no open spots in the session array, then no point in searching
    if(s_freeSessionSlots == 0)
	return TPM_RC_SESSION_MEMORY;
    // Find a space for loading a session
    for(slotIndex = 0; slotIndex < MAX_LOADED_SESSIONS; slotIndex++)
	{
	    // Is this available?
	    if(s_sessions[slotIndex].occupied == FALSE)
		{
		    session = &s_sessions[slotIndex].session;
		    break;
		}
	}
    // if no spot found, then this is an internal error
    if(slotIndex >= MAX_LOADED_SESSIONS) {		// libtpms changed
	FAIL(FATAL_ERROR_INTERNAL);
	// should never get here due to longjmp	in FAIL()  libtpms added begin; cppcheck
	return TPM_RC_FAILURE;
    }							// libtpms added end
    // Call context ID function to get a handle.  TPM_RC_SESSION_HANDLE may be
    // returned from ContextIdHandelAssign()
    result = ContextIdSessionCreate(sessionHandle, slotIndex);
    if(result != TPM_RC_SUCCESS)
	return result;
    //*** Only return from this point on is TPM_RC_SUCCESS
    // Can now indicate that the session array entry is occupied.
    s_freeSessionSlots--;
    s_sessions[slotIndex].occupied = TRUE;
    // Initialize the session data
    MemorySet(session, 0, sizeof(SESSION));
    // Initialize internal session data
    session->authHashAlg = authHash;
    // Initialize session type
    if(sessionType == TPM_SE_HMAC)
	{
	    *sessionHandle += HMAC_SESSION_FIRST;
	}
    else
	{
	    *sessionHandle += POLICY_SESSION_FIRST;
	    // For TPM_SE_POLICY or TPM_SE_TRIAL
	    session->attributes.isPolicy = SET;
	    if(sessionType == TPM_SE_TRIAL)
		session->attributes.isTrialPolicy = SET;
	    SessionSetStartTime(session);
	    // Initialize policyDigest.  policyDigest is initialized with a string of 0
	    // of session algorithm digest size. Since the session is already clear.
	    // Just need to set the size
	    session->u2.policyDigest.t.size =
		CryptHashGetDigestSize(session->authHashAlg);
	}
    // Create initial session nonce
    session->nonceTPM.t.size = nonceCaller->t.size;
    CryptRandomGenerate(session->nonceTPM.t.size, session->nonceTPM.t.buffer);
    MemoryCopy2B(&nonceTpm->b, &session->nonceTPM.b,
		 sizeof(nonceTpm->t.buffer));
    // Set up session parameter encryption algorithm
    session->symmetric = *symmetric;
    // If there is a bind object or a session secret, then need to compute
    // a sessionKey.
    if(bind != TPM_RH_NULL || seed->t.size != 0)
	{
	    // sessionKey = KDFa(hash, (authValue || seed), "ATH", nonceTPM,
	    //                      nonceCaller, bits)
	    // The HMAC key for generating the sessionSecret can be the concatenation
	    // of an authorization value and a seed value
	    TPM2B_TYPE(KEY, (sizeof(TPMT_HA) + sizeof(seed->t.buffer)));
	    TPM2B_KEY            key;
	    // Get hash size, which is also the length of sessionKey
	    session->sessionKey.t.size = CryptHashGetDigestSize(session->authHashAlg);
	    // Get authValue of associated entity
	    EntityGetAuthValue(bind, (TPM2B_AUTH *)&key);
	    pAssert(key.t.size + seed->t.size <= sizeof(key.t.buffer));
	    // Concatenate authValue and seed
	    MemoryConcat2B(&key.b, &seed->b, sizeof(key.t.buffer));
	    // Compute the session key
	    CryptKDFa(session->authHashAlg, &key.b, SESSION_KEY, &session->nonceTPM.b,
		      &nonceCaller->b,
		      session->sessionKey.t.size * 8, session->sessionKey.t.buffer,
		      NULL, FALSE);
	}
    // Copy the name of the entity that the HMAC session is bound to
    // Policy session is not bound to an entity
    if(bind != TPM_RH_NULL && sessionType == TPM_SE_HMAC)
	{
	    session->attributes.isBound = SET;
	    SessionComputeBoundEntity(bind, &session->u1.boundEntity);
	}
    // If there is a bind object and it is subject to DA, then use of this session
    // is subject to DA regardless of how it is used.
    session->attributes.isDaBound = (bind != TPM_RH_NULL)
				    && (IsDAExempted(bind) == FALSE);
    // If the session is bound, then check to see if it is bound to lockoutAuth
    session->attributes.isLockoutBound = (session->attributes.isDaBound == SET)
					 && (bind == TPM_RH_LOCKOUT);
    return TPM_RC_SUCCESS;
}
/* 8.9.6.3 SessionContextSave() */
/* This function is called when a session context is to be saved.  The contextID of the saved
   session is returned.  If no contextID can be assigned, then the routine returns
   TPM_RC_CONTEXT_GAP. If the function completes normally, the session slot will be freed. */
/* This function requires that handle references a loaded session. Otherwise, it should not be
   called at the first place. */
/* Error Returns Meaning */
/* TPM_RC_CONTEXT_GAP a contextID could not be assigned. */
/* TPM_RC_TOO_MANY_CONTEXTS the counter maxed out */
TPM_RC
SessionContextSave(
		   TPM_HANDLE           handle,        // IN: session handle
		   CONTEXT_COUNTER     *contextID      // OUT: assigned contextID
		   )
{
    UINT32                      contextIndex;
    CONTEXT_SLOT                slotIndex;
    pAssert(SessionIsLoaded(handle));
    pAssert(s_ContextSlotMask == 0xff || s_ContextSlotMask == 0xffff); // libtpms added
    // check to see if the gap is already maxed out
    // Need to have a saved session
    if(s_oldestSavedSession < MAX_ACTIVE_SESSIONS
       // if the oldest saved session has the same value as the low bits
       // of the contextCounter, then the GAP is maxed out.
       && gr.contextArray[s_oldestSavedSession] == CONTEXT_SLOT_MASKED(gr.contextCounter)) // libtpms changed
	return TPM_RC_CONTEXT_GAP;
    // if the caller wants the context counter, set it
    if(contextID != NULL)
	*contextID = gr.contextCounter;
    contextIndex = handle & HR_HANDLE_MASK;
    pAssert(contextIndex < MAX_ACTIVE_SESSIONS);
    // Extract the session slot number referenced by the contextArray
    // because we are going to overwrite this with the low order
    // contextID value.
    slotIndex = gr.contextArray[contextIndex] - 1;
    // Set the contextID for the contextArray
    gr.contextArray[contextIndex] = CONTEXT_SLOT_MASKED(gr.contextCounter); // libtpms changed
    // Increment the counter
    gr.contextCounter++;
    // In the unlikely event that the 64-bit context counter rolls over...
    if(gr.contextCounter == 0)
	{
	    // back it up
	    gr.contextCounter--;
	    // return an error
	    return TPM_RC_TOO_MANY_CONTEXTS;
	}
    // if the low-order bits wrapped, need to advance the value to skip over
    // the values used to indicate that a session is loaded
    if(CONTEXT_SLOT_MASKED(gr.contextCounter) == 0) // libtpms changed
	gr.contextCounter += MAX_LOADED_SESSIONS + 1;
    // If no other sessions are saved, this is now the oldest.
    if(s_oldestSavedSession >= MAX_ACTIVE_SESSIONS)
	s_oldestSavedSession = contextIndex;
    // Mark the session slot as unoccupied
    s_sessions[slotIndex].occupied = FALSE;
    // and indicate that there is an additional open slot
    s_freeSessionSlots++;
    return TPM_RC_SUCCESS;
}
/* 8.9.6.4 SessionContextLoad() */
/* This function is used to load a session from saved context.  The session handle must be for a
   saved context. */
/* If the gap is at a maximum, then the only session that can be loaded is the oldest session,
   otherwise TPM_RC_CONTEXT_GAP is returned. */
/* This function requires that handle references a valid saved session. */
/* Error Returns Meaning */
/* TPM_RC_SESSION_MEMORY no free session slots */
/* TPM_RC_CONTEXT_GAP the gap count is maximum and this is not the oldest saved context */
TPM_RC
SessionContextLoad(
		   SESSION_BUF     *session,       // IN: session structure from saved context
		   TPM_HANDLE      *handle         // IN/OUT: session handle
		   )
{
    UINT32              contextIndex;
    CONTEXT_SLOT        slotIndex;
    pAssert(s_ContextSlotMask == 0xff || s_ContextSlotMask == 0xffff); // libtpms added
    pAssert(HandleGetType(*handle) == TPM_HT_POLICY_SESSION
	    || HandleGetType(*handle) == TPM_HT_HMAC_SESSION);
    // Don't bother looking if no openings
    if(s_freeSessionSlots == 0)
	return TPM_RC_SESSION_MEMORY;
    // Find a free session slot to load the session
    for(slotIndex = 0; slotIndex < MAX_LOADED_SESSIONS; slotIndex++)
	if(s_sessions[slotIndex].occupied == FALSE) break;
    // if no spot found, then this is an internal error
    pAssert(slotIndex < MAX_LOADED_SESSIONS);
    // libtpms: besides the s_freeSessionSlots guard add another array index guard
    if (slotIndex >= MAX_LOADED_SESSIONS) {	// libtpms added begin; cppcheck
	FAIL(FATAL_ERROR_INTERNAL);
	// should never get here due to longjmp	in FAIL()
	return TPM_RC_FAILURE;
    }						// libtpms added end
    contextIndex = *handle & HR_HANDLE_MASK;   // extract the index
    // If there is only one slot left, and the gap is at maximum, the only session
    // context that we can safely load is the oldest one.
    if(s_oldestSavedSession < MAX_ACTIVE_SESSIONS
       && s_freeSessionSlots == 1
       && CONTEXT_SLOT_MASKED(gr.contextCounter) == gr.contextArray[s_oldestSavedSession] // libtpms changed
       && contextIndex != s_oldestSavedSession)
	return TPM_RC_CONTEXT_GAP;
    pAssert(contextIndex < MAX_ACTIVE_SESSIONS);
    // set the contextArray value to point to the session slot where
    // the context is loaded
    gr.contextArray[contextIndex] = slotIndex + 1;
    // if this was the oldest context, find the new oldest
    if(contextIndex == s_oldestSavedSession)
	ContextIdSetOldest();
    // Copy session data to session slot
    MemoryCopy(&s_sessions[slotIndex].session, session, sizeof(SESSION));
    // Set session slot as occupied
    s_sessions[slotIndex].occupied = TRUE;
    // Reduce the number of open spots
    s_freeSessionSlots--;
    return TPM_RC_SUCCESS;
}
/* 8.9.6.5 SessionFlush() */
/* This function is used to flush a session referenced by its handle.  If the session associated
   with handle is loaded, the session array entry is marked as available. */
/* This function requires that handle be a valid active session. */
void
SessionFlush(
	     TPM_HANDLE       handle         // IN: loaded or saved session handle
	     )
{
    CONTEXT_SLOT         slotIndex;
    UINT32               contextIndex;   // Index into contextArray
    pAssert((HandleGetType(handle) == TPM_HT_POLICY_SESSION
	     || HandleGetType(handle) == TPM_HT_HMAC_SESSION
	     )
	    && (SessionIsLoaded(handle) || SessionIsSaved(handle))
	    );
    // Flush context ID of this session
    // Convert handle to an index into the contextArray
    contextIndex = handle & HR_HANDLE_MASK;
    pAssert(contextIndex < sizeof(gr.contextArray) / sizeof(gr.contextArray[0]));
    // Get the current contents of the array
    slotIndex = gr.contextArray[contextIndex];
    // Mark context array entry as available
    gr.contextArray[contextIndex] = 0;
    // Is this a saved session being flushed
    if(slotIndex > MAX_LOADED_SESSIONS)
	{
	    // Flushing the oldest session?
	    if(contextIndex == s_oldestSavedSession)
		// If so, find a new value for oldest.
		ContextIdSetOldest();
	}
    else
	{
	    // Adjust slot index to point to session array index
	    slotIndex -= 1;
	    // Free session array index
	    s_sessions[slotIndex].occupied = FALSE;
	    s_freeSessionSlots++;
	}
    return;
}
/* 8.9.6.6 SessionComputeBoundEntity() */
/* This function computes the binding value for a session.  The binding value for a reserved handle
   is the handle itself.  For all the other entities, the authValue at the time of binding is
   included to prevent squatting. For those values, the Name and the authValue are concatenated into
   the bind buffer.  If they will not both fit, the will be overlapped by XORing() bytes.  If XOR is
   required, the bind value will be full. */
void
SessionComputeBoundEntity(
			  TPMI_DH_ENTITY       entityHandle,  // IN: handle of entity
			  TPM2B_NAME          *bind           // OUT: binding value
			  )
{
    TPM2B_AUTH           auth;
    BYTE                *pAuth = auth.t.buffer;
    UINT16               i;
    // Get name
    EntityGetName(entityHandle, bind);
    //    // The bound value of a reserved handle is the handle itself
    //    if(bind->t.size == sizeof(TPM_HANDLE)) return;
    // For all the other entities, concatenate the authorization value to the name.
    // Get a local copy of the authorization value because some overlapping
    // may be necessary.
    EntityGetAuthValue(entityHandle, &auth);
    // Make sure that the extra space is zeroed
    MemorySet(&bind->t.name[bind->t.size], 0, sizeof(bind->t.name) - bind->t.size);
    // XOR the authValue at the end of the name
    for(i = sizeof(bind->t.name) - auth.t.size; i < sizeof(bind->t.name); i++)
	bind->t.name[i] ^= *pAuth++;
    // Set the bind value to the maximum size
    bind->t.size = sizeof(bind->t.name);
    return;
}
/* 8.9.6.7 SessionSetStartTime() */
/* This function is used to initialize the session timing */
void
SessionSetStartTime(
		    SESSION         *session        // IN: the session to update
		    )
{
    session->startTime = g_time;
    session->epoch = g_timeEpoch;
    session->timeout = 0;
}
/* 8.9.6.8 SessionResetPolicyData() */
/* This function is used to reset the policy data without changing the nonce or the start time of
   the session. */
void
SessionResetPolicyData(
		       SESSION         *session        // IN: the session to reset
		       )
{
    SESSION_ATTRIBUTES      oldAttributes;
    pAssert(session != NULL);
    // Will need later
    oldAttributes = session->attributes;
    // No command
    session->commandCode = 0;
    // No locality selected
    MemorySet(&session->commandLocality, 0, sizeof(session->commandLocality));
    // The cpHash size to zero
    session->u1.cpHash.b.size = 0;
    // No timeout
    session->timeout = 0;
    // Reset the pcrCounter
    session->pcrCounter = 0;
    // Reset the policy hash
    MemorySet(&session->u2.policyDigest.t.buffer, 0,
	      session->u2.policyDigest.t.size);
    // Reset the session attributes
    MemorySet(&session->attributes, 0, sizeof(SESSION_ATTRIBUTES));
    // Restore the policy attributes
    session->attributes.isPolicy = SET;
    session->attributes.isTrialPolicy = oldAttributes.isTrialPolicy;
    // Restore the bind attributes
    session->attributes.isDaBound = oldAttributes.isDaBound;
    session->attributes.isLockoutBound = oldAttributes.isLockoutBound;
}
/* 8.9.6.9 SessionCapGetLoaded() */
/* This function returns a list of handles of loaded session, started from input handle */
/* Handle must be in valid loaded session handle range, but does not have to point to a loaded
   session. */
/* Return Values Meaning */
/* YES if there are more handles available */
/* NO all the available handles has been returned */
TPMI_YES_NO
SessionCapGetLoaded(
		    TPMI_SH_POLICY   handle,        // IN: start handle
		    UINT32           count,         // IN: count of returned handles
		    TPML_HANDLE     *handleList     // OUT: list of handle
		    )
{
    TPMI_YES_NO     more = NO;
    UINT32          i;
    pAssert(HandleGetType(handle) == TPM_HT_LOADED_SESSION);
    // Initialize output handle list
    handleList->count = 0;
    // The maximum count of handles we may return is MAX_CAP_HANDLES
    if(count > MAX_CAP_HANDLES) count = MAX_CAP_HANDLES;
    // Iterate session context ID slots to get loaded session handles
    for(i = handle & HR_HANDLE_MASK; i < MAX_ACTIVE_SESSIONS; i++)
	{
	    // If session is active
	    if(gr.contextArray[i] != 0)
		{
		    // If session is loaded
		    if(gr.contextArray[i] <= MAX_LOADED_SESSIONS)
			{
			    if(handleList->count < count)
				{
				    SESSION         *session;
				    // If we have not filled up the return list, add this
				    // session handle to it
				    // assume that this is going to be an HMAC session
				    handle = i + HMAC_SESSION_FIRST;
				    session = SessionGet(handle);
				    if(session->attributes.isPolicy)
					handle = i + POLICY_SESSION_FIRST;
				    handleList->handle[handleList->count] = handle;
				    handleList->count++;
				}
			    else
				{
				    // If the return list is full but we still have loaded object
				    // available, report this and stop iterating
				    more = YES;
				    break;
				}
			}
		}
	}
    return more;
}
/* 8.9.6.10 SessionCapGetSaved() */
/* This function returns a list of handles for saved session, starting at handle. */
/* Handle must be in a valid handle range, but does not have to point to a saved session */
/* Return Values Meaning */
/* YES if there are more handles available */
/* NO all the available handles has been returned */
TPMI_YES_NO
SessionCapGetSaved(
		   TPMI_SH_HMAC     handle,        // IN: start handle
		   UINT32           count,         // IN: count of returned handles
		   TPML_HANDLE     *handleList     // OUT: list of handle
		   )
{
    TPMI_YES_NO     more = NO;
    UINT32          i;
#ifdef  TPM_HT_SAVED_SESSION
    pAssert(HandleGetType(handle) == TPM_HT_SAVED_SESSION);
#else
    pAssert(HandleGetType(handle) == TPM_HT_ACTIVE_SESSION);
#endif
    // Initialize output handle list
    handleList->count = 0;
    // The maximum count of handles we may return is MAX_CAP_HANDLES
    if(count > MAX_CAP_HANDLES) count = MAX_CAP_HANDLES;
    // Iterate session context ID slots to get loaded session handles
    for(i = handle & HR_HANDLE_MASK; i < MAX_ACTIVE_SESSIONS; i++)
	{
	    // If session is active
	    if(gr.contextArray[i] != 0)
		{
		    // If session is saved
		    if(gr.contextArray[i] > MAX_LOADED_SESSIONS)
			{
			    if(handleList->count < count)
				{
				    // If we have not filled up the return list, add this
				    // session handle to it
				    handleList->handle[handleList->count] = i + HMAC_SESSION_FIRST;
				    handleList->count++;
				}
			    else
				{
				    // If the return list is full but we still have loaded object
				    // available, report this and stop iterating
				    more = YES;
				    break;
				}
			}
		}
	}
    return more;
}
/* 8.9.6.11 SessionCapGetLoadedNumber() */
/* This function return the number of authorization sessions currently loaded into TPM RAM. */
UINT32
SessionCapGetLoadedNumber(
			  void
			  )
{
    return MAX_LOADED_SESSIONS - s_freeSessionSlots;
}
/* 8.9.6.12 SessionCapGetLoadedAvail() */
/* This function returns the number of additional authorization sessions, of any type, that could be
   loaded into TPM RAM. */
/* NOTE: In other implementations, this number may just be an estimate. The only requirement for the
   estimate is, if it is one or more, then at least one session must be loadable. */
UINT32
SessionCapGetLoadedAvail(
			 void
			 )
{
    return s_freeSessionSlots;
}
/* 8.9.6.13 SessionCapGetActiveNumber() */
/* This function returns the number of active authorization sessions currently being tracked by the
   TPM. */
UINT32
SessionCapGetActiveNumber(
			  void
			  )
{
    UINT32              i;
    UINT32              num = 0;
    // Iterate the context array to find the number of non-zero slots
    for(i = 0; i < MAX_ACTIVE_SESSIONS; i++)
	{
	    if(gr.contextArray[i] != 0) num++;
	}
    return num;
}
/* 8.9.6.14 SessionCapGetActiveAvail() */
/* This function returns the number of additional authorization sessions, of any type, that could be
   created. This not the number of slots for sessions, but the number of additional sessions that
   the TPM is capable of tracking. */
UINT32
SessionCapGetActiveAvail(
			 void
			 )
{
    UINT32              i;
    UINT32              num = 0;
    // Iterate the context array to find the number of zero slots
    for(i = 0; i < MAX_ACTIVE_SESSIONS; i++)
	{
	    if(gr.contextArray[i] == 0) num++;
	}
    return num;
}
