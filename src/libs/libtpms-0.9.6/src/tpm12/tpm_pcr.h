/********************************************************************************/
/*                                                                              */
/*                              PCR Handler                                     */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_pcr.h $               */
/*                                                                              */
/* (c) Copyright IBM Corporation 2006, 2010.					*/
/*										*/
/* All rights reserved.								*/
/* 										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are	*/
/* met:										*/
/* 										*/
/* Redistributions of source code must retain the above copyright notice,	*/
/* this list of conditions and the following disclaimer.			*/
/* 										*/
/* Redistributions in binary form must reproduce the above copyright		*/
/* notice, this list of conditions and the following disclaimer in the		*/
/* documentation and/or other materials provided with the distribution.		*/
/* 										*/
/* Neither the names of the IBM Corporation nor the names of its		*/
/* contributors may be used to endorse or promote products derived from		*/
/* this software without specific prior written permission.			*/
/* 										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		*/
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		*/
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	*/
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		*/
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	*/
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		*/
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	*/
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	*/
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	*/
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		*/
/********************************************************************************/

#ifndef TPM_PCR_H
#define TPM_PCR_H

#include "tpm_global.h"
#include "tpm_sizedbuffer.h"
#include "tpm_store.h"

/*
  Locality Utilities
*/

TPM_RESULT TPM_Locality_Set(TPM_LOCALITY_SELECTION *tpm_locality_selection,
                               TPM_MODIFIER_INDICATOR tpm_modifier_indicator);
TPM_RESULT TPM_Locality_Check(TPM_LOCALITY_SELECTION tpm_locality_selection,
                              TPM_MODIFIER_INDICATOR localityModifier);

TPM_RESULT TPM_LocalitySelection_CheckLegal(TPM_LOCALITY_SELECTION tpm_locality_selection);
TPM_RESULT TPM_LocalityModifier_CheckLegal(TPM_MODIFIER_INDICATOR localityModifier);

void       TPM_PCRLocality_Compare(TPM_BOOL *match,
                                   TPM_LOCALITY_SELECTION tpm_locality_selection1,
                                   TPM_LOCALITY_SELECTION tpm_locality_selection2);

/*
  state PCR's
*/

TPM_RESULT TPM_PCR_CheckRange(TPM_PCRINDEX index);
void       TPM_PCR_Init(TPM_PCRVALUE *tpm_pcrs,
                        const TPM_PCR_ATTRIBUTES *tpm_pcr_attributes,
                        size_t pcrIndex);
void       TPM_PCR_Reset(TPM_PCRVALUE *tpm_pcrs,
                         TPM_BOOL TOSPresent,
                         TPM_PCRINDEX pcrIndex); 
TPM_RESULT TPM_PCR_Load(TPM_PCRVALUE dest_pcr,
                        TPM_PCRVALUE *tpm_pcrs,
                        TPM_PCRINDEX index);
TPM_RESULT TPM_PCR_Store(TPM_PCRVALUE *tpm_pcrs,
                         TPM_PCRINDEX index,
                         TPM_PCRVALUE src_pcr);

/*
  TPM_SELECT_SIZE
*/

void       TPM_SelectSize_Init(TPM_SELECT_SIZE *tpm_select_size);
TPM_RESULT TPM_SelectSize_Load(TPM_SELECT_SIZE *tpm_select_size,
                               unsigned char **stream,
                               uint32_t *stream_size);


/*
  TPM_PCR_SELECTION
*/

void       TPM_PCRSelection_Init(TPM_PCR_SELECTION *tpm_pcr_selection);
TPM_RESULT TPM_PCRSelection_Load(TPM_PCR_SELECTION *tpm_pcr_selection,
                                 unsigned char **stream,
                                 uint32_t *stream_size);
TPM_RESULT TPM_PCRSelection_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_PCR_SELECTION *tpm_pcr_selection);
void       TPM_PCRSelection_Delete(TPM_PCR_SELECTION *tpm_pcr_selection);
/* copy */
TPM_RESULT TPM_PCRSelection_Copy(TPM_PCR_SELECTION *destination,
                                 TPM_PCR_SELECTION *source);
