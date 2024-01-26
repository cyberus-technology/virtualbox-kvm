/********************************************************************************/
/*                                                                              */
/*                      NVRAM File Abstraction Layer                            */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_nvfile.c $            */
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

/* This module abstracts out all NVRAM read and write operations.

   This implementation uses standard, portable C files.

   The basic high level abstractions are:

        TPM_NVRAM_LoadData();
        TPM_NVRAM_StoreData();
        TPM_NVRAM_DeleteName();

   They take a 'name' that is mapped to a rooted file name.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_memory.h"

#include "tpm_nvfile.h"

#ifdef TPM_LIBTPMS_CALLBACKS
#include "tpm_library_intern.h"
#include "tpm_library.h"
#endif


/* local prototypes */

static TPM_RESULT TPM_NVRAM_GetFilenameForName(char *filename,
                                               size_t filename_len,
					       uint32_t tpm_number,
                                               const char *name);


/* A file name in NVRAM is composed of 3 parts:

  1 - 'state_directory' is the rooted path to the TPM state home directory
  2 = 'tpm_number' is the TPM instance, 00 for a single TPM
  2 - the file name

  For the IBM cryptographic coprocessor version, the root path is hard coded.
  
  For the Linux and Windows versions, the path comes from an environment variable.  This variable is
  used once in TPM_NVRAM_Init().

  One root path is used for all virtual TPM's, so it can be a static variable.
*/

char state_directory[FILENAME_MAX];

/* TPM_NVRAM_Init() is called once at startup.  It does any NVRAM required initialization.

   This function sets some static variables that are used by all TPM's.
*/

TPM_RESULT TPM_NVRAM_Init(void)
{
    TPM_RESULT  rc = 0;
    char        *tpm_state_path = NULL;
    size_t      length;

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    /* call user-provided function if available, otherwise execute
       default behavior */
    if (cbs->tpm_nvram_init) {
        rc = cbs->tpm_nvram_init();
        return rc;
    }
#endif

    printf(" TPM_NVRAM_Init:\n");
#ifdef TPM_NV_DISK
    /* TPM_NV_DISK TPM emulation stores in local directory determined by environment variable. */
    if (rc == 0) {
        tpm_state_path = getenv("TPM_PATH");
        if (tpm_state_path == NULL) {
            printf("TPM_NVRAM_Init: Error (fatal), TPM_PATH environment variable not set\n");
            rc = TPM_FAIL;
        }
    }
#endif
    /* check that the directory name plus a file name will not overflow FILENAME_MAX */
    if (rc == 0) {
        length = strlen(tpm_state_path);
        if ((length + TPM_FILENAME_MAX) > FILENAME_MAX) {
            printf("TPM_NVRAM_Init: Error (fatal), TPM state path name %s too large\n",
		   tpm_state_path);
            rc = TPM_FAIL;
        }
    }
    if (rc == 0) {
        strcpy(state_directory, tpm_state_path);
        printf("TPM_NVRAM_Init: Rooted state path %s\n", state_directory);
    }
    return rc;
}

/* Load 'data' of 'length' from the 'name'.

   'data' must be freed after use.
   
   Returns
        0 on success.
        TPM_RETRY and NULL,0 on non-existent file (non-fatal, first time start up)
        TPM_FAIL on failure to load (fatal), since it should never occur
*/

