/** @file
 * MS COM / XPCOM Abstraction Layer - Automatic locks, implementation.
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

#ifndef VBOX_INCLUDED_com_AutoLock_h
#define VBOX_INCLUDED_com_AutoLock_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>


/** @defgroup grp_com_autolock  Automatic Locks
 * @ingroup grp_com
 * @{
 */

// macros for automatic lock validation; these will amount to nothing
// unless lock validation is enabled for the runtime
#if defined(RT_LOCK_STRICT)
# define VBOX_WITH_MAIN_LOCK_VALIDATION
# define COMMA_LOCKVAL_SRC_POS , RT_SRC_POS
# define LOCKVAL_SRC_POS_DECL RT_SRC_POS_DECL
# define COMMA_LOCKVAL_SRC_POS_DECL , RT_SRC_POS_DECL
# define LOCKVAL_SRC_POS_ARGS RT_SRC_POS_ARGS
# define COMMA_LOCKVAL_SRC_POS_ARGS , RT_SRC_POS_ARGS
#else
# define COMMA_LOCKVAL_SRC_POS
# define LOCKVAL_SRC_POS_DECL
# define COMMA_LOCKVAL_SRC_POS_DECL
# define LOCKVAL_SRC_POS_ARGS
# define COMMA_LOCKVAL_SRC_POS_ARGS
#endif

namespace util
{

////////////////////////////////////////////////////////////////////////////////
//
// Order classes for lock validation
//
////////////////////////////////////////////////////////////////////////////////

/**
 * IPRT now has a sophisticated system of run-time locking classes to validate
 * locking order. Since the Main code is handled by simpler minds, we want
 * compile-time constants for simplicity, and we'll look up the run-time classes
 * in AutoLock.cpp transparently. These are passed to the constructors of the
 * LockHandle classes.
 */
enum VBoxLockingClass
{
    LOCKCLASS_NONE = 0,
    LOCKCLASS_WEBSERVICE = 1,               // highest order: webservice locks
    LOCKCLASS_VIRTUALBOXOBJECT = 2,         // highest order within Main itself: VirtualBox object lock
    LOCKCLASS_HOSTOBJECT = 3,               // Host object lock
    LOCKCLASS_LISTOFMACHINES = 4,           // list of machines in VirtualBox object
    LOCKCLASS_MACHINEOBJECT = 5,            // Machine object lock
    LOCKCLASS_SNAPSHOTOBJECT = 6,           // snapshot object locks
                                            // (the snapshots tree, including the child pointers in Snapshot,
                                            // is protected by the normal Machine object lock)
    LOCKCLASS_MEDIUMQUERY = 7,              // lock used to protect Machine::queryInfo
    LOCKCLASS_LISTOFMEDIA = 8,              // list of media (hard disks, DVDs, floppies) in VirtualBox object
    LOCKCLASS_LISTOFOTHEROBJECTS = 9,       // any other list of objects
    LOCKCLASS_OTHEROBJECT = 10,             // any regular object member variable lock
    LOCKCLASS_PROGRESSLIST = 11,            // list of progress objects in VirtualBox; no other object lock
                                            // may be held after this!
    LOCKCLASS_OBJECTSTATE = 12,             // object state lock (handled by AutoCaller classes)
    LOCKCLASS_TRANSLATOR = 13               // translator internal lock
};

void InitAutoLockSystem();

/**
 * Check whether the current thread holds any locks in the given class
 *
 * @return true if any such locks are held, false otherwise. If the lock
 *              validator is not compiled in, always returns false.
 * @param lockClass     Which lock class to check.
 */
bool AutoLockHoldsLocksInClass(VBoxLockingClass lockClass);

////////////////////////////////////////////////////////////////////////////////
//
// LockHandle and friends
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for semaphore handles (RWLockHandle and WriteLockHandle).
 * Don't use this directly, but this implements lock validation for them.
 */
class LockHandle
{
public:
    LockHandle()
    {}

    virtual ~LockHandle()
    {}

    /**
     * Returns @c true if the current thread holds a write lock on this
     * read/write semaphore. Intended for debugging only.
     */
    virtual bool isWriteLockOnCurrentThread() const = 0;

    /**
     * Returns @c true if the current thread holds a read lock on this
     * read/write semaphore. Intended for debugging only as it isn't always
     * accurate given @a fWannaHear.
     */
    virtual bool isReadLockedOnCurrentThread(bool fWannaHear = true) const = 0;