/* setters */
TPM_RESULT TPM_PCRSelection_GenerateDigest(TPM_DIGEST tpm_digest,
                                           TPM_PCR_SELECTION *tpm_pcr_selection,
                                           TPM_PCRVALUE *tpm_pcrs);
TPM_RESULT TPM_PCRSelection_GenerateDigest2(TPM_DIGEST tpm_digest,
                                           TPM_PCR_COMPOSITE *tpm_pcr_composite,
                                           TPM_PCR_SELECTION *tpm_pcr_selection,
                                           TPM_PCRVALUE *tpm_pcrs);
/* getters */
TPM_RESULT TPM_PCRSelection_GetPCRUsage(TPM_BOOL *pcrUsage,
                                        const TPM_PCR_SELECTION *tpm_pcr_selection,
                                        size_t start_index);
/* checkers */
TPM_RESULT TPM_PCRSelection_CheckRange(const TPM_PCR_SELECTION *tpm_pcr_selection);
void       TPM_PCRSelection_Compare(TPM_BOOL *match,
                                    TPM_PCR_SELECTION *tpm_pcr_selection1,
                                    TPM_PCR_SELECTION *tpm_pcr_selection2);
#if 0
void TPM_PCRSelection_LessThan(TPM_BOOL *lessThan,
                               TPM_PCR_SELECTION *tpm_pcr_selection_new,
                               TPM_PCR_SELECTION *tpm_pcr_selection_old);
#endif

/* TPM_PCR_ATTRIBUTES */

void       TPM_PCRAttributes_Init(TPM_PCR_ATTRIBUTES *tpm_pcr_attributes);

void       TPM_PCRInfo_Trace(const char *message,
			     TPM_PCR_SELECTION pcrSelection,
			     TPM_COMPOSITE_HASH digestAtRelease);
/*
  PCRs - Functions that act on the entire set of PCRs
*/

void       TPM_PCRs_Init(TPM_PCRVALUE *tpm_pcrs,
			 const TPM_PCR_ATTRIBUTES *tpm_pcr_attributes);
TPM_RESULT TPM_PCRs_Load(TPM_PCRVALUE *tpm_pcrs,
                         const TPM_PCR_ATTRIBUTES *tpm_pcr_attributes,
                         unsigned char **stream,
                         uint32_t *stream_size);
TPM_RESULT TPM_PCRs_Store(TPM_STORE_BUFFER *sbuffer,
                          TPM_PCRVALUE *tpm_pcrs,
                          const TPM_PCR_ATTRIBUTES *tpm_pcr_attributes);

/*
  TPM_PCR_INFO
*/

void       TPM_PCRInfo_Init(TPM_PCR_INFO *tpm_pcr_info);
TPM_RESULT TPM_PCRInfo_Load(TPM_PCR_INFO *tpm_pcr_info,
                            unsigned char **stream,
                            uint32_t *stream_size);
TPM_RESULT TPM_PCRInfo_Store(TPM_STORE_BUFFER *sbuffer,
                             const TPM_PCR_INFO *tpm_pcr_info);
void       TPM_PCRInfo_Delete(TPM_PCR_INFO *tpm_pcr_info);
/* create */
TPM_RESULT TPM_PCRInfo_Create(TPM_PCR_INFO **tpm_pcr_info);
/* load */
TPM_RESULT TPM_PCRInfo_LoadFromBuffer(TPM_PCR_INFO *tpm_pcr_info,
                                     const TPM_SIZED_BUFFER *tpm_sized_buffer);
TPM_RESULT TPM_PCRInfo_CreateFromBuffer(TPM_PCR_INFO **tpm_pcr_info,
                                        const TPM_SIZED_BUFFER *tpm_sized_buffer);
