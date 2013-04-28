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

/*
 * Linux Userspace OS Wrapper macros
 */

#ifndef _ICS_LINUX_USER_H
#define _ICS_LINUX_USER_H

#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

extern int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);

#define _ICS_OS_EXPORT 			extern
#define _ICS_OS_INLINE_FUNC_PREFIX 	static __inline__

#define _ICS_OS_MAX_TASK_NAME		16	/* OS specific maximum length of a task name */

/* Console/tty output function */
#define _ICS_OS_PRINTF			printf

/* 
 * Memory allocation
 *
 * Debug wrappers of these are defined in _ics_debug.h
 */
#define _ICS_OS_PAGESIZE		sysconf(_SC_PAGESIZE)
#define _ics_os_malloc(S)		malloc((S))
#define _ics_os_zalloc(P, S)		((P) = zalloc((S))
#define _ics_os_free(P)			free((P))


/* Data movement */
#define _ICS_OS_MEMSET(P, V, S)		memset((P), (V), (S))
#define _ICS_OS_MEMCPY(D, S, N)		memcpy((D), (S), (N))


/*
 * Mutual exclusion
 */
typedef pthread_mutex_t _ICS_OS_MUTEX;
 
#define _ICS_OS_MUTEX_INIT(pMutex)	(pthread_mutex_init((pMutex), NULL) == 0)
#define _ICS_OS_MUTEX_DESTROY(pMutex)	pthread_mutex_destroy((pMutex))
#define _ICS_OS_MUTEX_TAKE(pMutex)	pthread_mutex_lock((pMutex))
#define _ICS_OS_MUTEX_RELEASE(pMutex)	pthread_mutex_unlock((pMutex))
#define _ICS_OS_MUTEX_HELD(pMutex)	pthread_mutex_trylock((pMutex))

/*
 * Blocking Events
 */
typedef sem_t _ICS_OS_EVENT;

_ICS_OS_INLINE_FUNC_PREFIX
ICS_ERROR ics_os_event_wait (_ICS_OS_EVENT *ev, ICS_UINT timeout)
{
	int res;
  
	if (timeout == ICS_TIMEOUT_INFINITE)
	{
		res = sem_wait(ev);
	}
	else
	{
		struct timespec ts;

		ts.tv_sec  = time(NULL) + (timeout/1000);
		ts.tv_nsec = 0;
	
		res = sem_timedwait(ev, &ts);
	}

	if (res == 0)
		/* Normal wakeup */
		return ICS_SUCCESS;
    
	/* Failure: distinguish between a timeout and an interrupt */
	return ((errno == ETIMEDOUT) ? ICS_SYSTEM_TIMEOUT : ICS_SYSTEM_INTERRUPT);
}

#define _ICS_OS_EVENT_INIT(pEvent)	(sem_init((pEvent), 0, 0) == 0)
#define _ICS_OS_EVENT_DESTROY(pEvent)	sem_destroy((pEvent))
#define _ICS_OS_EVENT_WAIT(pEvent,t,i)	ics_os_event_wait((pEvent), (t))
#define _ICS_OS_EVENT_POST(pEvent)	sem_post((pEvent))

/* 
 * Thread/Task functions
 */
typedef pthread_t _ICS_OS_TASK;

typedef struct ics_task_info
{
	_ICS_OS_TASK	    task;
	void              (*entry)(void *);
	void               *param;
	ICS_INT             priority;
	_ICS_OS_EVENT       event;     
} _ICS_OS_TASK_INFO;

ICS_ERROR ics_os_task_create (void (*entry)(void *), void *param, ICS_INT priority, const ICS_CHAR *name,
			      _ICS_OS_TASK_INFO *task);
ICS_ERROR ics_os_task_destroy (_ICS_OS_TASK_INFO *task);

#define _ICS_OS_TASK_CREATE(FUNC, PARAM, PRIO, NAME, T)	ics_os_task_create((FUNC), (PARAM), (PRIO), (NAME), (T))
#define _ICS_OS_TASK_DESTROY(T)		ics_os_task_destroy((T))

extern pid_t gettid(void);
#define _ICS_OS_TASK_SELF()		(void *)gettid()

#endif /* _ICS_LINUX_USER_H */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */

