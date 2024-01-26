/** @file
 * IPRT - Abstract Table/Trees.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_table_h
#define IPRT_INCLUDED_table_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

/** @defgroup grp_rt_tab    RTTab - Generic Tree and Table Interface.
 * @ingroup grp_rt
 * @{
 */

RT_C_DECLS_BEGIN

/** Pointer to an allocator. */
typedef struct RTTABALLOCATOR *PRTTABALLOCATOR;

/**
 * Allocates memory.
 *
 * @returns Pointer to the allocated memory.
 * @returns NULL on failure. (don't throw!)
 * @param   pAllocator  The allocator structure.
 * @param   cb          The number of bytes to allocate. (Never 0.)
 */
typedef DECLCALLBACKTYPE(void *, FNRTTABALLOC,(PRTTABALLOCATOR pAllocator, size_t cb));
/** Pointer to a FNRTTABALLOC() function. */
typedef FNRTTABALLOC *PFNRTTABALLOC;

/**
 * Frees memory.
 *
 * @param   pAllocator  The allocator structure.
 * @param   pv          The memory to free. (can be NULL)
 */
typedef DECLCALLBACKTYPE(void *, FNRTTABFREE,(PRTTABALLOCATOR pAllocator, void *pv));
/** Pointer to a FNRTTABFREE() function. */
typedef FNRTTABFREE *PFNRTTABFREE;

/**
 * The allocator structure.
 * (Hint: use this as like 'base class' for your custom allocators.)
 */
typedef struct RTTABALLOCATOR
{
    /** The allocation function. */
    PFNRTTABALLOC  pfnAlloc;
    /** The free function. */
    PFNRTTABFREE   pfnFree;
} RTTABALLOCATOR;

/**
 * Gets the default allocator.
 *
 * @returns Pointer to the default allocator.
 */
RTDECL(RTTABALLOCATOR) RTTabDefaultAllocator(void);


/**
 * Compares two table items.
 *
 * @returns 0 if equal
 * @returns <0 if pvItem1 is less than pvItem2 (pvItem2 is then greater than pvItem1).
 * @returns >0 if pvItem1 is less than pvItem2 (pvItem1 is then greater than pvItem2).
 *
 * @param   pvItem1     The first item.
 * @param   pvItem2     The second item.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACKTYPE(int, FNRTTABCOMP,(const void *pvItem1, const void *pvItem2, void *pvUser));
/** Pointer to a FNRTTABCOMP() function. */
typedef FNRTTABCOMP *PFNRTTABCOMP;

/**
 * Duplicates a table item.
 * This is used when duplicating or copying a table.
 *
 * @returns Pointer to the copy.
 * @returns NULL on failure.
 *
 * @param   pvItem      The item to copy.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACKTYPE(void *, FNRTTABDUPLICATE,(const void *pvItem, void *pvUser));
/** Pointer to a FNRTTABDUPLICATE() function. */
typedef FNRTTABDUPLICATE *PFNRTTABDUPLICATE;

/**
 * Callback function for doing something with an item.
 *
 * What exactly we're doing is specific to the context of the call.
 *
 * @param   pvItem      The item.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACKTYPE(void, FNRTTABCALLBACK,(const void *pvItem, void *pvUser));
/** Pointer to a FNRTTABCALLBACK() function. */
typedef FNRTTABCALLBACK *PFNRTTABCALLBACK;


/** Pointer to const table operations. */
typedef const struct RTTABOPS              *PCRTTABOPS;
/** Pointer to a table. */
typedef struct RTTAB                       *PRTTAB;
/** Pointer to a const table. */
typedef const struct RTTAB                 *PCRTTAB;
/** Pointer to a traverser. */
typedef struct RTTABTRAVERSER              *PRTTABTRAVERSER;
/** Pointer to a const traverser. */
typedef const struct RTTABTRAVERSER        *PCRTTABTRAVERSER;
/** Pointer to a traverser core. */
typedef struct RTTABTRAVERSERCORE          *PRTTABTRAVERSERCORE;
/** Pointer to a const traverser core. */
typedef const struct RTTABTRAVERSERCORE    *PCRTTABTRAVERSERCORE;


/**
 * Table operations.
 */
