
/* Prototypes for functions that the DDX must provide */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _DPMSPROC_H_
#define _DPMSPROC_H_

void DPMSSet(int level);
int  DPMSGet(int *plevel);
Bool DPMSSupported(void);

#endif