/* copy */
TPM_RESULT TPM_PCRInfo_Copy(TPM_PCR_INFO *dest_tpm_pcr_info,
                            TPM_PCR_INFO *src_tpm_pcr_info);
TPM_RESULT TPM_PCRInfo_CopyInfoLong(TPM_PCR_INFO *dest_tpm_pcr_info,
                                    TPM_PCR_INFO_LONG *src_tpm_pcr_info_long);
TPM_RESULT TPM_PCRInfo_CreateFromInfo(TPM_PCR_INFO **dest_tpm_pcr_info,
                                      TPM_PCR_INFO *src_tpm_pcr_info);
TPM_RESULT TPM_PCRInfo_CreateFromInfoLong(TPM_PCR_INFO **dest_tpm_pcr_info,
                                          TPM_PCR_INFO_LONG *src_tpm_pcr_info_long);
TPM_RESULT TPM_PCRInfo_CreateFromKey(TPM_PCR_INFO **dest_tpm_pcr_info,
                                     TPM_KEY *tpm_key);

/* setters */
TPM_RESULT TPM_PCRInfo_GenerateDigest(TPM_DIGEST tpm_digest,
                                      TPM_PCR_INFO *tpm_pcr_info,
                                      TPM_PCRVALUE *tpm_pcrs);
TPM_RESULT TPM_PCRInfo_CheckDigest(TPM_PCR_INFO *tpm_pcr_info,
                                   TPM_PCRVALUE *tpm_pcrs);
TPM_RESULT TPM_PCRInfo_SetDigestAtCreation(TPM_PCR_INFO *tpm_pcr_info,
                                           TPM_PCRVALUE *tpm_pcrs);
/* getters */
TPM_RESULT TPM_PCRInfo_GetPCRUsage(TPM_BOOL *pcrUsage,
                                   TPM_PCR_INFO *tpm_pcr_info,
                                   size_t start_index);

/*
  TPM_PCR_INFO_LONG
*/

void       TPM_PCRInfoLong_Init(TPM_PCR_INFO_LONG *tpm_pcr_info_long);
TPM_RESULT TPM_PCRInfoLong_Load(TPM_PCR_INFO_LONG *tpm_pcr_info_long,
                                unsigned char **stream,
                                uint32_t *stream_size);
TPM_RESULT TPM_PCRInfoLong_Store(TPM_STORE_BUFFER *sbuffer,
                                 const TPM_PCR_INFO_LONG *tpm_pcr_info_long);
void       TPM_PCRInfoLong_Delete(TPM_PCR_INFO_LONG *tpm_pcr_info_long);
/* create */
TPM_RESULT TPM_PCRInfoLong_Create(TPM_PCR_INFO_LONG **tpm_pcr_info_long);
/* load */
TPM_RESULT TPM_PCRInfoLong_LoadFromBuffer(TPM_PCR_INFO_LONG *tpm_pcr_info_long,
                                          const TPM_SIZED_BUFFER *tpm_sized_buffer);
TPM_RESULT TPM_PCRInfoLong_CreateFromBuffer(TPM_PCR_INFO_LONG **tpm_pcr_info_long,
                                            const TPM_SIZED_BUFFER *tpm_sized_buffer);
/* copy */
TPM_RESULT TPM_PCRInfoLong_Copy(TPM_PCR_INFO_LONG *dest_tpm_pcr_info_long,
                                TPM_PCR_INFO_LONG *src_tpm_pcr_info_long);
TPM_RESULT TPM_PCRInfoLong_CreateFromInfoLong(TPM_PCR_INFO_LONG **dest_tpm_pcr_info_long,
                                              TPM_PCR_INFO_LONG *src_tpm_pcr_info_long);
/* setters */
TPM_RESULT TPM_PCRInfoLong_GenerateDigest(TPM_DIGEST tpm_digest,
                                          TPM_PCR_INFO_LONG *tpm_pcr_info_long,
                                          TPM_PCRVALUE *tpm_pcrs);