    /**
     * Returns the current write lock level of this semaphore. The lock level
     * determines the number of nested #lockWrite() calls on the given
     * semaphore handle.
     *
     * Note that this call is valid only when the current thread owns a write
     * lock on the given semaphore handle and will assert otherwise.
     */
    virtual uint32_t writeLockLevel() const = 0;

    virtual void lockWrite(LOCKVAL_SRC_POS_DECL) = 0;
    virtual void unlockWrite() = 0;
    virtual void lockRead(LOCKVAL_SRC_POS_DECL) = 0;
    virtual void unlockRead() = 0;

#ifdef VBOX_WITH_MAIN_LOCK_VALIDATION
    virtual const char* describe() const = 0;
#endif

private:
    // prohibit copy + assignment
    LockHandle(const LockHandle&);
    LockHandle& operator=(const LockHandle&);
};

/**
 * Full-featured read/write semaphore handle implementation.
 *
 * This is an auxiliary base class for classes that need full-featured
 * read/write locking as described in the AutoWriteLock class documentation.
 * Instances of classes inherited from this class can be passed as arguments to
 * the AutoWriteLock and AutoReadLock constructors.
 */
class RWLockHandle : public LockHandle
{
public:
    RWLockHandle(VBoxLockingClass lockClass);
    virtual ~RWLockHandle();

    virtual bool isWriteLockOnCurrentThread() const;
    virtual bool isReadLockedOnCurrentThread(bool fWannaHear = true) const;

    virtual void lockWrite(LOCKVAL_SRC_POS_DECL);
    virtual void unlockWrite();
    virtual void lockRead(LOCKVAL_SRC_POS_DECL);
    virtual void unlockRead();

    virtual uint32_t writeLockLevel() const;

#ifdef VBOX_WITH_MAIN_LOCK_VALIDATION
    virtual const char* describe() const;
#endif

private:
    struct Data;
    Data *m;

    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(RWLockHandle); /* Shuts up MSC warning C4625. */
};

/**
 * Write-only semaphore handle implementation.
 *
 * This is an auxiliary base class for classes that need write-only (exclusive)
 * locking and do not need read (shared) locking. This implementation uses a
 * cheap and fast critical section for both lockWrite() and lockRead() methods
 * which makes a lockRead() call fully equivalent to the lockWrite() call and
 * therefore makes it pointless to use instahces of this class with
 * AutoReadLock instances -- shared locking will not be possible anyway and
 * any call to lock() will block if there are lock owners on other threads.
 *
 * Use with care only when absolutely sure that shared locks are not necessary.
 */
class WriteLockHandle : public LockHandle
{
public:
    WriteLockHandle(VBoxLockingClass lockClass);
    virtual ~WriteLockHandle();
    virtual bool isWriteLockOnCurrentThread() const;
    virtual bool isReadLockedOnCurrentThread(bool fWannaHear = true) const;

    virtual void lockWrite(LOCKVAL_SRC_POS_DECL);
    virtual void unlockWrite();
    virtual void lockRead(LOCKVAL_SRC_POS_DECL);
    virtual void unlockRead();
    virtual uint32_t writeLockLevel() const;

#ifdef VBOX_WITH_MAIN_LOCK_VALIDATION
    virtual const char* describe() const;
#endif

private:
    struct Data;
    Data *m;

    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(WriteLockHandle); /* Shuts up MSC warning C4625. */
};

////////////////////////////////////////////////////////////////////////////////
//
// Lockable
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Lockable interface.
 *
 * This is an abstract base for classes that need read/write locking. Unlike
 * RWLockHandle and other classes that makes the read/write semaphore a part of
 * class data, this class allows subclasses to decide which semaphore handle to
 * use.
 */
class Lockable
{
public:
    virtual ~Lockable() { } /* To make VC++ 2019 happy. */

    /**
     * Returns a pointer to a LockHandle used by AutoWriteLock/AutoReadLock
     * for locking. Subclasses are allowed to return @c NULL -- in this case,
     * the AutoWriteLock/AutoReadLock object constructed using an instance of
     * such subclass will simply turn into no-op.
     */
    virtual LockHandle *lockHandle() const = 0;

    /**
     * Equivalent to <tt>#lockHandle()->isWriteLockOnCurrentThread()</tt>.
     * Returns @c false if lockHandle() returns @c NULL.
     */
    bool isWriteLockOnCurrentThread()
    {
        LockHandle *h = lockHandle();
        return h ? h->isWriteLockOnCurrentThread() : false;
    }