typedef struct RTTABOPS
{
    /**
     * Create a table.
     *
     * @returns Pointer to the new table.
     * @returns NULL if we're out of memory or some other resource.
     * @param   pOps            The table operations.
     * @param   fCreateFlags    The table type specific creation flags.
     * @param   pAllocator      Custom allocator. Pass NULL for the default allocator.
     * @param   pfnComp         The comparision function.
     */
    DECLCALLBACKMEMBER(PRTTAB, pfnCreate,(PCRTTABOPS pOps, unsigned fCreateFlags, PRTTABALLOCATOR pAllocator, PFNRTTABCOMP pfnComp));

    /**
     * Duplicates a table to a table of the same type.
     *
     * @returns Pointer to the new table.
     * @returns NULL if we're out of memory or some other resource.
     * @param   pTab            The table to duplicate.
     * @param   pfnDuplicate    Pointer to the item duplication function. If NULL the new table will
     *                          be referencing the same data as the old one.
     * @param   pfnNewCB        Callback which is called for all the items in the new table. Optional.
     * @param   pAllocator      Custom allocator. Pass NULL to use the same allocator as pTab.
     */
    DECLCALLBACKMEMBER(PRTTAB, pfnDuplicate,(PCRTTAB pTab, PFNRTTABDUPLICATE pfnDuplicate, PFNRTTABCALLBACK pfnNewCB, PRTTABALLOCATOR pAllocator));

    /**
     * Destroys a table.
     *
     * @param   pTab        The table to destroy.
     */
    DECLCALLBACKMEMBER(void, pfnDestroy,(PRTTAB pTab));

    /**
     * Inserts an item into the table, if a matching item is encountered
     * the pointer to the pointer to it will be returned.
     *
     * @returns Pointer to the item pointer in the table.
     *          This can be used to replace existing items (don't break anything, dude).
     * @returns NULL if we failed to allocate memory for the new node.
     * @param   pTab            The table.
     * @param   pvItem          The item which will be inserted if an matching item was not found in the table.
     */
    DECLCALLBACKMEMBER(void **, pfnProbe,(PRTTAB pTab, void *pvItem));

    /**
     * Inserts an item into the table, fail if a matching item exists.
     *
     * @returns NULL on success and allocation failure.
     * @returns Pointer to the matching item.
     * @param   pTab            The table.
     * @param   pvItem          The item which is to be inserted.
     */
    DECLCALLBACKMEMBER(void *, pfnInsert,(PRTTAB pTab, void *pvItem));

    /**
     * Inserts an item into the table, if a matching item is encountered
     * it will be replaced and returned.
     *
     * @returns NULL if inserted and allocation failure.
     * @returns Pointer to the replaced item.
     * @param   pTab            The table.
     * @param   pvItem          The item which is to be inserted.
     */
    DECLCALLBACKMEMBER(void *, pfnReplace,(PRTTAB pTab, void *pvItem));

    /**
     * Removes an item from the table if found.
     *
     * @returns Pointer to the removed item.
     * @returns NULL if no item matched pvItem.
     * @param   pTab            The table.
     * @param   pvItem          The item which is to be inserted.
     */
    DECLCALLBACKMEMBER(void *, pfnRemove,(PRTTAB pTab, const void *pvItem));

    /**
     * Finds an item in the table.
     *
     * @returns Pointer to the item it found.
     * @returns NULL if no item matched pvItem.
     * @param   pTab            The table.
     * @param   pvItem          The item which is to be inserted.
     */
    DECLCALLBACKMEMBER(void *, pfnFind,(PRTTAB pTab, const void *pvItem));

    /**
     * Initializes a traverser to the NULL item.
     *
     * The NULL item is an imaginary table item before the first and after
     * the last items in the table.
     *
     * @returns Pointer to the traverser positioned at the NULL item.
     * @returns NULL on failure to allocate the traverser.
     *
     * @param   pTab            The table.
     * @param   pTravNew        Pointer to a preallocated structure. Optional.
     */
    DECLCALLBACKMEMBER(PRTTABTRAVERSERCORE, pfnTravInit,(PRTTAB pTab, PRTTABTRAVERSER pTravNew));

    /**
     * Initializes a traverser to the first item in the table.
     *
     * If the table is empty, the traverser will be positioned at the NULL item
     * like with RTTabTravInit().
     *
     * @returns Pointer to the traverser positioned at the first item or NULL item.
     * @returns NULL on failure to allocate the traverser.
     *
     * @param   pTab            The table.
     * @param   pTravNew        Pointer to a preallocated structure. Optional.
     */
    DECLCALLBACKMEMBER(PRTTABTRAVERSERCORE, pfnTravFirst,(PRTTAB pTab, PRTTABTRAVERSER pTravNew));

    /**
     * Initializes a traverser to the last item in the table.
     *
     * If the table is empty, the traverser will be positioned at the NULL item
     * like with RTTabTravInit().
     *
     * @returns Pointer to the traverser positioned at the last item or NULL item.
     * @returns NULL on failure to allocate the traverser.
     *
     * @param   pTab            The table.
     * @param   pTravNew        Pointer to a preallocated structure. Optional.
     */
    DECLCALLBACKMEMBER(PRTTABTRAVERSERCORE, pfnTravLast,(PRTTAB pTab, PRTTABTRAVERSER pTravNew));

    /**
     * Initializes a traverser to an item matching the given one.
     *
     * If the item isn't found, the traverser will be positioned at the NULL item
     * like with RTTabTravInit().
     *
     * @returns Pointer to the traverser positioned at the matching item or NULL item.
     * @returns NULL on failure to allocate the traverser.
     *
     * @param   pTab            The table.
     * @param   pTravNew        Pointer to a preallocated structure. Optional.
     * @param   pvItem          The item to find the match to.
     */
    DECLCALLBACKMEMBER(PRTTABTRAVERSERCORE, pfnTravFind,(PRTTAB pTab, PRTTABTRAVERSER pTravNew, const void *pvItem));

    /**
     * Initializes a traverser to the inserted item.
     *
     * If there already exists an item in the tree matching pvItem, the traverser
     * is positioned at that item like with RTTabTravFind().
     *
     * If the insert operation failes because of an out of memory condition, the
     * traverser will be positioned at the NULL item like with RTTabTravInit().
     *
     * @returns Pointer to the traverser positioned at the inserted, existing or NULL item.
     * @returns NULL on failure to allocate the traverser.
     *
     * @param   pTab            The table.
     * @param   pTravNew        Pointer to a preallocated structure. Optional.
     * @param   pvItem          The item to be inserted.
     */
    DECLCALLBACKMEMBER(PRTTABTRAVERSERCORE, pfnTravInsert,(PRTTAB pTab, PRTTABTRAVERSER pTravNew, void *pvItem));

    /**
     * Duplicates a traverser.
     *
     * @returns The pointer to the duplicate.
     * @returns NULL on allocation failure.
     *
     * @param   pTrav           The traverser to duplicate.
     * @param   pTravNew        Pointer to a preallocated structure. Optional.
     */
    DECLCALLBACKMEMBER(PRTTABTRAVERSERCORE, pfnTravDuplicate,(PRTTABTRAVERSERCORE pTrav, PCRTTABTRAVERSER pTravNew));

    /**
     * Frees a traverser.
     *
     * This can safely be called even if the traverser structure
     * wasn't dynamically allocated or the constructor failed.
     *
     * @param   pTrav           The traverser which is to be free.
     */
    DECLCALLBACKMEMBER(void, pfnTravFree,(PRTTABTRAVERSERCORE pTrav));

    /**
     * Gets the current item.
     *
     * @returns The current item. (NULL indicates the imaginary NULL item.)
     * @param   pTrav           The traverser.
     */
    DECLCALLBACKMEMBER(void *, pfnTravCur,(PCRTTABTRAVERSERCORE pTrav));

    /**
     * Advances to the next item.
     *
     * @returns The new current item. (NULL indicates the imaginary NULL item.)
     * @param   pTrav           The traverser.
     */
    DECLCALLBACKMEMBER(void *, pfnTravNext,(PRTTABTRAVERSERCORE pTrav));

    /**
     * Advances to the previous item.
     *
     * @returns The new current item. (NULL indicates the imaginary NULL item.)
     * @param   pTrav           The traverser.
     */
    DECLCALLBACKMEMBER(void *, pfnTravPrev,(PRTTABTRAVERSERCORE pTrav));

    /**
     * Replaces the current item.
     *
     * This has the same restrictions as RTTabProbe(), e.g. it's not permitted to
     * break the order of the table.
     *
     * @returns The replaced item.
     * @returns NULL if the current item is the NULL item. The traverser
     *          and table remains unchanged.
     * @param   pTrav           The traverser.
     * @param   pvItem          The item to be inserted.
     */
    DECLCALLBACKMEMBER(void *, pfnTravReplace,(PRTTABTRAVERSERCORE pTrav, void *pvItem));

    /** The type of table type. */
    const char                *pszType;
} RTTABOPS;

