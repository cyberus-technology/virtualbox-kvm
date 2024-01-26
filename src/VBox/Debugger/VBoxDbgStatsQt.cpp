/* $Id: VBoxDbgStatsQt.cpp $ */
/** @file
 * VBox Debugger GUI - Statistics.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DBGG
#include "VBoxDbgStatsQt.h"

#include <QLocale>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QClipboard>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeySequence>
#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>

#include <iprt/errcore.h>
#include <VBox/log.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/assert.h>

#include "VBoxDbgGui.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The number of column. */
#define DBGGUI_STATS_COLUMNS    9


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * The state of a statistics sample node.
 *
 * This is used for two pass refresh (1. get data, 2. update the view) and
 * for saving the result of a diff.
 */
typedef enum DBGGUISTATSNODESTATE
{
    /** The typical invalid zeroth entry. */
    kDbgGuiStatsNodeState_kInvalid = 0,
    /** The node is the root node. */
    kDbgGuiStatsNodeState_kRoot,
    /** The node is visible. */
    kDbgGuiStatsNodeState_kVisible,
    /** The node should be refreshed. */
    kDbgGuiStatsNodeState_kRefresh,
    /** diff: The node equals. */
    kDbgGuiStatsNodeState_kDiffEqual,
    /** diff: The node in set 1 is less than the one in set 2. */
    kDbgGuiStatsNodeState_kDiffSmaller,
    /** diff: The node in set 1 is greater than the one in set 2. */
    kDbgGuiStatsNodeState_kDiffGreater,
    /** diff: The node is only in set 1. */
    kDbgGuiStatsNodeState_kDiffOnlyIn1,
    /** diff: The node is only in set 2. */
    kDbgGuiStatsNodeState_kDiffOnlyIn2,
    /** The end of the valid state values. */
    kDbgGuiStatsNodeState_kEnd
} DBGGUISTATENODESTATE;


/**
 * A tree node representing a statistic sample.
 *
 * The nodes carry a reference to the parent and to its position among its
 * siblings. Both of these need updating when the grand parent or parent adds a
 * new child. This will hopefully not be too expensive but rather pay off when
 * we need to create a parent index.
 */
typedef struct DBGGUISTATSNODE
{
    /** Pointer to the parent. */
    PDBGGUISTATSNODE        pParent;
    /** Array of pointers to the child nodes. */
    PDBGGUISTATSNODE       *papChildren;
    /** The number of children. */
    uint32_t                cChildren;
    /** Our index among the parent's children. */
    uint32_t                iSelf;
    /** The unit string. (not allocated) */
    const char             *pszUnit;
    /** The data type.
     * For filler nodes not containing data, this will be set to STAMTYPE_INVALID. */
    STAMTYPE                enmType;
    /** The data at last update. */
    union
    {
        /** STAMTYPE_COUNTER. */
        STAMCOUNTER         Counter;
        /** STAMTYPE_PROFILE. */
        STAMPROFILE         Profile;
        /** STAMTYPE_PROFILE_ADV. */
        STAMPROFILEADV      ProfileAdv;
        /** STAMTYPE_RATIO_U32. */
        STAMRATIOU32        RatioU32;
        /** STAMTYPE_U8 & STAMTYPE_U8_RESET. */
        uint8_t             u8;
        /** STAMTYPE_U16 & STAMTYPE_U16_RESET. */
        uint16_t            u16;
        /** STAMTYPE_U32 & STAMTYPE_U32_RESET. */
        uint32_t            u32;
        /** STAMTYPE_U64 & STAMTYPE_U64_RESET. */
        uint64_t            u64;
        /** STAMTYPE_BOOL and STAMTYPE_BOOL_RESET. */
        bool                f;
        /** STAMTYPE_CALLBACK. */
        QString            *pStr;
    } Data;
    /** The delta. */
    int64_t                 i64Delta;
    /** The name. */
    char                   *pszName;
    /** The length of the name. */
    size_t                  cchName;
    /** The description string. */
    QString                *pDescStr;
    /** The node state. */
    DBGGUISTATENODESTATE    enmState;
} DBGGUISTATSNODE;


/**
 * Recursion stack.
 */
typedef struct DBGGUISTATSSTACK
{
    /** The top stack entry. */
    int32_t iTop;
    /** The stack array. */
    struct DBGGUISTATSSTACKENTRY
    {
        /** The node. */
        PDBGGUISTATSNODE    pNode;
        /** The current child. */
        int32_t             iChild;
        /** Name string offset (if used). */
        uint16_t            cchName;
    } a[32];
} DBGGUISTATSSTACK;




/**
 * The item model for the statistics tree view.
 *
 * This manages the DBGGUISTATSNODE trees.
 */
class VBoxDbgStatsModel : public QAbstractItemModel
{
protected:
    /** The root of the sample tree. */
    PDBGGUISTATSNODE m_pRoot;

private:
    /** Next update child. This is UINT32_MAX when invalid. */
    uint32_t m_iUpdateChild;
    /** Pointer to the node m_szUpdateParent represent and m_iUpdateChild refers to. */
    PDBGGUISTATSNODE m_pUpdateParent;
    /** The length of the path. */
    size_t m_cchUpdateParent;
    /** The path to the current update parent, including a trailing slash. */
    char m_szUpdateParent[1024];
    /** Inserted or/and removed nodes during the update. */
    bool m_fUpdateInsertRemove;


public:
    /**
     * Constructor.
     *
     * @param   a_pParent       The parent object. See QAbstractItemModel in the Qt
     *                          docs for details.
     */
    VBoxDbgStatsModel(QObject *a_pParent);

    /**
     * Destructor.
     *
     * This will free all the data the model holds.
     */
    virtual ~VBoxDbgStatsModel();

    /**
     * Updates the data matching the specified pattern.
     *
     * This will should invoke updatePrep, updateCallback and updateDone.
     *
     * It is vitally important that updateCallback is fed the data in the right
     * order. The code make very definite ASSUMPTIONS about the ordering being
     * strictly sorted and taking the slash into account when doing so.
     *
     * @returns true if we reset the model and it's necessary to set the root index.
     * @param   a_rPatStr       The selection pattern.
     *
     * @remarks The default implementation is an empty stub.
     */
    virtual bool updateStatsByPattern(const QString &a_rPatStr);

    /**
     * Similar to updateStatsByPattern, except that it only works on a sub-tree and
     * will not remove anything that's outside that tree.
     *
     * @param   a_rIndex    The sub-tree root. Invalid index means root.
     *
     * @todo    Create a default implementation using updateStatsByPattern.
     */
    virtual void updateStatsByIndex(QModelIndex const &a_rIndex);

    /**
     * Reset the stats matching the specified pattern.
     *
     * @param   a_rPatStr   The selection pattern.
     *
     * @remarks The default implementation is an empty stub.
     */
    virtual void resetStatsByPattern(QString const &a_rPatStr);

    /**
     * Reset the stats of a sub-tree.
     *
     * @param   a_rIndex    The sub-tree root. Invalid index means root.
     * @param   a_fSubTree  Whether to reset the sub-tree as well. Default is true.
     *
     * @remarks The default implementation makes use of resetStatsByPattern
     */
    virtual void resetStatsByIndex(QModelIndex const &a_rIndex, bool a_fSubTree = true);

    /**
     * Iterator callback function.
     * @returns true to continue, false to stop.
     */
    typedef bool FNITERATOR(PDBGGUISTATSNODE pNode, QModelIndex const &a_rIndex, const char *pszFullName, void *pvUser);

    /**
     * Callback iterator.
     *
     * @param   a_rPatStr           The selection pattern.
     * @param   a_pfnCallback       Callback function.
     * @param   a_pvUser            Callback argument.
     * @param   a_fMatchChildren    How to handle children of matching nodes:
     *                                - @c true: continue with the children,
     *                                - @c false: skip children.
     */
    virtual void iterateStatsByPattern(QString const &a_rPatStr, FNITERATOR *a_pfnCallback, void *a_pvUser,
                                       bool a_fMatchChildren = true);

    /**
     * Gets the model index of the root node.
     *
     * @returns root index.
     */
    QModelIndex getRootIndex(void) const;


protected:
    /**
     * Set the root node.
     *
     * This will free all the current data before taking the ownership of the new
     * root node and its children.
     *
     * @param   a_pRoot         The new root node.
     */
    void setRootNode(PDBGGUISTATSNODE a_pRoot);

    /** Creates the root node. */
    static PDBGGUISTATSNODE createRootNode(void);

    /** Creates and insert a node under the given parent. */
    static PDBGGUISTATSNODE createAndInsertNode(PDBGGUISTATSNODE pParent, const char *pszName, size_t cchName, uint32_t iPosition);

    /** Creates and insert a node under the given parent with correct Qt
     * signalling. */
    PDBGGUISTATSNODE createAndInsert(PDBGGUISTATSNODE pParent, const char *pszName, size_t cchName, uint32_t iPosition);

    /**
     * Resets the node to a pristine state.
     *
     * @param   pNode       The node.
     */
    static void resetNode(PDBGGUISTATSNODE pNode);

    /**
     * Initializes a pristine node.
     */
    static int initNode(PDBGGUISTATSNODE pNode, STAMTYPE enmType, void *pvSample, const char *pszUnit, const char *pszDesc);

    /**
     * Updates (or reinitializes if you like) a node.
     */
    static void updateNode(PDBGGUISTATSNODE pNode, STAMTYPE enmType, void *pvSample, const char *pszUnit, const char *pszDesc);

    /**
     * Called by updateStatsByPattern(), makes the necessary preparations.
     *
     * @returns Success indicator.
     */
    bool updatePrepare(void);

    /**
     * Called by updateStatsByPattern(), finalizes the update.
     *
     * @return See updateStatsByPattern().
     *
     * @param  a_fSuccess   Whether the update was successful or not.
     *
     */
    bool updateDone(bool a_fSuccess);

    /**
     * updateCallback() worker taking care of in-tree inserts and removals.
     *
     * @returns The current node.
     * @param   pszName     The name of the tree element to update.
     */
    PDBGGUISTATSNODE updateCallbackHandleOutOfOrder(const char *pszName);

    /**
     * updateCallback() worker taking care of tail insertions.
     *
     * @returns The current node.
     * @param   pszName     The name of the tree element to update.
     */
    PDBGGUISTATSNODE updateCallbackHandleTail(const char *pszName);

    /**
     * updateCallback() worker that advances the update state to the next data node
     * in anticipation of the next updateCallback call.
     *
     * @param   pNode       The current node.
     */
    void updateCallbackAdvance(PDBGGUISTATSNODE pNode);

    /** Callback used by updateStatsByPattern() and updateStatsByIndex() to feed
     *  changes.
     * @copydoc FNSTAMR3ENUM */
    static DECLCALLBACK(int) updateCallback(const char *pszName, STAMTYPE enmType, void *pvSample, STAMUNIT enmUnit,
                                            const char *pszUnit, STAMVISIBILITY enmVisibility, const char *pszDesc, void *pvUser);

    /**
     * Calculates the full path of a node.
     *
     * @returns Number of bytes returned, negative value on buffer overflow
     *
     * @param   pNode       The node.
     * @param   psz         The output buffer.
     * @param   cch         The size of the buffer.
     */
    static ssize_t getNodePath(PCDBGGUISTATSNODE pNode, char *psz, ssize_t cch);

    /**
     * Calculates the full path of a node, returning the string pointer.
     *
     * @returns @a psz. On failure, NULL.
     *
     * @param   pNode       The node.
     * @param   psz         The output buffer.
     * @param   cch         The size of the buffer.
     */
    static char *getNodePath2(PCDBGGUISTATSNODE pNode, char *psz, ssize_t cch);

    /**
     * Check if the first node is an ancestor to the second one.
     *
     * @returns true/false.
     * @param   pAncestor   The first node, the alleged ancestor.
     * @param   pDescendant The second node, the alleged descendant.
     */
    static bool isNodeAncestorOf(PCDBGGUISTATSNODE pAncestor, PCDBGGUISTATSNODE pDescendant);

    /**
     * Advance to the next node in the tree.
     *
     * @returns Pointer to the next node, NULL if we've reached the end or
     *          was handed a NULL node.
     * @param   pNode       The current node.
     */
    static PDBGGUISTATSNODE nextNode(PDBGGUISTATSNODE pNode);

    /**
     * Advance to the next node in the tree that contains data.
     *
     * @returns Pointer to the next data node, NULL if we've reached the end or
     *          was handed a NULL node.
     * @param   pNode       The current node.
     */
    static PDBGGUISTATSNODE nextDataNode(PDBGGUISTATSNODE pNode);

    /**
     * Advance to the previous node in the tree.
     *
     * @returns Pointer to the previous node, NULL if we've reached the end or
     *          was handed a NULL node.
     * @param   pNode       The current node.
     */
    static PDBGGUISTATSNODE prevNode(PDBGGUISTATSNODE pNode);

    /**
     * Advance to the previous node in the tree that contains data.
     *
     * @returns Pointer to the previous data node, NULL if we've reached the end or
     *          was handed a NULL node.
     * @param   pNode       The current node.
     */
    static PDBGGUISTATSNODE prevDataNode(PDBGGUISTATSNODE pNode);

