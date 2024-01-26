/********************************************************************************/
/*										*/
/*		Functions That Manage the Object Store of the TPM	  	*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Object_fp.h $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2021				*/
/*										*/
/********************************************************************************/

#ifndef OBJECT_FP_H
#define OBJECT_FP_H

void
ObjectFlush(
	    OBJECT          *object
	    );
void
ObjectSetInUse(
	       OBJECT          *object
	       );
BOOL
ObjectStartup(
	      void
	      );
void
ObjectCleanupEvict(
		   void
		   );
BOOL
IsObjectPresent(
		TPMI_DH_OBJECT   handle         // IN: handle to be checked
		);
BOOL
ObjectIsSequence(
		 OBJECT          *object         // IN: handle to be checked
		 );
OBJECT*
HandleToObject(
	       TPMI_DH_OBJECT   handle         // IN: handle of the object
	       );
UINT16
GetName(
	TPMI_DH_OBJECT   handle,        // IN: handle of the object
	NAME            *name           // OUT: name of the object
	);
void
GetQualifiedName(
		 TPMI_DH_OBJECT   handle,        // IN: handle of the object
		 TPM2B_NAME      *qualifiedName  // OUT: qualified name of the object
		 );
TPMI_RH_HIERARCHY
ObjectGetHierarchy(
		   OBJECT          *object         // IN :object
		   );
TPMI_RH_HIERARCHY
GetHieriarchy(
	     TPMI_DH_OBJECT   handle         // IN :object handle
	     );
OBJECT *
FindEmptyObjectSlot(
		    TPMI_DH_OBJECT  *handle         // OUT: (optional)
		    );
OBJECT *
ObjectAllocateSlot(
		   TPMI_DH_OBJECT  *handle        // OUT: handle of allocated object
		   );
void
ObjectSetLoadedAttributes(
			  OBJECT          *object,        // IN: object attributes to finalize
			  TPM_HANDLE       parentHandle,  // IN: the parent handle
			  SEED_COMPAT_LEVEL seedCompatLevel    // IN: seedCompatLevel to use for children
			  );
TPM_RC
ObjectLoad(
	   OBJECT          *object,        // IN: pointer to object slot
	   //     object
	   OBJECT          *parent,        // IN: (optional) the parent object
	   TPMT_PUBLIC     *publicArea,    // IN: public area to be installed in the object
	   TPMT_SENSITIVE  *sensitive,     // IN: (optional) sensitive area to be
	   //      installed in the object
	   TPM_RC           blamePublic,   // IN: parameter number to associate with the
	   //     publicArea errors
	   TPM_RC           blameSensitive,// IN: parameter number to associate with the
	   //     sensitive area errors
	   TPM2B_NAME      *name           // IN: (optional)
	   );
TPM_RC
ObjectCreateHMACSequence(
			 TPMI_ALG_HASH    hashAlg,       // IN: hash algorithm
			 OBJECT          *keyObject,     // IN: the object containing the HMAC key
			 TPM2B_AUTH      *auth,          // IN: authValue
			 TPMI_DH_OBJECT  *newHandle      // OUT: HMAC sequence object handle
			 );
TPM_RC
ObjectCreateHashSequence(
			 TPMI_ALG_HASH    hashAlg,       // IN: hash algorithm
			 TPM2B_AUTH      *auth,          // IN: authValue
			 TPMI_DH_OBJECT  *newHandle      // OUT: sequence object handle
			 );
TPM_RC
ObjectCreateEventSequence(
			  TPM2B_AUTH      *auth,          // IN: authValue
			  TPMI_DH_OBJECT  *newHandle      // OUT: sequence object handle
			  );
void
ObjectTerminateEvent(
		     void
		     );
#if 0	// libtpms added
OBJECT *
ObjectContextLoad(
		  ANY_OBJECT_BUFFER   *object,        // IN: pointer to object structure in saved
		  //     context
		  TPMI_DH_OBJECT      *handle         // OUT: object handle
		  );
#endif	// libtpms added begin
OBJECT *
ObjectContextLoadLibtpms(BYTE           *buffer,      // IN: buffer holding the marshaled object
                         INT32           size,        // IN: size of buffer
                         TPMI_DH_OBJECT *handle       // OUT: object handle
                         );
	// libtpms added end
void
FlushObject(
	    TPMI_DH_OBJECT   handle         // IN: handle to be freed
	    );
void
ObjectFlushHierarchy(
		     TPMI_RH_HIERARCHY    hierarchy      // IN: hierarchy to be flush
		     );
TPM_RC
ObjectLoadEvict(
		TPM_HANDLE      *handle,        // IN:OUT: evict object handle.  If success, it
		// will be replace by the loaded object handle
		COMMAND_INDEX    commandIndex   // IN: the command being processed
		);
TPM2B_NAME *
ObjectComputeName(
		  UINT32           size,          // IN: the size of the area to digest
		  BYTE            *publicArea,    // IN: the public area to digest area
		  TPM_ALG_ID       nameAlg,       // IN: the hash algorithm to use
		  TPM2B_NAME      *name           // OUT: Computed name
		  );
TPM2B_NAME *
PublicMarshalAndComputeName(
			    TPMT_PUBLIC     *publicArea,    // IN: public area of an object
			    TPM2B_NAME      *name           // OUT: name of the object
			    );
TPMI_ALG_HASH
AlgOfName(
	  TPM2B_NAME      *name
	  );
void
ComputeQualifiedName(
		     TPM_HANDLE       parentHandle,  // IN: parent's name
		     TPM_ALG_ID       nameAlg,       // IN: name hash
		     TPM2B_NAME      *name,          // IN: name of the object
		     TPM2B_NAME      *qualifiedName  // OUT: qualified name of the object
		     );
BOOL
ObjectIsStorage(
		TPMI_DH_OBJECT   handle         // IN: object handle
		);
TPMI_YES_NO
ObjectCapGetLoaded(
		   TPMI_DH_OBJECT   handle,        // IN: start handle
		   UINT32           count,         // IN: count of returned handles
		   TPML_HANDLE     *handleList     // OUT: list of handle
		   );
UINT32
ObjectCapGetTransientAvail(
			   void
			   );
TPMA_OBJECT
ObjectGetPublicAttributes(
			  TPM_HANDLE       handle
			  );
OBJECT_ATTRIBUTES
ObjectGetProperties(
		    TPM_HANDLE       handle
		    );


#endif
