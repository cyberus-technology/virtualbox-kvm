
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "dt_grammar.y"

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef VBOX
#pragma ident	"%Z%%M%	%I%	%E% SMI"
#else
# ifdef _MSC_VER
#  pragma warning(disable:4255 4702)
# endif
#endif

#include <dt_impl.h>

#define	OP1(op, c)	dt_node_op1(op, c)
#define	OP2(op, l, r)	dt_node_op2(op, l, r)
#define	OP3(x, y, z)	dt_node_op3(x, y, z)
#define	LINK(l, r)	dt_node_link(l, r)
#define	DUP(s)		strdup(s)

#ifdef VBOX
# define YYMALLOC RTMemAlloc
# define YYFREE   RTMemFree
#endif




/* Line 189 of yacc.c  */
#line 124 "dt_grammar.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DT_TOK_EOF = 0,
     DT_TOK_COMMA = 258,
     DT_TOK_ELLIPSIS = 259,
     DT_TOK_ASGN = 260,
     DT_TOK_ADD_EQ = 261,
     DT_TOK_SUB_EQ = 262,
     DT_TOK_MUL_EQ = 263,
     DT_TOK_DIV_EQ = 264,
     DT_TOK_MOD_EQ = 265,
     DT_TOK_AND_EQ = 266,
     DT_TOK_XOR_EQ = 267,
     DT_TOK_OR_EQ = 268,
     DT_TOK_LSH_EQ = 269,
     DT_TOK_RSH_EQ = 270,
     DT_TOK_QUESTION = 271,
     DT_TOK_COLON = 272,
     DT_TOK_LOR = 273,
     DT_TOK_LXOR = 274,
     DT_TOK_LAND = 275,
     DT_TOK_BOR = 276,
     DT_TOK_XOR = 277,
     DT_TOK_BAND = 278,
     DT_TOK_EQU = 279,
     DT_TOK_NEQ = 280,
     DT_TOK_LT = 281,
     DT_TOK_LE = 282,
     DT_TOK_GT = 283,
     DT_TOK_GE = 284,
     DT_TOK_LSH = 285,
     DT_TOK_RSH = 286,
     DT_TOK_ADD = 287,
     DT_TOK_SUB = 288,
     DT_TOK_MUL = 289,
     DT_TOK_DIV = 290,
     DT_TOK_MOD = 291,
     DT_TOK_LNEG = 292,
     DT_TOK_BNEG = 293,
     DT_TOK_ADDADD = 294,
     DT_TOK_SUBSUB = 295,
     DT_TOK_PREINC = 296,
     DT_TOK_POSTINC = 297,
     DT_TOK_PREDEC = 298,
     DT_TOK_POSTDEC = 299,
     DT_TOK_IPOS = 300,
     DT_TOK_INEG = 301,
     DT_TOK_DEREF = 302,
     DT_TOK_ADDROF = 303,
     DT_TOK_OFFSETOF = 304,
     DT_TOK_SIZEOF = 305,
     DT_TOK_STRINGOF = 306,
     DT_TOK_XLATE = 307,
     DT_TOK_LPAR = 308,
     DT_TOK_RPAR = 309,
     DT_TOK_LBRAC = 310,
     DT_TOK_RBRAC = 311,
     DT_TOK_PTR = 312,
     DT_TOK_DOT = 313,
     DT_TOK_STRING = 314,
     DT_TOK_IDENT = 315,
     DT_TOK_PSPEC = 316,
     DT_TOK_AGG = 317,
     DT_TOK_TNAME = 318,
     DT_TOK_INT = 319,
     DT_KEY_AUTO = 320,
     DT_KEY_BREAK = 321,
     DT_KEY_CASE = 322,
     DT_KEY_CHAR = 323,
     DT_KEY_CONST = 324,
     DT_KEY_CONTINUE = 325,
     DT_KEY_COUNTER = 326,
     DT_KEY_DEFAULT = 327,
     DT_KEY_DO = 328,
     DT_KEY_DOUBLE = 329,
     DT_KEY_ELSE = 330,
     DT_KEY_ENUM = 331,
     DT_KEY_EXTERN = 332,
     DT_KEY_FLOAT = 333,
     DT_KEY_FOR = 334,
     DT_KEY_GOTO = 335,
     DT_KEY_IF = 336,
     DT_KEY_IMPORT = 337,
     DT_KEY_INLINE = 338,
     DT_KEY_INT = 339,
     DT_KEY_LONG = 340,
     DT_KEY_PROBE = 341,
     DT_KEY_PROVIDER = 342,
     DT_KEY_REGISTER = 343,
     DT_KEY_RESTRICT = 344,
     DT_KEY_RETURN = 345,
     DT_KEY_SELF = 346,
     DT_KEY_SHORT = 347,
     DT_KEY_SIGNED = 348,
     DT_KEY_STATIC = 349,
     DT_KEY_STRING = 350,
     DT_KEY_STRUCT = 351,
     DT_KEY_SWITCH = 352,
     DT_KEY_THIS = 353,
     DT_KEY_TYPEDEF = 354,
     DT_KEY_UNION = 355,
     DT_KEY_UNSIGNED = 356,
     DT_KEY_VOID = 357,
     DT_KEY_VOLATILE = 358,
     DT_KEY_WHILE = 359,
     DT_KEY_XLATOR = 360,
     DT_TOK_EPRED = 361,
     DT_CTX_DEXPR = 362,
     DT_CTX_DPROG = 363,
     DT_CTX_DTYPE = 364
   };
#endif
/* Tokens.  */
#define DT_TOK_EOF 0
#define DT_TOK_COMMA 258
#define DT_TOK_ELLIPSIS 259
#define DT_TOK_ASGN 260
#define DT_TOK_ADD_EQ 261
#define DT_TOK_SUB_EQ 262
#define DT_TOK_MUL_EQ 263
#define DT_TOK_DIV_EQ 264
#define DT_TOK_MOD_EQ 265
#define DT_TOK_AND_EQ 266
#define DT_TOK_XOR_EQ 267
#define DT_TOK_OR_EQ 268
#define DT_TOK_LSH_EQ 269
#define DT_TOK_RSH_EQ 270
#define DT_TOK_QUESTION 271
#define DT_TOK_COLON 272
#define DT_TOK_LOR 273
#define DT_TOK_LXOR 274
#define DT_TOK_LAND 275
#define DT_TOK_BOR 276
#define DT_TOK_XOR 277
#define DT_TOK_BAND 278
#define DT_TOK_EQU 279
#define DT_TOK_NEQ 280
#define DT_TOK_LT 281
#define DT_TOK_LE 282
#define DT_TOK_GT 283
#define DT_TOK_GE 284
#define DT_TOK_LSH 285
#define DT_TOK_RSH 286
#define DT_TOK_ADD 287
#define DT_TOK_SUB 288
#define DT_TOK_MUL 289
#define DT_TOK_DIV 290
#define DT_TOK_MOD 291
#define DT_TOK_LNEG 292
#define DT_TOK_BNEG 293
#define DT_TOK_ADDADD 294
#define DT_TOK_SUBSUB 295
#define DT_TOK_PREINC 296
#define DT_TOK_POSTINC 297
#define DT_TOK_PREDEC 298
#define DT_TOK_POSTDEC 299
#define DT_TOK_IPOS 300
#define DT_TOK_INEG 301
#define DT_TOK_DEREF 302
#define DT_TOK_ADDROF 303
#define DT_TOK_OFFSETOF 304
#define DT_TOK_SIZEOF 305
#define DT_TOK_STRINGOF 306
#define DT_TOK_XLATE 307
#define DT_TOK_LPAR 308
#define DT_TOK_RPAR 309
#define DT_TOK_LBRAC 310
#define DT_TOK_RBRAC 311
#define DT_TOK_PTR 312
#define DT_TOK_DOT 313
#define DT_TOK_STRING 314
#define DT_TOK_IDENT 315
#define DT_TOK_PSPEC 316
#define DT_TOK_AGG 317
#define DT_TOK_TNAME 318
#define DT_TOK_INT 319
#define DT_KEY_AUTO 320
#define DT_KEY_BREAK 321
#define DT_KEY_CASE 322
#define DT_KEY_CHAR 323
#define DT_KEY_CONST 324
#define DT_KEY_CONTINUE 325
#define DT_KEY_COUNTER 326
#define DT_KEY_DEFAULT 327
#define DT_KEY_DO 328
#define DT_KEY_DOUBLE 329
#define DT_KEY_ELSE 330
#define DT_KEY_ENUM 331
#define DT_KEY_EXTERN 332
#define DT_KEY_FLOAT 333
#define DT_KEY_FOR 334
#define DT_KEY_GOTO 335
#define DT_KEY_IF 336
#define DT_KEY_IMPORT 337
#define DT_KEY_INLINE 338
#define DT_KEY_INT 339
#define DT_KEY_LONG 340
#define DT_KEY_PROBE 341
#define DT_KEY_PROVIDER 342
#define DT_KEY_REGISTER 343
#define DT_KEY_RESTRICT 344
#define DT_KEY_RETURN 345
#define DT_KEY_SELF 346
#define DT_KEY_SHORT 347
#define DT_KEY_SIGNED 348
#define DT_KEY_STATIC 349
#define DT_KEY_STRING 350
#define DT_KEY_STRUCT 351
#define DT_KEY_SWITCH 352
#define DT_KEY_THIS 353
#define DT_KEY_TYPEDEF 354
#define DT_KEY_UNION 355
#define DT_KEY_UNSIGNED 356
#define DT_KEY_VOID 357
#define DT_KEY_VOLATILE 358
#define DT_KEY_WHILE 359
#define DT_KEY_XLATOR 360
#define DT_TOK_EPRED 361
#define DT_CTX_DEXPR 362
#define DT_CTX_DPROG 363
#define DT_CTX_DTYPE 364




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 51 "dt_grammar.y"

	dt_node_t *l_node;
	dt_decl_t *l_decl;
	char *l_str;
	uintmax_t l_int;
	int l_tok;



