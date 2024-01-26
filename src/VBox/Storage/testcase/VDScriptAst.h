/** @file
 * VBox HDD container test utility - scripting engine, AST related structures.
 */

/*
 * Copyright (C) 2013-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef VBOX_INCLUDED_SRC_testcase_VDScriptAst_h
#define VBOX_INCLUDED_SRC_testcase_VDScriptAst_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/list.h>

/**
 * Position information.
 */
typedef struct VDSRCPOS
{
    /** Line in the source. */
    unsigned       iLine;
    /** Current start character .*/
    unsigned       iChStart;
    /** Current end character. */
    unsigned       iChEnd;
} VDSRCPOS;
/** Pointer to a source position. */
typedef struct VDSRCPOS *PVDSRCPOS;

/**
 * AST node classes.
 */
typedef enum VDSCRIPTASTCLASS
{
    /** Invalid. */
    VDSCRIPTASTCLASS_INVALID = 0,
    /** Function node. */
    VDSCRIPTASTCLASS_FUNCTION,
    /** Function argument. */
    VDSCRIPTASTCLASS_FUNCTIONARG,
    /** Identifier node. */
    VDSCRIPTASTCLASS_IDENTIFIER,
    /** Declaration node. */
    VDSCRIPTASTCLASS_DECLARATION,
    /** Statement node. */
    VDSCRIPTASTCLASS_STATEMENT,
    /** Expression node. */
    VDSCRIPTASTCLASS_EXPRESSION,
    /** Type name node. */
    VDSCRIPTASTCLASS_TYPENAME,
    /** Type specifier node. */
    VDSCRIPTASTCLASS_TYPESPECIFIER,
    /** 32bit blowup. */
    VDSCRIPTASTCLASS_32BIT_HACK = 0x7fffffff
} VDSCRIPTASTCLASS;
/** Pointer to an AST node class. */
typedef VDSCRIPTASTCLASS *PVDSCRIPTASTCLASS;

/**
 * Core AST structure.
 */
typedef struct VDSCRIPTASTCORE
{
    /** The node class, used for verification. */
    VDSCRIPTASTCLASS enmClass;
    /** List which might be used. */
    RTLISTNODE       ListNode;
    /** Position in the source file of this node. */
    VDSRCPOS         Pos;
} VDSCRIPTASTCORE;
/** Pointer to an AST core structure. */
typedef VDSCRIPTASTCORE *PVDSCRIPTASTCORE;

/** Pointer to an statement node - forward declaration. */
typedef struct VDSCRIPTASTSTMT *PVDSCRIPTASTSTMT;
/** Pointer to an expression node - forward declaration. */
typedef struct VDSCRIPTASTEXPR *PVDSCRIPTASTEXPR;

/**
 * AST identifier node.
 */
typedef struct VDSCRIPTASTIDE
{
    /** Core structure. */
    VDSCRIPTASTCORE    Core;
    /** Number of characters in the identifier, excluding the zero terminator. */
    unsigned           cchIde;
    /** Identifier, variable size. */
    char               aszIde[1];
} VDSCRIPTASTIDE;
/** Pointer to an identifer node. */
typedef VDSCRIPTASTIDE *PVDSCRIPTASTIDE;

/**
 * Type specifier.
 */
typedef enum VDSCRIPTASTTYPESPECIFIER
{
    /** Invalid type specifier. */
    VDSCRIPTASTTYPESPECIFIER_INVALID = 0,
    /** Union type specifier. */
    VDSCRIPTASTTYPESPECIFIER_UNION,
    /** Struct type specifier. */
    VDSCRIPTASTTYPESPECIFIER_STRUCT,
    /** Identifier of a typedefed type. */
    VDSCRIPTASTTYPESPECIFIER_IDE,
    /** 32bit hack. */
    VDSCRIPTASTTYPESPECIFIER_32BIT_HACK = 0x7fffffff
} VDSCRIPTASTTYPESPECIFIER;
/** Pointer to a typespecifier. */
typedef VDSCRIPTASTTYPESPECIFIER *PVDSCRIPTASTTYPESPECIFIER;

/**
 * AST type specifier.
 */
