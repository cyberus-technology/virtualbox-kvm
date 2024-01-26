




/* $XConsortium: cards.h /main/3 1996/02/21 18:12:53 kaleb $ */

#ifndef CARD_DATABASE_FILE
#define CARD_DATABASE_FILE "Cards"
#endif

#define MAX_CARDS 1000

typedef struct {
	char *name;		/* Name of the card. */
	char *chipset;		/* Chipset (decriptive). */
	char *server;		/* Server identifier. */
        char *driver;		/* Driver identifier. */
        char *ramdac;		/* Ramdac identifier. */
	char *clockchip;	/* Clockchip identifier. */
	char *dacspeed;		/* DAC speed rating. */
	int flags;
	char *lines;		/* Additional Device section lines. */
} Card;

/* Flags: */
#define NOCLOCKPROBE	0x1	/* Never probe clocks of the card. */
#define UNSUPPORTED	0x2	/* Card is not supported (only VGA). */

extern int lastcard;

extern Card card[MAX_CARDS];

extern int lookupcard ( char *name );
extern int parse_database ( void );
extern void sort_database ( void );
extern void keypress ( void );