/**
 * A table.
 */
typedef struct RTTAB
{
    /** The table operations. */
    PCRTTABOPS      pOps;
    /** The function for comparing table items. */
    PFNRTTABCOMP    pfnComp;
    /** The number of items in the table. */
    RTUINT          cItems;
    /** The table generation number.
     * This must be updated whenever the table changes. */
    RTUINT          idGeneration;
} RTTAB;


/**
 * Create a table.
 *
 * @returns Pointer to the new table.
 * @returns NULL if we're out of memory or some other resource.
 * @param   pOps            The table operations.
 * @param   fCreateFlags    The table type specific creation flags.
 * @param   pAllocator      Custom allocator. Pass NULL for the default allocator.
 * @param   pfnComp         The comparision function.
 */
DECLINLINE(PRTTAB) RTTabCreate(PCRTTABOPS pOps, unsigned fCreateFlags, PRTTABALLOCATOR pAllocator, PFNRTTABCOMP pfnComp)
{
    return pOps->pfnCreate(pOps, fCreateFlags, pAllocator, pfnComp);
}

/**
 * Duplicates a table to a table of the same type.
 *
 * @returns Pointer to the new table.
 * @returns NULL if we're out of memory or some other resource.
 * @param   pTab            The table to duplicate.
 * @param   pfnDuplicate    Pointer to the item duplication function. If NULL the new table will
 *                          be referencing the same data as the old one.
 * @param   pfnNewCB        Callback which is called for all the items in the new table. Optional.
 * @param   pAllocator      Custom allocator. Pass NULL to use the same allocator as pTab.
 */