typedef struct VDSCRIPTASTTYPESPEC
{
    /** Core structure. */
    VDSCRIPTASTCORE          Core;
    /** Specifier type. */
    VDSCRIPTASTTYPESPECIFIER enmType;
    /** Type dependent data .*/
    union
    {
        /** Pointer to an identifier for typedefed types. */
        PVDSCRIPTASTIDE      pIde;
        /** struct or union specifier. */
        struct
        {
            /** Pointer to the identifier, optional. */
            PVDSCRIPTASTIDE  pIde;
            /** Declaration list - VDSCRIPTAST. */
            RTLISTANCHOR     ListDecl;
        } StructUnion;
    };
} VDSCRIPTASTTYPESPEC;
/** Pointer to an AST type specifier. */
typedef VDSCRIPTASTTYPESPEC *PVDSCRIPTASTTYPESPEC;

/**
 * Storage clase specifier.
 */
typedef enum VDSCRIPTASTSTORAGECLASS
{
    /** Invalid storage class sepcifier. */
    VDSCRIPTASTSTORAGECLASS_INVALID = 0,
    /** A typedef type. */
    VDSCRIPTASTSTORAGECLASS_TYPEDEF,
    /** An external declared object. */
    VDSCRIPTASTSTORAGECLASS_EXTERN,
    /** A static declared object. */
    VDSCRIPTASTSTORAGECLASS_STATIC,
    /** Auto object. */
    VDSCRIPTASTSTORAGECLASS_AUTO,
    /** Object should be stored in a register. */
    VDSCRIPTASTSTORAGECLASS_REGISTER,
    /** 32bit hack. */
    VDSCRIPTASTSTORAGECLASS_32BIT_HACK = 0x7fffffff
} VDSCRIPTASTSTORAGECLASS;
/** Pointer to a storage class. */
typedef VDSCRIPTASTSTORAGECLASS *PVDSCRIPTASTSTORAGECLASS;

/**
 * Type qualifier.
 */
typedef enum VDSCRIPTASTTYPEQUALIFIER
{
    /** Invalid type qualifier. */
    VDSCRIPTASTTYPEQUALIFIER_INVALID = 0,
    /** Const type qualifier. */
    VDSCRIPTASTTYPEQUALIFIER_CONST,
    /** Restrict type qualifier. */
    VDSCRIPTASTTYPEQUALIFIER_RESTRICT,
    /** Volatile type qualifier. */
    VDSCRIPTASTTYPEQUALIFIER_VOLATILE,
    /** 32bit hack. */
    VDSCRIPTASTTYPEQUALIFIER_32BIT_HACK = 0x7fffffff
} VDSCRIPTASTTYPEQUALIFIER;
/** Pointer to a type qualifier. */
typedef VDSCRIPTASTTYPEQUALIFIER *PVDSCRIPTASTTYPEQUALIFIER;

/**
 * AST type name node.
 */
typedef struct VDSCRIPTASTTYPENAME
{
    /** Core structure. */
    VDSCRIPTASTCORE    Core;
} VDSCRIPTASTTYPENAME;
/** Pointer to a type name node. */
typedef VDSCRIPTASTTYPENAME *PVDSCRIPTASTTYPENAME;

/**
 * AST declaration node.
 */
typedef struct VDSCRIPTASTDECL
{
    /** Core structure. */
    VDSCRIPTASTCORE    Core;
    /** @todo */
} VDSCRIPTASTDECL;
/** Pointer to an declaration node. */
typedef VDSCRIPTASTDECL *PVDSCRIPTASTDECL;

/**
 * Expression types.
 */
