/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2010 Francisco Jerez <currojerez@riseup.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _LIST_H_
#define _LIST_H_

/**
 * @file Classic doubly-link circular list implementation.
 *
 * Example:
 * We need to keep a list of struct foo in the parent struct bar, i.e. what
 * we want is something like this.
 *
 *     struct bar {
 *          ...
 *          struct foo *foos; -----> struct foo {}, struct foo {}, struct foo{}
 *          ...
 *     }
 *
 * We need one list head in bar and a list element in all foos (both are of
 * data type 'struct list').
 *
 *     struct bar {
 *          ...
 *          struct list foos;
 *          ...
 *     }
 *
 *     struct foo {
 *          ...
 *          struct list entry;
 *          ...
 *     }
 *
 * Now we initialize the list head:
 *
 *     struct bar bar;
 *     ...
 *     list_init(&bar.foos);
 *
 * Then we create the first element and add it to this list:
 *
 *     struct foo *foo = malloc(...);
 *     ....
 *     list_add(&foo->entry, &bar.foos);
 *
 * Repeat the above for each element you want to add to the list. Deleting
 * works with the element itself.
 *      list_del(&foo->entry);
 *      free(foo);
 *
 * Note: calling list_del(&bar.foos) will set bar.foos to an empty
 * list again.
 *
 * Looping through the list requires a 'struct foo' as iterator and the
 * name of the field the subnodes use.
 *
 * struct foo *iterator;
 * list_for_each_entry(iterator, &bar.foos, entry) {
 *      if (iterator->something == ...)
 *             ...
 * }
 *
 * Note: You must not call list_del() on the iterator if you continue the
 * loop. You need to run the safe for-each loop instead:
 *
 * struct foo *iterator, *next;
 * list_for_each_entry_safe(iterator, next, &bar.foos, entry) {
 *      if (...)
 *              list_del(&iterator->entry);
 * }
 *
 */

/**
 * The linkage struct for list nodes. This struct must be part of your
 * to-be-linked struct.
 *
 * Example:
 * struct foo {
 *      int a;
 *      void *b;
 *      struct list *mylist;
 * }
 *
 * Position and name of the struct list field is irrelevant.
 * There are no requirements that elements of a list are of the same type.
 * There are no requirements for a list head, any struct list can be a list
 * head.
 */
struct list {
    struct list *next, *prev;
};

/**
 * Initialize the list as an empty list.
 *
 * Example:
 * list_init(&foo->mylist);
 *
 * @param The list to initialized.
 */
static void
list_init(struct list *list)
{
    list->next = list->prev = list;
}

static inline void
__list_add(struct list *entry,
	    struct list *prev,
	    struct list *next)
{
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

/**
 * Insert a new element after the given list head.
 * The list changes from:
 *      head → some element → ...
 * to
 *      head → new element → older element → ...
 *
 * Example:
 * struct foo *newfoo = malloc(...);
 * list_add(&newfoo->mylist, &foo->mylist);
 *
 * @param entry The new element to prepend to the list.
 * @param head The existing list.
 */
static inline void
list_add(struct list *entry, struct list *head)
{
    __list_add(entry, head, head->next);
}

static inline void
__list_del(struct list *prev, struct list *next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * Remove the element from the list it is in. Using this function will reset
 * the pointers to/from this element so it is removed from the list. It does
 * NOT free the element itself or manipulate it otherwise.
 *
 * Using list_del on a pure list head (like in the example at the top of
 * this file) will NOT remove the first element from
 * the list but rather reset the list as empty list.
 *
 * Example:
 * list_del(&newfoo->mylist);
 *
 * @param entry The element to remove.
 */
static inline void
list_del(struct list *entry)
{
    __list_del(entry->prev, entry->next);
    list_init(entry);
}

/**
 * Check if the list is empty.
 *
 * Example:
 * list_is_empty(&foo->mylist);
 *
 * @return True if the list contains one or more elements or False otherwise.
 */
static inline Bool
list_is_empty(struct list *head)
{
    return head->next == head;
}

/**
 * Returns a pointer to the container of this list element.
 *
 * Example:
 * struct foo* f;
 * f = container_of(&foo->mylist, struct foo, mylist);
 * assert(f == foo);
 *
 * @param ptr Pointer to the struct list.
 * @param type Data type of the list element.
 * @param member Member name of the struct list field in the list element.
 * @return A pointer to the data struct containing the list head.
 */
#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - (char *) &((type *)0)->member)
#endif

/**
 * Alias of container_of
 */
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

/**
 * Retrieve the first list entry for the given list pointer.
 *
 * Example:
 * struct foo *first;
 * first = list_first_entry(&foo->mylist, struct foo, mylist);
 *
 * @param ptr The list head
 * @param type Data type of the list element to retrieve
 * @param member Member name of the struct list field in the list element.
 * @return A pointer to the first list element.
 */
#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define __container_of(ptr, sample, member)				\
    (void *)((char *)(ptr)						\
	     - ((char *)&(sample)->member - (char *)(sample)))
/**
 * Loop through the list given by head and set pos to struct in the list.
 *
 * Example:
 * struct foo *iterator;
 * list_for_each_entry(iterator, &foo->mylist, mylist) {
 *      [modify iterator]
 * }
 *
 * This macro is not safe for node deletion. Use list_for_each_entry_safe
 * instead.
 *
 * @param pos Iterator variable of the type of the list elements.
 * @param head List head
 * @param member Member name of the struct list in the list elements.
 *
 */
#define list_for_each_entry(pos, head, member)				\
    for (pos = __container_of((head)->next, pos, member);		\
	 &pos->member != (head);					\
	 pos = __container_of(pos->member.next, pos, member))

/**
 * Loop through the list, keeping a backup pointer to the element. This
 * macro allows for the deletion of a list element while looping through the
 * list.
 *
 * See list_for_each_entry for more details.
 */
#define list_for_each_entry_safe(pos, tmp, head, member)		\
    for (pos = __container_of((head)->next, pos, member),		\
	 tmp = __container_of(pos->member.next, pos, member);		\
	 &pos->member != (head);					\
	 pos = tmp, tmp = __container_of(pos->member.next, tmp, member))

#endif
