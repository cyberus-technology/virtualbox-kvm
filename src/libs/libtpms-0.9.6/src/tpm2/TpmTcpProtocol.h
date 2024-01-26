/********************************************************************************/
/*										*/
/*	TPM commands are communicated as BYTE streams on a TCP connection	*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmTcpProtocol.h $	*/
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

/* D.3 TpmTcpProtocol.h */
/* D.3.1. Introduction */
/* TPM commands are communicated as uint8_t streams on a TCP connection.  The TPM command protocol is
   enveloped with the interface protocol described in this file. The command is indicated by a
   uint32_t with one of the values below.  Most commands take no parameters and return no TPM errors. In
   these cases the TPM interface protocol acknowledges that command processing is completed by
   returning a uint32_t = 0. The command TPM_SIGNAL_HASH_DATA takes a uint32_t-prepended variable length
   byte array and the interface protocol acknowledges command completion with a uint32_t = 0. Most TPM
   commands are enveloped using the TPM_SEND_COMMAND interface command. The parameters are as
   indicated below.  The interface layer also appends a uin32_t = 0 to the TPM response for
   regularity. */
/* D.3.2. Typedefs and Defines */
#ifndef     TCP_TPM_PROTOCOL_H
#define     TCP_TPM_PROTOCOL_H
/* D.3.3. TPM Commands All commands acknowledge processing by returning a uint32_t = 0 except where
   noted */
#define TPM_SIGNAL_POWER_ON         1
#define TPM_SIGNAL_POWER_OFF        2
#define TPM_SIGNAL_PHYS_PRES_ON     3
#define TPM_SIGNAL_PHYS_PRES_OFF    4
#define TPM_SIGNAL_HASH_START       5
#define TPM_SIGNAL_HASH_DATA        6
    // {uint32_t BufferSize, uint8_t[BufferSize] Buffer}
#define TPM_SIGNAL_HASH_END         7
#define TPM_SEND_COMMAND            8
// {uint8_t Locality, uint32_t InBufferSize, uint8_t[InBufferSize] InBuffer} ->
//     {uint32_t OutBufferSize, uint8_t[OutBufferSize] OutBuffer}
#define TPM_SIGNAL_CANCEL_ON        9
#define TPM_SIGNAL_CANCEL_OFF       10
#define TPM_SIGNAL_NV_ON            11
#define TPM_SIGNAL_NV_OFF           12
#define TPM_SIGNAL_KEY_CACHE_ON     13
#define TPM_SIGNAL_KEY_CACHE_OFF    14
#define TPM_REMOTE_HANDSHAKE        15
#define TPM_SET_ALTERNATIVE_RESULT  16
#define TPM_SIGNAL_RESET            17
#define TPM_SIGNAL_RESTART          18
#define TPM_SESSION_END             20
#define TPM_STOP                    21
#define TPM_GET_COMMAND_RESPONSE_SIZES  25
#define TPM_ACT_GET_SIGNALED        26
#define TPM_TEST_FAILURE_MODE       30

// D.3.4.	Enumerations and Structures

enum TpmEndPointInfo
    {
	tpmPlatformAvailable = 0x01,
	tpmUsesTbs = 0x02,
	tpmInRawMode = 0x04,
	tpmSupportsPP = 0x08
    };

#ifdef _MSC_VER
#   pragma warning(push, 3)
#endif

// Existing RPC interface type definitions retained so that the implementation
// can be re-used
typedef struct in_buffer
{
    unsigned long BufferSize;
    unsigned char *Buffer;
} _IN_BUFFER;
typedef unsigned char *_OUTPUT_BUFFER;
typedef struct out_buffer
{
    uint32_t         BufferSize;
    _OUTPUT_BUFFER   Buffer;
} _OUT_BUFFER;
#ifdef _MSC_VER
#   pragma warning(pop)
#endif
#ifndef WIN32
typedef unsigned long        DWORD;
typedef void                *LPVOID;
#undef WINAPI
#endif
#endif