typedef enum VDSCRIPTEXPRTYPE
{
    /** Invalid. */
    VDSCRIPTEXPRTYPE_INVALID = 0,
    /** Numerical constant. */
    VDSCRIPTEXPRTYPE_PRIMARY_NUMCONST,
    /** String constant. */
    VDSCRIPTEXPRTYPE_PRIMARY_STRINGCONST,
    /** Boolean constant. */
    VDSCRIPTEXPRTYPE_PRIMARY_BOOLEAN,
    /** Identifier. */
    VDSCRIPTEXPRTYPE_PRIMARY_IDENTIFIER,
    /** List of assignment expressions as in a = b = c = ... . */
    VDSCRIPTEXPRTYPE_ASSIGNMENT_LIST,
    /** Postfix increment expression. */
    VDSCRIPTEXPRTYPE_POSTFIX_INCREMENT,
    /** Postfix decrement expression. */
    VDSCRIPTEXPRTYPE_POSTFIX_DECREMENT,
    /** Postfix function call expression. */
    VDSCRIPTEXPRTYPE_POSTFIX_FNCALL,
    /** Postfix dereference expression. */
    VDSCRIPTEXPRTYPE_POSTFIX_DEREFERENCE,
    /** Dot operator (@todo: Is there a better name for it?). */
    VDSCRIPTEXPRTYPE_POSTFIX_DOT,
    /** Unary increment expression. */
    VDSCRIPTEXPRTYPE_UNARY_INCREMENT,
    /** Unary decrement expression. */
    VDSCRIPTEXPRTYPE_UNARY_DECREMENT,
    /** Unary positive sign expression. */
    VDSCRIPTEXPRTYPE_UNARY_POSSIGN,
    /** Unary negtive sign expression. */
    VDSCRIPTEXPRTYPE_UNARY_NEGSIGN,
    /** Unary invert expression. */
    VDSCRIPTEXPRTYPE_UNARY_INVERT,
    /** Unary negate expression. */
    VDSCRIPTEXPRTYPE_UNARY_NEGATE,
    /** Unary reference expression. */
    VDSCRIPTEXPRTYPE_UNARY_REFERENCE,
    /** Unary dereference expression. */
    VDSCRIPTEXPRTYPE_UNARY_DEREFERENCE,
    /** Cast expression. */
    VDSCRIPTEXPRTYPE_CAST,
    /** Multiplicative expression. */
    VDSCRIPTEXPRTYPE_MULTIPLICATION,
    /** Division expression. */
    VDSCRIPTEXPRTYPE_DIVISION,
    /** Modulus expression. */
    VDSCRIPTEXPRTYPE_MODULUS,
    /** Addition expression. */
    VDSCRIPTEXPRTYPE_ADDITION,
    /** Subtraction expression. */
    VDSCRIPTEXPRTYPE_SUBTRACTION,
    /** Logical shift right. */
    VDSCRIPTEXPRTYPE_LSR,
    /** Logical shift left. */
    VDSCRIPTEXPRTYPE_LSL,
    /** Lower than expression */
    VDSCRIPTEXPRTYPE_LOWER,
    /** Higher than expression */
    VDSCRIPTEXPRTYPE_HIGHER,
    /** Lower or equal than expression */
    VDSCRIPTEXPRTYPE_LOWEREQUAL,
    /** Higher or equal than expression */
    VDSCRIPTEXPRTYPE_HIGHEREQUAL,
    /** Equals expression */
    VDSCRIPTEXPRTYPE_EQUAL,
    /** Not equal expression */
    VDSCRIPTEXPRTYPE_NOTEQUAL,
    /** Bitwise and expression */
    VDSCRIPTEXPRTYPE_BITWISE_AND,
    /** Bitwise xor expression */
    VDSCRIPTEXPRTYPE_BITWISE_XOR,
    /** Bitwise or expression */
    VDSCRIPTEXPRTYPE_BITWISE_OR,
    /** Logical and expression */
    VDSCRIPTEXPRTYPE_LOGICAL_AND,
    /** Logical or expression */
    VDSCRIPTEXPRTYPE_LOGICAL_OR,
    /** Assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN,
    /** Multiplicative assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_MULT,
    /** Division assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_DIV,
    /** Modulus assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_MOD,
    /** Additive assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_ADD,
    /** Subtractive assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_SUB,
    /** Bitwise left shift assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_LSL,
    /** Bitwise right shift assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_LSR,
    /** Bitwise and assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_AND,
    /** Bitwise xor assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_XOR,
    /** Bitwise or assign expression */
    VDSCRIPTEXPRTYPE_ASSIGN_OR,
    /** 32bit hack. */
    VDSCRIPTEXPRTYPE_32BIT_HACK = 0x7fffffff
} VDSCRIPTEXPRTYPE;
/** Pointer to an expression type. */
typedef VDSCRIPTEXPRTYPE *PVDSCRIPTEXPRTYPE;

/**
 * AST expression node.
 */