TPM_RESULT TPM_PCRInfoLong_CheckDigest(TPM_PCR_INFO_LONG *tpm_pcr_info_long,
                                       TPM_PCRVALUE *tpm_pcrs,
                                       TPM_MODIFIER_INDICATOR localityModifier);
TPM_RESULT TPM_PCRInfoLong_SetDigestAtCreation(TPM_PCR_INFO_LONG *tpm_pcr_info_long,
                                               TPM_PCRVALUE *tpm_pcrs);
/* getters */
TPM_RESULT TPM_PCRInfoLong_GetPCRUsage(TPM_BOOL *pcrUsage,
                                       TPM_PCR_INFO_LONG *tpm_pcr_info_long,
                                       size_t start_index);

/*
  TPM_PCR_INFO_SHORT
*/

void       TPM_PCRInfoShort_Init(TPM_PCR_INFO_SHORT *tpm_pcr_info_short);
TPM_RESULT TPM_PCRInfoShort_Load(TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
                                 unsigned char **stream,
                                 uint32_t *stream_size,
				 TPM_BOOL optimize);
TPM_RESULT TPM_PCRInfoShort_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
				  TPM_BOOL optimize);
void       TPM_PCRInfoShort_Delete(TPM_PCR_INFO_SHORT *tpm_pcr_info_short);
/* create */
TPM_RESULT TPM_PCRInfoShort_Create(TPM_PCR_INFO_SHORT **tpm_pcr_info_short);
/* load */
TPM_RESULT TPM_PCRInfoShort_LoadFromBuffer(TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
                                           const TPM_SIZED_BUFFER *tpm_sized_buffer);
TPM_RESULT TPM_PCRInfoShort_CreateFromBuffer(TPM_PCR_INFO_SHORT **tpm_pcr_info_short,
                                             const TPM_SIZED_BUFFER *tpm_sized_buffer);
/* copy */
TPM_RESULT TPM_PCRInfoShort_Copy(TPM_PCR_INFO_SHORT *dest_tpm_pcr_info_short,
                                 TPM_PCR_INFO_SHORT *src_tpm_pcr_info_short);
TPM_RESULT TPM_PCRInfoShort_CopyInfo(TPM_PCR_INFO_SHORT *dest_tpm_pcr_info_short,
                                     TPM_PCR_INFO *src_tpm_pcr_info);
TPM_RESULT TPM_PCRInfoShort_CopyInfoLong(TPM_PCR_INFO_SHORT *dest_tpm_pcr_info_short,
                                         TPM_PCR_INFO_LONG *src_tpm_pcr_info_long);
TPM_RESULT TPM_PCRInfoShort_CreateFromInfo(TPM_PCR_INFO_SHORT **dest_tpm_pcr_info_short,
                                           TPM_PCR_INFO *src_tpm_pcr_info);
TPM_RESULT TPM_PCRInfoShort_CreateFromInfoLong(TPM_PCR_INFO_SHORT **dest_tpm_pcr_info_short,
                                               TPM_PCR_INFO_LONG *src_tpm_pcr_info_long);
TPM_RESULT TPM_PCRInfoShort_CreateFromKey(TPM_PCR_INFO_SHORT **dest_tpm_pcr_info_short,
                                          TPM_KEY *tpm_key);

/* setters */
TPM_RESULT TPM_PCRInfoShort_GenerateDigest(TPM_DIGEST tpm_digest,
                                           TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
                                           TPM_PCRVALUE *tpm_pcrs);
TPM_RESULT TPM_PCRInfoShort_CheckDigest(TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
                                        TPM_PCRVALUE *tpm_pcrs,
                                        TPM_MODIFIER_INDICATOR localityModifier);

/* getters */
TPM_RESULT TPM_PCRInfoShort_GetPCRUsage(TPM_BOOL *pcrUsage,
                                        TPM_PCR_INFO_SHORT *tpm_pcr_info_short);

/*
  TPM_PCR_COMPOSITE
*/

