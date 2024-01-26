/********************************************************************************/
/*										*/
/*			     	Vendor String					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: VendorString.h $		*/
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

#ifndef VENDORSTRING_H
#define VENDORSTRING_H

/* Define up to 4-byte values for MANUFACTURER.  This value defines the response for
   TPM_PT_MANUFACTURER in TPM2_GetCapability(). The following line should be un-commented and a
   vendor specific string should be provided here. */
#define    MANUFACTURER    "IBM"

/*     The following #if macro may be deleted after a proper MANUFACTURER is provided. */
#ifndef MANUFACTURER
#error MANUFACTURER is not provided.				 \
    Please modify VendorString.h to provide a specific	  	 \
    manufacturer name.
#endif

/*     Define up to 4, 4-byte, vendor-specific values. The values must each be 4 bytes long and the
       last value used may contain trailing zeros. These values define the response for
       TPM_PT_VENDOR_STRING_(1-4) in TPM2_GetCapability(). The following line should be un-commented
       and a vendor specific string.  The vendor strings 2-4 may also be defined as appropriate. */

#define       VENDOR_STRING_1       "SW  "
#define       VENDOR_STRING_2       " TPM"
//#define       VENDOR_STRING_3
//#define       VENDOR_STRING_4

/*     The following #if macro may be deleted after a proper VENDOR_STRING_1 is provided. */
#ifndef VENDOR_STRING_1
#error VENDOR_STRING_1 is not provided.					\
    Please modify VendorString.h to provide a vendor specific string.
#endif

/* the more significant 32-bits of a vendor-specific value indicating the version of the firmware
   The following line should be un-commented and a vendor specific firmware V1 should be provided
   here. The FIRMWARE_V2 may also be defined as appropriate. */
#define   FIRMWARE_V1         (0x20191023)

// the less significant 32-bits of a vendor-specific value indicating the version of the firmware
#define   FIRMWARE_V2         (0x00163636)

// The following #if macro may be deleted after a proper FIRMWARE_V1 is provided.
#ifndef FIRMWARE_V1
#error  FIRMWARE_V1 is not provided.					\
    Please modify VendorString.h to provide a vendor specific firmware \
    version
#endif

#endif