typedef struct VDSCRIPTASTEXPR
{
    /** Core structure. */
    VDSCRIPTASTCORE    Core;
    /** Expression type. */
    VDSCRIPTEXPRTYPE   enmType;
    /** Expression type dependent data. */
    union
    {
        /** Numerical constant. */
        uint64_t          u64;
        /** Primary identifier. */
        PVDSCRIPTASTIDE   pIde;
        /** String literal */
        const char       *pszStr;
        /** Boolean constant. */
        bool              f;
        /** List of expressions - VDSCRIPTASTEXPR. */
        RTLISTANCHOR      ListExpr;
        /** Pointer to another expression. */
        PVDSCRIPTASTEXPR  pExpr;
        /** Function call expression. */
        struct
        {
            /** Other postfix expression used as the identifier for the function. */
            PVDSCRIPTASTEXPR pFnIde;
            /** Argument list if existing. */
            RTLISTANCHOR     ListArgs;
        } FnCall;
        /** Binary operation. */
        struct
        {
            /** Left operator. */
            PVDSCRIPTASTEXPR pLeftExpr;
            /** Right operator. */
            PVDSCRIPTASTEXPR pRightExpr;
        } BinaryOp;
        /** Dereference or dot operation. */
        struct
        {
            /** The identifier to access. */
            PVDSCRIPTASTIDE  pIde;
            /** Postfix expression coming after this. */
            PVDSCRIPTASTEXPR pExpr;
        } Deref;
        /** Cast expression. */
        struct
        {
            /** Type name. */
            PVDSCRIPTASTTYPENAME pTypeName;
            /** Following cast expression. */
            PVDSCRIPTASTEXPR     pExpr;
        } Cast;
    };
} VDSCRIPTASTEXPR;

/**
 * AST if node.
 */
typedef struct VDSCRIPTASTIF
{
    /** Conditional expression. */
    PVDSCRIPTASTEXPR   pCond;
    /** The true branch */
    PVDSCRIPTASTSTMT   pTrueStmt;
    /** The else branch, can be NULL if no else branch. */
    PVDSCRIPTASTSTMT   pElseStmt;
} VDSCRIPTASTIF;
/** Pointer to an expression node. */
typedef VDSCRIPTASTIF *PVDSCRIPTASTIF;

/**
 * AST switch node.
 */
typedef struct VDSCRIPTASTSWITCH
{
    /** Conditional expression. */
    PVDSCRIPTASTEXPR   pCond;
    /** The statement to follow. */
    PVDSCRIPTASTSTMT   pStmt;
} VDSCRIPTASTSWITCH;
/** Pointer to an expression node. */
typedef VDSCRIPTASTSWITCH *PVDSCRIPTASTSWITCH;

/**
 * AST while or do ... while node.
 */
typedef struct VDSCRIPTASTWHILE
{
    /** Flag whether this is a do while loop. */
    bool               fDoWhile;
    /** Conditional expression. */
    PVDSCRIPTASTEXPR   pCond;
    /** The statement to follow. */
    PVDSCRIPTASTSTMT   pStmt;
} VDSCRIPTASTWHILE;
/** Pointer to an expression node. */
typedef VDSCRIPTASTWHILE *PVDSCRIPTASTWHILE;

/**
 * AST for node.
 */
typedef struct VDSCRIPTASTFOR
{
    /** Initializer expression. */
    PVDSCRIPTASTEXPR   pExprStart;
    /** The exit condition. */
    PVDSCRIPTASTEXPR   pExprCond;
    /** The third expression (normally used to increase/decrease loop variable). */
    PVDSCRIPTASTEXPR   pExpr3;
    /** The for loop body. */
    PVDSCRIPTASTSTMT   pStmt;
} VDSCRIPTASTFOR;
/** Pointer to an expression node. */
typedef VDSCRIPTASTFOR *PVDSCRIPTASTFOR;

/**
 * Statement types.
 */
typedef enum VDSCRIPTSTMTTYPE
{
    /** Invalid. */
    VDSCRIPTSTMTTYPE_INVALID = 0,
    /** Compound statement. */
    VDSCRIPTSTMTTYPE_COMPOUND,
    /** Expression statement. */
    VDSCRIPTSTMTTYPE_EXPRESSION,
    /** if statement. */
    VDSCRIPTSTMTTYPE_IF,
    /** switch statement. */
    VDSCRIPTSTMTTYPE_SWITCH,
    /** while statement. */
    VDSCRIPTSTMTTYPE_WHILE,
    /** for statement. */
    VDSCRIPTSTMTTYPE_FOR,
    /** continue statement. */
    VDSCRIPTSTMTTYPE_CONTINUE,
    /** break statement. */
    VDSCRIPTSTMTTYPE_BREAK,
    /** return statement. */
    VDSCRIPTSTMTTYPE_RETURN,
    /** case statement. */
    VDSCRIPTSTMTTYPE_CASE,
    /** default statement. */
    VDSCRIPTSTMTTYPE_DEFAULT,
    /** 32bit hack. */
    VDSCRIPTSTMTTYPE_32BIT_HACK = 0x7fffffff
} VDSCRIPTSTMTTYPE;
/** Pointer to a statement type. */
typedef VDSCRIPTSTMTTYPE *PVDSCRIPTSTMTTYPE;