DECLINLINE(PRTTAB) RTTabDuplicate(PCRTTAB pTab, PFNRTTABDUPLICATE pfnDuplicate, PFNRTTABCALLBACK pfnNewCB, PRTTABALLOCATOR pAllocator)
{
    return pTab->pOps->pfnDuplicate(pTab, pfnDuplicate, pfnNewCB, pAllocator);
}

/**
 * Destroys a table.
 *
 * @param   pTab        The table to destroy.
 */
DECLINLINE(void) RTTabDestroy(PRTTAB pTab)
{
    pTab->pOps->pfnDestroy(pTab);
}

/**
 * Count the item in the table.
 *
 * @returns Number of items in the table.
 * @param   pTab            The table to count.
 */
DECLINLINE(RTUINT) RTTabCount(PRTTAB pTab)
{
    return pTab->cItems;
}

/**
 * Inserts an item into the table, if a matching item is encountered
 * the pointer to the pointer to it will be returned.
 *
 * @returns Pointer to the item pointer in the table.
 *          This can be used to replace existing items (don't break anything, dude).
 * @returns NULL if we failed to allocate memory for the new node.
 * @param   pTab            The table.
 * @param   pvItem          The item which will be inserted if an matching item was not found in the table.
 */
DECLINLINE(void **) RTTabProbe(PRTTAB pTab, void *pvItem)
{
    return pTab->pOps->pfnProbe(pTab, pvItem);
}

/**
 * Inserts an item into the table, fail if a matching item exists.
 *
 * @returns NULL on success and allocation failure.
 * @returns Pointer to the matching item.
 * @param   pTab            The table.
 * @param   pvItem          The item which is to be inserted.
 */
DECLINLINE(void *) RTTabInsert(PRTTAB pTab, void *pvItem)
{
    return pTab->pOps->pfnInsert(pTab, pvItem);
}