TPM_RESULT TPM_NVRAM_LoadData(unsigned char **data,     /* freed by caller */
                              uint32_t *length,
			      uint32_t tpm_number,
                              const char *name) 
{
    TPM_RESULT  rc = 0;
    long        lrc;
    size_t      src;
    int         irc;
    FILE        *file = NULL;
    char        filename[FILENAME_MAX]; /* rooted file name from name */

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs;
    bool is_empty_buffer;

    /* try to get state blob set with TPMLIB_SetState() */
    GetCachedState(TPMLIB_NameToStateType(name), data, length, &is_empty_buffer);
    if (is_empty_buffer)
        return TPM_RETRY;
    if (*data)
        return TPM_SUCCESS;

    cbs = TPMLIB_GetCallbacks();

    /* call user-provided function if available, otherwise execute
       default behavior */
    if (cbs->tpm_nvram_loaddata) {
        rc = cbs->tpm_nvram_loaddata(data, length, tpm_number, name);
        return rc;
    }
#endif

    printf(" TPM_NVRAM_LoadData: From file %s\n", name);
    *data = NULL;
    *length = 0;
    /* open the file */
    if (rc == 0) {
        /* map name to the rooted filename */
        rc = TPM_NVRAM_GetFilenameForName(filename, sizeof(filename),
                                          tpm_number, name);
    }
    if (rc == 0) {
        printf("  TPM_NVRAM_LoadData: Opening file %s\n", filename);
        file = fopen(filename, "rb");                           /* closed @1 */
        if (file == NULL) {     /* if failure, determine cause */
            if (errno == ENOENT) {
                printf("TPM_NVRAM_LoadData: No such file %s\n", filename);
                rc = TPM_RETRY;         /* first time start up */
            }
            else {
                printf("TPM_NVRAM_LoadData: Error (fatal) opening %s for read, %s\n",
                       filename, strerror(errno));
                rc = TPM_FAIL;
            }
        }
    }
    /* determine the file length */
    if (rc == 0) {
        irc = fseek(file, 0L, SEEK_END);        /* seek to end of file */
        if (irc == -1L) {
            printf("TPM_NVRAM_LoadData: Error (fatal) fseek'ing %s, %s\n",
                   filename, strerror(errno));
            rc = TPM_FAIL;
        }
    }
    if (rc == 0) {
        lrc = ftell(file);                      /* get position in the stream */
        if (lrc == -1L) {
            printf("TPM_NVRAM_LoadData: Error (fatal) ftell'ing %s, %s\n",
                   filename, strerror(errno));
            rc = TPM_FAIL;
        }
        else {
            *length = (uint32_t)lrc;      	/* save the length */
        }
    }
    if (rc == 0) {
        irc = fseek(file, 0L, SEEK_SET);        /* seek back to the beginning of the file */
        if (irc == -1L) {
            printf("TPM_NVRAM_LoadData: Error (fatal) fseek'ing %s, %s\n",
                   filename, strerror(errno));
            rc = TPM_FAIL;
        }
    }
    /* allocate a buffer for the actual data */
    if ((rc == 0) && *length != 0) {
        printf(" TPM_NVRAM_LoadData: Reading %u bytes of data\n", *length);
        rc = TPM_Malloc(data, *length);
	if (rc != 0) {
            printf("TPM_NVRAM_LoadData: Error (fatal) allocating %u bytes\n", *length);
            rc = TPM_FAIL;
	}
    }
    /* read the contents of the file into the data buffer */
    if ((rc == 0) && *length != 0) {
        src = fread(*data, 1, *length, file);
        if (src != *length) {
            printf("TPM_NVRAM_LoadData: Error (fatal), data read of %u only read %lu\n",
                   *length, (unsigned long)src);
            rc = TPM_FAIL;
        }
    }
    /* close the file */
    if (file != NULL) {
        printf(" TPM_NVRAM_LoadData: Closing file %s\n", filename);
        irc = fclose(file);             /* @1 */
        if (irc != 0) {
            printf("TPM_NVRAM_LoadData: Error (fatal) closing file %s\n", filename);
            rc = TPM_FAIL;
        }
        else {
            printf(" TPM_NVRAM_LoadData: Closed file %s\n", filename);
        }
    }
    return rc;
}

/* TPM_NVRAM_StoreData stores 'data' of 'length' to the rooted 'filename'

   Returns
        0 on success
        TPM_FAIL for other fatal errors
*/

