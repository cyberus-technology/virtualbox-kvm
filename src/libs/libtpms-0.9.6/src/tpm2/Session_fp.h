/********************************************************************************/
/*										*/
/*			     				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Session_fp.h $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2019				*/
/*										*/
/********************************************************************************/

#ifndef SESSION_FP_H
#define SESSION_FP_H

BOOL
SessionStartup(
	       STARTUP_TYPE     type
	       );
BOOL
SessionIsLoaded(
		TPM_HANDLE       handle         // IN: session handle
		);
BOOL
SessionIsSaved(
	       TPM_HANDLE       handle         // IN: session handle
	       );
BOOL
SequenceNumberForSavedContextIsValid(
				      TPMS_CONTEXT    *context        // IN: pointer to a context structure to be
				      //     validated
				      );
BOOL
SessionPCRValueIsCurrent(
			 SESSION         *session        // IN: session structure
			 );
SESSION *
SessionGet(
	   TPM_HANDLE       handle         // IN: session handle
	   );
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
	      );
TPM_RC
SessionContextSave(
		   TPM_HANDLE           handle,        // IN: session handle
		   CONTEXT_COUNTER     *contextID      // OUT: assigned contextID
		   );
TPM_RC
SessionContextLoad(
		   SESSION_BUF     *session,       // IN: session structure from saved context
		   TPM_HANDLE      *handle         // IN/OUT: session handle
		   );
void
SessionFlush(
	     TPM_HANDLE       handle         // IN: loaded or saved session handle
	     );
void
SessionComputeBoundEntity(
			  TPMI_DH_ENTITY       entityHandle,  // IN: handle of entity
			  TPM2B_NAME          *bind           // OUT: binding value
			  );
void
SessionSetStartTime(
		    SESSION         *session        // IN: the session to update
		    );
void
SessionResetPolicyData(
		       SESSION         *session        // IN: the session to reset
		       );
TPMI_YES_NO
SessionCapGetLoaded(
		    TPMI_SH_POLICY   handle,        // IN: start handle
		    UINT32           count,         // IN: count of returned handles
		    TPML_HANDLE     *handleList     // OUT: list of handle
		    );
TPMI_YES_NO
SessionCapGetSaved(
		   TPMI_SH_HMAC     handle,        // IN: start handle
		   UINT32           count,         // IN: count of returned handles
		   TPML_HANDLE     *handleList     // OUT: list of handle
		   );
UINT32
SessionCapGetLoadedNumber(
			  void
			  );
UINT32
SessionCapGetLoadedAvail(
			 void
			 );
UINT32
SessionCapGetActiveNumber(
			  void
			  );
UINT32
SessionCapGetActiveAvail(
			 void
			 );


#endif
