/********************************************************************************/
/*										*/
/*			     Command Attributes					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CommandAttributes.h $	*/
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

#ifndef COMMANDATTRIBUTES_H
#define COMMANDATTRIBUTES_H

/* 5.7	CommandAttributes.h */
/* The attributes defined in this file are produced by the parser that creates the structure
   definitions from Part 3. The attributes are defined in that parser and should track the
   attributes being tested in CommandCodeAttributes.c. Generally, when an attribute is added to this
   list, new code will be needed in CommandCodeAttributes.c to test it. */

typedef  UINT16             COMMAND_ATTRIBUTES;
#define NOT_IMPLEMENTED     (COMMAND_ATTRIBUTES)(0)
#define ENCRYPT_2           ((COMMAND_ATTRIBUTES)1 << 0)
#define ENCRYPT_4           ((COMMAND_ATTRIBUTES)1 << 1)
#define DECRYPT_2           ((COMMAND_ATTRIBUTES)1 << 2)
#define DECRYPT_4           ((COMMAND_ATTRIBUTES)1 << 3)
#define HANDLE_1_USER       ((COMMAND_ATTRIBUTES)1 << 4)
#define HANDLE_1_ADMIN      ((COMMAND_ATTRIBUTES)1 << 5)
#define HANDLE_1_DUP        ((COMMAND_ATTRIBUTES)1 << 6)
#define HANDLE_2_USER       ((COMMAND_ATTRIBUTES)1 << 7)
#define PP_COMMAND          ((COMMAND_ATTRIBUTES)1 << 8)
#define IS_IMPLEMENTED      ((COMMAND_ATTRIBUTES)1 << 9)
#define NO_SESSIONS         ((COMMAND_ATTRIBUTES)1 << 10)
#define NV_COMMAND          ((COMMAND_ATTRIBUTES)1 << 11)
#define PP_REQUIRED         ((COMMAND_ATTRIBUTES)1 << 12)
#define R_HANDLE            ((COMMAND_ATTRIBUTES)1 << 13)
#define ALLOW_TRIAL         ((COMMAND_ATTRIBUTES)1 << 14)
#endif // COMMAND_ATTRIBUTES_H
