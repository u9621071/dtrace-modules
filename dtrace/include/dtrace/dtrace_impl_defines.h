#ifndef _LINUX_DTRACE_IMPL_DEFINES_H
#define _LINUX_DTRACE_IMPL_DEFINES_H

/* 
 * DTrace Dynamic Tracing Software: DTrace Implementation defines
 * 
 * Note: The contents of this file are private to the implementation of the
 * DTrace subsystem and are subject to change at any time without notice.
 */

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Copyright 2009 -- 2013 Oracle, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <linux/dtrace/universal.h>
#include <asm/ptrace.h>

typedef typeof(((struct pt_regs *)0)->ip)	pc_t;

typedef enum dtrace_activity {
	DTRACE_ACTIVITY_INACTIVE = 0,
	DTRACE_ACTIVITY_WARMUP,
	DTRACE_ACTIVITY_ACTIVE,
	DTRACE_ACTIVITY_DRAINING,
	DTRACE_ACTIVITY_COOLDOWN,
	DTRACE_ACTIVITY_STOPPED,
	DTRACE_ACTIVITY_KILLED
} dtrace_activity_t;

typedef enum dtrace_dstate_state {
	DTRACE_DSTATE_CLEAN = 0,
	DTRACE_DSTATE_EMPTY,
	DTRACE_DSTATE_DIRTY,
	DTRACE_DSTATE_RINSING
} dtrace_dstate_state_t;

typedef enum dtrace_dynvar_op {
	DTRACE_DYNVAR_ALLOC,
	DTRACE_DYNVAR_NOALLOC,
	DTRACE_DYNVAR_DEALLOC
} dtrace_dynvar_op_t;

#define DTRACE_MSTATE_ARGS		0x00000001
#define DTRACE_MSTATE_PROBE		0x00000002
#define DTRACE_MSTATE_EPID		0x00000004
#define DTRACE_MSTATE_TIMESTAMP		0x00000008
#define DTRACE_MSTATE_STACKDEPTH	0x00000010
#define DTRACE_MSTATE_CALLER		0x00000020
#define DTRACE_MSTATE_IPL		0x00000040
#define DTRACE_MSTATE_FLTOFFS		0x00000080
#define DTRACE_MSTATE_USTACKDEPTH	0x00000100
#define DTRACE_MSTATE_UCALLER		0x00000200

#define DTRACE_PROBEKEY_MAXDEPTH	8

typedef enum dtrace_speculation_state {
	DTRACESPEC_INACTIVE = 0,
	DTRACESPEC_ACTIVE,
	DTRACESPEC_ACTIVEONE,
	DTRACESPEC_ACTIVEMANY,
	DTRACESPEC_COMMITTING,
	DTRACESPEC_COMMITTINGMANY,
	DTRACESPEC_DISCARDING
} dtrace_speculation_state_t;

#define DTRACE_HELPER_ACTION_USTACK	0
#define DTRACE_NHELPER_ACTIONS		1

#define DTRACE_HELPTRACE_NEXT	(-1)
#define DTRACE_HELPTRACE_DONE	(-2)
#define DTRACE_HELPTRACE_ERR	(-3)

#undef ASSERT
#ifdef CONFIG_DT_DEBUG
# define ASSERT(x)	((void)((x) || dtrace_assfail(#x, __FILE__, __LINE__)))
#else
# define ASSERT(x)	((void)0)
#endif

/*
 * DTrace Probe Hashing
 */

#define DTRACE_HASHNEXT(hash, probe)					\
	(dtrace_probe_t **)((uintptr_t)(probe) + (hash)->dth_nextoffs)
#define DTRACE_HASHPREV(hash, probe)	\
	(dtrace_probe_t **)((uintptr_t)(probe) + (hash)->dth_prevoffs)

/*
 * DTrace Probe Management
 */
#define DTRACE_ANCHORED(probe)	((probe)->dtpr_func[0] != '\0')
#define DTRACE_FLAGS2FLT(flags)						\
	(((flags) & CPU_DTRACE_BADADDR) ? DTRACEFLT_BADADDR :		\
	 ((flags) & CPU_DTRACE_ILLOP) ? DTRACEFLT_ILLOP :		\
	 ((flags) & CPU_DTRACE_DIVZERO) ? DTRACEFLT_DIVZERO :		\
	 ((flags) & CPU_DTRACE_KPRIV) ? DTRACEFLT_KPRIV :		\
	 ((flags) & CPU_DTRACE_UPRIV) ? DTRACEFLT_UPRIV :		\
	 ((flags) & CPU_DTRACE_TUPOFLOW) ?  DTRACEFLT_TUPOFLOW :	\
	 ((flags) & CPU_DTRACE_BADALIGN) ?  DTRACEFLT_BADALIGN :	\
	 ((flags) & CPU_DTRACE_NOSCRATCH) ?  DTRACEFLT_NOSCRATCH :	\
	 ((flags) & CPU_DTRACE_BADSTACK) ?  DTRACEFLT_BADSTACK :	\
	 DTRACEFLT_UNKNOWN)

