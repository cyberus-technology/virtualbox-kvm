/********************************************************************************/
/*										*/
/*			      TPM ASN.1						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmAsn1_fp.h $		*/
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
/*  (c) Copyright IBM Corp. and others, 2019					*/
/*										*/
/********************************************************************************/

#ifndef TPMASN1_FP_H
#define TPMASN1_FP_H

BOOL
ASN1UnmarshalContextInitialize(
			       ASN1UnmarshalContext    *ctx,
			       INT16                    size,
			       BYTE                    *buffer
			       );
INT16
ASN1DecodeLength(
		 ASN1UnmarshalContext        *ctx
		 );
INT16
ASN1NextTag(
	    ASN1UnmarshalContext    *ctx
	    );
BOOL
ASN1GetBitStringValue(
		      ASN1UnmarshalContext        *ctx,
		      UINT32                      *val
		      );
void
ASN1InitialializeMarshalContext(
				ASN1MarshalContext      *ctx,
				INT16                    length,
				BYTE                    *buffer
				);
void
ASN1StartMarshalContext(
			ASN1MarshalContext      *ctx
			);
INT16
ASN1EndMarshalContext(
		      ASN1MarshalContext      *ctx
		      );
UINT16
ASN1EndEncapsulation(
		     ASN1MarshalContext          *ctx,
		     BYTE                         tag
		     );
BOOL
ASN1PushByte(
	     ASN1MarshalContext          *ctx,
	     BYTE                         b
	     );
INT16
ASN1PushBytes(
	      ASN1MarshalContext          *ctx,
	      INT16                        count,
	      const BYTE                  *buffer
	      );
INT16
ASN1PushNull(
	     ASN1MarshalContext      *ctx
	     );
INT16
ASN1PushLength(
	       ASN1MarshalContext          *ctx,
	       INT16                        len
	       );
INT16
ASN1PushTagAndLength(
		     ASN1MarshalContext          *ctx,
		     BYTE                         tag,
		     INT16                        length
		     );
INT16
ASN1PushTaggedOctetString(
			  ASN1MarshalContext          *ctx,
			  INT16                        size,
			  const BYTE                  *string,
			  BYTE                         tag
			  );
INT16
ASN1PushUINT(
	     ASN1MarshalContext      *ctx,
	     UINT32                   integer
	     );
INT16
ASN1PushInteger(
		ASN1MarshalContext  *ctx,           // IN/OUT: buffer context
		INT16                iLen,          // IN: octets of the integer
		BYTE                *integer        // IN: big-endian integer
		);
INT16
ASN1PushOID(
	    ASN1MarshalContext          *ctx,
	    const BYTE                  *OID
	    );
#endif