/**
 * AST statement node.
 */
typedef struct VDSCRIPTASTSTMT
{
    /** Core structure. */
    VDSCRIPTASTCORE    Core;
    /** Statement type */
    VDSCRIPTSTMTTYPE   enmStmtType;
    /** Statement type dependent data. */
    union
    {
        /** Compound statement. */
        struct
        {
            /** List of declarations - VDSCRIPTASTDECL. */
            RTLISTANCHOR       ListDecls;
            /** List of statements - VDSCRIPTASTSTMT. */
            RTLISTANCHOR       ListStmts;
        } Compound;
        /** case, default statement. */
        struct
        {
            /** Pointer to the expression. */
            PVDSCRIPTASTEXPR   pExpr;
            /** Pointer to the statement. */
            PVDSCRIPTASTSTMT   pStmt;
        } Case;
        /** "if" statement. */
        VDSCRIPTASTIF          If;
        /** "switch" statement. */
        VDSCRIPTASTSWITCH      Switch;
        /** "while" or "do ... while" loop. */
        VDSCRIPTASTWHILE       While;
        /** "for" loop. */
        VDSCRIPTASTFOR         For;
        /** Pointer to another statement. */
        PVDSCRIPTASTSTMT       pStmt;
        /** Expression statement. */
        PVDSCRIPTASTEXPR       pExpr;
    };
} VDSCRIPTASTSTMT;

/**
 * AST node for one function argument.
 */
typedef struct VDSCRIPTASTFNARG
{
    /** Core structure. */
    VDSCRIPTASTCORE    Core;
    /** Identifier describing the type of the argument. */
    PVDSCRIPTASTIDE    pType;
    /** The name of the argument. */
    PVDSCRIPTASTIDE    pArgIde;
} VDSCRIPTASTFNARG;
/** Pointer to a AST function argument node. */
typedef VDSCRIPTASTFNARG *PVDSCRIPTASTFNARG;

/**
 * AST node describing a function.
 */
typedef struct VDSCRIPTASTFN
{
    /** Core structure. */
    VDSCRIPTASTCORE      Core;
    /** Identifier describing the return type. */
    PVDSCRIPTASTIDE      pRetType;
    /** Name of the function. */
    PVDSCRIPTASTIDE      pFnIde;
    /** Number of arguments in the list. */
    unsigned             cArgs;
    /** Argument list - VDSCRIPTASTFNARG. */
    RTLISTANCHOR         ListArgs;
    /** Compound statement node. */
    PVDSCRIPTASTSTMT     pCompoundStmts;
} VDSCRIPTASTFN;
/** Pointer to a function AST node. */
typedef VDSCRIPTASTFN *PVDSCRIPTASTFN;

/**
 * Free the given AST node and all subsequent nodes pointed to
 * by the given node.
 *
 * @param   pAstNode    The AST node to free.
 */
DECLHIDDEN(void) vdScriptAstNodeFree(PVDSCRIPTASTCORE pAstNode);

/**
 * Allocate a non variable in size AST node of the given class.
 *
 * @returns Pointer to the allocated node.
 *          NULL if out of memory.
 * @param   enmClass    The class of the AST node.
 */
DECLHIDDEN(PVDSCRIPTASTCORE) vdScriptAstNodeAlloc(VDSCRIPTASTCLASS enmClass);

/**
 * Allocate a IDE node which can hold the given number of characters.
 *
 * @returns Pointer to the allocated node.
 *          NULL if out of memory.
 * @param   cchIde      Number of characters which can be stored in the node.
 */
DECLHIDDEN(PVDSCRIPTASTIDE) vdScriptAstNodeIdeAlloc(size_t cchIde);

#endif /* !VBOX_INCLUDED_SRC_testcase_VDScriptAst_h */

