/********************************************************************************/
/*										*/
/*			  Object Command Support   				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Object_spt_fp.h $		*/
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

#ifndef OBJECT_SPT_FP_H
#define OBJECT_SPT_FP_H

BOOL
AdjustAuthSize(
	       TPM2B_AUTH          *auth,          // IN/OUT: value to adjust
	       TPMI_ALG_HASH        nameAlg        // IN:
	       );
BOOL
ObjectIsParent(
	       OBJECT          *parentObject   // IN: parent handle
	       );
TPM_RC
CreateChecks(
	     OBJECT              *parentObject,
	     TPMT_PUBLIC         *publicArea,
	     UINT16               sensitiveDataSize
	     );
TPM_RC
SchemeChecks(
	     OBJECT          *parentObject,  // IN: parent (null if primary seed)
	     TPMT_PUBLIC     *publicArea     // IN: public area of the object
	     );
TPM_RC
PublicAttributesValidation(
			   OBJECT          *parentObject,  // IN: input parent object
			   TPMT_PUBLIC     *publicArea     // IN: public area of the object
			   );
void
FillInCreationData(
		   TPMI_DH_OBJECT           parentHandle,  // IN: handle of parent
		   TPMI_ALG_HASH            nameHashAlg,   // IN: name hash algorithm
		   TPML_PCR_SELECTION      *creationPCR,   // IN: PCR selection
		   TPM2B_DATA              *outsideData,   // IN: outside data
		   TPM2B_CREATION_DATA     *outCreation,   // OUT: creation data for output
		   TPM2B_DIGEST            *creationDigest // OUT: creation digest
		   );
const TPM2B *
GetSeedForKDF(
	      OBJECT          *protector         // IN: the protector handle
	      );
UINT16
ProduceOuterWrap(
		 OBJECT          *protector,     // IN: The handle of the object that provides
		 //     protection.  For object, it is parent
		 //     handle. For credential, it is the handle
		 //     of encrypt object.
		 TPM2B           *name,          // IN: the name of the object
		 TPM_ALG_ID       hashAlg,       // IN: hash algorithm for outer wrap
		 TPM2B           *seed,          // IN: an external seed may be provided for
		 //     duplication blob. For non duplication
		 //     blob, this parameter should be NULL
		 BOOL             useIV,         // IN: indicate if an IV is used
		 UINT16           dataSize,      // IN: the size of sensitive data, excluding the
		 //     leading integrity buffer size or the
		 //     optional iv size
		 BYTE            *outerBuffer    // IN/OUT: outer buffer with sensitive data in
		 //     it
		 );
TPM_RC
UnwrapOuter(
	    OBJECT          *protector,     // IN: The object that provides
	    //     protection.  For object, it is parent
	    //     handle. For credential, it is the
	    //     encrypt object.
	    TPM2B           *name,          // IN: the name of the object
	    TPM_ALG_ID       hashAlg,       // IN: hash algorithm for outer wrap
	    TPM2B           *seed,          // IN: an external seed may be provided for
	    //     duplication blob. For non duplication
	    //     blob, this parameter should be NULL.
	    BOOL             useIV,         // IN: indicates if an IV is used
	    UINT16           dataSize,      // IN: size of sensitive data in outerBuffer,
	    //     including the leading integrity buffer
	    //     size, and an optional iv area
	    BYTE            *outerBuffer    // IN/OUT: sensitive data
	    );
void
SensitiveToPrivate(
		   TPMT_SENSITIVE  *sensitive,     // IN: sensitive structure
		   TPM2B_NAME      *name,          // IN: the name of the object
		   OBJECT          *parent,        // IN: The parent object
		   TPM_ALG_ID       nameAlg,       // IN: hash algorithm in public area.  This
		   //     parameter is used when parentHandle is
		   //     NULL, in which case the object is
		   //     temporary.
		   TPM2B_PRIVATE   *outPrivate     // OUT: output private structure
		   );