/*
 * Test whether alloc_sz bytes will fit in the scratch region.  We isolate
 * alloc_sz on the righthand side of the comparison in order to avoid overflow
 * or underflow in the comparison with it.  This is simpler than the INRANGE
 * check above, because we know that the dtms_scratch_ptr is valid in the
 * range.  Allocations of size zero are allowed.
 */
#define DTRACE_INSCRATCH(mstate, alloc_sz) \
	((mstate)->dtms_scratch_base + (mstate)->dtms_scratch_size - \
	 (mstate)->dtms_scratch_ptr >= (alloc_sz))

/*
 * Buffering.
 */

#define DTRACEBUF_RING		0x0001		/* bufpolicy set to "ring" */
#define DTRACEBUF_FILL		0x0002		/* bufpolicy set to "fill" */
#define DTRACEBUF_NOSWITCH	0x0004		/* do not switch buffer */
#define DTRACEBUF_WRAPPED	0x0008		/* ring buffer has wrapped */
#define DTRACEBUF_DROPPED	0x0010		/* drops occurred */
#define DTRACEBUF_ERROR		0x0020		/* errors occurred */
#define DTRACEBUF_FULL		0x0040		/* "fill" buffer is full */
#define DTRACEBUF_CONSUMED	0x0080		/* buffer has been consumed */
#define DTRACEBUF_INACTIVE	0x0100		/* buffer is not yet active */

#define DTRACE_STORE(type, tomax, offset, what) \
	do { \
	*((type *)((uintptr_t)(tomax) + (uintptr_t)(offset))) = (type)(what); \
	} while (0)

typedef enum dtrace_vtime_state {
	DTRACE_VTIME_INACTIVE = 0,	/* No DTrace, no TNF */
	DTRACE_VTIME_ACTIVE,		/* DTrace virtual time, no TNF */
	DTRACE_VTIME_INACTIVE_TNF,	/* No DTrace, TNF active */
	DTRACE_VTIME_ACTIVE_TNF		/* DTrace virtual time _and_ TNF */
} dtrace_vtime_state_t;

#define KERNELBASE	(uintptr_t)_text

/*
 * regset.h information
 */
#ifdef __i386__
# define REG_SS		18      /* only stored on a privilege transition */
# define REG_UESP	17      /* only stored on a privilege transition */
# define REG_EFL	16
# define REG_CS		15
# define REG_EIP	14
# define REG_ERR	13
# define REG_TRAPNO	12
# define REG_EAX	11
# define REG_ECX	10
# define REG_EDX	9
# define REG_EBX	8
# define REG_ESP	7
# define REG_EBP	6
# define REG_ESI	5
# define REG_EDI	4
# define REG_DS		3
# define REG_ES		2
# define REG_FS		1
# define REG_GS		0
#else
# define REG_GS		24
# define REG_FS		23
# define REG_ES		22
# define REG_DS		21
# define REG_SS		20
# define REG_RSP	19
# define REG_RFL	18
# define REG_CS		17
# define REG_RIP	16
# define REG_ERR	15
# define REG_TRAPNO	15
# define REG_RDI	14
# define REG_RSI	13
# define REG_RDX	12
# define REG_RCX	11
# define REG_RAX	10
# define REG_R8		9
# define REG_R9		8
# define REG_R10	7
# define REG_R11	6
# define REG_RBX	5
# define REG_RBP	4
# define REG_R12	3
# define REG_R13	2
# define REG_R14	1
# define REG_R15	0
#endif

#if defined(__i386__) || defined(__x86_64__)
# define DTRACE_INVOP_PUSHL_EBP	1
# define DTRACE_INVOP_POPL_EBP	2
# define DTRACE_INVOP_LEAVE	3
# define DTRACE_INVOP_NOP	4
# define DTRACE_INVOP_RET	5
#endif

#ifdef CONFIG_DT_DEBUG_MUTEX
# define _mutex_lock(x)		mutex_lock(x)
# define _mutex_unlock(x)	mutex_unlock(x)

# define mutex_lock(x)		do {					      \
				    printk(KERN_DEBUG			      \
					   "mutex_lock(%s) at %s::%d\n",      \
					   __stringify(x),		      \
					   __FILE__, __LINE__);		      \
				    _mutex_lock(x);			      \
				} while (0)
# define mutex_unlock(x)	do {					      \
				    printk(KERN_DEBUG			      \
					   "mutex_unlock(%s) at %s::%d\n",    \
					   __stringify(x),		      \
					   __FILE__, __LINE__);		      \
				    _mutex_unlock(x);			      \
				} while (0)
#endif

#define MUTEX_HELD(lock)	mutex_owned(lock)

#endif /* _LINUX_DTRACE_IMPL_DEFINES_H */