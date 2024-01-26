/********************************************************************************/
/*										*/
/*		Dynamic space for user defined NV 				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: NVDynamic_fp.h $		*/
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

#ifndef NVDYNAMIC_FP_H
#define NVDYNAMIC_FP_H

NV_REF
NvWriteNvListEnd(
		 NV_REF           end
		 );
void
NvUpdateIndexOrderlyData(
			 void
			 );
void
NvReadNvIndexInfo(
		  NV_REF           ref,           // IN: points to NV where index is located
		  NV_INDEX        *nvIndex        // OUT: place to receive index data
		  );
void
NvReadObject(
	     NV_REF           ref,           // IN: points to NV where index is located
	     OBJECT          *object         // OUT: place to receive the object data
	     );
BOOL
NvIndexIsDefined(
		 TPM_HANDLE       nvHandle       // IN: Index to look for
		 );
BOOL
NvIsPlatformPersistentHandle(
			     TPM_HANDLE       handle         // IN: handle
			     );
BOOL
NvIsOwnerPersistentHandle(
			  TPM_HANDLE       handle         // IN: handle
			  );
TPM_RC
NvIndexIsAccessible(
		    TPMI_RH_NV_INDEX     handle        // IN: handle
		    );
TPM_RC
NvGetEvictObject(
		 TPM_HANDLE       handle,        // IN: handle
		 OBJECT          *object         // OUT: object data
		 );
void
NvIndexCacheInit(
		 void
		 );
void
NvGetIndexData(
	       NV_INDEX        *nvIndex,       // IN: the in RAM index descriptor
	       NV_REF           locator,       // IN: where the data is located
	       UINT32           offset,        // IN: offset of NV data
	       UINT16           size,          // IN: size of NV data
	       void            *data           // OUT: data buffer
	       );
void
NvHashIndexData(
		HASH_STATE          *hashState,     // IN: Initialized hash state
		NV_INDEX            *nvIndex,       // IN: Index
		NV_REF               locator,       // IN: where the data is located
		UINT32               offset,        // IN: starting offset
		UINT16               size           // IN: amount to hash
		);
UINT64
NvGetUINT64Data(
		NV_INDEX        *nvIndex,       // IN: the in RAM index descriptor
		NV_REF           locator        // IN: where index exists in NV
		);
TPM_RC
NvWriteIndexAttributes(
		       TPM_HANDLE       handle,
		       NV_REF           locator,       // IN: location of the index
		       TPMA_NV          attributes     // IN: attributes to write
		       );
TPM_RC
NvWriteIndexAuth(
		 NV_REF           locator,       // IN: location of the index
		 TPM2B_AUTH      *authValue      // IN: the authValue to write
		 );
NV_INDEX *
NvGetIndexInfo(
	       TPM_HANDLE       nvHandle,      // IN: the index handle
	       NV_REF          *locator        // OUT: location of the index
	       );
TPM_RC
NvWriteIndexData(
		 NV_INDEX        *nvIndex,       // IN: the description of the index
		 UINT32           offset,        // IN: offset of NV data
		 UINT32           size,          // IN: size of NV data
		 void            *data           // IN: data buffer
		 );
TPM_RC
NvWriteUINT64Data(
		  NV_INDEX        *nvIndex,       // IN: the description of the index
		  UINT64           intValue       // IN: the value to write
		  );
TPM2B_NAME *
NvGetIndexName(
	       NV_INDEX        *nvIndex,       // IN: the index over which the name is to be
	       //     computed
	       TPM2B_NAME      *name           // OUT: name of the index
	       );
TPM2B_NAME *
NvGetNameByIndexHandle(
		       TPMI_RH_NV_INDEX     handle,        // IN: handle of the index
		       TPM2B_NAME          *name           // OUT: name of the index
		       );
TPM_RC
NvDefineIndex(
	      TPMS_NV_PUBLIC  *publicArea,    // IN: A template for an area to create.
	      TPM2B_AUTH      *authValue      // IN: The initial authorization value
	      );
TPM_RC
NvAddEvictObject(
		 TPMI_DH_OBJECT   evictHandle,   // IN: new evict handle
		 OBJECT          *object         // IN: object to be added
		 );
TPM_RC
NvDeleteIndex(
	      NV_INDEX        *nvIndex,       // IN: an in RAM index descriptor
	      NV_REF           entityAddr     // IN: location in NV
	      );
TPM_RC
NvDeleteEvict(
	      TPM_HANDLE       handle         // IN: handle of entity to be deleted
	      );
TPM_RC
NvFlushHierarchy(
		 TPMI_RH_HIERARCHY    hierarchy      // IN: hierarchy to be flushed.
		 );
TPM_RC
NvSetGlobalLock(
		void
		);
TPMI_YES_NO
NvCapGetPersistent(
		   TPMI_DH_OBJECT   handle,        // IN: start handle
		   UINT32           count,         // IN: maximum number of returned handles
		   TPML_HANDLE     *handleList     // OUT: list of handle
		   );
TPMI_YES_NO
NvCapGetIndex(
	      TPMI_DH_OBJECT   handle,        // IN: start handle
	      UINT32           count,         // IN: max number of returned handles
	      TPML_HANDLE     *handleList     // OUT: list of handle
	      );
UINT32
NvCapGetIndexNumber(
		    void
		    );
UINT32
NvCapGetPersistentNumber(
			 void
			 );
UINT32
NvCapGetPersistentAvail(
			void
			);
UINT32
NvCapGetCounterNumber(
		      void
		      );
BOOL
NvEntityStartup(
		STARTUP_TYPE     type           // IN: start up type
		);
UINT32
NvCapGetCounterAvail(
		     void
		     );
NV_REF
NvFindHandle(
	     TPM_HANDLE       handle
	     );
UINT64
NvReadMaxCount(
	       void
	       );
void
NvUpdateMaxCount(
		 UINT64           count
		 );
void
NvSetMaxCount(
	      UINT64          value
	      );
UINT64
NvGetMaxCount(
	      void
	      );


#endif