TPM_RC
PrivateToSensitive(
		   TPM2B           *inPrivate,     // IN: input private structure
		   TPM2B           *name,          // IN: the name of the object
		   OBJECT          *parent,        // IN: parent object
		   TPM_ALG_ID       nameAlg,       // IN: hash algorithm in public area.  It is
		   //     passed separately because we only pass
		   //     name, rather than the whole public area
		   //     of the object.  This parameter is used in
		   //     the following two cases: 1. primary
		   //     objects. 2. duplication blob with inner
		   //     wrap.  In other cases, this parameter
		   //     will be ignored
		   TPMT_SENSITIVE  *sensitive      // OUT: sensitive structure
		   );
void
SensitiveToDuplicate(
		     TPMT_SENSITIVE      *sensitive,     // IN: sensitive structure
		     TPM2B               *name,          // IN: the name of the object
		     OBJECT              *parent,        // IN: The new parent object
		     TPM_ALG_ID           nameAlg,       // IN: hash algorithm in public area. It
		     //     is passed separately because we
		     //     only pass name, rather than the
		     //     whole public area of the object.
		     TPM2B               *seed,          // IN: the external seed. If external
		     //     seed is provided with size of 0,
		     //     no outer wrap should be applied
		     //     to duplication blob.
		     TPMT_SYM_DEF_OBJECT *symDef,        // IN: Symmetric key definition. If the
		     //     symmetric key algorithm is NULL,
		     //     no inner wrap should be applied.
		     TPM2B_DATA          *innerSymKey,   // IN/OUT: a symmetric key may be
		     //     provided to encrypt the inner
		     //     wrap of a duplication blob. May
		     //     be generated here if needed.
		     TPM2B_PRIVATE       *outPrivate     // OUT: output private structure
		     );
TPM_RC
DuplicateToSensitive(
		     TPM2B               *inPrivate,     // IN: input private structure
		     TPM2B               *name,          // IN: the name of the object
		     OBJECT              *parent,        // IN: the parent
		     TPM_ALG_ID           nameAlg,       // IN: hash algorithm in public area.
		     TPM2B               *seed,          // IN: an external seed may be provided.
		     //     If external seed is provided with
		     //     size of 0, no outer wrap is
		     //     applied
		     TPMT_SYM_DEF_OBJECT *symDef,        // IN: Symmetric key definition. If the
		     //     symmetric key algorithm is NULL,
		     //     no inner wrap is applied
		     TPM2B               *innerSymKey,   // IN: a symmetric key may be provided
		     //     to decrypt the inner wrap of a
		     //     duplication blob.
		     TPMT_SENSITIVE      *sensitive      // OUT: sensitive structure
		     );
void
SecretToCredential(
		   TPM2B_DIGEST        *secret,        // IN: secret information
		   TPM2B               *name,          // IN: the name of the object
		   TPM2B               *seed,          // IN: an external seed.
		   OBJECT              *protector,     // IN: the protector
		   TPM2B_ID_OBJECT     *outIDObject    // OUT: output credential
		   );
TPM_RC
CredentialToSecret(
		   TPM2B               *inIDObject,    // IN: input credential blob
		   TPM2B               *name,          // IN: the name of the object
		   TPM2B               *seed,          // IN: an external seed.
		   OBJECT              *protector,     // IN: the protector
		   TPM2B_DIGEST        *secret         // OUT: secret information
		   );
UINT16
MemoryRemoveTrailingZeros(
			  TPM2B_AUTH      *auth           // IN/OUT: value to adjust
			  );
TPM_RC
SetLabelAndContext(
		   TPMS_DERIVE             *labelContext,  // OUT: the recovered label and context
		   TPM2B_SENSITIVE_DATA    *sensitive      // IN: the sensitive data
		   );
TPM_RC
UnmarshalToPublic(
		  TPMT_PUBLIC         *tOut,       // OUT: output
		  TPM2B_TEMPLATE      *tIn,        // IN:
		  BOOL                 derivation,  // IN: indicates if this is for a derivation
		  TPMS_DERIVE         *labelContext // OUT: label and context if derivation
		  );
void
ObjectSetHierarchy(
		   OBJECT              *object,
		   TPM_HANDLE           parentHandle,
		   OBJECT              *parent
		   );
#if 0 /* libtpms added */
void
ObjectSetExternal(
		  OBJECT      *object
		  );
#endif /* libtpms added */


#endif