    /**
     * Equivalent to <tt>#lockHandle()->isReadLockedOnCurrentThread()</tt>.
     * Returns @c false if lockHandle() returns @c NULL.
     * @note Use with care, simple debug assertions and similar only.
     */
    bool isReadLockedOnCurrentThread(bool fWannaHear = true) const
    {
        LockHandle *h = lockHandle();
        return h ? h->isReadLockedOnCurrentThread(fWannaHear) : false;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoLockBase
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for all autolocks.
 *
 * This cannot be used directly. Use AutoReadLock or AutoWriteLock or AutoMultiWriteLock2/3
 * which directly and indirectly derive from this.
 *
 * In the implementation, the instance data contains a list of lock handles.
 * The class provides some utility functions to help locking and unlocking
 * them.
 */

class AutoLockBase
{
protected:
    AutoLockBase(uint32_t cHandles
                 COMMA_LOCKVAL_SRC_POS_DECL);
    AutoLockBase(uint32_t cHandles,
                 LockHandle *pHandle
                 COMMA_LOCKVAL_SRC_POS_DECL);
    virtual ~AutoLockBase();

    struct Data;
    Data *m;

    virtual void callLockImpl(LockHandle &l) = 0;
    virtual void callUnlockImpl(LockHandle &l) = 0;

    void callLockOnAllHandles();
    void callUnlockOnAllHandles();

    void cleanup();

public:
    void acquire();
    void release();

private:
    // prohibit copy + assignment
    AutoLockBase(const AutoLockBase&);
    AutoLockBase& operator=(const AutoLockBase&);
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoReadLock
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Automatic read lock. Use this with a RWLockHandle to request a read/write
 * semaphore in read mode. You can also use this with a WriteLockHandle but
 * that makes little sense since they treat read mode like write mode.
 *
 * If constructed with a RWLockHandle or an instance of Lockable (which in
 * practice means any VirtualBoxBase derivative), it autoamtically requests
 * the lock in read mode and releases the read lock in the destructor.
 */
class AutoReadLock : public AutoLockBase
{
public:

    /**
     * Constructs a null instance that does not manage any read/write
     * semaphore.
     *
     * Note that all method calls on a null instance are no-ops. This allows to
     * have the code where lock protection can be selected (or omitted) at
     * runtime.
     */
    AutoReadLock(LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       NULL
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    { }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a read lock.
     */
    AutoReadLock(LockHandle *aHandle
                 COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       aHandle
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a read lock.
     */
    AutoReadLock(LockHandle &aHandle
                 COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       &aHandle
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a read lock.
     */
    AutoReadLock(const Lockable &aLockable
                 COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       aLockable.lockHandle()
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a read lock.
     */
    AutoReadLock(const Lockable *aLockable
                 COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       aLockable ? aLockable->lockHandle() : NULL
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    virtual ~AutoReadLock();

    virtual void callLockImpl(LockHandle &l);
    virtual void callUnlockImpl(LockHandle &l);

private:
    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AutoReadLock); /* Shuts up MSC warning C4625. */
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoWriteLockBase
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Base class for all auto write locks.
 *
 * This cannot be used directly. Use AutoWriteLock or AutoMultiWriteLock2/3
 * which derive from this.
 *
 * It has some utility methods for subclasses.
 */
class AutoWriteLockBase : public AutoLockBase
{
protected:
    AutoWriteLockBase(uint32_t cHandles
                      COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(cHandles
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    { }

    AutoWriteLockBase(uint32_t cHandles,
                      LockHandle *pHandle
                      COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(cHandles,
                       pHandle
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    { }

    virtual ~AutoWriteLockBase()
    { }

    virtual void callLockImpl(LockHandle &l);
    virtual void callUnlockImpl(LockHandle &l);

private:
    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AutoWriteLockBase); /* Shuts up MSC warning C4625. */
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoWriteLock
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Automatic write lock. Use this with a RWLockHandle to request a read/write
 * semaphore in write mode. There can only ever be one writer of a read/write
 * semaphore: while the lock is held in write mode, no other writer or reader
 * can request the semaphore and will block.
 *
 * If constructed with a RWLockHandle or an instance of Lockable (which in
 * practice means any VirtualBoxBase derivative), it autoamtically requests
 * the lock in write mode and releases the write lock in the destructor.
 *
 * When used with a WriteLockHandle, it requests the semaphore contained therein
 * exclusively.
 */
class AutoWriteLock : public AutoWriteLockBase
{
public:

    /**
     * Constructs a null instance that does not manage any read/write
     * semaphore.
     *
     * Note that all method calls on a null instance are no-ops. This allows to
     * have the code where lock protection can be selected (or omitted) at
     * runtime.
     */
    AutoWriteLock(LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            NULL
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    { }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(LockHandle *aHandle
                  COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            aHandle
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(LockHandle &aHandle
                  COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            &aHandle
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(const Lockable &aLockable
                  COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            aLockable.lockHandle()
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(const Lockable *aLockable
                  COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            aLockable ? aLockable->lockHandle() : NULL
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(uint32_t cHandles,
                  LockHandle** pHandles
                  COMMA_LOCKVAL_SRC_POS_DECL);

    /**
     * Release all write locks acquired by this instance through the #acquire()
     * call and destroys the instance.
     *
     * Note that if there there are nested #acquire() calls without the
     * corresponding number of #release() calls when the destructor is called, it
     * will assert. This is because having an unbalanced number of nested locks
     * is a program logic error which must be fixed.
     */
    virtual ~AutoWriteLock()
    {
        cleanup();
    }

    void attach(LockHandle *aHandle);

    /** @see attach (LockHandle *) */
    void attach(LockHandle &aHandle)
    {
        attach(&aHandle);
    }

    /** @see attach (LockHandle *) */
    void attach(const Lockable &aLockable)
    {
        attach(aLockable.lockHandle());
    }

    /** @see attach (LockHandle *) */
    void attach(const Lockable *aLockable)
    {
        attach(aLockable ? aLockable->lockHandle() : NULL);
    }

    bool isWriteLockOnCurrentThread() const;
    uint32_t writeLockLevel() const;

    bool isReadLockedOnCurrentThread(bool fWannaHear = true) const;

private:
    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AutoWriteLock); /* Shuts up MSC warning C4625. */
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoMultiWriteLock*
//
////////////////////////////////////////////////////////////////////////////////

/**
 * A multi-write-lock containing two other write locks.
 *
 */
class AutoMultiWriteLock2 : public AutoWriteLockBase
{
public:
    AutoMultiWriteLock2(Lockable *pl1,
                        Lockable *pl2
                        COMMA_LOCKVAL_SRC_POS_DECL);
    AutoMultiWriteLock2(LockHandle *pl1,
                        LockHandle *pl2
                        COMMA_LOCKVAL_SRC_POS_DECL);

    virtual ~AutoMultiWriteLock2()
    {
        cleanup();
    }

private:
    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AutoMultiWriteLock2); /* Shuts up MSC warning C4625. */
};

/**
 * A multi-write-lock containing three other write locks.
 *
 */
class AutoMultiWriteLock3 : public AutoWriteLockBase
{
public:
    AutoMultiWriteLock3(Lockable *pl1,
                        Lockable *pl2,
                        Lockable *pl3
                        COMMA_LOCKVAL_SRC_POS_DECL);
    AutoMultiWriteLock3(LockHandle *pl1,
                        LockHandle *pl2,
                        LockHandle *pl3
                        COMMA_LOCKVAL_SRC_POS_DECL);

    virtual ~AutoMultiWriteLock3()
    {
        cleanup();
    }

private:
    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AutoMultiWriteLock3); /* Shuts up MSC warning C4625. */
};

/**
 * A multi-write-lock containing four other write locks.
 *
 */
class AutoMultiWriteLock4 : public AutoWriteLockBase
{
public:
    AutoMultiWriteLock4(Lockable *pl1,
                        Lockable *pl2,
                        Lockable *pl3,
                        Lockable *pl4
                        COMMA_LOCKVAL_SRC_POS_DECL);
    AutoMultiWriteLock4(LockHandle *pl1,
                        LockHandle *pl2,
                        LockHandle *pl3,
                        LockHandle *pl4
                        COMMA_LOCKVAL_SRC_POS_DECL);

    virtual ~AutoMultiWriteLock4()
    {
        cleanup();
    }

private:
    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AutoMultiWriteLock4); /* Shuts up MSC warning C4625. */
};

} /* namespace util */

/** @} */

#endif /* !VBOX_INCLUDED_com_AutoLock_h */

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
