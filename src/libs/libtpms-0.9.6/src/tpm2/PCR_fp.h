/********************************************************************************/
/*										*/
/*		Functions Needed for PCR Access and Manipulation		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: PCR_fp.h $			*/
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

#ifndef PCR_FP_H
#define PCR_FP_H

BOOL
PCRBelongsAuthGroup(
		    TPMI_DH_PCR      handle,        // IN: handle of PCR
		    UINT32          *groupIndex     // OUT: group index if PCR belongs a
		    //      group that allows authValue.  If PCR
		    //      does not belong to an authorization
		    //      group, the value in this parameter is
		    //      invalid
		    );
BOOL
PCRBelongsPolicyGroup(
		      TPMI_DH_PCR      handle,        // IN: handle of PCR
		      UINT32          *groupIndex     // OUT: group index if PCR belongs a group that
		      //     allows policy.  If PCR does not belong to
		      //     a policy group, the value in this
		      //     parameter is invalid
		      );
BOOL
PCRPolicyIsAvailable(
		     TPMI_DH_PCR      handle         // IN: PCR handle
		     );
TPM2B_AUTH *
PCRGetAuthValue(
		TPMI_DH_PCR      handle         // IN: PCR handle
		);
TPMI_ALG_HASH
PCRGetAuthPolicy(
		 TPMI_DH_PCR      handle,        // IN: PCR handle
		 TPM2B_DIGEST    *policy         // OUT: policy of PCR
		 );
void
PCRSimStart(
	    void
	    );
BOOL
PcrIsAllocated(
	       UINT32           pcr,           // IN: The number of the PCR
	       TPMI_ALG_HASH    hashAlg        // IN: The PCR algorithm
	       );
void
PcrDrtm(
	const TPMI_DH_PCR        pcrHandle,     // IN: the index of the PCR to be
	//     modified
	const TPMI_ALG_HASH      hash,          // IN: the bank identifier
	const TPM2B_DIGEST      *digest         // IN: the digest to modify the PCR
	);
void
PCR_ClearAuth(
	      void
	      );
BOOL
PCRStartup(
	   STARTUP_TYPE     type,          // IN: startup type
	   BYTE             locality       // IN: startup locality
	   );
void
PCRStateSave(
	     TPM_SU           type           // IN: startup type
	     );
BOOL
PCRIsStateSaved(
		TPMI_DH_PCR      handle         // IN: PCR handle to be extended
		);
BOOL
PCRIsResetAllowed(
		  TPMI_DH_PCR      handle         // IN: PCR handle to be extended
		  );
void
PCRChanged(
	   TPM_HANDLE       pcrHandle      // IN: the handle of the PCR that changed.
	   );
BOOL
PCRIsExtendAllowed(
		   TPMI_DH_PCR      handle         // IN: PCR handle to be extended
		   );
void
PCRExtend(
	  TPMI_DH_PCR      handle,        // IN: PCR handle to be extended
	  TPMI_ALG_HASH    hash,          // IN: hash algorithm of PCR
	  UINT32           size,          // IN: size of data to be extended
	  BYTE            *data           // IN: data to be extended
	  );
void
PCRComputeCurrentDigest(
			TPMI_ALG_HASH        hashAlg,       // IN: hash algorithm to compute digest
			TPML_PCR_SELECTION  *selection,     // IN/OUT: PCR selection (filtered on
			//     output)
			TPM2B_DIGEST        *digest         // OUT: digest
			);
void
PCRRead(
	TPML_PCR_SELECTION  *selection,     // IN/OUT: PCR selection (filtered on
	//     output)
	TPML_DIGEST         *digest,        // OUT: digest
	UINT32              *pcrCounter     // OUT: the current value of PCR generation
	//     number
	);
TPM_RC
PCRAllocate(
	    TPML_PCR_SELECTION  *allocate,      // IN: required allocation
	    UINT32              *maxPCR,        // OUT: Maximum number of PCR
	    UINT32              *sizeNeeded,    // OUT: required space
	    UINT32              *sizeAvailable  // OUT: available space
	    );
void
PCRSetValue(
	    TPM_HANDLE       handle,        // IN: the handle of the PCR to set
	    INT8             initialValue   // IN: the value to set
	    );
void
PCRResetDynamics(
		 void
		 );
TPMI_YES_NO
PCRCapGetAllocation(
		    UINT32               count,         // IN: count of return
		    TPML_PCR_SELECTION  *pcrSelection   // OUT: PCR allocation list
		    );
TPMI_YES_NO
PCRCapGetProperties(
		    TPM_PT_PCR                   property,      // IN: the starting PCR property
		    UINT32                       count,         // IN: count of returned properties
		    TPML_TAGGED_PCR_PROPERTY    *select         // OUT: PCR select
		    );
TPMI_YES_NO
PCRCapGetHandles(
		 TPMI_DH_PCR      handle,        // IN: start handle
		 UINT32           count,         // IN: count of returned handles
		 TPML_HANDLE     *handleList     // OUT: list of handle
		 );


#endif
