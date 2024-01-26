/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Config.h,v 1.5 2000/02/24 05:36:50 tsi Exp $ */
/*
 * Copyright 1997 by The XFree86 Project, Inc
 */

#ifndef _xf86_config_h
#define _xf86_config_h

#ifdef HAVE_PARSER_DECLS
/*
 * global structure that holds the result of parsing the config file
 */
extern XF86ConfigPtr xf86configptr;
#endif

/*
 * prototypes
 */
char ** xf86ModulelistFromConfig(pointer **);
char ** xf86DriverlistFromConfig(void);
char ** xf86DriverlistFromCompile(void);
char ** xf86InputDriverlistFromConfig(void);
char ** xf86InputDriverlistFromCompile(void);
Bool xf86BuiltinInputDriver(const char *);
Bool xf86HandleConfigFile(void);

#endif /* _xf86_config_h */
