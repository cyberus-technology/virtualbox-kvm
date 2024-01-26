
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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

/* Line 1676 of yacc.c  */
#line 51 "dt_grammar.y"

	dt_node_t *l_node;
	dt_decl_t *l_decl;
	char *l_str;
	uintmax_t l_int;
	int l_tok;



/* Line 1676 of yacc.c  */
#line 282 "dt_grammar.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