/**
 * Inserts an item into the table, if a matching item is encountered
 * it will be replaced and returned.
 *
 * @returns NULL if inserted and allocation failure.
 * @returns Pointer to the replaced item.
 * @param   pTab            The table.
 * @param   pvItem          The item which is to be inserted.
 */
DECLINLINE(void *) RTTabReplace(PRTTAB pTab, void *pvItem)
{
    return pTab->pOps->pfnReplace(pTab, pvItem);
}

/**
 * Removes an item from the table if found.
 *
 * @returns Pointer to the removed item.
 * @returns NULL if no item matched pvItem.
 * @param   pTab            The table.
 * @param   pvItem          The item which is to be inserted.
 */
DECLINLINE(void *) RTTabRemove(PRTTAB pTab, const void *pvItem)
{
    return pTab->pOps->pfnRemove(pTab, pvItem);
}

/**
 * Finds an item in the table.
 *
 * @returns Pointer to the item it found.
 * @returns NULL if no item matched pvItem.
 * @param   pTab            The table.
 * @param   pvItem          The item to find the match to.
 */
DECLINLINE(void *) RTTabFind(PRTTAB pTab, const void *pvItem)
{
    return pTab->pOps->pfnFind(pTab, pvItem);
}


/**
 * Common traverser core.
 */
typedef struct RTTABTRAVERSERCORE
{
    /** The table being traversed. */
    PRTTAB      pTab;
    /** Indicates that this traverser was allocated. */
    bool        fAllocated;
    /** The table generation id this traverser was last updated for.
     * This is used to catch up with table changes. */
    RTUINT      idGeneration;
} RTTABTRAVERSERCORE;

/**
 * Generic traverser structure.
 *
 * Tree implementations will use the tree specific part by mapping
 * this structure onto their own internal traverser structure.
 *
 * @remark  It would be better to use alloca() for allocating the structure,
 *          OTOH this is simpler for the user.
 */
typedef struct RTTABTRAVERSER
{
    /** The common core of the traverser data. */
    RTTABTRAVERSERCORE  Core;
    /** The tree specific data. */
    void                *apvTreeSpecific[32];
} RTTABTRAVERSER;


/**
 * Initializes a traverser to the NULL item.
 *
 * The NULL item is an imaginary table item before the first and after
 * the last items in the table.
 *
 * @returns Pointer to the traverser positioned at the NULL item.
 * @returns NULL on failure to allocate the traverser.
 *
 * @param   pTab            The table.
 * @param   pTravNew        Pointer to a preallocated structure. Optional.
 */
DECLINLINE(PRTTABTRAVERSERCORE) RTTabTravInit(PRTTAB pTab, PRTTABTRAVERSER pTravNew)
{
    return pTab->pOps->pfnTravInit(pTab, pTravNew);
}

/**
 * Initializes a traverser to the first item in the table.
 *
 * If the table is empty, the traverser will be positioned at the NULL item
 * like with RTTabTravInit().
 *
 * @returns Pointer to the traverser positioned at the first item or NULL item.
 * @returns NULL on failure to allocate the traverser.
 *
 * @param   pTab            The table.
 * @param   pTravNew        Pointer to a preallocated structure. Optional.
 */
DECLINLINE(PRTTABTRAVERSERCORE) RTTabTravFirst(PRTTAB pTab, PRTTABTRAVERSER pTravNew)
{
    return pTab->pOps->pfnTravFirst(pTab, pTravNew);
}

/**
 * Initializes a traverser to the last item in the table.
 *
 * If the table is empty, the traverser will be positioned at the NULL item
 * like with RTTabTravInit().
 *
 * @returns Pointer to the traverser positioned at the last item or NULL item.
 * @returns NULL on failure to allocate the traverser.
 *
 * @param   pTab            The table.
 * @param   pTravNew        Pointer to a preallocated structure. Optional.
 */
DECLINLINE(PRTTABTRAVERSERCORE) RTTabTravLast(PRTTAB pTab, PRTTABTRAVERSER pTravNew)
{
    return pTab->pOps->pfnTravLast(pTab, pTravNew);
}