TPM_RESULT TPM_NVRAM_StoreData(const unsigned char *data,
                               uint32_t length,
			       uint32_t tpm_number,
                               const char *name)
{
    TPM_RESULT  rc = 0;
    uint32_t      lrc;
    int         irc;
    FILE        *file = NULL;
    char        filename[FILENAME_MAX]; /* rooted file name from name */

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    /* call user-provided function if available, otherwise execute
       default behavior */
    if (cbs->tpm_nvram_storedata) {
        rc = cbs->tpm_nvram_storedata(data, length, tpm_number, name);
        return rc;
    }
#endif

    printf(" TPM_NVRAM_StoreData: To name %s\n", name);
    if (rc == 0) {
        /* map name to the rooted filename */
        rc = TPM_NVRAM_GetFilenameForName(filename, sizeof(filename),
                                          tpm_number, name);
    }
    if (rc == 0) {
        /* open the file */
        printf(" TPM_NVRAM_StoreData: Opening file %s\n", filename);
        file = fopen(filename, "wb");                           /* closed @1 */
        if (file == NULL) {
            printf("TPM_NVRAM_StoreData: Error (fatal) opening %s for write failed, %s\n",
                   filename, strerror(errno));
            rc = TPM_FAIL;
        }
    }
    /* write the data to the file */
    if (rc == 0) {
        printf("  TPM_NVRAM_StoreData: Writing %u bytes of data\n", length);
        lrc = fwrite(data, 1, length, file);
        if (lrc != length) {
            printf("TPM_NVRAM_StoreData: Error (fatal), data write of %u only wrote %u\n",
                   length, lrc);
            rc = TPM_FAIL;
        }
    }
    if (file != NULL) {
        printf("  TPM_NVRAM_StoreData: Closing file %s\n", filename);
        irc = fclose(file);             /* @1 */
        if (irc != 0) {
            printf("TPM_NVRAM_StoreData: Error (fatal) closing file\n");
            rc = TPM_FAIL;
        }
        else {
            printf("  TPM_NVRAM_StoreData: Closed file %s\n", filename);
        }
    }
    return rc;
}


/* TPM_NVRAM_GetFilenameForName() constructs a rooted file name from the name.

   The filename is of the form:

   state_directory/tpm_number.name
*/

static TPM_RESULT TPM_NVRAM_GetFilenameForName(char *filename,        /* output: rooted filename */
					       size_t filename_len,
					       uint32_t tpm_number,
                                               const char *name)      /* input: abstract name */
{
    int n;
    TPM_RESULT rc = TPM_FAIL;

    printf(" TPM_NVRAM_GetFilenameForName: For name %s\n", name);
    n = snprintf(filename, filename_len,
                 "%s/%02lx.%s", state_directory, (unsigned long)tpm_number,
                 name);
    if (n < 0) {
        printf(" TPM_NVRAM_GetFilenameForName: Error (fatal), snprintf failed\n");
    } else if ((size_t)n >= filename_len) {
        printf(" TPM_NVRAM_GetFilenameForName: Error (fatal), buffer too small\n");
    } else {
        printf("  TPM_NVRAM_GetFilenameForName: File name %s\n", filename);
        rc = TPM_SUCCESS;
    }
    return rc;
}

/* TPM_NVRAM_DeleteName() deletes the 'name' from NVRAM

   Returns:
        0 on success, or if the file does not exist and mustExist is FALSE
        TPM_FAIL if the file could not be removed, since this should never occur and there is
		no recovery

   NOTE: Not portable code, but supported by Linux and Windows
*/

TPM_RESULT TPM_NVRAM_DeleteName(uint32_t tpm_number,
				const char *name,
                                TPM_BOOL mustExist)
{
    TPM_RESULT  rc = 0;
    int         irc;
    char        filename[FILENAME_MAX]; /* rooted file name from name */

#ifdef TPM_LIBTPMS_CALLBACKS
    struct libtpms_callbacks *cbs = TPMLIB_GetCallbacks();

    /* call user-provided function if available, otherwise execute
       default behavior */
    if (cbs->tpm_nvram_deletename) {
        rc = cbs->tpm_nvram_deletename(tpm_number, name, mustExist);
        return rc;
    }
#endif
    
    printf(" TPM_NVRAM_DeleteName: Name %s\n", name);
    /* map name to the rooted filename */
    if (rc == 0) {
        rc = TPM_NVRAM_GetFilenameForName(filename, sizeof(filename),
                                          tpm_number, name);
    }
    if (rc == 0) {
        irc = remove(filename);
        if ((irc != 0) &&               /* if the remove failed */
            (mustExist ||               /* if any error is a failure, or */
             (errno != ENOENT))) {      /* if error other than no such file */
            printf("TPM_NVRAM_DeleteName: Error, (fatal) file remove failed, errno %d\n",
                   errno);
            rc = TPM_FAIL;
        }
    }
    return rc;
}