/* Line 214 of yacc.c  */
#line 390 "dt_grammar.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 402 "dt_grammar.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  99
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   837

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  113
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  81
/* YYNRULES -- Number of rules.  */
#define YYNRULES  239
/* YYNRULES -- Number of states.  */
#define YYNSTATES  363

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   364

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   110,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   111,     2,   112,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     9,    12,    14,    17,    19,    22,
      24,    27,    29,    32,    34,    36,    38,    40,    42,    43,
      51,    62,    72,    74,    77,    82,    89,    95,    97,   100,
     107,   112,   114,   119,   124,   132,   134,   136,   140,   142,
     144,   146,   150,   151,   153,   155,   159,   161,   163,   165,
     167,   169,   171,   175,   177,   182,   186,   191,   195,   199,
     203,   207,   210,   213,   220,   227,   235,   237,   240,   243,
     246,   249,   254,   257,   259,   261,   263,   265,   267,   269,
     271,   276,   278,   282,   286,   290,   292,   296,   300,   302,
     306,   310,   312,   316,   320,   324,   328,   330,   334,   338,
     340,   344,   346,   350,   352,   356,   358,   362,   364,   368,
     370,   374,   376,   378,   384,   386,   390,   392,   394,   396,
     398,   400,   402,   404,   406,   408,   410,   412,   414,   418,
     421,   425,   427,   430,   432,   435,   437,   440,   442,   445,
     447,   450,   452,   455,   457,   459,   461,   463,   465,   467,
     469,   471,   473,   475,   477,   479,   481,   483,   485,   487,
     489,   491,   493,   495,   497,   499,   501,   503,   507,   510,
     513,   516,   520,   524,   526,   528,   530,   533,   535,   539,
     541,   545,   547,   550,   552,   555,   557,   561,   563,   566,
     570,   574,   577,   580,   583,   587,   591,   593,   597,   599,
     603,   605,   608,   610,   614,   617,   620,   622,   624,   627,
     630,   634,   636,   639,   641,   643,   647,   649,   653,   655,
     658,   661,   663,   666,   668,   670,   673,   677,   680,   682,
     685,   687,   688,   693,   694,   696,   698,   699,   704,   705
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     114,     0,    -1,   115,     0,    -1,   116,     0,    -1,   117,
       0,    -1,   107,    -1,   107,   155,    -1,   108,    -1,   108,
     118,    -1,   109,    -1,   109,   185,    -1,   119,    -1,   118,
     119,    -1,   120,    -1,   122,    -1,   125,    -1,   128,    -1,
     156,    -1,    -1,    83,   157,   177,   121,     5,   153,   110,
      -1,   105,   185,    26,   185,    60,    28,   111,   123,   112,
     110,    -1,   105,   185,    26,   185,    60,    28,   111,   112,
     110,    -1,   124,    -1,   123,   124,    -1,    60,     5,   153,
     110,    -1,    87,    60,   111,   126,   112,   110,    -1,    87,
      60,   111,   112,   110,    -1,   127,    -1,   126,   127,    -1,
      86,    60,   191,    17,   191,   110,    -1,    86,    60,   191,
     110,    -1,   129,    -1,   129,   111,   132,   112,    -1,   129,
      35,   155,   106,    -1,   129,    35,   155,   106,   111,   132,
     112,    -1,   130,    -1,   131,    -1,   130,     3,   131,    -1,
      61,    -1,    64,    -1,   133,    -1,   132,   110,   133,    -1,
      -1,   155,    -1,   153,    -1,   134,     3,   153,    -1,    60,
      -1,    62,    -1,    64,    -1,    59,    -1,    91,    -1,    98,
      -1,    53,   155,    54,    -1,   135,    -1,   136,    55,   134,
      56,    -1,   136,    53,    54,    -1,   136,    53,   134,    54,
      -1,   136,    58,    60,    -1,   136,    58,    63,    -1,   136,
      57,    60,    -1,   136,    57,    63,    -1,   136,    39,    -1,
     136,    40,    -1,    49,    53,   185,     3,    60,    54,    -1,
      49,    53,   185,     3,    63,    54,    -1,    52,    26,   185,
      28,    53,   155,    54,    -1,   136,    -1,    39,   137,    -1,
      40,   137,    -1,   138,   139,    -1,    50,   137,    -1,    50,
      53,   185,    54,    -1,    51,   137,    -1,    23,    -1,    34,
      -1,    32,    -1,    33,    -1,    38,    -1,    37,    -1,   137,
      -1,    53,   185,    54,   139,    -1,   139,    -1,   140,    34,
     139,    -1,   140,    35,   139,    -1,   140,    36,   139,    -1,
     140,    -1,   141,    32,   140,    -1,   141,    33,   140,    -1,
     141,    -1,   142,    30,   141,    -1,   142,    31,   141,    -1,
     142,    -1,   143,    26,   142,    -1,   143,    28,   142,    -1,
     143,    27,   142,    -1,   143,    29,   142,    -1,   143,    -1,
     144,    24,   143,    -1,   144,    25,   143,    -1,   144,    -1,
     145,    23,   144,    -1,   145,    -1,   146,    22,   145,    -1,
     146,    -1,   147,    21,   146,    -1,   147,    -1,   148,    20,
     147,    -1,   148,    -1,   149,    19,   148,    -1,   149,    -1,
     150,    18,   149,    -1,   152,    -1,   150,    -1,   150,    16,
     155,    17,   152,    -1,   152,    -1,   137,   154,   153,    -1,
       5,    -1,     8,    -1,     9,    -1,    10,    -1,     6,    -1,
       7,    -1,    14,    -1,    15,    -1,    11,    -1,    12,    -1,
      13,    -1,   153,    -1,   155,     3,   153,    -1,   157,   110,
      -1,   157,   167,   110,    -1,   160,    -1,   160,   157,    -1,
     161,    -1,   161,   157,    -1,   162,    -1,   162,   157,    -1,
     159,    -1,   159,   157,    -1,   161,    -1,   161,   157,    -1,
     162,    -1,   162,   157,    -1,    65,    -1,    88,    -1,    94,
      -1,    77,    -1,    99,    -1,   159,    -1,    91,    -1,    98,
      -1,   102,    -1,    68,    -1,    92,    -1,    84,    -1,    85,
      -1,    78,    -1,    74,    -1,    93,    -1,   101,    -1,    95,
      -1,    63,    -1,   163,    -1,   173,    -1,    69,    -1,    89,
      -1,   103,    -1,   164,   166,   112,    -1,   165,    60,    -1,
     165,    63,    -1,   165,   111,    -1,   165,    60,   111,    -1,
     165,    63,   111,    -1,    96,    -1,   100,    -1,   169,    -1,
     166,   169,    -1,   168,    -1,   167,     3,   168,    -1,   177,
      -1,   170,   171,   110,    -1,   161,    -1,   161,   170,    -1,
     162,    -1,   162,   170,    -1,   172,    -1,   171,     3,   172,
      -1,   177,    -1,    17,   151,    -1,   177,    17,   151,    -1,
     174,   175,   112,    -1,    76,    60,    -1,    76,    63,    -1,
      76,   111,    -1,    76,    60,   111,    -1,    76,    63,   111,
      -1,   176,    -1,   175,     3,   176,    -1,    60,    -1,    60,
       5,   155,    -1,   178,    -1,   180,   178,    -1,    60,    -1,
     179,   177,    54,    -1,   178,   188,    -1,   178,   191,    -1,
      53,    -1,    34,    -1,    34,   181,    -1,    34,   180,    -1,
      34,   181,   180,    -1,   162,    -1,   181,   162,    -1,   183,
      -1,     4,    -1,   183,     3,     4,    -1,   184,    -1,   183,
       3,   184,    -1,   158,    -1,   158,   177,    -1,   158,   186,
      -1,   170,    -1,   170,   186,    -1,   180,    -1,   187,    -1,
     180,   187,    -1,   179,   186,    54,    -1,   187,   188,    -1,
     188,    -1,   187,   191,    -1,   191,    -1,    -1,    55,   189,
     190,    56,    -1,    -1,   151,    -1,   182,    -1,    -1,    53,
     192,   193,    54,    -1,    -1,   182,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   221,   221,   222,   223,   226,   227,   230,   231,   234,
     235,   239,   240,   244,   245,   246,   247,   248,   253,   252,
     268,   272,   279,   280,   284,   290,   293,   299,   300,   304,
     307,   314,   330,   333,   337,   344,   348,   349,   355,   356,
     359,   360,   363,   364,   368,   369,   375,   376,   377,   378,
     379,   380,   381,   385,   386,   390,   393,   397,   400,   403,
     406,   409,   412,   415,   419,   423,   430,   431,   432,   433,
     434,   435,   438,   443,   444,   445,   446,   447,   448,   452,
     453,   459,   460,   463,   466,   472,   473,   476,   482,   483,
     486,   492,   493,   496,   499,   502,   508,   509,   512,   518,
     519,   525,   526,   532,   533,   539,   540,   546,   547,   553,
     554,   559,   563,   564,   569,   570,   576,   577,   578,   579,
     580,   581,   582,   583,   584,   585,   586,   589,   590,   595,
     600,   608,   609,   610,   611,   612,   613,   617,   618,   619,
     620,   621,   622,   626,   627,   628,   629,   630,   634,   635,
     636,   639,   640,   641,   642,   643,   644,   645,   646,   647,
     648,   651,   652,   653,   656,   657,   658,   662,   665,   666,
     670,   671,   672,   676,   677,   681,   682,   686,   687,   693,
     700,   706,   707,   708,   709,   713,   714,   718,   719,   720,
     726,   727,   728,   732,   733,   734,   738,   739,   742,   743,
     748,   749,   753,   754,   755,   756,   759,   762,   763,   764,
     765,   769,   770,   774,   775,   776,   781,   782,   788,   791,
     794,   799,   802,   808,   809,   810,   814,   815,   816,   817,
     818,   821,   821,   829,   830,   831,   834,   834,   842,   843
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "DT_TOK_EOF", "error", "$undefined", "DT_TOK_COMMA", "DT_TOK_ELLIPSIS",
  "DT_TOK_ASGN", "DT_TOK_ADD_EQ", "DT_TOK_SUB_EQ", "DT_TOK_MUL_EQ",
  "DT_TOK_DIV_EQ", "DT_TOK_MOD_EQ", "DT_TOK_AND_EQ", "DT_TOK_XOR_EQ",
  "DT_TOK_OR_EQ", "DT_TOK_LSH_EQ", "DT_TOK_RSH_EQ", "DT_TOK_QUESTION",
  "DT_TOK_COLON", "DT_TOK_LOR", "DT_TOK_LXOR", "DT_TOK_LAND", "DT_TOK_BOR",
  "DT_TOK_XOR", "DT_TOK_BAND", "DT_TOK_EQU", "DT_TOK_NEQ", "DT_TOK_LT",
  "DT_TOK_LE", "DT_TOK_GT", "DT_TOK_GE", "DT_TOK_LSH", "DT_TOK_RSH",
  "DT_TOK_ADD", "DT_TOK_SUB", "DT_TOK_MUL", "DT_TOK_DIV", "DT_TOK_MOD",
  "DT_TOK_LNEG", "DT_TOK_BNEG", "DT_TOK_ADDADD", "DT_TOK_SUBSUB",
  "DT_TOK_PREINC", "DT_TOK_POSTINC", "DT_TOK_PREDEC", "DT_TOK_POSTDEC",
  "DT_TOK_IPOS", "DT_TOK_INEG", "DT_TOK_DEREF", "DT_TOK_ADDROF",
  "DT_TOK_OFFSETOF", "DT_TOK_SIZEOF", "DT_TOK_STRINGOF", "DT_TOK_XLATE",
  "DT_TOK_LPAR", "DT_TOK_RPAR", "DT_TOK_LBRAC", "DT_TOK_RBRAC",
  "DT_TOK_PTR", "DT_TOK_DOT", "DT_TOK_STRING", "DT_TOK_IDENT",
  "DT_TOK_PSPEC", "DT_TOK_AGG", "DT_TOK_TNAME", "DT_TOK_INT",
  "DT_KEY_AUTO", "DT_KEY_BREAK", "DT_KEY_CASE", "DT_KEY_CHAR",
  "DT_KEY_CONST", "DT_KEY_CONTINUE", "DT_KEY_COUNTER", "DT_KEY_DEFAULT",
  "DT_KEY_DO", "DT_KEY_DOUBLE", "DT_KEY_ELSE", "DT_KEY_ENUM",
  "DT_KEY_EXTERN", "DT_KEY_FLOAT", "DT_KEY_FOR", "DT_KEY_GOTO",
  "DT_KEY_IF", "DT_KEY_IMPORT", "DT_KEY_INLINE", "DT_KEY_INT",
  "DT_KEY_LONG", "DT_KEY_PROBE", "DT_KEY_PROVIDER", "DT_KEY_REGISTER",
  "DT_KEY_RESTRICT", "DT_KEY_RETURN", "DT_KEY_SELF", "DT_KEY_SHORT",
  "DT_KEY_SIGNED", "DT_KEY_STATIC", "DT_KEY_STRING", "DT_KEY_STRUCT",
  "DT_KEY_SWITCH", "DT_KEY_THIS", "DT_KEY_TYPEDEF", "DT_KEY_UNION",
  "DT_KEY_UNSIGNED", "DT_KEY_VOID", "DT_KEY_VOLATILE", "DT_KEY_WHILE",
  "DT_KEY_XLATOR", "DT_TOK_EPRED", "DT_CTX_DEXPR", "DT_CTX_DPROG",
  "DT_CTX_DTYPE", "';'", "'{'", "'}'", "$accept", "dtrace_program",
  "d_expression", "d_program", "d_type", "translation_unit",
  "external_declaration", "inline_definition", "$@1",
  "translator_definition", "translator_member_list", "translator_member",
  "provider_definition", "provider_probe_list", "provider_probe",
  "probe_definition", "probe_specifiers", "probe_specifier_list",
  "probe_specifier", "statement_list", "statement",
  "argument_expression_list", "primary_expression", "postfix_expression",
  "unary_expression", "unary_operator", "cast_expression",
  "multiplicative_expression", "additive_expression", "shift_expression",
  "relational_expression", "equality_expression", "and_expression",
  "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_xor_expression",
  "logical_or_expression", "constant_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "declaration", "declaration_specifiers",
  "parameter_declaration_specifiers", "storage_class_specifier",
  "d_storage_class_specifier", "type_specifier", "type_qualifier",
  "struct_or_union_specifier", "struct_or_union_definition",
  "struct_or_union", "struct_declaration_list", "init_declarator_list",
  "init_declarator", "struct_declaration", "specifier_qualifier_list",
  "struct_declarator_list", "struct_declarator", "enum_specifier",
  "enum_definition", "enumerator_list", "enumerator", "declarator",
  "direct_declarator", "lparen", "pointer", "type_qualifier_list",
  "parameter_type_list", "parameter_list", "parameter_declaration",
  "type_name", "abstract_declarator", "direct_abstract_declarator",
  "array", "$@2", "array_parameters", "function", "$@3",
  "function_parameters", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
      59,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   113,   114,   114,   114,   115,   115,   116,   116,   117,
     117,   118,   118,   119,   119,   119,   119,   119,   121,   120,
     122,   122,   123,   123,   124,   125,   125,   126,   126,   127,
     127,   128,   128,   128,   128,   129,   130,   130,   131,   131,
     132,   132,   133,   133,   134,   134,   135,   135,   135,   135,
     135,   135,   135,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   137,   137,   137,   137,
     137,   137,   137,   138,   138,   138,   138,   138,   138,   139,
     139,   140,   140,   140,   140,   141,   141,   141,   142,   142,
     142,   143,   143,   143,   143,   143,   144,   144,   144,   145,
     145,   146,   146,   147,   147,   148,   148,   149,   149,   150,
     150,   151,   152,   152,   153,   153,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   154,   154,   155,   155,   156,
     156,   157,   157,   157,   157,   157,   157,   158,   158,   158,
     158,   158,   158,   159,   159,   159,   159,   159,   160,   160,
     160,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   162,   162,   162,   163,   163,   163,
     164,   164,   164,   165,   165,   166,   166,   167,   167,   168,
     169,   170,   170,   170,   170,   171,   171,   172,   172,   172,
     173,   173,   173,   174,   174,   174,   175,   175,   176,   176,
     177,   177,   178,   178,   178,   178,   179,   180,   180,   180,
     180,   181,   181,   182,   182,   182,   183,   183,   184,   184,
     184,   185,   185,   186,   186,   186,   187,   187,   187,   187,
     187,   189,   188,   190,   190,   190,   192,   191,   193,   193
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     2,     2,     1,     2,     1,     2,     1,
       2,     1,     2,     1,     1,     1,     1,     1,     0,     7,
      10,     9,     1,     2,     4,     6,     5,     1,     2,     6,
       4,     1,     4,     4,     7,     1,     1,     3,     1,     1,
       1,     3,     0,     1,     1,     3,     1,     1,     1,     1,
       1,     1,     3,     1,     4,     3,     4,     3,     3,     3,
       3,     2,     2,     6,     6,     7,     1,     2,     2,     2,
       2,     4,     2,     1,     1,     1,     1,     1,     1,     1,
       4,     1,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     1,     5,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     2,
       3,     1,     2,     1,     2,     1,     2,     1,     2,     1,
       2,     1,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     2,     2,
       2,     3,     3,     1,     1,     1,     2,     1,     3,     1,
       3,     1,     2,     1,     2,     1,     3,     1,     2,     3,
       3,     2,     2,     2,     3,     3,     1,     3,     1,     3,
       1,     2,     1,     3,     2,     2,     1,     1,     2,     2,
       3,     1,     2,     1,     1,     3,     1,     3,     1,     2,
       2,     1,     2,     1,     1,     2,     3,     2,     1,     2,
       1,     0,     4,     0,     1,     1,     0,     4,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     5,     7,     9,     0,     0,     0,     0,    73,    75,
      76,    74,    78,    77,     0,     0,     0,     0,     0,     0,
       0,    49,    46,    47,    48,    50,    51,    53,    66,    79,
       0,    81,    85,    88,    91,    96,    99,   101,   103,   105,
     107,   109,   112,   114,   127,     6,    38,   161,    39,   143,
     152,   164,   157,     0,   146,   156,     0,   154,   155,     0,
     144,   165,   149,   153,   158,   145,   160,   173,   150,   147,
     174,   159,   151,   166,     0,     8,    11,    13,    14,    15,
      16,    31,    35,    36,    17,     0,   148,   131,   133,   135,
     162,     0,     0,   163,     0,   181,   183,   221,    10,     1,
       2,     3,     4,     0,    67,    68,     0,     0,    70,    72,
       0,     0,     0,    61,    62,     0,     0,     0,     0,   116,
     120,   121,   117,   118,   119,   124,   125,   126,   122,   123,
       0,    79,    69,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   191,   192,   193,     0,     0,     0,
      12,     0,    42,     0,   207,   206,   202,   129,     0,   177,
     179,   200,     0,     0,   132,   134,   136,     0,   175,     0,
     168,   169,   170,   198,     0,   196,   182,   184,   236,   231,
       0,   223,   222,   224,   228,   230,     0,     0,     0,    52,
       0,    55,     0,    44,     0,    59,    60,    57,    58,   115,
      82,    83,    84,    86,    87,    89,    90,    92,    94,    93,
      95,    97,    98,   100,   102,   104,   106,   108,     0,   110,
     128,   194,   195,    18,     0,     0,     0,     0,    40,    43,
      37,   211,   209,   208,     0,   130,   236,   204,   205,     0,
     201,   167,   176,     0,     0,   185,   187,   171,   172,     0,
       0,   190,   238,   233,     0,   225,   227,   229,     0,    71,
       0,    80,     0,    56,    54,     0,     0,     0,     0,     0,
      27,     0,    33,    42,    32,   212,   210,   178,   203,   188,
     111,     0,   180,     0,   199,   197,   214,   218,   137,   139,
     141,   239,   213,   216,     0,   234,   235,     0,   226,     0,
       0,     0,    45,   113,     0,     0,    26,     0,    28,     0,
      42,    41,   186,   189,   219,     0,   223,   220,   138,   140,
     142,     0,   237,   232,    63,    64,     0,     0,     0,    25,
       0,     0,   215,   217,    65,    19,     0,    30,     0,    34,
       0,     0,     0,     0,    22,    29,     0,    21,     0,    23,
       0,    20,    24
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     4,     5,     6,     7,    75,    76,    77,   276,    78,
     353,   354,    79,   279,   280,    80,    81,    82,    83,   237,
     238,   202,    27,    28,   131,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,   289,    43,
      44,   130,   111,    84,    85,   297,    86,    87,    95,    96,
      90,    91,    92,   177,   168,   169,   178,    97,   254,   255,
      93,    94,   184,   185,   170,   171,   172,   173,   243,   301,
     302,   303,    98,   264,   193,   194,   263,   307,   195,   262,
     304
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -224
static const yytype_int16 yypact[] =
{
     148,   553,   641,   723,    26,    49,    57,    76,  -224,  -224,
    -224,  -224,  -224,  -224,   596,   596,    88,   629,   596,    72,
     446,  -224,  -224,  -224,  -224,  -224,  -224,  -224,   179,   822,
     553,  -224,   152,    37,   118,   109,    56,   132,   137,   154,
     149,   162,   175,  -224,  -224,   205,  -224,  -224,  -224,  -224,
    -224,  -224,  -224,    11,  -224,  -224,   682,  -224,  -224,   155,
    -224,  -224,  -224,  -224,  -224,  -224,  -224,  -224,  -224,  -224,
    -224,  -224,  -224,  -224,   723,   641,  -224,  -224,  -224,  -224,
    -224,    10,   217,  -224,  -224,     7,  -224,   682,   682,   682,
    -224,   723,    22,  -224,   167,   723,   723,   139,  -224,  -224,
    -224,  -224,  -224,   553,  -224,  -224,   723,   446,  -224,  -224,
     723,    58,   177,  -224,  -224,   520,   553,    -5,    80,  -224,
    -224,  -224,  -224,  -224,  -224,  -224,  -224,  -224,  -224,  -224,
     553,  -224,  -224,   553,   553,   553,   553,   553,   553,   553,
     553,   553,   553,   553,   553,   553,   553,   553,   553,   553,
     553,   553,   553,   553,   127,   131,  -224,   110,   133,   209,
    -224,   553,   553,    81,     4,  -224,  -224,  -224,    17,  -224,
    -224,   145,   110,    50,  -224,  -224,  -224,   121,  -224,    94,
     159,   161,  -224,   243,    18,  -224,  -224,  -224,   119,  -224,
     139,   156,  -224,   145,  -224,  -224,   247,   213,   231,  -224,
     553,  -224,    62,  -224,    48,  -224,  -224,  -224,  -224,  -224,
    -224,  -224,  -224,   152,   152,    37,    37,   118,   118,   118,
     118,   109,   109,    56,   132,   137,   154,   149,    45,   162,
    -224,  -224,  -224,  -224,   -49,   723,    19,   116,  -224,   205,
    -224,  -224,  -224,     4,   110,  -224,  -224,  -224,  -224,   219,
     145,  -224,  -224,   553,    21,  -224,   257,  -224,  -224,   553,
     167,  -224,   331,   289,   221,   145,  -224,  -224,   117,  -224,
     223,  -224,   553,  -224,  -224,   553,   272,   218,   169,   -44,
    -224,   220,   170,   553,  -224,  -224,  -224,  -224,  -224,  -224,
    -224,    94,  -224,   553,  -224,  -224,  -224,   123,   682,   682,
     682,  -224,   280,  -224,   230,  -224,  -224,   229,  -224,   232,
     233,   553,  -224,  -224,   553,   236,  -224,   180,  -224,   263,
     553,  -224,  -224,  -224,  -224,   123,    79,  -224,  -224,  -224,
    -224,   372,  -224,  -224,  -224,  -224,    69,   182,    16,  -224,
     183,   129,  -224,  -224,  -224,  -224,   236,  -224,   -35,  -224,
     185,   291,   187,   -13,  -224,  -224,   553,  -224,   194,  -224,
     195,  -224,  -224
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -224,  -224,  -224,  -224,  -224,  -224,   234,  -224,  -224,  -224,
    -224,   -47,  -224,  -224,    28,  -224,  -224,  -224,   151,   -12,
      32,   201,  -224,  -224,    -1,  -224,   -15,    66,   122,   111,
     124,   165,   171,   172,   181,   174,   184,  -224,  -210,  -209,
    -107,  -224,     5,  -224,   -53,  -224,  -223,  -224,     3,     0,
    -224,  -224,  -224,  -224,  -224,    89,   160,   -64,  -224,    41,
    -224,  -224,  -224,    83,  -129,  -155,   -96,   -85,  -224,    82,
    -224,    13,   -10,   -93,  -180,  -141,  -224,  -224,  -164,  -224,
    -224
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -207
static const yytype_int16 yytable[] =
{
      29,   190,    89,   157,   192,    88,    45,   248,   203,   203,
     112,   265,   191,   104,   105,   132,   108,   109,   250,    29,
     244,   260,   153,   209,   291,   351,    99,   179,   233,   267,
     247,   186,   187,   346,   174,   175,   176,   277,   164,   298,
     298,   164,   277,   249,   290,   161,   230,   351,   153,   100,
     256,   272,   266,   305,   290,   205,    89,   101,   206,    88,
     165,   153,   275,   278,   159,   272,   313,   166,   317,   136,
     137,   154,   153,    51,   155,    89,   102,   352,    88,   242,
     144,   145,   180,   323,   290,   181,   248,    89,    89,    89,
      88,    88,    88,    61,   190,   190,   196,   197,   110,   358,
     198,   267,    29,   165,   274,   191,    29,    73,   298,   247,
     166,   253,   199,   179,    29,    29,   273,   167,   210,   211,
     212,   162,   156,   344,   266,   282,   347,   245,   164,    29,
     261,   292,   188,   182,   189,   140,   141,   142,   143,   166,
     207,   106,    46,   208,   164,    48,   265,   165,   138,   139,
      29,   338,    29,  -206,   166,   146,   228,   164,   286,   147,
      29,    29,   256,   165,   241,   312,   236,   239,   324,   149,
     166,   250,  -206,   164,  -206,   148,   188,   309,   189,  -206,
     310,   150,   350,   166,    47,   271,   133,   134,   135,    50,
      51,   151,   188,   152,   189,    52,   249,    53,   246,    55,
     189,   325,   213,   214,   327,    57,    58,   337,   153,   188,
      61,   189,   326,    63,    64,   158,    66,    67,   113,   114,
     163,    70,    71,    72,    73,   281,   283,   183,   284,   325,
     325,   200,   115,   251,   116,   235,   117,   118,   231,   283,
     326,   349,   232,   285,   234,   328,   329,   330,   259,   360,
     268,   217,   218,   219,   220,     1,     2,     3,    29,   270,
     215,   216,   300,   300,   294,   299,   299,   269,   221,   222,
     257,    29,   258,   288,   293,   308,   311,   314,   315,   316,
     319,   320,    29,   331,   332,   333,   334,   335,   239,   246,
     339,   340,   345,   296,   348,   355,   356,   357,    89,    89,
      89,    88,    88,    88,   361,   362,   359,   318,   341,   160,
      29,   223,     8,    29,   240,   321,   336,   204,   224,    29,
     225,     9,    10,    11,   227,   239,    12,    13,    14,    15,
     226,   300,   322,   287,   299,   296,   229,   252,    16,    17,
      18,    19,    20,   295,   343,   306,     0,     0,    21,    22,
       0,    23,    47,    24,    49,    29,     0,    50,    51,     0,
       0,     0,     0,    52,     0,    53,    54,    55,     0,     0,
       0,     0,     0,    57,    58,     0,   342,    60,    61,     0,
      25,    63,    64,    65,    66,    67,     0,    26,    69,    70,
      71,    72,    73,     0,    47,     0,    49,     0,     0,    50,
      51,     0,     0,     0,     0,    52,     0,    53,    54,    55,
       0,     0,     0,     0,     0,    57,    58,     0,     0,    60,
      61,     0,     0,    63,    64,    65,    66,    67,     0,     0,
      69,    70,    71,    72,    73,    47,     0,    49,     0,     0,
      50,    51,     0,     0,     0,     0,    52,     0,    53,    54,
      55,     0,     0,     0,     0,     0,    57,    58,     0,     0,
      60,    61,     0,     0,    63,    64,    65,    66,    67,     8,
       0,    69,    70,    71,    72,    73,     0,     0,     9,    10,
      11,     0,     0,    12,    13,    14,    15,     0,     0,     0,
       0,     0,     0,     0,     0,    16,    17,    18,    19,    20,
       0,     0,     0,     0,     0,    21,    22,     0,    23,    47,
      24,     0,     0,     0,    50,    51,     0,     0,     0,     0,
      52,     0,    53,     0,    55,     0,     0,     0,     0,     0,
      57,    58,     0,     0,     0,    61,     0,    25,    63,    64,
       0,    66,    67,     8,    26,     0,    70,    71,    72,    73,
       0,     0,     9,    10,    11,     0,     0,    12,    13,    14,
      15,     0,     0,     0,     0,     0,     0,     0,     0,    16,
      17,    18,    19,    20,   201,     0,     8,     0,     0,    21,
      22,     0,    23,     0,    24,     9,    10,    11,     0,     0,
      12,    13,    14,    15,     0,     0,     0,     0,     0,     0,
       0,     0,    16,    17,    18,    19,    20,     0,     0,     0,
       0,    25,    21,    22,     0,    23,     0,    24,    26,     8,
       0,     0,     0,     0,     0,     0,     0,     0,     9,    10,
      11,     0,     0,    12,    13,    14,    15,     0,     0,     0,
       0,     0,     0,     0,    25,    16,    17,    18,    19,   103,
       0,    26,     8,     0,     0,    21,    22,     0,    23,     0,
      24,     9,    10,    11,     0,     0,    12,    13,    14,    15,
       0,     0,     0,     0,     0,     0,     0,     0,    16,    17,
      18,    19,   107,     0,     0,     0,     0,    25,    21,    22,
       0,    23,     0,    24,    26,     0,     0,     0,     0,     0,
       0,     0,    46,     0,    47,    48,    49,     0,     0,    50,
      51,     0,     0,     0,     0,    52,     0,    53,    54,    55,
      25,     0,     0,     0,    56,    57,    58,    26,    59,    60,
      61,     0,    62,    63,    64,    65,    66,    67,     0,    68,
      69,    70,    71,    72,    73,    47,    74,    49,     0,     0,
      50,    51,     0,     0,     0,     0,    52,     0,    53,    54,
      55,     0,     0,     0,     0,     0,    57,    58,     0,     0,
      60,    61,     0,    62,    63,    64,    65,    66,    67,     0,
      68,    69,    70,    71,    72,    73,    47,     0,     0,     0,
       0,    50,    51,     0,     0,     0,     0,    52,     0,    53,
       0,    55,     0,     0,     0,     0,     0,    57,    58,     0,
       0,     0,    61,     0,     0,    63,    64,     0,    66,    67,
       0,     0,     0,    70,    71,    72,    73,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129
};

static const yytype_int16 yycheck[] =
{
       1,    97,     2,    56,    97,     2,     1,   171,   115,   116,
      20,   191,    97,    14,    15,    30,    17,    18,   173,    20,
       3,     3,     3,   130,     3,    60,     0,    91,   157,   193,
     171,    95,    96,    17,    87,    88,    89,    86,    34,   262,
     263,    34,    86,   172,   253,    35,   153,    60,     3,     0,
     179,     3,   193,   263,   263,    60,    56,     0,    63,    56,
      53,     3,    17,   112,    74,     3,   275,    60,   112,    32,
      33,    60,     3,    69,    63,    75,     0,   112,    75,   164,
      24,    25,    60,   293,   293,    63,   250,    87,    88,    89,
      87,    88,    89,    89,   190,   191,   106,   107,    26,   112,
     110,   265,   103,    53,    56,   190,   107,   103,   331,   250,
      60,    17,    54,   177,   115,   116,    54,   110,   133,   134,
     135,   111,   111,    54,   265,   106,   110,   110,    34,   130,
     112,   110,    53,   111,    55,    26,    27,    28,    29,    60,
      60,    53,    61,    63,    34,    64,   326,    53,    30,    31,
     151,   315,   153,    34,    60,    23,   151,    34,   243,    22,
     161,   162,   291,    53,   164,   272,   161,   162,   297,    20,
      60,   326,    53,    34,    55,    21,    53,    60,    55,    60,
      63,    19,   346,    60,    63,   200,    34,    35,    36,    68,
      69,    16,    53,    18,    55,    74,   325,    76,    53,    78,
      55,   297,   136,   137,   297,    84,    85,   314,     3,    53,
      89,    55,   297,    92,    93,    60,    95,    96,    39,    40,
       3,   100,   101,   102,   103,   235,   110,    60,   112,   325,
     326,    54,    53,   112,    55,    26,    57,    58,   111,   110,
     325,   112,   111,   243,   111,   298,   299,   300,     5,   356,
       3,   140,   141,   142,   143,   107,   108,   109,   259,    28,
     138,   139,   262,   263,   259,   262,   263,    54,   144,   145,
     111,   272,   111,    54,    17,    54,    53,     5,    60,   110,
      60,   111,   283,     3,    54,    56,    54,    54,   283,    53,
     110,    28,   110,     4,   111,   110,     5,   110,   298,   299,
     300,   298,   299,   300,   110,   110,   353,   279,   320,    75,
     311,   146,    23,   314,   163,   283,   311,   116,   147,   320,
     148,    32,    33,    34,   150,   320,    37,    38,    39,    40,
     149,   331,   291,   244,   331,     4,   152,   177,    49,    50,
      51,    52,    53,   260,   331,   263,    -1,    -1,    59,    60,
      -1,    62,    63,    64,    65,   356,    -1,    68,    69,    -1,
      -1,    -1,    -1,    74,    -1,    76,    77,    78,    -1,    -1,
      -1,    -1,    -1,    84,    85,    -1,     4,    88,    89,    -1,
      91,    92,    93,    94,    95,    96,    -1,    98,    99,   100,
     101,   102,   103,    -1,    63,    -1,    65,    -1,    -1,    68,
      69,    -1,    -1,    -1,    -1,    74,    -1,    76,    77,    78,
      -1,    -1,    -1,    -1,    -1,    84,    85,    -1,    -1,    88,
      89,    -1,    -1,    92,    93,    94,    95,    96,    -1,    -1,
      99,   100,   101,   102,   103,    63,    -1,    65,    -1,    -1,
      68,    69,    -1,    -1,    -1,    -1,    74,    -1,    76,    77,
      78,    -1,    -1,    -1,    -1,    -1,    84,    85,    -1,    -1,
      88,    89,    -1,    -1,    92,    93,    94,    95,    96,    23,
      -1,    99,   100,   101,   102,   103,    -1,    -1,    32,    33,
      34,    -1,    -1,    37,    38,    39,    40,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      -1,    -1,    -1,    -1,    -1,    59,    60,    -1,    62,    63,
      64,    -1,    -1,    -1,    68,    69,    -1,    -1,    -1,    -1,
      74,    -1,    76,    -1,    78,    -1,    -1,    -1,    -1,    -1,
      84,    85,    -1,    -1,    -1,    89,    -1,    91,    92,    93,
      -1,    95,    96,    23,    98,    -1,   100,   101,   102,   103,
      -1,    -1,    32,    33,    34,    -1,    -1,    37,    38,    39,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    53,    54,    -1,    23,    -1,    -1,    59,
      60,    -1,    62,    -1,    64,    32,    33,    34,    -1,    -1,
      37,    38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    -1,    -1,    -1,
      -1,    91,    59,    60,    -1,    62,    -1,    64,    98,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,    33,
      34,    -1,    -1,    37,    38,    39,    40,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    91,    49,    50,    51,    52,    53,
      -1,    98,    23,    -1,    -1,    59,    60,    -1,    62,    -1,
      64,    32,    33,    34,    -1,    -1,    37,    38,    39,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    -1,    -1,    -1,    -1,    91,    59,    60,
      -1,    62,    -1,    64,    98,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    61,    -1,    63,    64,    65,    -1,    -1,    68,
      69,    -1,    -1,    -1,    -1,    74,    -1,    76,    77,    78,
      91,    -1,    -1,    -1,    83,    84,    85,    98,    87,    88,
      89,    -1,    91,    92,    93,    94,    95,    96,    -1,    98,
      99,   100,   101,   102,   103,    63,   105,    65,    -1,    -1,
      68,    69,    -1,    -1,    -1,    -1,    74,    -1,    76,    77,
      78,    -1,    -1,    -1,    -1,    -1,    84,    85,    -1,    -1,
      88,    89,    -1,    91,    92,    93,    94,    95,    96,    -1,
      98,    99,   100,   101,   102,   103,    63,    -1,    -1,    -1,
      -1,    68,    69,    -1,    -1,    -1,    -1,    74,    -1,    76,
      -1,    78,    -1,    -1,    -1,    -1,    -1,    84,    85,    -1,
      -1,    -1,    89,    -1,    -1,    92,    93,    -1,    95,    96,
      -1,    -1,    -1,   100,   101,   102,   103,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   107,   108,   109,   114,   115,   116,   117,    23,    32,
      33,    34,    37,    38,    39,    40,    49,    50,    51,    52,
      53,    59,    60,    62,    64,    91,    98,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   152,   153,   155,    61,    63,    64,    65,
      68,    69,    74,    76,    77,    78,    83,    84,    85,    87,
      88,    89,    91,    92,    93,    94,    95,    96,    98,    99,
     100,   101,   102,   103,   105,   118,   119,   120,   122,   125,
     128,   129,   130,   131,   156,   157,   159,   160,   161,   162,
     163,   164,   165,   173,   174,   161,   162,   170,   185,     0,
       0,     0,     0,    53,   137,   137,    53,    53,   137,   137,
      26,   155,   185,    39,    40,    53,    55,    57,    58,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
     154,   137,   139,    34,    35,    36,    32,    33,    30,    31,
      26,    27,    28,    29,    24,    25,    23,    22,    21,    20,
      19,    16,    18,     3,    60,    63,   111,   157,    60,   185,
     119,    35,   111,     3,    34,    53,    60,   110,   167,   168,
     177,   178,   179,   180,   157,   157,   157,   166,   169,   170,
      60,    63,   111,    60,   175,   176,   170,   170,    53,    55,
     179,   180,   186,   187,   188,   191,   185,   185,   185,    54,
      54,    54,   134,   153,   134,    60,    63,    60,    63,   153,
     139,   139,   139,   140,   140,   141,   141,   142,   142,   142,
     142,   143,   143,   144,   145,   146,   147,   148,   155,   149,
     153,   111,   111,   177,   111,    26,   155,   132,   133,   155,
     131,   162,   180,   181,     3,   110,    53,   188,   191,   177,
     178,   112,   169,    17,   171,   172,   177,   111,   111,     5,
       3,   112,   192,   189,   186,   187,   188,   191,     3,    54,
      28,   139,     3,    54,    56,    17,   121,    86,   112,   126,
     127,   185,   106,   110,   112,   162,   180,   168,    54,   151,
     152,     3,   110,    17,   155,   176,     4,   158,   159,   161,
     162,   182,   183,   184,   193,   151,   182,   190,    54,    60,
      63,    53,   153,   152,     5,    60,   110,   112,   127,    60,
     111,   133,   172,   151,   177,   179,   180,   186,   157,   157,
     157,     3,    54,    56,    54,    54,   155,   153,   191,   110,
      28,   132,     4,   184,    54,   110,    17,   110,   111,   112,
     191,    60,   112,   123,   124,   110,     5,   110,   112,   124,
     153,   110,   110
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 221 "dt_grammar.y"
    { return (dt_node_root((yyvsp[(1) - (2)].l_node))); }
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 222 "dt_grammar.y"
    { return (dt_node_root((yyvsp[(1) - (2)].l_node))); }
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 223 "dt_grammar.y"
    { return (dt_node_root((yyvsp[(1) - (2)].l_node))); }
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 226 "dt_grammar.y"
    { (yyval.l_node) = NULL; }
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 227 "dt_grammar.y"
    { (yyval.l_node) = (yyvsp[(2) - (2)].l_node); }
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 230 "dt_grammar.y"
    { (yyval.l_node) = dt_node_program(NULL); }
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 231 "dt_grammar.y"
    { (yyval.l_node) = dt_node_program((yyvsp[(2) - (2)].l_node)); }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 234 "dt_grammar.y"
    { (yyval.l_node) = NULL; }
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 235 "dt_grammar.y"
    { (yyval.l_node) = (dt_node_t *)(yyvsp[(2) - (2)].l_decl); }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 240 "dt_grammar.y"
    { (yyval.l_node) = LINK((yyvsp[(1) - (2)].l_node), (yyvsp[(2) - (2)].l_node)); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 253 "dt_grammar.y"
    { dt_scope_push(NULL, CTF_ERR); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 254 "dt_grammar.y"
    {
			/*
			 * We push a new declaration scope before shifting the
			 * assignment_expression in order to preserve ds_class
			 * and ds_ident for use in dt_node_inline().  Once the
			 * entire inline_definition rule is matched, pop the
			 * scope and construct the inline using the saved decl.
			 */
			dt_scope_pop();
			(yyval.l_node) = dt_node_inline((yyvsp[(6) - (7)].l_node));
		}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 269 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_xlator((yyvsp[(2) - (10)].l_decl), (yyvsp[(4) - (10)].l_decl), (yyvsp[(5) - (10)].l_str), (yyvsp[(8) - (10)].l_node));
		}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 273 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_xlator((yyvsp[(2) - (9)].l_decl), (yyvsp[(4) - (9)].l_decl), (yyvsp[(5) - (9)].l_str), NULL);
		}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 280 "dt_grammar.y"
    { (yyval.l_node) = LINK((yyvsp[(1) - (2)].l_node),(yyvsp[(2) - (2)].l_node)); }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 284 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_member(NULL, (yyvsp[(1) - (4)].l_str), (yyvsp[(3) - (4)].l_node));
		}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 290 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_provider((yyvsp[(2) - (6)].l_str), (yyvsp[(4) - (6)].l_node));
		}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 293 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_provider((yyvsp[(2) - (5)].l_str), NULL);
		}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 300 "dt_grammar.y"
    { (yyval.l_node) = LINK((yyvsp[(1) - (2)].l_node), (yyvsp[(2) - (2)].l_node)); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 304 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_probe((yyvsp[(2) - (6)].l_str), 2, (yyvsp[(3) - (6)].l_node), (yyvsp[(5) - (6)].l_node));
		}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 307 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_probe((yyvsp[(2) - (4)].l_str), 1, (yyvsp[(3) - (4)].l_node), NULL);
		}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 314 "dt_grammar.y"
    {
			/*
			 * If the input stream is a file, do not permit a probe
			 * specification without / <pred> / or { <act> } after
			 * it.  This can only occur if the next token is EOF or
			 * an ambiguous predicate was slurped up as a comment.
			 * We cannot perform this check if input() is a string
			 * because dtrace(1M) [-fmnP] also use the compiler and
			 * things like dtrace -n BEGIN have to be accepted.
			 */
			if (yypcb->pcb_fileptr != NULL) {
				dnerror((yyvsp[(1) - (1)].l_node), D_SYNTAX, "expected predicate and/"
				    "or actions following probe description\n");
			}
			(yyval.l_node) = dt_node_clause((yyvsp[(1) - (1)].l_node), NULL, NULL);
		}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 330 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_clause((yyvsp[(1) - (4)].l_node), NULL, (yyvsp[(3) - (4)].l_node));
		}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 333 "dt_grammar.y"
    {
			dnerror((yyvsp[(3) - (4)].l_node), D_SYNTAX, "expected actions { } following "
			    "probe description and predicate\n");
		}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 338 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_clause((yyvsp[(1) - (7)].l_node), (yyvsp[(3) - (7)].l_node), (yyvsp[(6) - (7)].l_node));
		}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 344 "dt_grammar.y"
    { yybegin(YYS_EXPR); (yyval.l_node) = (yyvsp[(1) - (1)].l_node); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 349 "dt_grammar.y"
    {
			(yyval.l_node) = LINK((yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 355 "dt_grammar.y"
    { (yyval.l_node) = dt_node_pdesc_by_name((yyvsp[(1) - (1)].l_str)); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 356 "dt_grammar.y"
    { (yyval.l_node) = dt_node_pdesc_by_id((yyvsp[(1) - (1)].l_int)); }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 359 "dt_grammar.y"
    { (yyval.l_node) = (yyvsp[(1) - (1)].l_node); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 360 "dt_grammar.y"
    { (yyval.l_node) = LINK((yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node)); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 363 "dt_grammar.y"
    { (yyval.l_node) = NULL; }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 364 "dt_grammar.y"
    { (yyval.l_node) = dt_node_statement((yyvsp[(1) - (1)].l_node)); }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 369 "dt_grammar.y"
    {
			(yyval.l_node) = LINK((yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 375 "dt_grammar.y"
    { (yyval.l_node) = dt_node_ident((yyvsp[(1) - (1)].l_str)); }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 376 "dt_grammar.y"
    { (yyval.l_node) = dt_node_ident((yyvsp[(1) - (1)].l_str)); }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 377 "dt_grammar.y"
    { (yyval.l_node) = dt_node_int((yyvsp[(1) - (1)].l_int)); }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 378 "dt_grammar.y"
    { (yyval.l_node) = dt_node_string((yyvsp[(1) - (1)].l_str)); }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 379 "dt_grammar.y"
    { (yyval.l_node) = dt_node_ident(DUP("self")); }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 380 "dt_grammar.y"
    { (yyval.l_node) = dt_node_ident(DUP("this")); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 381 "dt_grammar.y"
    { (yyval.l_node) = (yyvsp[(2) - (3)].l_node); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 387 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_LBRAC, (yyvsp[(1) - (4)].l_node), (yyvsp[(3) - (4)].l_node));
		}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 390 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_func((yyvsp[(1) - (3)].l_node), NULL);
		}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 394 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_func((yyvsp[(1) - (4)].l_node), (yyvsp[(3) - (4)].l_node));
		}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 397 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_DOT, (yyvsp[(1) - (3)].l_node), dt_node_ident((yyvsp[(3) - (3)].l_str)));
		}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 400 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_DOT, (yyvsp[(1) - (3)].l_node), dt_node_ident((yyvsp[(3) - (3)].l_str)));
		}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 403 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_PTR, (yyvsp[(1) - (3)].l_node), dt_node_ident((yyvsp[(3) - (3)].l_str)));
		}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 406 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_PTR, (yyvsp[(1) - (3)].l_node), dt_node_ident((yyvsp[(3) - (3)].l_str)));
		}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 409 "dt_grammar.y"
    {
			(yyval.l_node) = OP1(DT_TOK_POSTINC, (yyvsp[(1) - (2)].l_node));
		}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 412 "dt_grammar.y"
    {
			(yyval.l_node) = OP1(DT_TOK_POSTDEC, (yyvsp[(1) - (2)].l_node));
		}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 416 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_offsetof((yyvsp[(3) - (6)].l_decl), (yyvsp[(5) - (6)].l_str));
		}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 420 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_offsetof((yyvsp[(3) - (6)].l_decl), (yyvsp[(5) - (6)].l_str));
		}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 424 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_XLATE, dt_node_type((yyvsp[(3) - (7)].l_decl)), (yyvsp[(6) - (7)].l_node));
		}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 431 "dt_grammar.y"
    { (yyval.l_node) = OP1(DT_TOK_PREINC, (yyvsp[(2) - (2)].l_node)); }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 432 "dt_grammar.y"
    { (yyval.l_node) = OP1(DT_TOK_PREDEC, (yyvsp[(2) - (2)].l_node)); }
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 433 "dt_grammar.y"
    { (yyval.l_node) = OP1((yyvsp[(1) - (2)].l_tok), (yyvsp[(2) - (2)].l_node)); }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 434 "dt_grammar.y"
    { (yyval.l_node) = OP1(DT_TOK_SIZEOF, (yyvsp[(2) - (2)].l_node)); }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 435 "dt_grammar.y"
    {
			(yyval.l_node) = OP1(DT_TOK_SIZEOF, dt_node_type((yyvsp[(3) - (4)].l_decl)));
		}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 438 "dt_grammar.y"
    {
			(yyval.l_node) = OP1(DT_TOK_STRINGOF, (yyvsp[(2) - (2)].l_node));
		}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 443 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_ADDROF; }
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 444 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_DEREF; }
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 445 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_IPOS; }
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 446 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_INEG; }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 447 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_BNEG; }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 448 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_LNEG; }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 453 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_LPAR, dt_node_type((yyvsp[(2) - (4)].l_decl)), (yyvsp[(4) - (4)].l_node));
		}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 460 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_MUL, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 463 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_DIV, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 466 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_MOD, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 473 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_ADD, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 476 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_SUB, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 483 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_LSH, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 486 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_RSH, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 493 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_LT, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 496 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_GT, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 499 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_LE, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 502 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_GE, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 509 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_EQU, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 512 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_NEQ, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 519 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_BAND, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 526 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_XOR, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 533 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_BOR, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 540 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_LAND, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 547 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_LXOR, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 554 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_LOR, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 565 "dt_grammar.y"
    { (yyval.l_node) = OP3((yyvsp[(1) - (5)].l_node), (yyvsp[(3) - (5)].l_node), (yyvsp[(5) - (5)].l_node)); }
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 570 "dt_grammar.y"
    {
			(yyval.l_node) = OP2((yyvsp[(2) - (3)].l_tok), (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 576 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_ASGN; }
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 577 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_MUL_EQ; }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 578 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_DIV_EQ; }
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 579 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_MOD_EQ; }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 580 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_ADD_EQ; }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 581 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_SUB_EQ; }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 582 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_LSH_EQ; }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 583 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_RSH_EQ; }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 584 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_AND_EQ; }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 585 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_XOR_EQ; }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 586 "dt_grammar.y"
    { (yyval.l_tok) = DT_TOK_OR_EQ; }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 590 "dt_grammar.y"
    {
			(yyval.l_node) = OP2(DT_TOK_COMMA, (yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 595 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_decl();
			dt_decl_free(dt_decl_pop());
			yybegin(YYS_CLAUSE);
		}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 600 "dt_grammar.y"
    {
			(yyval.l_node) = (yyvsp[(2) - (3)].l_node);
			dt_decl_free(dt_decl_pop());
			yybegin(YYS_CLAUSE);
		}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 626 "dt_grammar.y"
    { dt_decl_class(DT_DC_AUTO); }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 627 "dt_grammar.y"
    { dt_decl_class(DT_DC_REGISTER); }
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 628 "dt_grammar.y"
    { dt_decl_class(DT_DC_STATIC); }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 629 "dt_grammar.y"
    { dt_decl_class(DT_DC_EXTERN); }
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 630 "dt_grammar.y"
    { dt_decl_class(DT_DC_TYPEDEF); }
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 635 "dt_grammar.y"
    { dt_decl_class(DT_DC_SELF); }
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 636 "dt_grammar.y"
    { dt_decl_class(DT_DC_THIS); }
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 639 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec(CTF_K_INTEGER, DUP("void")); }
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 640 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec(CTF_K_INTEGER, DUP("char")); }
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 641 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_attr(DT_DA_SHORT); }
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 642 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec(CTF_K_INTEGER, DUP("int")); }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 643 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_attr(DT_DA_LONG); }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 644 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec(CTF_K_FLOAT, DUP("float")); }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 645 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec(CTF_K_FLOAT, DUP("double")); }
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 646 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_attr(DT_DA_SIGNED); }
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 647 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_attr(DT_DA_UNSIGNED); }
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 648 "dt_grammar.y"
    {
			(yyval.l_decl) = dt_decl_spec(CTF_K_TYPEDEF, DUP("string"));
		}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 651 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec(CTF_K_TYPEDEF, (yyvsp[(1) - (1)].l_str)); }
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 656 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_attr(DT_DA_CONST); }
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 657 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_attr(DT_DA_RESTRICT); }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 658 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_attr(DT_DA_VOLATILE); }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 662 "dt_grammar.y"
    {
			(yyval.l_decl) = dt_scope_pop();
		}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 665 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec((yyvsp[(1) - (2)].l_tok), (yyvsp[(2) - (2)].l_str)); }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 666 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec((yyvsp[(1) - (2)].l_tok), (yyvsp[(2) - (2)].l_str)); }
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 670 "dt_grammar.y"
    { dt_decl_sou((yyvsp[(1) - (2)].l_tok), NULL); }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 671 "dt_grammar.y"
    { dt_decl_sou((yyvsp[(1) - (3)].l_tok), (yyvsp[(2) - (3)].l_str)); }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 672 "dt_grammar.y"
    { dt_decl_sou((yyvsp[(1) - (3)].l_tok), (yyvsp[(2) - (3)].l_str)); }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 676 "dt_grammar.y"
    { (yyval.l_tok) = CTF_K_STRUCT; }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 677 "dt_grammar.y"
    { (yyval.l_tok) = CTF_K_UNION; }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 687 "dt_grammar.y"
    {
			(yyval.l_node) = LINK((yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 693 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_decl();
			dt_decl_reset();
		}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 700 "dt_grammar.y"
    {
			dt_decl_free(dt_decl_pop());
		}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 707 "dt_grammar.y"
    { (yyval.l_decl) = (yyvsp[(2) - (2)].l_decl); }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 709 "dt_grammar.y"
    { (yyval.l_decl) = (yyvsp[(2) - (2)].l_decl); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 718 "dt_grammar.y"
    { dt_decl_member(NULL); }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 719 "dt_grammar.y"
    { dt_decl_member((yyvsp[(2) - (2)].l_node)); }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 720 "dt_grammar.y"
    {
			dt_decl_member((yyvsp[(3) - (3)].l_node));
		}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 726 "dt_grammar.y"
    { (yyval.l_decl) = dt_scope_pop(); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 727 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec(CTF_K_ENUM, (yyvsp[(2) - (2)].l_str)); }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 728 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_spec(CTF_K_ENUM, (yyvsp[(2) - (2)].l_str)); }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 732 "dt_grammar.y"
    { dt_decl_enum(NULL); }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 733 "dt_grammar.y"
    { dt_decl_enum((yyvsp[(2) - (3)].l_str)); }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 734 "dt_grammar.y"
    { dt_decl_enum((yyvsp[(2) - (3)].l_str)); }
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 742 "dt_grammar.y"
    { dt_decl_enumerator((yyvsp[(1) - (1)].l_str), NULL); }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 743 "dt_grammar.y"
    {
			dt_decl_enumerator((yyvsp[(1) - (3)].l_str), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 753 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_ident((yyvsp[(1) - (1)].l_str)); }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 754 "dt_grammar.y"
    { (yyval.l_decl) = (yyvsp[(2) - (3)].l_decl); }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 755 "dt_grammar.y"
    { dt_decl_array((yyvsp[(2) - (2)].l_node)); }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 756 "dt_grammar.y"
    { dt_decl_func((yyvsp[(1) - (2)].l_decl), (yyvsp[(2) - (2)].l_node)); }
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 759 "dt_grammar.y"
    { dt_decl_top()->dd_attr |= DT_DA_PAREN; }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 762 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_ptr(); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 763 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_ptr(); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 764 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_ptr(); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 765 "dt_grammar.y"
    { (yyval.l_decl) = dt_decl_ptr(); }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 770 "dt_grammar.y"
    { (yyval.l_decl) = (yyvsp[(2) - (2)].l_decl); }
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 775 "dt_grammar.y"
    { (yyval.l_node) = dt_node_vatype(); }
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 776 "dt_grammar.y"
    {
			(yyval.l_node) = LINK((yyvsp[(1) - (3)].l_node), dt_node_vatype());
		}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 782 "dt_grammar.y"
    {
			(yyval.l_node) = LINK((yyvsp[(1) - (3)].l_node), (yyvsp[(3) - (3)].l_node));
		}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 788 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_type(NULL);
		}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 791 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_type(NULL);
		}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 794 "dt_grammar.y"
    {
			(yyval.l_node) = dt_node_type(NULL);
		}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 799 "dt_grammar.y"
    {
			(yyval.l_decl) = dt_decl_pop();
		}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 802 "dt_grammar.y"
    {
			(yyval.l_decl) = dt_decl_pop();
		}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 814 "dt_grammar.y"
    { (yyval.l_decl) = (yyvsp[(2) - (3)].l_decl); }
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 815 "dt_grammar.y"
    { dt_decl_array((yyvsp[(2) - (2)].l_node)); }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 816 "dt_grammar.y"
    { dt_decl_array((yyvsp[(1) - (1)].l_node)); (yyval.l_decl) = NULL; }
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 817 "dt_grammar.y"
    { dt_decl_func((yyvsp[(1) - (2)].l_decl), (yyvsp[(2) - (2)].l_node)); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 818 "dt_grammar.y"
    { dt_decl_func(NULL, (yyvsp[(1) - (1)].l_node)); }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 821 "dt_grammar.y"
    { dt_scope_push(NULL, CTF_ERR); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 822 "dt_grammar.y"
    {
			dt_scope_pop();
			(yyval.l_node) = (yyvsp[(3) - (4)].l_node);
		}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 829 "dt_grammar.y"
    { (yyval.l_node) = NULL; }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 830 "dt_grammar.y"
    { (yyval.l_node) = (yyvsp[(1) - (1)].l_node); }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 831 "dt_grammar.y"
    { (yyval.l_node) = (yyvsp[(1) - (1)].l_node); }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 834 "dt_grammar.y"
    { dt_scope_push(NULL, CTF_ERR); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 835 "dt_grammar.y"
    {
			dt_scope_pop();
			(yyval.l_node) = (yyvsp[(3) - (4)].l_node);
		}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 842 "dt_grammar.y"
    { (yyval.l_node) = NULL; }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 843 "dt_grammar.y"
    { (yyval.l_node) = (yyvsp[(1) - (1)].l_node); }
    break;



/* Line 1455 of yacc.c  */
#line 3516 "dt_grammar.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 846 "dt_grammar.y"