/**
 * Initializes a traverser to an item matching the given one.
 *
 * If the item isn't found, the traverser will be positioned at the NULL item
 * like with RTTabTravInit().
 *
 * @returns Pointer to the traverser positioned at the matching item or NULL item.
 * @returns NULL on failure to allocate the traverser.
 *
 * @param   pTab            The table.
 * @param   pTravNew        Pointer to a preallocated structure. Optional.
 * @param   pvItem          The item to find the match to.
 */
DECLINLINE(PRTTABTRAVERSERCORE) RTTabTravFind(PRTTAB pTab, PRTTABTRAVERSER pTravNew, const void *pvItem)
{
    return pTab->pOps->pfnTravFind(pTab, pTravNew, pvItem);
}

/**
 * Initializes a traverser to the inserted item.
 *
 * If there already exists an item in the tree matching pvItem, the traverser
 * is positioned at that item like with RTTabTravFind().
 *
 * If the insert operation failes because of an out of memory condition, the
 * traverser will be positioned at the NULL item like with RTTabTravInit().
 *
 * @returns Pointer to the traverser positioned at the inserted, existing or NULL item.
 * @returns NULL on failure to allocate the traverser.
 *
 * @param   pTab            The table.
 * @param   pTravNew        Pointer to a preallocated structure. Optional.
 * @param   pvItem          The item to be inserted.
 */
DECLINLINE(PRTTABTRAVERSERCORE) RTTabTravInsert(PRTTAB pTab, PRTTABTRAVERSER pTravNew, void *pvItem)
{
    return pTab->pOps->pfnTravInsert(pTab, pTravNew, pvItem);
}

/**
 * Duplicates a traverser.
 *
 * @returns The pointer to the duplicate.
 * @returns NULL on allocation failure.
 *
 * @param   pTrav           The traverser to duplicate.
 * @param   pTravNew        Pointer to a preallocated structure. Optional.
 */
DECLINLINE(PRTTABTRAVERSERCORE) RTTabTravDuplicate(PRTTABTRAVERSERCORE pTrav, PCRTTABTRAVERSER pTravNew)
{
    if (pTrav)
        return pTrav->pTab->pOps->pfnTravDuplicate(pTrav, pTravNew);
    return NULL;
}

/**
 * Frees a traverser.
 *
 * This can safely be called even if the traverser structure
 * wasn't dynamically allocated or the constructor failed.
 *
 * @param   pTrav           The traverser which is to be free.
 */
DECLINLINE(void) RTTabTravFree(PRTTABTRAVERSERCORE pTrav)
{
    if (pTrav && pTrav->fAllocated)
        pTrav->pTab->pOps->pfnTravFree(pTrav);
}

/**
 * Gets the current item.
 *
 * @returns The current item. (NULL indicates the imaginary NULL item.)
 * @param   pTrav           The traverser.
 */
DECLINLINE(void *) RTTabTravCur(PCRTTABTRAVERSERCORE pTrav)
{
    return pTrav->pTab->pOps->pfnTravCur(pTrav);
}

/**
 * Advances to the next item.
 *
 * @returns The new current item. (NULL indicates the imaginary NULL item.)
 * @param   pTrav           The traverser.
 */
DECLINLINE(void *) RTTabTravNext(PRTTABTRAVERSERCORE pTrav)
{
    return pTrav->pTab->pOps->pfnTravNext(pTrav);
}

/**
 * Advances to the previous item.
 *
 * @returns The new current item. (NULL indicates the imaginary NULL item.)
 * @param   pTrav           The traverser.
 */
DECLINLINE(void *) RTTabTravPrev(PRTTABTRAVERSERCORE pTrav)
{
    return pTrav->pTab->pOps->pfnTravPrev(pTrav);
}

/**
 * Replaces the current item.
 *
 * This has the same restrictions as RTTabProbe(), e.g. it's not permitted to
 * break the order of the table.
 *
 * @returns The replaced item.
 * @returns NULL if the current item is the NULL item. The traverser
 *          and table remains unchanged.
 * @param   pTrav           The traverser.
 * @param   pvItem          The item to be inserted.
 */
DECLINLINE(void *) RTTabTravReplace(PRTTABTRAVERSERCORE pTrav, void *pvItem)
{
    return pTrav->pTab->pOps->pfnTravReplace(pTrav, pvItem);
}

RT_C_DECLS_END

/** @} */

#endif /* !IPRT_INCLUDED_table_h */
