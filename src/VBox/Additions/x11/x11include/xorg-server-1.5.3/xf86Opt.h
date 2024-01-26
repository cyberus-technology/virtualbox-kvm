
/*
 * Copyright (c) 1998-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/* Option handling things that ModuleSetup procs can use */

#ifndef _XF86_OPT_H_
#define _XF86_OPT_H_

typedef struct {
    double freq;
    int units;
} OptFrequency;

typedef union {
    unsigned long       num;
    char *              str;
    double              realnum;
    Bool		bool;
    OptFrequency	freq;
} ValueUnion;
    
typedef enum {
    OPTV_NONE = 0,
    OPTV_INTEGER,
    OPTV_STRING,                /* a non-empty string */
    OPTV_ANYSTR,                /* Any string, including an empty one */
    OPTV_REAL,
    OPTV_BOOLEAN,
    OPTV_FREQ
} OptionValueType;

typedef enum {
    OPTUNITS_HZ = 1,
    OPTUNITS_KHZ,
    OPTUNITS_MHZ
} OptFreqUnits;

typedef struct {
    int                 token;
    const char*         name;
    OptionValueType     type;
    ValueUnion          value;
    Bool                found;
} OptionInfoRec, *OptionInfoPtr;

int xf86SetIntOption(pointer optlist, const char *name, int deflt);
double xf86SetRealOption(pointer optlist, const char *name, double deflt);
char *xf86SetStrOption(pointer optlist, const char *name, char *deflt);
int xf86SetBoolOption(pointer list, const char *name, int deflt );
int xf86CheckIntOption(pointer optlist, const char *name, int deflt);
double xf86CheckRealOption(pointer optlist, const char *name, double deflt);
char *xf86CheckStrOption(pointer optlist, const char *name, char *deflt);
int xf86CheckBoolOption(pointer list, const char *name, int deflt );
pointer xf86AddNewOption(pointer head, const char *name, const char *val );
pointer xf86NewOption(char *name, char *value );
pointer xf86NextOption(pointer list );
pointer xf86OptionListCreate(const char **options, int count, int used);
pointer xf86OptionListMerge(pointer head, pointer tail);
void xf86OptionListFree(pointer opt);
char *xf86OptionName(pointer opt);
char *xf86OptionValue(pointer opt);
void xf86OptionListReport(pointer parm);
pointer xf86FindOption(pointer options, const char *name);
char *xf86FindOptionValue(pointer options, const char *name);
void xf86MarkOptionUsed(pointer option);
void xf86MarkOptionUsedByName(pointer options, const char *name);
Bool xf86CheckIfOptionUsed(pointer option);
Bool xf86CheckIfOptionUsedByName(pointer options, const char *name);
void xf86ShowUnusedOptions(int scrnIndex, pointer options);
void xf86ProcessOptions(int scrnIndex, pointer options, OptionInfoPtr optinfo);
OptionInfoPtr xf86TokenToOptinfo(const OptionInfoRec *table, int token);
const char *xf86TokenToOptName(const OptionInfoRec *table, int token);
Bool xf86IsOptionSet(const OptionInfoRec *table, int token);
char *xf86GetOptValString(const OptionInfoRec *table, int token);
Bool xf86GetOptValInteger(const OptionInfoRec *table, int token, int *value);
Bool xf86GetOptValULong(const OptionInfoRec *table, int token, unsigned long *value);
Bool xf86GetOptValReal(const OptionInfoRec *table, int token, double *value);
Bool xf86GetOptValFreq(const OptionInfoRec *table, int token,
			OptFreqUnits expectedUnits, double *value);
Bool xf86GetOptValBool(const OptionInfoRec *table, int token, Bool *value);
Bool xf86ReturnOptValBool(const OptionInfoRec *table, int token, Bool def);
int xf86NameCmp(const char *s1, const char *s2);
char *xf86NormalizeName(const char *s);
pointer xf86ReplaceIntOption(pointer optlist,  const char *name, const int val);
pointer xf86ReplaceRealOption(pointer optlist,  const char *name, const double val);
pointer xf86ReplaceBoolOption(pointer optlist, const char *name, const Bool val);
pointer xf86ReplaceStrOption(pointer optlist,  const char *name, const char* val);        
#endif
