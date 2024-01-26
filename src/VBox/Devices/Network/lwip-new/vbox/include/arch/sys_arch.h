#ifndef VBOX_ARCH_SYS_ARCH_H_
#define VBOX_ARCH_SYS_ARCH_H_

#include <iprt/semaphore.h>
#include <iprt/thread.h>
#ifdef RT_OS_DARWIN
#include <sys/time.h>
#endif

/** NULL value for a mbox. */
#define SYS_MBOX_NULL NULL

/** NULL value for a mutex semaphore. */
#define SYS_SEM_NULL NIL_RTSEMEVENT

/** The IPRT event semaphore ID just works fine for this type. */
typedef RTSEMEVENT sys_sem_t;


/** The opaque type of a mbox. */
typedef void *sys_mbox_t;

/** The IPRT thread ID just works fine for this type. */
typedef RTTHREAD sys_thread_t;

#if SYS_LIGHTWEIGHT_PROT
/** This is just a dummy. The implementation doesn't need anything. */
typedef void *sys_prot_t;
#endif

/** Check if an mbox is valid/allocated: return 1 for valid, 0 for invalid */
int sys_mbox_valid(sys_mbox_t *mbox);
/** Set an mbox invalid so that sys_mbox_valid returns 0 */
void sys_mbox_set_invalid(sys_mbox_t *mbox);

#define sys_sem_valid(sem) ((sem) && (*(sem)))
#define sys_sem_set_invalid(sem) do {} while(0)

#endif /* !VBOX_ARCH_SYS_ARCH_H_ */