     /**
     * Removes a node from the tree.
     *
     * @returns pNode.
     * @param   pNode       The node.
     */
    static PDBGGUISTATSNODE removeNode(PDBGGUISTATSNODE pNode);

    /**
     * Removes a node from the tree and destroys it and all its descendants.
     *
     * @param   pNode       The node.
     */
    static void removeAndDestroyNode(PDBGGUISTATSNODE pNode);

    /** Removes a node from the tree and destroys it and all its descendants
     * performing the required Qt signalling. */
    void removeAndDestroy(PDBGGUISTATSNODE pNode);

    /**
     * Destroys a statistics tree.
     *
     * @param   a_pRoot     The root of the tree. NULL is fine.
     */
    static void destroyTree(PDBGGUISTATSNODE a_pRoot);

    /**
     * Stringifies exactly one node, no children.
     *
     * This is for logging and clipboard.
     *
     * @param   a_pNode     The node.
     * @param   a_rString   The string to append the stringified node to.
     */
    static void stringifyNodeNoRecursion(PDBGGUISTATSNODE a_pNode, QString &a_rString);

    /**
     * Stringifies a node and its children.
     *
     * This is for logging and clipboard.
     *
     * @param   a_pNode     The node.
     * @param   a_rString   The string to append the stringified node to.
     */
    static void stringifyNode(PDBGGUISTATSNODE a_pNode, QString &a_rString);

public:
    /**
     * Converts the specified tree to string.
     *
     * This is for logging and clipboard.
     *
     * @param   a_rRoot         Where to start. Use QModelIndex() to start at the root.
     * @param   a_rString       Where to return to return the string dump.
     */
    void stringifyTree(QModelIndex &a_rRoot, QString &a_rString) const;

    /**
     * Dumps the given (sub-)tree as XML.
     *
     * @param   a_rRoot         Where to start. Use QModelIndex() to start at the root.
     * @param   a_rString       Where to return to return the XML dump.
     */
    void xmlifyTree(QModelIndex &a_rRoot, QString &a_rString) const;

    /**
     * Puts the stringified tree on the clipboard.
     *
     * @param   a_rRoot         Where to start. Use QModelIndex() to start at the root.
     */
    void copyTreeToClipboard(QModelIndex &a_rRoot) const;


protected:
    /** Worker for logTree. */
    static void logNode(PDBGGUISTATSNODE a_pNode, bool a_fReleaseLog);

public:
    /** Logs a (sub-)tree.
     *
     * @param   a_rRoot         Where to start. Use QModelIndex() to start at the root.
     * @param   a_fReleaseLog   Whether to use the release log (true) or the debug log (false).
     */
    void logTree(QModelIndex &a_rRoot, bool a_fReleaseLog) const;

protected:
    /** Gets the unit. */
    static QString strUnit(PCDBGGUISTATSNODE pNode);
    /** Gets the value/times. */
    static QString strValueTimes(PCDBGGUISTATSNODE pNode);
    /** Gets the minimum value. */
    static QString strMinValue(PCDBGGUISTATSNODE pNode);
    /** Gets the average value. */
    static QString strAvgValue(PCDBGGUISTATSNODE pNode);
    /** Gets the maximum value. */
    static QString strMaxValue(PCDBGGUISTATSNODE pNode);
    /** Gets the total value. */
    static QString strTotalValue(PCDBGGUISTATSNODE pNode);
    /** Gets the delta value. */
    static QString strDeltaValue(PCDBGGUISTATSNODE pNode);

    /**
     * Destroys a node and all its children.
     *
     * @param   a_pNode         The node to destroy.
     */
    static void destroyNode(PDBGGUISTATSNODE a_pNode);

    /**
     * Converts an index to a node pointer.
     *
     * @returns Pointer to the node, NULL if invalid reference.
     * @param   a_rIndex        Reference to the index
     */
    inline PDBGGUISTATSNODE nodeFromIndex(const QModelIndex &a_rIndex) const
    {
        if (RT_LIKELY(a_rIndex.isValid()))
            return (PDBGGUISTATSNODE)a_rIndex.internalPointer();
        return NULL;
    }

public:

    /** @name Overridden QAbstractItemModel methods
     * @{ */
    virtual int columnCount(const QModelIndex &a_rParent) const;
    virtual QVariant data(const QModelIndex &a_rIndex, int a_eRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &a_rIndex) const;
    virtual bool hasChildren(const QModelIndex &a_rParent) const;
    virtual QVariant headerData(int a_iSection, Qt::Orientation a_ePrientation, int a_eRole) const;
    virtual QModelIndex index(int a_iRow, int a_iColumn, const QModelIndex &a_rParent) const;
    virtual QModelIndex parent(const QModelIndex &a_rChild) const;
    virtual int rowCount(const QModelIndex &a_rParent) const;
    ///virtual void sort(int a_iColumn, Qt::SortOrder a_eOrder);
    /** @}  */
};


/**
 * Model using the VM / STAM interface as data source.
 */
class VBoxDbgStatsModelVM : public VBoxDbgStatsModel, public VBoxDbgBase
{
public:
    /**
     * Constructor.
     *
     * @param   a_pDbgGui       Pointer to the debugger gui object.
     * @param   a_rPatStr       The selection pattern.
     * @param   a_pParent       The parent object. NULL is fine.
     * @param   a_pVMM          The VMM function table.
     */
    VBoxDbgStatsModelVM(VBoxDbgGui *a_pDbgGui, QString &a_rPatStr, QObject *a_pParent, PCVMMR3VTABLE a_pVMM);

    /** Destructor */
    virtual ~VBoxDbgStatsModelVM();

    virtual bool updateStatsByPattern(const QString &a_rPatStr);
    virtual void resetStatsByPattern(const QString &a_rPatStr);

protected:
    /**
     * Enumeration callback used by createNewTree.
     */
    static DECLCALLBACK(int) createNewTreeCallback(const char *pszName, STAMTYPE enmType, void *pvSample, STAMUNIT enmUnit,
                                                   const char *pszUnit, STAMVISIBILITY enmVisibility, const char *pszDesc,
                                                   void *pvUser);

    /**
     * Constructs a new statistics tree by query data from the VM.
     *
     * @returns Pointer to the root of the tree we've constructed. This will be NULL
     *          if the STAM API throws an error or we run out of memory.
     * @param   a_rPatStr       The selection pattern.
     */
    PDBGGUISTATSNODE createNewTree(QString &a_rPatStr);

