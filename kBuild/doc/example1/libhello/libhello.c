/* $Id: libhello.c 2343 2009-04-19 21:44:50Z bird $ */
/** @file
 * Example no. 1 - libhello.c - Hello world library.
 */

/*
 * The author disclaims copyright to this example code and places
 * it in the public domain.
 *
 * #include <full-legal-disclaimer.h>
 *
 */

#include <stdio.h>

extern int print_hello_world(void);

int print_hello_world(void)
{
    printf("Hello library world!\n");
    return 0;
}