void       TPM_PCRComposite_Init(TPM_PCR_COMPOSITE *tpm_pcr_composite);
TPM_RESULT TPM_PCRComposite_Store(TPM_STORE_BUFFER *sbuffer,
                                  const TPM_PCR_COMPOSITE *tpm_pcr_composite);
void       TPM_PCRComposite_Delete(TPM_PCR_COMPOSITE *tpm_pcr_composite);

TPM_RESULT TPM_PCRComposite_Set(TPM_PCR_COMPOSITE *tpm_pcr_composite,
                                TPM_PCR_SELECTION *tpm_pcr_selection,
                                TPM_PCRVALUE *tpm_pcrs);

/*
  TPM_QUOTE_INFO
*/

void       TPM_QuoteInfo_Init(TPM_QUOTE_INFO *tpm_quote_info);
#if 0
TPM_RESULT TPM_QuoteInfo_Load(TPM_QUOTE_INFO *tpm_quote_info,
                              unsigned char **stream,
                              uint32_t *stream_size);
#endif
TPM_RESULT TPM_QuoteInfo_Store(TPM_STORE_BUFFER *sbuffer,
                               const TPM_QUOTE_INFO *tpm_quote_info);
void       TPM_QuoteInfo_Delete(TPM_QUOTE_INFO *tpm_quote_info);

/*
  TPM_QUOTE_INFO2
*/

void       TPM_QuoteInfo2_Init(TPM_QUOTE_INFO2 *tpm_quote_info2);
#if 0
TPM_RESULT TPM_QuoteInfo2_Load(TPM_QUOTE_INFO2 *tpm_quote_info2,
                               unsigned char **stream,
                               uint32_t *stream_size);
#endif
TPM_RESULT TPM_QuoteInfo2_Store(TPM_STORE_BUFFER *sbuffer,
                                const TPM_QUOTE_INFO2 *tpm_quote_info2);
void       TPM_QuoteInfo2_Delete(TPM_QUOTE_INFO2 *tpm_quote_info2);


/*
  Common command processing
*/

TPM_RESULT TPM_ExtendCommon(TPM_PCRVALUE outDigest,
                            tpm_state_t *tpm_state,
                            TPM_COMMAND_CODE ordinal,
                            TPM_PCRINDEX pcrNum,
                            TPM_DIGEST inDigest);
/*
  Command Processing
*/

TPM_RESULT TPM_Process_PcrRead(tpm_state_t *tpm_state,
                               TPM_STORE_BUFFER *response,
                               TPM_TAG tag,
                               uint32_t paramSize,
                               TPM_COMMAND_CODE ordinal,
                               unsigned char *command,
                               TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_Quote(tpm_state_t *tpm_state,
                             TPM_STORE_BUFFER *response,
                             TPM_TAG tag,
                             uint32_t paramSize,
                             TPM_COMMAND_CODE ordinal,
                             unsigned char *command,
                             TPM_TRANSPORT_INTERNAL *transportInternal);
TPM_RESULT TPM_Process_Quote2(tpm_state_t *tpm_state,
                              TPM_STORE_BUFFER *response,
                              TPM_TAG tag,
                              uint32_t paramSize,
                              TPM_COMMAND_CODE ordinal,
                              unsigned char *command,
                              TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_Extend(tpm_state_t *tpm_state,
                              TPM_STORE_BUFFER *response,
                              TPM_TAG tag,
                              uint32_t paramSize,
                              TPM_COMMAND_CODE ordinal,
                              unsigned char *command,
                              TPM_TRANSPORT_INTERNAL *transportInternal);

TPM_RESULT TPM_Process_PcrReset(tpm_state_t *tpm_state,
                                TPM_STORE_BUFFER *response,
                                TPM_TAG tag,
                                uint32_t paramSize,
                                TPM_COMMAND_CODE ordinal,
                                unsigned char *command,
                                TPM_TRANSPORT_INTERNAL *transportInternal);
#endif