    /** The VMM function table. */
    PCVMMR3VTABLE m_pVMM;
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/


/**
 * Formats a number into a 64-byte buffer.
 */
static char *formatNumber(char *psz, uint64_t u64)
{
    static const char s_szDigits[] = "0123456789";
    psz += 63;
    *psz-- = '\0';
    unsigned cDigits = 0;
    for (;;)
    {
        const unsigned iDigit = u64 % 10;
        u64 /= 10;
        *psz = s_szDigits[iDigit];
        if (!u64)
            break;
        psz--;
        if (!(++cDigits % 3))
            *psz-- = ',';
    }
    return psz;
}


/**
 * Formats a number into a 64-byte buffer.
 * (18 446 744 073 709 551 615)
 */
static char *formatNumberSigned(char *psz, int64_t i64)
{
    static const char s_szDigits[] = "0123456789";
    psz += 63;
    *psz-- = '\0';
    const bool fNegative = i64 < 0;
    uint64_t u64 = fNegative ? -i64 : i64;
    unsigned cDigits = 0;
    for (;;)
    {
        const unsigned iDigit = u64 % 10;
        u64 /= 10;
        *psz = s_szDigits[iDigit];
        if (!u64)
            break;
        psz--;
        if (!(++cDigits % 3))
            *psz-- = ',';
    }
    if (fNegative)
        *--psz = '-';
    return psz;
}


/**
 * Formats a unsigned hexadecimal number into a into a 64-byte buffer.
 */
static char *formatHexNumber(char *psz, uint64_t u64, unsigned cZeros)
{
    static const char s_szDigits[] = "0123456789abcdef";
    psz += 63;
    *psz-- = '\0';
    unsigned cDigits = 0;
    for (;;)
    {
        const unsigned iDigit = u64 % 16;
        u64 /= 16;
        *psz = s_szDigits[iDigit];
        ++cDigits;
        if (!u64 && cDigits >= cZeros)
            break;
        psz--;
        if (!(cDigits % 8))
            *psz-- = '\'';
    }
    return psz;
}


#if 0/* unused */
/**
 * Formats a sort key number.
 */
static void formatSortKey(char *psz, uint64_t u64)
{
    static const char s_szDigits[] = "0123456789abcdef";
    /* signed */
    *psz++ = '+';

    /* 16 hex digits */
    psz[16] = '\0';
    unsigned i = 16;
    while (i-- > 0)
    {
        if (u64)
        {
            const unsigned iDigit = u64 % 16;
            u64 /= 16;
            psz[i] = s_szDigits[iDigit];
        }
        else
            psz[i] = '0';
    }
}
#endif


#if 0/* unused */
/**
 * Formats a sort key number.
 */
static void formatSortKeySigned(char *psz, int64_t i64)
{
    static const char s_szDigits[] = "0123456789abcdef";

    /* signed */
    uint64_t u64;
    if (i64 >= 0)
    {
        *psz++ = '+';
        u64 = i64;
    }
    else
    {
        *psz++ = '-';
        u64 = -i64;
    }

    /* 16 hex digits */
    psz[16] = '\0';
    unsigned i = 16;
    while (i-- > 0)
    {
        if (u64)
        {
            const unsigned iDigit = u64 % 16;
            u64 /= 16;
            psz[i] = s_szDigits[iDigit];
        }
        else
            psz[i] = '0';
    }
}
#endif



/*
 *
 *      V B o x D b g S t a t s M o d e l
 *      V B o x D b g S t a t s M o d e l
 *      V B o x D b g S t a t s M o d e l
 *
 *
 */


VBoxDbgStatsModel::VBoxDbgStatsModel(QObject *a_pParent)
    : QAbstractItemModel(a_pParent),
    m_pRoot(NULL), m_iUpdateChild(UINT32_MAX), m_pUpdateParent(NULL), m_cchUpdateParent(0)
{
}



VBoxDbgStatsModel::~VBoxDbgStatsModel()
{
    destroyTree(m_pRoot);
    m_pRoot = NULL;
}


/*static*/ void
VBoxDbgStatsModel::destroyTree(PDBGGUISTATSNODE a_pRoot)
{
    if (!a_pRoot)
        return;
    Assert(!a_pRoot->pParent);
    Assert(!a_pRoot->iSelf);

    destroyNode(a_pRoot);
}


/* static*/ void
VBoxDbgStatsModel::destroyNode(PDBGGUISTATSNODE a_pNode)
{
    /* destroy all our children */
    uint32_t i = a_pNode->cChildren;
    while (i-- > 0)
    {
        destroyNode(a_pNode->papChildren[i]);
        a_pNode->papChildren[i] = NULL;
    }

    /* free the resources we're using */
    a_pNode->pParent = NULL;

    RTMemFree(a_pNode->papChildren);
    a_pNode->papChildren = NULL;

    if (a_pNode->enmType == STAMTYPE_CALLBACK)
    {
        delete a_pNode->Data.pStr;
        a_pNode->Data.pStr = NULL;
    }

    a_pNode->cChildren = 0;
    a_pNode->iSelf = UINT32_MAX;
    a_pNode->pszUnit = "";
    a_pNode->enmType = STAMTYPE_INVALID;

    RTMemFree(a_pNode->pszName);
    a_pNode->pszName = NULL;

    if (a_pNode->pDescStr)
    {
        delete a_pNode->pDescStr;
        a_pNode->pDescStr = NULL;
    }

#ifdef VBOX_STRICT
    /* poison it. */
    a_pNode->pParent++;
    a_pNode->Data.pStr++;
    a_pNode->pDescStr++;
    a_pNode->papChildren++;
    a_pNode->cChildren = 8442;
#endif

    /* Finally ourselves */
    a_pNode->enmState = kDbgGuiStatsNodeState_kInvalid;
    RTMemFree(a_pNode);
}


/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::createRootNode(void)
{
    PDBGGUISTATSNODE pRoot = (PDBGGUISTATSNODE)RTMemAllocZ(sizeof(DBGGUISTATSNODE));
    if (!pRoot)
        return NULL;
    pRoot->iSelf = 0;
    pRoot->enmType = STAMTYPE_INVALID;
    pRoot->pszUnit = "";
    pRoot->pszName = (char *)RTMemDup("/", sizeof("/"));
    pRoot->cchName = 1;
    pRoot->enmState = kDbgGuiStatsNodeState_kRoot;

    return pRoot;
}


/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::createAndInsertNode(PDBGGUISTATSNODE pParent, const char *pszName, size_t cchName, uint32_t iPosition)
{
    /*
     * Create it.
     */
    PDBGGUISTATSNODE pNode = (PDBGGUISTATSNODE)RTMemAllocZ(sizeof(DBGGUISTATSNODE));
    if (!pNode)
        return NULL;
    pNode->iSelf = UINT32_MAX;
    pNode->enmType = STAMTYPE_INVALID;
    pNode->pszUnit = "";
    pNode->pszName = (char *)RTMemDupEx(pszName, cchName, 1);
    pNode->cchName = cchName;
    pNode->enmState = kDbgGuiStatsNodeState_kVisible;

    /*
     * Do we need to expand the array?
     */
    if (!(pParent->cChildren & 31))
    {
        void *pvNew = RTMemRealloc(pParent->papChildren, sizeof(*pParent->papChildren) * (pParent->cChildren + 32));
        if (!pvNew)
        {
            destroyNode(pNode);
            return NULL;
        }
        pParent->papChildren = (PDBGGUISTATSNODE *)pvNew;
    }

    /*
     * Insert it.
     */
    pNode->pParent = pParent;
    if (iPosition >= pParent->cChildren)
        /* Last. */
        iPosition = pParent->cChildren;
    else
    {
        /* Shift all the items after ours. */
        uint32_t iShift = pParent->cChildren;
        while (iShift-- > iPosition)
        {
            PDBGGUISTATSNODE pChild = pParent->papChildren[iShift];
            pParent->papChildren[iShift + 1] = pChild;
            pChild->iSelf = iShift + 1;
        }
    }

    /* Insert ours */
    pNode->iSelf = iPosition;
    pParent->papChildren[iPosition] = pNode;
    pParent->cChildren++;

    return pNode;
}


PDBGGUISTATSNODE
VBoxDbgStatsModel::createAndInsert(PDBGGUISTATSNODE pParent, const char *pszName, size_t cchName, uint32_t iPosition)
{
    PDBGGUISTATSNODE pNode;
    if (m_fUpdateInsertRemove)
        pNode = createAndInsertNode(pParent, pszName, cchName, iPosition);
    else
    {
        beginInsertRows(createIndex(pParent->iSelf, 0, pParent), 0, 0);
        pNode = createAndInsertNode(pParent, pszName, cchName, iPosition);
        endInsertRows();
    }
    return pNode;
}

/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::removeNode(PDBGGUISTATSNODE pNode)
{
    PDBGGUISTATSNODE pParent = pNode->pParent;
    if (pParent)
    {
        uint32_t iPosition = pNode->iSelf;
        Assert(pParent->papChildren[iPosition] == pNode);
        uint32_t const cChildren = --pParent->cChildren;
        for (; iPosition < cChildren; iPosition++)
        {
            PDBGGUISTATSNODE pChild = pParent->papChildren[iPosition + 1];
            pParent->papChildren[iPosition] = pChild;
            pChild->iSelf = iPosition;
        }
#ifdef VBOX_STRICT /* poison */
        pParent->papChildren[iPosition] = (PDBGGUISTATSNODE)0x42;
#endif
    }
    return pNode;
}


/*static*/ void
VBoxDbgStatsModel::removeAndDestroyNode(PDBGGUISTATSNODE pNode)
{
    removeNode(pNode);
    destroyNode(pNode);
}


void
VBoxDbgStatsModel::removeAndDestroy(PDBGGUISTATSNODE pNode)
{
    if (m_fUpdateInsertRemove)
        removeAndDestroyNode(pNode);
    else
    {
        /*
         * Removing is fun since the docs are imprecise as to how persistent
         * indexes are updated (or aren't). So, let try a few different ideas
         * and see which works.
         */
#if 1
        /* destroy the children first with the appropriate begin/endRemoveRows signals. */
        DBGGUISTATSSTACK    Stack;
        Stack.a[0].pNode  = pNode;
        Stack.a[0].iChild = -1;
        Stack.iTop = 0;
        while (Stack.iTop >= 0)
        {
            /* get top element */
            PDBGGUISTATSNODE pCurNode = Stack.a[Stack.iTop].pNode;
            uint32_t         iChild   = ++Stack.a[Stack.iTop].iChild;
            if (iChild < pCurNode->cChildren)
            {
                /* push */
                Stack.iTop++;
                Assert(Stack.iTop < (int32_t)RT_ELEMENTS(Stack.a));
                Stack.a[Stack.iTop].pNode  = pCurNode->papChildren[iChild];
                Stack.a[Stack.iTop].iChild = 0;
            }
            else
            {
                /* pop and destroy all the children. */
                Stack.iTop--;
                uint32_t i = pCurNode->cChildren;
                if (i)
                {
                    beginRemoveRows(createIndex(pCurNode->iSelf, 0, pCurNode), 0, i - 1);
                    while (i-- > 0)
                        destroyNode(pCurNode->papChildren[i]);
                    pCurNode->cChildren = 0;
                    endRemoveRows();
                }
            }
        }
        Assert(!pNode->cChildren);

        /* finally the node it self. */
        PDBGGUISTATSNODE pParent = pNode->pParent;
        beginRemoveRows(createIndex(pParent->iSelf, 0, pParent), pNode->iSelf, pNode->iSelf);
        removeAndDestroyNode(pNode);
        endRemoveRows();

#elif 0
        /* This ain't working, leaves invalid indexes behind. */
        PDBGGUISTATSNODE pParent = pNode->pParent;
        beginRemoveRows(createIndex(pParent->iSelf, 0, pParent), pNode->iSelf, pNode->iSelf);
        removeAndDestroyNode(pNode);
        endRemoveRows();
#else
        /* Force reset() of the model after the update. */
        m_fUpdateInsertRemove = true;
        removeAndDestroyNode(pNode);
#endif
    }
}


/*static*/ void
VBoxDbgStatsModel::resetNode(PDBGGUISTATSNODE pNode)
{
    /* free and reinit the data. */
    if (pNode->enmType == STAMTYPE_CALLBACK)
    {
        delete pNode->Data.pStr;
        pNode->Data.pStr = NULL;
    }
    pNode->enmType = STAMTYPE_INVALID;

    /* free the description. */
    if (pNode->pDescStr)
    {
        delete pNode->pDescStr;
        pNode->pDescStr = NULL;
    }
}


/*static*/ int
VBoxDbgStatsModel::initNode(PDBGGUISTATSNODE pNode, STAMTYPE enmType, void *pvSample,
                            const char *pszUnit, const char *pszDesc)
{
    /*
     * Copy the data.
     */
    pNode->pszUnit = pszUnit;
    Assert(pNode->enmType == STAMTYPE_INVALID);
    pNode->enmType = enmType;
    if (pszDesc)
        pNode->pDescStr = new QString(pszDesc); /* ignore allocation failure (well, at least up to the point we can ignore it) */

    switch (enmType)
    {
        case STAMTYPE_COUNTER:
            pNode->Data.Counter = *(PSTAMCOUNTER)pvSample;
            break;

        case STAMTYPE_PROFILE:
        case STAMTYPE_PROFILE_ADV:
            pNode->Data.Profile = *(PSTAMPROFILE)pvSample;
            break;

        case STAMTYPE_RATIO_U32:
        case STAMTYPE_RATIO_U32_RESET:
            pNode->Data.RatioU32 = *(PSTAMRATIOU32)pvSample;
            break;

        case STAMTYPE_CALLBACK:
        {
            const char *pszString = (const char *)pvSample;
            pNode->Data.pStr = new QString(pszString);
            break;
        }

        case STAMTYPE_U8:
        case STAMTYPE_U8_RESET:
        case STAMTYPE_X8:
        case STAMTYPE_X8_RESET:
            pNode->Data.u8 = *(uint8_t *)pvSample;
            break;

        case STAMTYPE_U16:
        case STAMTYPE_U16_RESET:
        case STAMTYPE_X16:
        case STAMTYPE_X16_RESET:
            pNode->Data.u16 = *(uint16_t *)pvSample;
            break;

        case STAMTYPE_U32:
        case STAMTYPE_U32_RESET:
        case STAMTYPE_X32:
        case STAMTYPE_X32_RESET:
            pNode->Data.u32 = *(uint32_t *)pvSample;
            break;

        case STAMTYPE_U64:
        case STAMTYPE_U64_RESET:
        case STAMTYPE_X64:
        case STAMTYPE_X64_RESET:
            pNode->Data.u64 = *(uint64_t *)pvSample;
            break;

        case STAMTYPE_BOOL:
        case STAMTYPE_BOOL_RESET:
            pNode->Data.f = *(bool *)pvSample;
            break;

        default:
            AssertMsgFailed(("%d\n", enmType));
            break;
    }

    return VINF_SUCCESS;
}




/*static*/ void
VBoxDbgStatsModel::updateNode(PDBGGUISTATSNODE pNode, STAMTYPE enmType, void *pvSample, const char *pszUnit, const char *pszDesc)
{
    /*
     * Reset and init the node if the type changed.
     */
    if (enmType != pNode->enmType)
    {
        if (pNode->enmType != STAMTYPE_INVALID)
            resetNode(pNode);
        initNode(pNode, enmType, pvSample, pszUnit, pszDesc);
        pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
    }
    else
    {
        /*
         * ASSUME that only the sample value will change and that the unit, visibility
         * and description remains the same.
         */

        int64_t iDelta;
        switch (enmType)
        {
            case STAMTYPE_COUNTER:
            {
                uint64_t cPrev = pNode->Data.Counter.c;
                pNode->Data.Counter = *(PSTAMCOUNTER)pvSample;
                iDelta = pNode->Data.Counter.c - cPrev;
                if (iDelta || pNode->i64Delta)
                {
                    pNode->i64Delta = iDelta;
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            case STAMTYPE_PROFILE:
            case STAMTYPE_PROFILE_ADV:
            {
                uint64_t cPrevPeriods = pNode->Data.Profile.cPeriods;
                pNode->Data.Profile = *(PSTAMPROFILE)pvSample;
                iDelta = pNode->Data.Profile.cPeriods - cPrevPeriods;
                if (iDelta || pNode->i64Delta)
                {
                    pNode->i64Delta = iDelta;
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            case STAMTYPE_RATIO_U32:
            case STAMTYPE_RATIO_U32_RESET:
            {
                STAMRATIOU32 Prev = pNode->Data.RatioU32;
                pNode->Data.RatioU32 = *(PSTAMRATIOU32)pvSample;
                int32_t iDeltaA = pNode->Data.RatioU32.u32A - Prev.u32A;
                int32_t iDeltaB = pNode->Data.RatioU32.u32B - Prev.u32B;
                if (iDeltaA == 0 && iDeltaB == 0)
                {
                    if (pNode->i64Delta)
                    {
                        pNode->i64Delta = 0;
                        pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                    }
                }
                else
                {
                    if (iDeltaA >= 0)
                        pNode->i64Delta = iDeltaA + (iDeltaB >= 0 ? iDeltaB : -iDeltaB);
                    else
                        pNode->i64Delta = iDeltaA + (iDeltaB <  0 ? iDeltaB : -iDeltaB);
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            case STAMTYPE_CALLBACK:
            {
                const char *pszString = (const char *)pvSample;
                if (!pNode->Data.pStr)
                {
                    pNode->Data.pStr = new QString(pszString);
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                else if (*pNode->Data.pStr == pszString)
                {
                    delete pNode->Data.pStr;
                    pNode->Data.pStr = new QString(pszString);
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            case STAMTYPE_U8:
            case STAMTYPE_U8_RESET:
            case STAMTYPE_X8:
            case STAMTYPE_X8_RESET:
            {
                uint8_t uPrev = pNode->Data.u8;
                pNode->Data.u8 = *(uint8_t *)pvSample;
                iDelta = (int32_t)pNode->Data.u8 - (int32_t)uPrev;
                if (iDelta || pNode->i64Delta)
                {
                    pNode->i64Delta = iDelta;
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            case STAMTYPE_U16:
            case STAMTYPE_U16_RESET:
            case STAMTYPE_X16:
            case STAMTYPE_X16_RESET:
            {
                uint16_t uPrev = pNode->Data.u16;
                pNode->Data.u16 = *(uint16_t *)pvSample;
                iDelta = (int32_t)pNode->Data.u16 - (int32_t)uPrev;
                if (iDelta || pNode->i64Delta)
                {
                    pNode->i64Delta = iDelta;
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            case STAMTYPE_U32:
            case STAMTYPE_U32_RESET:
            case STAMTYPE_X32:
            case STAMTYPE_X32_RESET:
            {
                uint32_t uPrev = pNode->Data.u32;
                pNode->Data.u32 = *(uint32_t *)pvSample;
                iDelta = (int64_t)pNode->Data.u32 - (int64_t)uPrev;
                if (iDelta || pNode->i64Delta)
                {
                    pNode->i64Delta = iDelta;
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            case STAMTYPE_U64:
            case STAMTYPE_U64_RESET:
            case STAMTYPE_X64:
            case STAMTYPE_X64_RESET:
            {
                uint64_t uPrev = pNode->Data.u64;
                pNode->Data.u64 = *(uint64_t *)pvSample;
                iDelta = pNode->Data.u64 - uPrev;
                if (iDelta || pNode->i64Delta)
                {
                    pNode->i64Delta = iDelta;
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            case STAMTYPE_BOOL:
            case STAMTYPE_BOOL_RESET:
            {
                bool fPrev = pNode->Data.f;
                pNode->Data.f = *(bool *)pvSample;
                iDelta = pNode->Data.f - fPrev;
                if (iDelta || pNode->i64Delta)
                {
                    pNode->i64Delta = iDelta;
                    pNode->enmState = kDbgGuiStatsNodeState_kRefresh;
                }
                break;
            }

            default:
                AssertMsgFailed(("%d\n", enmType));
                break;
        }
    }
}


/*static*/ ssize_t
VBoxDbgStatsModel::getNodePath(PCDBGGUISTATSNODE pNode, char *psz, ssize_t cch)
{
    ssize_t off;
    if (!pNode->pParent)
    {
        /* root - don't add it's slash! */
        AssertReturn(cch >= 1, -1);
        off = 0;
        *psz = '\0';
    }
    else
    {
        cch -= pNode->cchName + 1;
        AssertReturn(cch > 0, -1);
        off = getNodePath(pNode->pParent, psz, cch);
        if (off >= 0)
        {
            psz[off++] = '/';
            memcpy(&psz[off], pNode->pszName, pNode->cchName + 1);
            off += pNode->cchName;
        }
    }
    return off;
}


/*static*/ char *
VBoxDbgStatsModel::getNodePath2(PCDBGGUISTATSNODE pNode, char *psz, ssize_t cch)
{
    if (VBoxDbgStatsModel::getNodePath(pNode, psz, cch) < 0)
        return NULL;
    return psz;
}



/*static*/ bool
VBoxDbgStatsModel::isNodeAncestorOf(PCDBGGUISTATSNODE pAncestor, PCDBGGUISTATSNODE pDescendant)
{
    while (pDescendant)
    {
        pDescendant = pDescendant->pParent;
        if (pDescendant == pAncestor)
            return true;
    }
    return false;
}


/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::nextNode(PDBGGUISTATSNODE pNode)
{
    if (!pNode)
        return NULL;

    /* descend to children. */
    if (pNode->cChildren)
        return pNode->papChildren[0];

    PDBGGUISTATSNODE pParent = pNode->pParent;
    if (!pParent)
        return NULL;

    /* next sibling. */
    if (pNode->iSelf + 1 < pNode->pParent->cChildren)
        return pParent->papChildren[pNode->iSelf + 1];

    /* ascend and advanced to a parent's sibiling. */
    for (;;)
    {
        uint32_t iSelf = pParent->iSelf;
        pParent = pParent->pParent;
        if (!pParent)
            return NULL;
        if (iSelf + 1 < pParent->cChildren)
            return pParent->papChildren[iSelf + 1];
    }
}


/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::nextDataNode(PDBGGUISTATSNODE pNode)
{
    do
        pNode = nextNode(pNode);
    while (     pNode
           &&   pNode->enmType == STAMTYPE_INVALID);
    return pNode;
}


/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::prevNode(PDBGGUISTATSNODE pNode)
{
    if (!pNode)
        return NULL;
    PDBGGUISTATSNODE pParent = pNode->pParent;
    if (!pParent)
        return NULL;

    /* previous sibling's latest descendant (better expression anyone?). */
    if (pNode->iSelf > 0)
    {
        pNode = pParent->papChildren[pNode->iSelf - 1];
        while (pNode->cChildren)
            pNode = pNode->papChildren[pNode->cChildren - 1];
        return pNode;
    }

    /* ascend to the parent. */
    return pParent;
}


/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::prevDataNode(PDBGGUISTATSNODE pNode)
{
    do
        pNode = prevNode(pNode);
    while (     pNode
           &&   pNode->enmType == STAMTYPE_INVALID);
    return pNode;
}


#if 0
/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::createNewTree(IMachineDebugger *a_pIMachineDebugger)
{
    /** @todo  */
    return NULL;
}
#endif


#if 0
/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::createNewTree(const char *pszFilename)
{
    /** @todo  */
    return NULL;
}
#endif


#if 0
/*static*/ PDBGGUISTATSNODE
VBoxDbgStatsModel::createDiffTree(PDBGGUISTATSNODE pTree1, PDBGGUISTATSNODE pTree2)
{
    /** @todo  */
    return NULL;
}
#endif


PDBGGUISTATSNODE
VBoxDbgStatsModel::updateCallbackHandleOutOfOrder(const char *pszName)
{
#if defined(VBOX_STRICT) || defined(LOG_ENABLED)
    char szStrict[1024];
#endif

    /*
     * We might be inserting a new node between pPrev and pNode
     * or we might be removing one or more nodes. Either case is
     * handled in the same rough way.
     *
     * Might consider optimizing insertion at some later point since this
     * is a normal occurrence (dynamic statistics in PATM, IOM, MM, ++).
     */
    Assert(pszName[0] == '/');
    Assert(m_szUpdateParent[m_cchUpdateParent - 1] == '/');

    /*
     * Start with the current parent node and look for a common ancestor
     * hoping that this is faster than going from the root (saves lookup).
     */
    PDBGGUISTATSNODE pNode = m_pUpdateParent->papChildren[m_iUpdateChild];
    PDBGGUISTATSNODE const pPrev = prevDataNode(pNode);
    AssertMsg(strcmp(pszName, getNodePath2(pNode, szStrict, sizeof(szStrict))), ("%s\n", szStrict));
    AssertMsg(!pPrev || strcmp(pszName, getNodePath2(pPrev, szStrict, sizeof(szStrict))), ("%s\n", szStrict));
    Log(("updateCallbackHandleOutOfOrder: pszName='%s' m_szUpdateParent='%s' m_cchUpdateParent=%u pNode='%s'\n",
         pszName, m_szUpdateParent, m_cchUpdateParent, getNodePath2(pNode, szStrict, sizeof(szStrict))));

    pNode = pNode->pParent;
    while (pNode != m_pRoot)
    {
        if (!strncmp(pszName, m_szUpdateParent, m_cchUpdateParent))
            break;
        Assert(m_cchUpdateParent > pNode->cchName);
        m_cchUpdateParent -= pNode->cchName + 1;
        m_szUpdateParent[m_cchUpdateParent] = '\0';
        Log2(("updateCallbackHandleOutOfOrder: m_szUpdateParent='%s' m_cchUpdateParent=%u, removed '/%s' (%u)\n", m_szUpdateParent, m_cchUpdateParent, pNode->pszName, __LINE__));
        pNode = pNode->pParent;
    }
    Assert(m_szUpdateParent[m_cchUpdateParent - 1] == '/');

    /*
     * Descend until we've found/created the node pszName indicates,
     * modifying m_szUpdateParent as we go along.
     */
    while (pszName[m_cchUpdateParent - 1] == '/')
    {
        /* Find the end of this component. */
        const char * const pszSubName = &pszName[m_cchUpdateParent];
        const char *pszEnd = strchr(pszSubName, '/');
        if (!pszEnd)
            pszEnd = strchr(pszSubName, '\0');
        size_t cchSubName = pszEnd - pszSubName;

        /* Add the name to the path. */
        memcpy(&m_szUpdateParent[m_cchUpdateParent], pszSubName, cchSubName);
        m_cchUpdateParent += cchSubName;
        m_szUpdateParent[m_cchUpdateParent++] = '/';
        m_szUpdateParent[m_cchUpdateParent] = '\0';
        Assert(m_cchUpdateParent < sizeof(m_szUpdateParent));
        Log2(("updateCallbackHandleOutOfOrder: m_szUpdateParent='%s' m_cchUpdateParent=%u (%u)\n", m_szUpdateParent, m_cchUpdateParent, __LINE__));

        if (!pNode->cChildren)
        {
            /* first child */
            pNode = createAndInsert(pNode, pszSubName, cchSubName, 0);
            AssertReturn(pNode, NULL);
        }
        else
        {
            /* binary search. */
            int32_t iStart = 0;
            int32_t iLast = pNode->cChildren - 1;
            for (;;)
            {
                int32_t i = iStart + (iLast + 1 - iStart) / 2;
                int iDiff;
                size_t const cchCompare = RT_MIN(pNode->papChildren[i]->cchName, cchSubName);
                iDiff = memcmp(pszSubName, pNode->papChildren[i]->pszName, cchCompare);
                if (!iDiff)
                {
                    iDiff = cchSubName == cchCompare ? 0 : cchSubName > cchCompare ? 1 : -1;
                    /* For cases when exisiting node name is same as new node name with additional characters. */
                    if (!iDiff)
                        iDiff = cchSubName == pNode->papChildren[i]->cchName ? 0 : cchSubName > pNode->papChildren[i]->cchName ? 1 : -1;
                }
                if (iDiff > 0)
                {
                    iStart = i + 1;
                    if (iStart > iLast)
                    {
                        pNode = createAndInsert(pNode, pszSubName, cchSubName, iStart);
                        AssertReturn(pNode, NULL);
                        break;
                    }
                }
                else if (iDiff < 0)
                {
                    iLast = i - 1;
                    if (iLast < iStart)
                    {
                        pNode = createAndInsert(pNode, pszSubName, cchSubName, i);
                        AssertReturn(pNode, NULL);
                        break;
                    }
                }
                else
                {
                    pNode = pNode->papChildren[i];
                    break;
                }
            }
        }
    }
    Assert(   !memcmp(pszName, m_szUpdateParent, m_cchUpdateParent - 2)
           && pszName[m_cchUpdateParent - 1] == '\0');

    /*
     * Remove all the nodes between pNode and pPrev but keep all
     * of pNode's ancestors (or it'll get orphaned).
     */
    PDBGGUISTATSNODE pCur = prevNode(pNode);
    while (pCur != pPrev)
    {
        PDBGGUISTATSNODE pAdv = prevNode(pCur); Assert(pAdv || !pPrev);
        if (!isNodeAncestorOf(pCur, pNode))
        {
            Assert(pCur != m_pRoot);
            removeAndDestroy(pCur);
        }
        pCur = pAdv;
    }

    /*
     * Remove the data from all ancestors of pNode that it doesn't
     * share them pPrev.
     */
    if (pPrev)
    {
        pCur = pNode->pParent;
        while (!isNodeAncestorOf(pCur, pPrev))
        {
            resetNode(pNode);
            pCur = pCur->pParent;
        }
    }

    /*
     * Finally, adjust the globals (szUpdateParent is one level too deep).
     */
    Assert(m_cchUpdateParent > pNode->cchName + 1);
    m_cchUpdateParent -= pNode->cchName + 1;
    m_szUpdateParent[m_cchUpdateParent] = '\0';
    m_pUpdateParent = pNode->pParent;
    m_iUpdateChild = pNode->iSelf;
    Log2(("updateCallbackHandleOutOfOrder: m_szUpdateParent='%s' m_cchUpdateParent=%u (%u)\n", m_szUpdateParent, m_cchUpdateParent, __LINE__));

    return pNode;
}


PDBGGUISTATSNODE
VBoxDbgStatsModel::updateCallbackHandleTail(const char *pszName)
{
    /*
     * Insert it at the end of the tree.
     *
     * Do the same as we're doing down in createNewTreeCallback, walk from the
     * root and create whatever we need.
     */
    AssertReturn(*pszName == '/' && pszName[1] != '/', NULL);
    PDBGGUISTATSNODE pNode = m_pRoot;
    const char *pszCur = pszName + 1;
    while (*pszCur)
    {
        /* Find the end of this component. */
        const char *pszNext = strchr(pszCur, '/');
        if (!pszNext)
            pszNext = strchr(pszCur, '\0');
        size_t cchCur = pszNext - pszCur;

        /* Create it if it doesn't exist (it will be last if it exists). */
        if (    !pNode->cChildren
            ||  strncmp(pNode->papChildren[pNode->cChildren - 1]->pszName, pszCur, cchCur)
            ||  pNode->papChildren[pNode->cChildren - 1]->pszName[cchCur])
        {
            pNode = createAndInsert(pNode, pszCur, pszNext - pszCur, pNode->cChildren);
            AssertReturn(pNode, NULL);
        }
        else
            pNode = pNode->papChildren[pNode->cChildren - 1];

        /* Advance */
        pszCur = *pszNext ? pszNext + 1 : pszNext;
    }

    return pNode;
}


void
VBoxDbgStatsModel::updateCallbackAdvance(PDBGGUISTATSNODE pNode)
{
    /*
     * Advance to the next node with data.
     *
     * ASSUMES a leaf *must* have data and again we're ASSUMING the sorting
     * on slash separated sub-strings.
     */
    if (m_iUpdateChild != UINT32_MAX)
    {
#ifdef VBOX_STRICT
        PDBGGUISTATSNODE const pCorrectNext = nextDataNode(pNode);
#endif
        PDBGGUISTATSNODE pParent = pNode->pParent;
        if (pNode->cChildren)
        {
            /* descend to the first child. */
            Assert(m_cchUpdateParent + pNode->cchName + 2 < sizeof(m_szUpdateParent));
            memcpy(&m_szUpdateParent[m_cchUpdateParent], pNode->pszName, pNode->cchName);
            m_cchUpdateParent += pNode->cchName;
            m_szUpdateParent[m_cchUpdateParent++] = '/';
            m_szUpdateParent[m_cchUpdateParent] = '\0';

            pNode = pNode->papChildren[0];
        }
        else if (pNode->iSelf + 1 < pParent->cChildren)
        {
            /* next sibling or one if its descendants. */
            Assert(m_pUpdateParent == pParent);
            pNode = pParent->papChildren[pNode->iSelf + 1];
        }
        else
        {
            /* move up and down- / on-wards */
            for (;;)
            {
                /* ascend */
                pNode = pParent;
                pParent = pParent->pParent;
                if (!pParent)
                {
                    Assert(pNode == m_pRoot);
                    m_iUpdateChild = UINT32_MAX;
                    m_szUpdateParent[0] = '\0';
                    m_cchUpdateParent = 0;
                    m_pUpdateParent = NULL;
                    break;
                }
                Assert(m_cchUpdateParent > pNode->cchName + 1);
                m_cchUpdateParent -= pNode->cchName + 1;

                /* try advance */
                if (pNode->iSelf + 1 < pParent->cChildren)
                {
                    pNode = pParent->papChildren[pNode->iSelf + 1];
                    m_szUpdateParent[m_cchUpdateParent] = '\0';
                    break;
                }
            }
        }

        /* descend to a node containing data and finalize the globals. (ASSUMES leaf has data.) */
        if (m_iUpdateChild != UINT32_MAX)
        {
            while (   pNode->enmType == STAMTYPE_INVALID
                   && pNode->cChildren > 0)
            {
                Assert(pNode->enmState == kDbgGuiStatsNodeState_kVisible);

                Assert(m_cchUpdateParent + pNode->cchName + 2 < sizeof(m_szUpdateParent));
                memcpy(&m_szUpdateParent[m_cchUpdateParent], pNode->pszName, pNode->cchName);
                m_cchUpdateParent += pNode->cchName;
                m_szUpdateParent[m_cchUpdateParent++] = '/';
                m_szUpdateParent[m_cchUpdateParent] = '\0';

                pNode = pNode->papChildren[0];
            }
            Assert(pNode->enmType != STAMTYPE_INVALID);
            m_iUpdateChild = pNode->iSelf;
            m_pUpdateParent = pNode->pParent;
            Assert(pNode == pCorrectNext);
        }
    }
    /* else: we're at the end */
}


/*static*/ DECLCALLBACK(int)
VBoxDbgStatsModel::updateCallback(const char *pszName, STAMTYPE enmType, void *pvSample, STAMUNIT enmUnit,
                                  const char *pszUnit, STAMVISIBILITY enmVisibility, const char *pszDesc, void *pvUser)
{
    VBoxDbgStatsModelVM *pThis = (VBoxDbgStatsModelVM *)pvUser;
    Log3(("updateCallback: %s\n", pszName));
    RT_NOREF(enmUnit);

    /*
     * Skip the ones which shouldn't be visible in the GUI.
     */
    if (enmVisibility == STAMVISIBILITY_NOT_GUI)
        return 0;

    /*
     * The default assumption is that nothing has changed.
     * For now we'll reset the model when ever something changes.
     */
    PDBGGUISTATSNODE pNode;
    if (pThis->m_iUpdateChild != UINT32_MAX)
    {
        pNode = pThis->m_pUpdateParent->papChildren[pThis->m_iUpdateChild];
        if (    !strncmp(pszName, pThis->m_szUpdateParent, pThis->m_cchUpdateParent)
            &&  !strcmp(pszName + pThis->m_cchUpdateParent, pNode->pszName))
            /* got it! */;
        else
        {
            /* insert/remove */
            pNode = pThis->updateCallbackHandleOutOfOrder(pszName);
            if (!pNode)
                return VERR_NO_MEMORY;
        }
    }
    else
    {
        /* append */
        pNode = pThis->updateCallbackHandleTail(pszName);
        if (!pNode)
            return VERR_NO_MEMORY;
    }

    /*
     * Perform the update and advance to the next one.
     */
    updateNode(pNode, enmType, pvSample, pszUnit, pszDesc);
    pThis->updateCallbackAdvance(pNode);

    return VINF_SUCCESS;
}


bool
VBoxDbgStatsModel::updatePrepare(void)
{
    /*
     * Find the first child with data and set it up as the 'next'
     * node to be updated.
     */
    Assert(m_pRoot);
    Assert(m_pRoot->enmType == STAMTYPE_INVALID);
    PDBGGUISTATSNODE pFirst = nextDataNode(m_pRoot);
    if (pFirst)
    {
        m_iUpdateChild = pFirst->iSelf;
        m_pUpdateParent = pFirst->pParent; Assert(m_pUpdateParent);
        m_cchUpdateParent = getNodePath(m_pUpdateParent, m_szUpdateParent, sizeof(m_szUpdateParent) - 1);
        AssertReturn(m_cchUpdateParent >= 1, false);
        m_szUpdateParent[m_cchUpdateParent++] = '/';
        m_szUpdateParent[m_cchUpdateParent] = '\0';
    }
    else
    {
        m_iUpdateChild = UINT32_MAX;
        m_pUpdateParent = NULL;
        m_szUpdateParent[0] = '\0';
        m_cchUpdateParent = 0;
    }

    /*
     * Set the flag and signal possible layout change.
     */
    m_fUpdateInsertRemove = false;
    /* emit layoutAboutToBeChanged(); - debug this, it gets stuck... */
    return true;
}


bool
VBoxDbgStatsModel::updateDone(bool a_fSuccess)
{
    /*
     * Remove any nodes following the last in the update (unless the update failed).
     */
    if (    a_fSuccess
        &&  m_iUpdateChild != UINT32_MAX)
    {
        PDBGGUISTATSNODE const pLast = prevDataNode(m_pUpdateParent->papChildren[m_iUpdateChild]);
        if (!pLast)
        {
            /* nuking the whole tree. */
            setRootNode(createRootNode());
            m_fUpdateInsertRemove = true;
        }
        else
        {
            PDBGGUISTATSNODE pNode;
            while ((pNode = nextNode(pLast)))
            {
                Assert(pNode != m_pRoot);
                removeAndDestroy(pNode);
            }
        }
    }

    /*
     * We're done making layout changes (if I understood it correctly), so,
     * signal this and then see what to do next. If we did too many removals
     * we'll just reset the whole shebang.
     */
    if (m_fUpdateInsertRemove)
    {
        /* emit layoutChanged(); - hrmpf, doesn't work reliably... */
        beginResetModel();
        endResetModel();
    }
    else
    {
        /*
         * Send dataChanged events.
         *
         * We do this here instead of from the updateCallback because it reduces
         * the clutter in that method and allow us to emit bulk signals in an
         * easier way because we can traverse the tree in a different fashion.
         */
        DBGGUISTATSSTACK    Stack;
        Stack.a[0].pNode = m_pRoot;
        Stack.a[0].iChild = -1;
        Stack.iTop = 0;

        while (Stack.iTop >= 0)
        {
            /* get top element */
            PDBGGUISTATSNODE pNode  = Stack.a[Stack.iTop].pNode;
            uint32_t         iChild = ++Stack.a[Stack.iTop].iChild;
            if (iChild < pNode->cChildren)
            {
                /* push */
                Stack.iTop++;
                Assert(Stack.iTop < (int32_t)RT_ELEMENTS(Stack.a));
                Stack.a[Stack.iTop].pNode = pNode->papChildren[iChild];
                Stack.a[Stack.iTop].iChild = 0;
            }
            else
            {
                /* pop */
                Stack.iTop--;

                /* do the actual work. */
                iChild = 0;
                while (iChild < pNode->cChildren)
                {
                    /* skip to the first needing updating. */
                    while (     iChild < pNode->cChildren
                           &&   pNode->papChildren[iChild]->enmState != kDbgGuiStatsNodeState_kRefresh)
                        iChild++;
                    if (iChild >= pNode->cChildren)
                        break;
                    QModelIndex TopLeft = createIndex(iChild, 0, pNode->papChildren[iChild]);
                    pNode->papChildren[iChild]->enmState = kDbgGuiStatsNodeState_kVisible;

                    /* any subsequent nodes that also needs refreshing? */
                    if (   ++iChild < pNode->cChildren
                        && pNode->papChildren[iChild]->enmState == kDbgGuiStatsNodeState_kRefresh)
                    {
                        do  pNode->papChildren[iChild]->enmState = kDbgGuiStatsNodeState_kVisible;
                        while (   ++iChild < pNode->cChildren
                               && pNode->papChildren[iChild]->enmState == kDbgGuiStatsNodeState_kRefresh);
                        QModelIndex BottomRight = createIndex(iChild - 1, DBGGUI_STATS_COLUMNS - 1, pNode->papChildren[iChild - 1]);

                        /* emit the refresh signal */
                        emit dataChanged(TopLeft, BottomRight);
                    }
                    else
                    {
                        /* emit the refresh signal */
                        emit dataChanged(TopLeft, TopLeft);
                    }
                }
            }
        }
        /* emit layoutChanged(); - hrmpf, doesn't work reliably... */
    }

    return m_fUpdateInsertRemove;
}


bool
VBoxDbgStatsModel::updateStatsByPattern(const QString &a_rPatStr)
{
    /* stub */
    NOREF(a_rPatStr);
    return false;
}


void
VBoxDbgStatsModel::updateStatsByIndex(QModelIndex const &a_rIndex)
{
    /** @todo implement this based on updateStatsByPattern. */
    NOREF(a_rIndex);
}


void
VBoxDbgStatsModel::resetStatsByPattern(QString const &a_rPatStr)
{
    /* stub */
    NOREF(a_rPatStr);
}


void
VBoxDbgStatsModel::resetStatsByIndex(QModelIndex const &a_rIndex, bool fSubTree /*= true*/)
{
    PCDBGGUISTATSNODE pNode = nodeFromIndex(a_rIndex);
    if (pNode == m_pRoot || !a_rIndex.isValid())
    {
        if (fSubTree)
        {
            /* everything from the root down. */
            resetStatsByPattern(QString());
        }
    }
    else if (pNode)
    {
        /* the node pattern. */
        char szPat[1024+1024+4];
        ssize_t cch = getNodePath(pNode, szPat, 1024);
        AssertReturnVoid(cch >= 0);

        /* the sub-tree pattern. */
        if (fSubTree && pNode->cChildren)
        {
            char *psz = &szPat[cch];
            *psz++ = '|';
            memcpy(psz, szPat, cch);
            psz += cch;
            *psz++ = '/';
            *psz++ = '*';
            *psz++ = '\0';
        }

        resetStatsByPattern(szPat);
    }
}


void
VBoxDbgStatsModel::iterateStatsByPattern(QString const &a_rPatStr, VBoxDbgStatsModel::FNITERATOR *a_pfnCallback, void *a_pvUser,
                                         bool a_fMatchChildren /*= true*/)
{
    const QByteArray   &PatBytes   = a_rPatStr.toUtf8();
    const char * const  pszPattern = PatBytes.constData();
    size_t const        cchPattern = strlen(pszPattern);

    DBGGUISTATSSTACK Stack;
    Stack.a[0].pNode   = m_pRoot;
    Stack.a[0].iChild  = 0;
    Stack.a[0].cchName = 0;
    Stack.iTop         = 0;

    char szName[1024];
    szName[0] = '\0';

    while (Stack.iTop >= 0)
    {
        /* get top element */
        PDBGGUISTATSNODE const pNode   = Stack.a[Stack.iTop].pNode;
        uint16_t               cchName = Stack.a[Stack.iTop].cchName;
        uint32_t const         iChild  = Stack.a[Stack.iTop].iChild++;
        if (iChild < pNode->cChildren)
        {
            PDBGGUISTATSNODE pChild = pNode->papChildren[iChild];

            /* Build the name and match the pattern. */
            Assert(cchName + 1 + pChild->cchName < sizeof(szName));
            szName[cchName++] = '/';
            memcpy(&szName[cchName], pChild->pszName, pChild->cchName);
            cchName += (uint16_t)pChild->cchName;
            szName[cchName] = '\0';

            if (RTStrSimplePatternMultiMatch(pszPattern, cchPattern, szName, cchName, NULL))
            {
                /* Do callback. */
                QModelIndex const Index = createIndex(iChild, 0, pChild);
                if (!a_pfnCallback(pChild, Index, szName, a_pvUser))
                    return;
                if (!a_fMatchChildren)
                    continue;
            }

            /* push */
            Stack.iTop++;
            Assert(Stack.iTop < (int32_t)RT_ELEMENTS(Stack.a));
            Stack.a[Stack.iTop].pNode   = pChild;
            Stack.a[Stack.iTop].iChild  = 0;
            Stack.a[Stack.iTop].cchName = cchName;
        }
        else
        {
            /* pop */
            Stack.iTop--;
        }
    }
}


QModelIndex
VBoxDbgStatsModel::getRootIndex(void) const
{
    if (!m_pRoot)
        return QModelIndex();
    return createIndex(0, 0, m_pRoot);
}


void
VBoxDbgStatsModel::setRootNode(PDBGGUISTATSNODE a_pRoot)
{
    PDBGGUISTATSNODE pOldTree = m_pRoot;
    m_pRoot = a_pRoot;
    destroyTree(pOldTree);
    beginResetModel();
    endResetModel();
}


Qt::ItemFlags
VBoxDbgStatsModel::flags(const QModelIndex &a_rIndex) const
{
    Qt::ItemFlags fFlags = QAbstractItemModel::flags(a_rIndex);
    return fFlags;
}


int
VBoxDbgStatsModel::columnCount(const QModelIndex &a_rParent) const
{
    NOREF(a_rParent);
    return DBGGUI_STATS_COLUMNS;
}


int
VBoxDbgStatsModel::rowCount(const QModelIndex &a_rParent) const
{
    PDBGGUISTATSNODE pParent = nodeFromIndex(a_rParent);
    return pParent ? pParent->cChildren : 1 /* root */;
}


bool
VBoxDbgStatsModel::hasChildren(const QModelIndex &a_rParent) const
{
    PDBGGUISTATSNODE pParent = nodeFromIndex(a_rParent);
    return pParent ? pParent->cChildren > 0 : true /* root */;
}


QModelIndex
VBoxDbgStatsModel::index(int iRow, int iColumn, const QModelIndex &a_rParent) const
{
    PDBGGUISTATSNODE pParent = nodeFromIndex(a_rParent);
    if (!pParent)
    {
        if (    a_rParent.isValid()
            ||  iRow
            ||  (unsigned)iColumn < DBGGUI_STATS_COLUMNS)
        {
            Assert(!a_rParent.isValid());
            Assert(!iRow);
            Assert((unsigned)iColumn < DBGGUI_STATS_COLUMNS);
            return QModelIndex();
        }

        /* root */
        return createIndex(0, iColumn, m_pRoot);
    }
    if ((unsigned)iRow >= pParent->cChildren)
    {
        Log(("index: iRow=%d >= cChildren=%u (iColumn=%d)\n", iRow, (unsigned)pParent->cChildren, iColumn));
        return QModelIndex(); /* bug? */
    }
    if ((unsigned)iColumn >= DBGGUI_STATS_COLUMNS)
    {
        Log(("index: iColumn=%d (iRow=%d)\n", iColumn, iRow));
        return QModelIndex(); /* bug? */
    }
    PDBGGUISTATSNODE pChild = pParent->papChildren[iRow];
    return createIndex(iRow, iColumn, pChild);
}


QModelIndex
VBoxDbgStatsModel::parent(const QModelIndex &a_rChild) const
{
    PDBGGUISTATSNODE pChild = nodeFromIndex(a_rChild);
    if (!pChild)
    {
        Log(("parent: invalid child\n"));
        return QModelIndex(); /* bug */
    }
    PDBGGUISTATSNODE pParent = pChild->pParent;
    if (!pParent)
        return QModelIndex(); /* ultimate root */

    return createIndex(pParent->iSelf, 0, pParent);
}


QVariant
VBoxDbgStatsModel::headerData(int a_iSection, Qt::Orientation a_eOrientation, int a_eRole) const
{
    if (    a_eOrientation == Qt::Horizontal
        &&  a_eRole == Qt::DisplayRole)
        switch (a_iSection)
        {
            case 0: return tr("Name");
            case 1: return tr("Unit");
            case 2: return tr("Value/Times");
            case 3: return tr("Min");
            case 4: return tr("Average");
            case 5: return tr("Max");
            case 6: return tr("Total");
            case 7: return tr("dInt");
            case 8: return tr("Description");
            default:
                AssertCompile(DBGGUI_STATS_COLUMNS == 9);
                return QVariant(); /* bug */
        }
    else if (   a_eOrientation == Qt::Horizontal
             && a_eRole == Qt::TextAlignmentRole)
        switch (a_iSection)
        {
            case 0:
            case 1:
                return QVariant();
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                return (int)(Qt::AlignRight | Qt::AlignVCenter);
            case 8:
                return QVariant();
            default:
                AssertCompile(DBGGUI_STATS_COLUMNS == 9);
                return QVariant(); /* bug */
        }

    return QVariant();
}


/*static*/ QString
VBoxDbgStatsModel::strUnit(PCDBGGUISTATSNODE pNode)
{
    return pNode->pszUnit;
}


/*static*/ QString
VBoxDbgStatsModel::strValueTimes(PCDBGGUISTATSNODE pNode)
{
    char sz[128];

    switch (pNode->enmType)
    {
        case STAMTYPE_COUNTER:
            return formatNumber(sz, pNode->Data.Counter.c);

        case STAMTYPE_PROFILE:
        case STAMTYPE_PROFILE_ADV:
            if (!pNode->Data.Profile.cPeriods)
                return "0";
            return formatNumber(sz, pNode->Data.Profile.cPeriods);

        case STAMTYPE_RATIO_U32:
        case STAMTYPE_RATIO_U32_RESET:
        {
            char szTmp[64];
            char *psz  = formatNumber(szTmp, pNode->Data.RatioU32.u32A);
            size_t off = strlen(psz);
            memcpy(sz, psz, off);
            sz[off++] = ':';
            strcpy(&sz[off], formatNumber(szTmp, pNode->Data.RatioU32.u32B));
            return sz;
        }

        case STAMTYPE_CALLBACK:
            return *pNode->Data.pStr;

        case STAMTYPE_U8:
        case STAMTYPE_U8_RESET:
            return formatNumber(sz, pNode->Data.u8);

        case STAMTYPE_X8:
        case STAMTYPE_X8_RESET:
            return formatHexNumber(sz, pNode->Data.u8, 2);

        case STAMTYPE_U16:
        case STAMTYPE_U16_RESET:
            return formatNumber(sz, pNode->Data.u16);

        case STAMTYPE_X16:
        case STAMTYPE_X16_RESET:
            return formatHexNumber(sz, pNode->Data.u16, 4);

        case STAMTYPE_U32:
        case STAMTYPE_U32_RESET:
            return formatNumber(sz, pNode->Data.u32);

        case STAMTYPE_X32:
        case STAMTYPE_X32_RESET:
            return formatHexNumber(sz, pNode->Data.u32, 8);

        case STAMTYPE_U64:
        case STAMTYPE_U64_RESET:
            return formatNumber(sz, pNode->Data.u64);

        case STAMTYPE_X64:
        case STAMTYPE_X64_RESET:
            return formatHexNumber(sz, pNode->Data.u64, 16);

        case STAMTYPE_BOOL:
        case STAMTYPE_BOOL_RESET:
            return pNode->Data.f ? "true" : "false";

        default:
            AssertMsgFailed(("%d\n", pNode->enmType));
            RT_FALL_THRU();
        case STAMTYPE_INVALID:
            return "";
    }
}


/*static*/ QString
VBoxDbgStatsModel::strMinValue(PCDBGGUISTATSNODE pNode)
{
    char sz[128];

    switch (pNode->enmType)
    {
        case STAMTYPE_PROFILE:
        case STAMTYPE_PROFILE_ADV:
            if (!pNode->Data.Profile.cPeriods)
                return "0";
            return formatNumber(sz, pNode->Data.Profile.cTicksMin);
        default:
            return "";
    }
}


/*static*/ QString
VBoxDbgStatsModel::strAvgValue(PCDBGGUISTATSNODE pNode)
{
    char sz[128];

    switch (pNode->enmType)
    {
        case STAMTYPE_PROFILE:
        case STAMTYPE_PROFILE_ADV:
            if (!pNode->Data.Profile.cPeriods)
                return "0";
            return formatNumber(sz, pNode->Data.Profile.cTicks / pNode->Data.Profile.cPeriods);
        default:
            return "";
    }
}


/*static*/ QString
VBoxDbgStatsModel::strMaxValue(PCDBGGUISTATSNODE pNode)
{
    char sz[128];

    switch (pNode->enmType)
    {
        case STAMTYPE_PROFILE:
        case STAMTYPE_PROFILE_ADV:
            if (!pNode->Data.Profile.cPeriods)
                return "0";
            return formatNumber(sz, pNode->Data.Profile.cTicksMax);
        default:
            return "";
    }
}


/*static*/ QString
VBoxDbgStatsModel::strTotalValue(PCDBGGUISTATSNODE pNode)
{
    char sz[128];

    switch (pNode->enmType)
    {
        case STAMTYPE_PROFILE:
        case STAMTYPE_PROFILE_ADV:
            if (!pNode->Data.Profile.cPeriods)
                return "0";
            return formatNumber(sz, pNode->Data.Profile.cTicks);
        default:
            return "";
    }
}


/*static*/ QString
VBoxDbgStatsModel::strDeltaValue(PCDBGGUISTATSNODE pNode)
{
    char sz[128];

    switch (pNode->enmType)
    {
        case STAMTYPE_PROFILE:
        case STAMTYPE_PROFILE_ADV:
            if (!pNode->Data.Profile.cPeriods)
                return "0";
            RT_FALL_THRU();
        case STAMTYPE_COUNTER:
        case STAMTYPE_RATIO_U32:
        case STAMTYPE_RATIO_U32_RESET:
        case STAMTYPE_U8:
        case STAMTYPE_U8_RESET:
        case STAMTYPE_X8:
        case STAMTYPE_X8_RESET:
        case STAMTYPE_U16:
        case STAMTYPE_U16_RESET:
        case STAMTYPE_X16:
        case STAMTYPE_X16_RESET:
        case STAMTYPE_U32:
        case STAMTYPE_U32_RESET:
        case STAMTYPE_X32:
        case STAMTYPE_X32_RESET:
        case STAMTYPE_U64:
        case STAMTYPE_U64_RESET:
        case STAMTYPE_X64:
        case STAMTYPE_X64_RESET:
        case STAMTYPE_BOOL:
        case STAMTYPE_BOOL_RESET:
            return formatNumberSigned(sz, pNode->i64Delta);
        default:
            return "";
    }
}


QVariant
VBoxDbgStatsModel::data(const QModelIndex &a_rIndex, int a_eRole) const
{
    unsigned iCol = a_rIndex.column();
    if (iCol >= DBGGUI_STATS_COLUMNS)
        return QVariant();

    if (a_eRole == Qt::DisplayRole)
    {
        PDBGGUISTATSNODE pNode = nodeFromIndex(a_rIndex);
        if (!pNode)
            return QVariant();

        switch (iCol)
        {
            case 0:
                return QString(pNode->pszName);
            case 1:
                return strUnit(pNode);
            case 2:
                return strValueTimes(pNode);
            case 3:
                return strMinValue(pNode);
            case 4:
                return strAvgValue(pNode);
            case 5:
                return strMaxValue(pNode);
            case 6:
                return strTotalValue(pNode);
            case 7:
                return strDeltaValue(pNode);
            case 8:
                return pNode->pDescStr ? QString(*pNode->pDescStr) : QString("");
            default:
                AssertCompile(DBGGUI_STATS_COLUMNS == 9);
                return QVariant();
        }
    }
    else if (a_eRole == Qt::TextAlignmentRole)
        switch (iCol)
        {
            case 0:
            case 1:
                return QVariant();
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                return (int)(Qt::AlignRight | Qt::AlignVCenter);
            case 8:
                return QVariant();
            default:
                AssertCompile(DBGGUI_STATS_COLUMNS == 9);
                return QVariant(); /* bug */
        }
    return QVariant();
}


/*static*/ void
VBoxDbgStatsModel::stringifyNodeNoRecursion(PDBGGUISTATSNODE a_pNode, QString &a_rString)
{
    /*
     * Get the path, padding it to 32-chars and add it to the string.
     */
    char szBuf[1024];
    ssize_t off = getNodePath(a_pNode, szBuf, sizeof(szBuf) - 2);
    AssertReturnVoid(off >= 0);
    if (off < 32)
    {
        memset(&szBuf[off], ' ', 32 - off);
        szBuf[32] = '\0';
        off = 32;
    }
    szBuf[off++] = ' ';
    szBuf[off]   = '\0';
    a_rString += szBuf;

    /*
     * The following is derived from stamR3PrintOne, except
     * we print to szBuf, do no visibility checks and can skip
     * the path bit.
     */
    switch (a_pNode->enmType)
    {
        case STAMTYPE_COUNTER:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8llu %s", a_pNode->Data.Counter.c, a_pNode->pszUnit);
            break;

        case STAMTYPE_PROFILE:
        case STAMTYPE_PROFILE_ADV:
        {
            uint64_t u64 = a_pNode->Data.Profile.cPeriods ? a_pNode->Data.Profile.cPeriods : 1;
            RTStrPrintf(szBuf, sizeof(szBuf),
                        "%8llu %s (%12llu ticks, %7llu times, max %9llu, min %7lld)",
                        a_pNode->Data.Profile.cTicks / u64, a_pNode->pszUnit,
                        a_pNode->Data.Profile.cTicks, a_pNode->Data.Profile.cPeriods, a_pNode->Data.Profile.cTicksMax, a_pNode->Data.Profile.cTicksMin);
            break;
        }

        case STAMTYPE_RATIO_U32:
        case STAMTYPE_RATIO_U32_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf),
                        "%8u:%-8u %s",
                        a_pNode->Data.RatioU32.u32A, a_pNode->Data.RatioU32.u32B, a_pNode->pszUnit);
            break;

        case STAMTYPE_CALLBACK:
            if (a_pNode->Data.pStr)
                a_rString += *a_pNode->Data.pStr;
            RTStrPrintf(szBuf, sizeof(szBuf), " %s", a_pNode->pszUnit);
            break;

        case STAMTYPE_U8:
        case STAMTYPE_U8_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8u %s", a_pNode->Data.u8, a_pNode->pszUnit);
            break;

        case STAMTYPE_X8:
        case STAMTYPE_X8_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8x %s", a_pNode->Data.u8, a_pNode->pszUnit);
            break;

        case STAMTYPE_U16:
        case STAMTYPE_U16_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8u %s", a_pNode->Data.u16, a_pNode->pszUnit);
            break;

        case STAMTYPE_X16:
        case STAMTYPE_X16_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8x %s", a_pNode->Data.u16, a_pNode->pszUnit);
            break;

        case STAMTYPE_U32:
        case STAMTYPE_U32_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8u %s", a_pNode->Data.u32, a_pNode->pszUnit);
            break;

        case STAMTYPE_X32:
        case STAMTYPE_X32_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8x %s", a_pNode->Data.u32, a_pNode->pszUnit);
            break;

        case STAMTYPE_U64:
        case STAMTYPE_U64_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8llu %s", a_pNode->Data.u64, a_pNode->pszUnit);
            break;

        case STAMTYPE_X64:
        case STAMTYPE_X64_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%8llx %s", a_pNode->Data.u64, a_pNode->pszUnit);
            break;

        case STAMTYPE_BOOL:
        case STAMTYPE_BOOL_RESET:
            RTStrPrintf(szBuf, sizeof(szBuf), "%s %s", a_pNode->Data.f ? "true    " : "false   ", a_pNode->pszUnit);
            break;

        default:
            AssertMsgFailed(("enmType=%d\n", a_pNode->enmType));
            return;
    }

    a_rString += szBuf;
}


/*static*/ void
VBoxDbgStatsModel::stringifyNode(PDBGGUISTATSNODE a_pNode, QString &a_rString)
{
    /* this node (if it has data) */
    if (a_pNode->enmType != STAMTYPE_INVALID)
    {
        if (!a_rString.isEmpty())
            a_rString += "\n";
        stringifyNodeNoRecursion(a_pNode, a_rString);
    }

    /* the children */
    uint32_t const cChildren = a_pNode->cChildren;
    for (uint32_t i = 0; i < cChildren; i++)
        stringifyNode(a_pNode->papChildren[i], a_rString);
}


void
VBoxDbgStatsModel::stringifyTree(QModelIndex &a_rRoot, QString &a_rString) const
{
    PDBGGUISTATSNODE pRoot = a_rRoot.isValid() ? nodeFromIndex(a_rRoot) : m_pRoot;
    if (pRoot)
        stringifyNode(pRoot, a_rString);
}


void
VBoxDbgStatsModel::copyTreeToClipboard(QModelIndex &a_rRoot) const
{
    QString String;
    stringifyTree(a_rRoot, String);

    QClipboard *pClipboard = QApplication::clipboard();
    if (pClipboard)
        pClipboard->setText(String, QClipboard::Clipboard);
}


/*static*/ void
VBoxDbgStatsModel::logNode(PDBGGUISTATSNODE a_pNode, bool a_fReleaseLog)
{
    /* this node (if it has data) */
    if (a_pNode->enmType != STAMTYPE_INVALID)
    {
        QString SelfStr;
        stringifyNodeNoRecursion(a_pNode, SelfStr);
        QByteArray SelfByteArray = SelfStr.toUtf8();
        if (a_fReleaseLog)
            RTLogRelPrintf("%s\n", SelfByteArray.constData());
        else
            RTLogPrintf("%s\n", SelfByteArray.constData());
    }

    /* the children */
    uint32_t const cChildren = a_pNode->cChildren;
    for (uint32_t i = 0; i < cChildren; i++)
        logNode(a_pNode->papChildren[i], a_fReleaseLog);
}


void
VBoxDbgStatsModel::logTree(QModelIndex &a_rRoot, bool a_fReleaseLog) const
{
    PDBGGUISTATSNODE pRoot = a_rRoot.isValid() ? nodeFromIndex(a_rRoot) : m_pRoot;
    if (pRoot)
        logNode(pRoot, a_fReleaseLog);
}







/*
 *
 *      V B o x D b g S t a t s M o d e l V M
 *      V B o x D b g S t a t s M o d e l V M
 *      V B o x D b g S t a t s M o d e l V M
 *
 *
 */


VBoxDbgStatsModelVM::VBoxDbgStatsModelVM(VBoxDbgGui *a_pDbgGui, QString &a_rPatStr, QObject *a_pParent, PCVMMR3VTABLE a_pVMM)
    : VBoxDbgStatsModel(a_pParent), VBoxDbgBase(a_pDbgGui), m_pVMM(a_pVMM)
{
    /*
     * Create a model containing the STAM entries matching the pattern.
     * (The original idea was to get everything and rely on some hide/visible
     * flag that it turned out didn't exist.)
     */
    PDBGGUISTATSNODE pTree = createNewTree(a_rPatStr);
    setRootNode(pTree);
}


VBoxDbgStatsModelVM::~VBoxDbgStatsModelVM()
{
    /* nothing to do here. */
}


bool
VBoxDbgStatsModelVM::updateStatsByPattern(const QString &a_rPatStr)
{
    /** @todo the way we update this stuff is independent of the source (XML, file, STAM), our only
     * ASSUMPTION is that the input is strictly ordered by (fully slashed) name. So, all this stuff
     * should really move up into the parent class. */
    bool fRc = updatePrepare();
    if (fRc)
    {
        int rc = stamEnum(a_rPatStr, updateCallback, this);
        fRc = updateDone(RT_SUCCESS(rc));
    }
    return fRc;
}


void
VBoxDbgStatsModelVM::resetStatsByPattern(QString const &a_rPatStr)
{
    stamReset(a_rPatStr);
}


/*static*/ DECLCALLBACK(int)
VBoxDbgStatsModelVM::createNewTreeCallback(const char *pszName, STAMTYPE enmType, void *pvSample, STAMUNIT enmUnit,
                                           const char *pszUnit, STAMVISIBILITY enmVisibility, const char *pszDesc, void *pvUser)
{
    PDBGGUISTATSNODE pRoot = (PDBGGUISTATSNODE)pvUser;
    Log3(("createNewTreeCallback: %s\n", pszName));
    RT_NOREF(enmUnit);

    /*
     * Skip the ones which shouldn't be visible in the GUI.
     */
    if (enmVisibility == STAMVISIBILITY_NOT_GUI)
        return 0;

    /*
     * Perform a mkdir -p like operation till we've walked / created the entire path down
     * to the node specfied node. Remember the last node as that will be the one we will
     * stuff the data into.
     */
    AssertReturn(*pszName == '/' && pszName[1] != '/', VERR_INTERNAL_ERROR);
    PDBGGUISTATSNODE pNode = pRoot;
    const char *pszCur = pszName + 1;
    while (*pszCur)
    {
        /* find the end of this component. */
        const char *pszNext = strchr(pszCur, '/');
        if (!pszNext)
            pszNext = strchr(pszCur, '\0');
        size_t cchCur = pszNext - pszCur;

        /* Create it if it doesn't exist (it will be last if it exists). */
        if (    !pNode->cChildren
            ||  strncmp(pNode->papChildren[pNode->cChildren - 1]->pszName, pszCur, cchCur)
            ||  pNode->papChildren[pNode->cChildren - 1]->pszName[cchCur])
        {
            pNode = createAndInsertNode(pNode, pszCur, pszNext - pszCur, UINT32_MAX);
            if (!pNode)
                return VERR_NO_MEMORY;
        }
        else
            pNode = pNode->papChildren[pNode->cChildren - 1];

        /* Advance */
        pszCur = *pszNext ? pszNext + 1 : pszNext;
    }

    /*
     * Save the data.
     */
    return initNode(pNode, enmType, pvSample, pszUnit, pszDesc);
}


PDBGGUISTATSNODE
VBoxDbgStatsModelVM::createNewTree(QString &a_rPatStr)
{
    PDBGGUISTATSNODE pRoot = createRootNode();
    if (pRoot)
    {
        int rc = stamEnum(a_rPatStr, createNewTreeCallback, pRoot);
        if (RT_SUCCESS(rc))
            return pRoot;

        /* failed, cleanup. */
        destroyTree(pRoot);
    }

    return NULL;
}








/*
 *
 *      V B o x D b g S t a t s V i e w
 *      V B o x D b g S t a t s V i e w
 *      V B o x D b g S t a t s V i e w
 *
 *
 */


VBoxDbgStatsView::VBoxDbgStatsView(VBoxDbgGui *a_pDbgGui, VBoxDbgStatsModel *a_pModel, VBoxDbgStats *a_pParent/* = NULL*/)
    : QTreeView(a_pParent), VBoxDbgBase(a_pDbgGui), m_pModel(a_pModel), m_PatStr(), m_pParent(a_pParent),
    m_pLeafMenu(NULL), m_pBranchMenu(NULL), m_pViewMenu(NULL), m_pCurMenu(NULL), m_CurIndex()

{
    /*
     * Set the model and view defaults.
     */
    setRootIsDecorated(true);
    setModel(m_pModel);
    QModelIndex RootIdx = m_pModel->getRootIndex(); /* This should really be QModelIndex(), but Qt on darwin does wrong things then. */
    setRootIndex(RootIdx);
    setItemsExpandable(true);
    setAlternatingRowColors(true);
    setSelectionBehavior(SelectRows);
    setSelectionMode(SingleSelection);
    /// @todo sorting setSortingEnabled(true);

    /*
     * Create and setup the actions.
     */
    m_pExpandAct   = new QAction("Expand Tree", this);
    m_pCollapseAct = new QAction("Collapse Tree", this);
    m_pRefreshAct  = new QAction("&Refresh", this);
    m_pResetAct    = new QAction("Rese&t", this);
    m_pCopyAct     = new QAction("&Copy", this);
    m_pToLogAct    = new QAction("To &Log", this);
    m_pToRelLogAct = new QAction("T&o Release Log", this);
    m_pAdjColumns  = new QAction("&Adjust Columns", this);

    m_pCopyAct->setShortcut(QKeySequence::Copy);
    m_pExpandAct->setShortcut(QKeySequence("Ctrl+E"));
    m_pCollapseAct->setShortcut(QKeySequence("Ctrl+D"));
    m_pRefreshAct->setShortcut(QKeySequence("Ctrl+R"));
    m_pResetAct->setShortcut(QKeySequence("Alt+R"));
    m_pToLogAct->setShortcut(QKeySequence("Ctrl+Z"));
    m_pToRelLogAct->setShortcut(QKeySequence("Alt+Z"));
    m_pAdjColumns->setShortcut(QKeySequence("Ctrl+A"));

    addAction(m_pCopyAct);
    addAction(m_pExpandAct);
    addAction(m_pCollapseAct);
    addAction(m_pRefreshAct);
    addAction(m_pResetAct);
    addAction(m_pToLogAct);
    addAction(m_pToRelLogAct);
    addAction(m_pAdjColumns);

    connect(m_pExpandAct,   SIGNAL(triggered(bool)), this, SLOT(actExpand()));
    connect(m_pCollapseAct, SIGNAL(triggered(bool)), this, SLOT(actCollapse()));
    connect(m_pRefreshAct,  SIGNAL(triggered(bool)), this, SLOT(actRefresh()));
    connect(m_pResetAct,    SIGNAL(triggered(bool)), this, SLOT(actReset()));
    connect(m_pCopyAct,     SIGNAL(triggered(bool)), this, SLOT(actCopy()));
    connect(m_pToLogAct,    SIGNAL(triggered(bool)), this, SLOT(actToLog()));
    connect(m_pToRelLogAct, SIGNAL(triggered(bool)), this, SLOT(actToRelLog()));
    connect(m_pAdjColumns,  SIGNAL(triggered(bool)), this, SLOT(actAdjColumns()));


    /*
     * Create the menus and populate them.
     */
    setContextMenuPolicy(Qt::DefaultContextMenu);

    m_pLeafMenu = new QMenu();
    m_pLeafMenu->addAction(m_pCopyAct);
    m_pLeafMenu->addAction(m_pRefreshAct);
    m_pLeafMenu->addAction(m_pResetAct);
    m_pLeafMenu->addAction(m_pToLogAct);
    m_pLeafMenu->addAction(m_pToRelLogAct);

    m_pBranchMenu = new QMenu(this);
    m_pBranchMenu->addAction(m_pCopyAct);
    m_pBranchMenu->addAction(m_pRefreshAct);
    m_pBranchMenu->addAction(m_pResetAct);
    m_pBranchMenu->addAction(m_pToLogAct);
    m_pBranchMenu->addAction(m_pToRelLogAct);
    m_pBranchMenu->addSeparator();
    m_pBranchMenu->addAction(m_pExpandAct);
    m_pBranchMenu->addAction(m_pCollapseAct);

    m_pViewMenu = new QMenu();
    m_pViewMenu->addAction(m_pCopyAct);
    m_pViewMenu->addAction(m_pRefreshAct);
    m_pViewMenu->addAction(m_pResetAct);
    m_pViewMenu->addAction(m_pToLogAct);
    m_pViewMenu->addAction(m_pToRelLogAct);
    m_pViewMenu->addSeparator();
    m_pViewMenu->addAction(m_pExpandAct);
    m_pViewMenu->addAction(m_pCollapseAct);
    m_pViewMenu->addSeparator();
    m_pViewMenu->addAction(m_pAdjColumns);

    /* the header menu */
    QHeaderView *pHdrView = header();
    pHdrView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(pHdrView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(headerContextMenuRequested(const QPoint &)));
}


VBoxDbgStatsView::~VBoxDbgStatsView()
{
    m_pParent = NULL;
    m_pCurMenu = NULL;
    m_CurIndex = QModelIndex();

#define DELETE_IT(m) if (m) { delete m; m = NULL; } else do {} while (0)
    DELETE_IT(m_pModel);

    DELETE_IT(m_pLeafMenu);
    DELETE_IT(m_pBranchMenu);
    DELETE_IT(m_pViewMenu);

    DELETE_IT(m_pExpandAct);
    DELETE_IT(m_pCollapseAct);
    DELETE_IT(m_pRefreshAct);
    DELETE_IT(m_pResetAct);
    DELETE_IT(m_pCopyAct);
    DELETE_IT(m_pToLogAct);
    DELETE_IT(m_pToRelLogAct);
    DELETE_IT(m_pAdjColumns);
#undef DELETE_IT
}


void
VBoxDbgStatsView::updateStats(const QString &rPatStr)
{
    m_PatStr = rPatStr;
    if (m_pModel->updateStatsByPattern(rPatStr))
        setRootIndex(m_pModel->getRootIndex()); /* hack */
}


void
VBoxDbgStatsView::resizeColumnsToContent()
{
    for (int i = 0; i <= 8; i++)
    {
        resizeColumnToContents(i);
        /* Some extra room for distinguishing numbers better in Value, Min, Avg, Max, Total, dInt columns. */
        if (i >= 2 && i <= 7)
            setColumnWidth(i, columnWidth(i) + 10);
    }
}


/*static*/ bool
VBoxDbgStatsView::expandMatchingCallback(PDBGGUISTATSNODE pNode, QModelIndex const &a_rIndex,
                                         const char *pszFullName, void *pvUser)
{
    VBoxDbgStatsView *pThis = (VBoxDbgStatsView *)pvUser;

    pThis->setExpanded(a_rIndex, true);

    QModelIndex ParentIndex = pThis->m_pModel->parent(a_rIndex);
    while (ParentIndex.isValid() && !pThis->isExpanded(ParentIndex))
    {
        pThis->setExpanded(ParentIndex, true);
        ParentIndex = pThis->m_pModel->parent(ParentIndex);
    }

    RT_NOREF(pNode, pszFullName);
    return true;
}


void
VBoxDbgStatsView::expandMatching(const QString &rPatStr)
{
    m_pModel->iterateStatsByPattern(rPatStr, expandMatchingCallback, this);
}


void
VBoxDbgStatsView::setSubTreeExpanded(QModelIndex const &a_rIndex, bool a_fExpanded)
{
    int cRows = m_pModel->rowCount(a_rIndex);
    if (a_rIndex.model())
        for (int i = 0; i < cRows; i++)
            setSubTreeExpanded(a_rIndex.model()->index(i, 0, a_rIndex), a_fExpanded);
    setExpanded(a_rIndex, a_fExpanded);
}


void
VBoxDbgStatsView::contextMenuEvent(QContextMenuEvent *a_pEvt)
{
    /*
     * Get the selected item.
     * If it's a mouse event select the item under the cursor (if any).
     */
    QModelIndex Idx;
    if (a_pEvt->reason() == QContextMenuEvent::Mouse)
    {
        Idx = indexAt(a_pEvt->pos());
        if (Idx.isValid())
            setCurrentIndex(Idx);
    }
    else
    {
        QModelIndexList SelIdx = selectedIndexes();
        if (!SelIdx.isEmpty())
            Idx = SelIdx.at(0);
    }

    /*
     * Popup the corresponding menu.
     */
    QMenu *pMenu;
    if (!Idx.isValid())
        pMenu = m_pViewMenu;
    else if (m_pModel->hasChildren(Idx))
        pMenu = m_pBranchMenu;
    else
        pMenu = m_pLeafMenu;
    if (pMenu)
    {
        m_pRefreshAct->setEnabled(!Idx.isValid() || Idx == m_pModel->getRootIndex());
        m_CurIndex = Idx;
        m_pCurMenu = pMenu;

        pMenu->exec(a_pEvt->globalPos());

        m_pCurMenu = NULL;
        m_CurIndex = QModelIndex();
        if (m_pRefreshAct)
            m_pRefreshAct->setEnabled(true);
    }
    a_pEvt->accept();
}


void
VBoxDbgStatsView::headerContextMenuRequested(const QPoint &a_rPos)
{
    /*
     * Show the view menu.
     */
    if (m_pViewMenu)
    {
        m_pRefreshAct->setEnabled(true);
        m_CurIndex = m_pModel->getRootIndex();
        m_pCurMenu = m_pViewMenu;

        m_pViewMenu->exec(header()->mapToGlobal(a_rPos));

        m_pCurMenu = NULL;
        m_CurIndex = QModelIndex();
        if (m_pRefreshAct)
            m_pRefreshAct->setEnabled(true);
    }
}


void
VBoxDbgStatsView::actExpand()
{
    QModelIndex Idx = m_pCurMenu ? m_CurIndex : currentIndex();
    if (Idx.isValid())
        setSubTreeExpanded(Idx, true /* a_fExpanded */);
}


void
VBoxDbgStatsView::actCollapse()
{
    QModelIndex Idx = m_pCurMenu ? m_CurIndex : currentIndex();
    if (Idx.isValid())
        setSubTreeExpanded(Idx, false /* a_fExpanded */);
}


void
VBoxDbgStatsView::actRefresh()
{
    QModelIndex Idx = m_pCurMenu ? m_CurIndex : currentIndex();
    if (!Idx.isValid() || Idx == m_pModel->getRootIndex())
    {
        if (m_pModel->updateStatsByPattern(m_PatStr))
            setRootIndex(m_pModel->getRootIndex()); /* hack */
    }
    else
        m_pModel->updateStatsByIndex(Idx);
}


void
VBoxDbgStatsView::actReset()
{
    QModelIndex Idx = m_pCurMenu ? m_CurIndex : currentIndex();
    if (!Idx.isValid() || Idx == m_pModel->getRootIndex())
        m_pModel->resetStatsByPattern(m_PatStr);
    else
        m_pModel->resetStatsByIndex(Idx);
}


void
VBoxDbgStatsView::actCopy()
{
    QModelIndex Idx = m_pCurMenu ? m_CurIndex : currentIndex();
    m_pModel->copyTreeToClipboard(Idx);
}


void
VBoxDbgStatsView::actToLog()
{
    QModelIndex Idx = m_pCurMenu ? m_CurIndex : currentIndex();
    m_pModel->logTree(Idx, false /* a_fReleaseLog */);
}


void
VBoxDbgStatsView::actToRelLog()
{
    QModelIndex Idx = m_pCurMenu ? m_CurIndex : currentIndex();
    m_pModel->logTree(Idx, true /* a_fReleaseLog */);
}


void
VBoxDbgStatsView::actAdjColumns()
{
    resizeColumnsToContent();
}






/*
 *
 *      V B o x D b g S t a t s
 *      V B o x D b g S t a t s
 *      V B o x D b g S t a t s
 *
 *
 */


VBoxDbgStats::VBoxDbgStats(VBoxDbgGui *a_pDbgGui, const char *pszFilter /*= NULL*/, const char *pszExpand /*= NULL*/,
                           unsigned uRefreshRate/* = 0*/, QWidget *pParent/* = NULL*/)
    : VBoxDbgBaseWindow(a_pDbgGui, pParent, "Statistics")
    , m_PatStr(pszFilter), m_pPatCB(NULL), m_uRefreshRate(0), m_pTimer(NULL), m_pView(NULL)
{
    /* Delete dialog on close: */
    setAttribute(Qt::WA_DeleteOnClose);

    /*
     * On top, a horizontal box with the pattern field, buttons and refresh interval.
     */
    QHBoxLayout *pHLayout = new QHBoxLayout;

    QLabel *pLabel = new QLabel(" Pattern ");
    pHLayout->addWidget(pLabel);
    pLabel->setMaximumSize(pLabel->sizeHint());
    pLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    m_pPatCB = new QComboBox();
    m_pPatCB->setCompleter(0);
    pHLayout->addWidget(m_pPatCB);
    if (!m_PatStr.isEmpty())
        m_pPatCB->addItem(m_PatStr);
    m_pPatCB->setDuplicatesEnabled(false);
    m_pPatCB->setEditable(true);
    connect(m_pPatCB, SIGNAL(activated(const QString &)), this, SLOT(apply(const QString &)));

    QPushButton *pPB = new QPushButton("&All");
    pHLayout->addWidget(pPB);
    pPB->setMaximumSize(pPB->sizeHint());
    connect(pPB, SIGNAL(clicked()), this, SLOT(applyAll()));

    pLabel = new QLabel("  Interval ");
    pHLayout->addWidget(pLabel);
    pLabel->setMaximumSize(pLabel->sizeHint());
    pLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QSpinBox *pSB = new QSpinBox();
    pHLayout->addWidget(pSB);
    pSB->setMinimum(0);
    pSB->setMaximum(60);
    pSB->setSingleStep(1);
    pSB->setValue(uRefreshRate);
    pSB->setSuffix(" s");
    pSB->setWrapping(false);
    pSB->setButtonSymbols(QSpinBox::PlusMinus);
    pSB->setMaximumSize(pSB->sizeHint());
    connect(pSB, SIGNAL(valueChanged(int)), this, SLOT(setRefresh(int)));

    /*
     * Create the tree view and setup the layout.
     */
    VBoxDbgStatsModelVM *pModel = new VBoxDbgStatsModelVM(a_pDbgGui, m_PatStr, NULL, a_pDbgGui->getVMMFunctionTable());
    m_pView = new VBoxDbgStatsView(a_pDbgGui, pModel, this);

    QWidget *pHBox = new QWidget;
    pHBox->setLayout(pHLayout);

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->addWidget(pHBox);
    pVLayout->addWidget(m_pView);
    setLayout(pVLayout);

    /*
     * Resize the columns.
     * Seems this has to be done with all nodes expanded.
     */
    m_pView->expandAll();
    m_pView->resizeColumnsToContent();
    m_pView->collapseAll();

    if (pszExpand && *pszExpand)
        m_pView->expandMatching(QString(pszExpand));

    /*
     * Create a refresh timer and start it.
     */
    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    setRefresh(uRefreshRate);

    /*
     * And some shortcuts.
     */
    m_pFocusToPat = new QAction("", this);
    m_pFocusToPat->setShortcut(QKeySequence("Ctrl+L"));
    addAction(m_pFocusToPat);
    connect(m_pFocusToPat, SIGNAL(triggered(bool)), this, SLOT(actFocusToPat()));
}


VBoxDbgStats::~VBoxDbgStats()
{
    if (m_pTimer)
    {
        delete m_pTimer;
        m_pTimer = NULL;
    }

    if (m_pPatCB)
    {
        delete m_pPatCB;
        m_pPatCB = NULL;
    }

    if (m_pView)
    {
        delete m_pView;
        m_pView = NULL;
    }
}


void
VBoxDbgStats::closeEvent(QCloseEvent *a_pCloseEvt)
{
    a_pCloseEvt->accept();
}


void
VBoxDbgStats::apply(const QString &Str)
{
    m_PatStr = Str;
    refresh();
}


void
VBoxDbgStats::applyAll()
{
    apply("");
}



void
VBoxDbgStats::refresh()
{
    m_pView->updateStats(m_PatStr);
}


void
VBoxDbgStats::setRefresh(int iRefresh)
{
    if ((unsigned)iRefresh != m_uRefreshRate)
    {
        if (!m_uRefreshRate || iRefresh)
            m_pTimer->start(iRefresh * 1000);
        else
            m_pTimer->stop();
        m_uRefreshRate = iRefresh;
    }
}


void
VBoxDbgStats::actFocusToPat()
{
    if (!m_pPatCB->hasFocus())
        m_pPatCB->setFocus(Qt::ShortcutFocusReason);
}

