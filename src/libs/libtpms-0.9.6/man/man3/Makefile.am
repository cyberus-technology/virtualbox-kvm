#
# man/man3/Makefile.am
#
# For the license, see the LICENSE file in the root directory.
#


man3_PODS = \
	TPM_IO_Hash_Start.pod \
	TPM_IO_TpmEstablished_Get.pod \
	TPMLIB_CancelCommand.pod \
	TPMLIB_ChooseTPMVersion.pod \
	TPMLIB_DecodeBlob.pod \
	TPMLIB_GetInfo.pod \
	TPMLIB_GetTPMProperty.pod \
	TPMLIB_GetVersion.pod \
	TPMLIB_MainInit.pod \
	TPMLIB_Process.pod \
	TPMLIB_RegisterCallbacks.pod \
	TPMLIB_SetBufferSize.pod \
	TPMLIB_SetDebugFD.pod \
	TPMLIB_SetState.pod \
	TPMLIB_ValidateState.pod \
	TPMLIB_VolatileAll_Store.pod \
	TPM_Malloc.pod

man3_MANS = \
	TPM_Free.3 \
	TPM_IO_Hash_Data.3 \
	TPM_IO_Hash_End.3 \
	TPMLIB_GetState.3 \
	TPMLIB_SetDebugPrefix.3 \
	TPMLIB_SetDebugLevel.3 \
	TPM_IO_TpmEstablished_Reset.3 \
	TPMLIB_Terminate.3 \
	TPM_Realloc.3

man3_MANS_generated = \
	TPM_IO_Hash_Start.3 \
	TPM_IO_TpmEstablished_Get.3 \
	TPMLIB_CancelCommand.3 \
	TPMLIB_ChooseTPMVersion.3 \
	TPMLIB_DecodeBlob.3 \
	TPMLIB_GetInfo.3 \
	TPMLIB_GetTPMProperty.3 \
	TPMLIB_GetVersion.3 \
	TPMLIB_MainInit.3 \
	TPMLIB_Process.3 \
	TPMLIB_SetDebugFD.3 \
	TPMLIB_SetBufferSize.3 \
	TPMLIB_SetState.3 \
	TPMLIB_RegisterCallbacks.3 \
	TPMLIB_ValidateState.3 \
	TPMLIB_VolatileAll_Store.3 \
	TPM_Malloc.3

man3_MANS += $(man3_MANS_generated)

%.3 : %.pod
	@pod2man -r "libtpms" \
		-c "" \
		-n $(basename $@) \
		--section=3 $< > $@

EXTRA_DIST = $(man3_MANS) $(man3_PODS)

CLEANFILES = $(man3_MANS_generated)
