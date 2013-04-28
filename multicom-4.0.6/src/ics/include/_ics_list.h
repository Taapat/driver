/**************************************************************
 * Copyright (C) 2010   STMicroelectronics. All Rights Reserved.
 * This file is part of the latest release of the Multicom4 project. This release 
 * is fully functional and provides all of the original MME functionality.This 
 * release  is now considered stable and ready for integration with other software 
 * components.

 * Multicom4 is a free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License as published by the Free Software Foundation 
 * version 2.

 * Multicom4 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along with Multicom4; 
 * see the file COPYING.  If not, write to the Free Software Foundation, 59 Temple Place - 
 * Suite 330, Boston, MA 02111-1307, USA.

 * Written by Multicom team at STMicroelectronics in November 2010.  
 * Contact multicom.support@st.com. 
**************************************************************/

/* 
 * 
 */ 

#ifndef _ICS_LIST_SYS_H
#define _ICS_LIST_SYS_H

/*
 * Simple doubly linked list implementation compatible with the one in the
 * Linux kernel tree
 * 
 */

#if defined(__KERNEL__)

#include <linux/list.h>

#else

#ifndef container_of
/*
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
      const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
      (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#define LIST_POISON1	((void *)0xb00bb11e)
#define LIST_POISON2	((void *)0xf00ff11e)

struct list_head {
  struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name)							\
  struct list_head name = LIST_HEAD_INIT(name)

_ICS_OS_INLINE_FUNC_PREFIX void INIT_LIST_HEAD(struct list_head *list)
{
  list->next = list;
  list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
_ICS_OS_INLINE_FUNC_PREFIX void __list_add(struct list_head *new,
					   struct list_head *prev,
					   struct list_head *next)
{
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
_ICS_OS_INLINE_FUNC_PREFIX void list_add(struct list_head *new, struct list_head *head)
{
  __list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
_ICS_OS_INLINE_FUNC_PREFIX void list_add_tail(struct list_head *new, struct list_head *head)
{
  __list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
_ICS_OS_INLINE_FUNC_PREFIX void __list_del(struct list_head * prev, struct list_head * next)
{
  next->prev = prev;
  prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
_ICS_OS_INLINE_FUNC_PREFIX void list_del(struct list_head *entry)
{
  __list_del(entry->prev, entry->next);
#if defined(ICS_DEBUG) || defined(ICS_DEBUG_SAFETY)
  entry->next = LIST_POISON1;
  entry->prev = LIST_POISON2;
#endif
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
_ICS_OS_INLINE_FUNC_PREFIX void list_del_init(struct list_head *entry)
{
  __list_del(entry->prev, entry->next);
  INIT_LIST_HEAD(entry);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
_ICS_OS_INLINE_FUNC_PREFIX int list_is_last(const struct list_head *list, const struct list_head *head)
{
  return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
_ICS_OS_INLINE_FUNC_PREFIX int list_empty(const struct list_head *head)
{
  return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
_ICS_OS_INLINE_FUNC_PREFIX int list_empty_careful(const struct list_head *head)
{
  struct list_head *next = head->next;
  return (next == head) && (next == head->prev);
}

    
/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member)				\
  container_of(ptr, type, member)


/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member)			\
  list_entry((ptr)->next, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head)				\
  for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head)				\
  for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head)			\
  for (pos = (head)->next, n = pos->next; pos != (head);	\
	   pos = n, n = pos->next)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)					\
  for (pos = list_entry((head)->next, __typeof__(*pos), member);		\
	   &pos->member != (head);		\
	   pos = list_entry(pos->member.next, __typeof__(*pos), member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)				\
  for (pos = list_entry((head)->prev, __typeof__(*pos), member);		\
	   &pos->member != (head);			\
	   pos = list_entry(pos->member.prev, __typeof__(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)				\
  for (pos = list_entry((head)->next, __typeof__(*pos), member),		\
		 n = list_entry(pos->member.next, __typeof__(*pos), member);	\
	   &pos->member != (head);						\
	   pos = n, n = list_entry(n->member.next, __typeof__(*n), member))

/**
 * list_for_each_entry_safe_continue
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define list_for_each_entry_safe_continue(pos, n, head, member)			\
  for (pos = list_entry(pos->member.next, __typeof__(*pos), member),		\
	 n = list_entry(pos->member.next, __typeof__(*pos), member);		\
	   &pos->member != (head);						\
	   pos = n, n = list_entry(n->member.next, __typeof__(*n), member))

/**
 * list_for_each_entry_safe_from
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define list_for_each_entry_safe_from(pos, n, head, member)			\
  for (n = list_entry(pos->member.next, __typeof__(*pos), member);		\
	   &pos->member != (head);						\
	   pos = n, n = list_entry(n->member.next, __typeof__(*n), member))

/**
 * list_for_each_entry_safe_reverse
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member)			\
  for (pos = list_entry((head)->prev, __typeof__(*pos), member),		\
		 n = list_entry(pos->member.prev, __typeof__(*pos), member);	\
	   &pos->member != (head);						\
	   pos = n, n = list_entry(n->member.prev, __typeof__(*n), member))

#endif /* ! __KERNEL__ */

/**
 * list_last_entry - get the tail element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

#endif /* _ICS_LIST_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
