/*
 * FILE:	dtrace_isa.c
 * DESCRIPTION:	Dynamic Tracing: architecture specific support functions
 *
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
 * Copyright 2010, 2011, 2012, 2013, 2014 Oracle, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <linux/dtrace_cpu.h>
#include <linux/hardirq.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/stacktrace.h>

#include "dtrace.h"

EXPORT_SYMBOL(dtrace_getfp);

DEFINE_MUTEX(cpu_lock);
EXPORT_SYMBOL(cpu_lock);

int dtrace_getipl(void)
{
	return in_interrupt();
}

static void dtrace_sync_func(void)
{
}

void dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	if (cpu == DTRACE_CPUALL) {
		smp_call_function(func, arg, 1);
	} else
		smp_call_function_single(cpu, func, arg, 1);
}

void dtrace_sync(void)
{
	dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
}

void dtrace_toxic_ranges(void (*func)(uintptr_t, uintptr_t))
{
	/* FIXME */
}

ktime_t dtrace_gethrestime(void)
{
	return dtrace_gethrtime();
}

void dtrace_getpcstack(uint64_t *pcstack, int pcstack_limit, int aframes,
		       uint32_t *intrpc)
{
	struct stacktrace_state	st = {
					pcstack,
					NULL,
					pcstack_limit,
					aframes,
					STACKTRACE_KERNEL
				     };

	dtrace_stacktrace(&st);

	while (st.depth < st.limit)
		pcstack[st.depth++] = 0;
}
EXPORT_SYMBOL(dtrace_getpcstack);

static struct vm_area_struct *find_user_vma(struct task_struct *tsk,
					    struct mm_struct *mm,
					    struct page **page,
					    unsigned long addr,
					    int need_incore)
{
	struct vm_area_struct *vma = NULL;
	int nonblocking = 1;
	int flags = FOLL_IMMED;
	int ret;

	if (page)
		flags |= FOLL_GET;

	ret = __get_user_pages(tsk, mm, addr, 1, flags, page, &vma,
			       &nonblocking);

	if ((nonblocking == 0) && need_incore) {
		if ((ret > 0) && page) {
			size_t i;
			for (i = 0; i < ret; i++)
				put_page(page[i]);
		}
		return NULL;
	}
	else if (ret <= 0)
		return NULL;
	else
		return vma;
}

/*
 * Get user stack entries up to the pcstack_limit; return the number of entries
 * acquired.  If pcstack is NULL, return the number of entries potentially
 * acquirable.
 */
unsigned long dtrace_getufpstack(uint64_t *pcstack, uint64_t *fpstack,
				 int pcstack_limit)
{
	struct task_struct	*p = current;
	struct mm_struct	*mm = p->mm;
	unsigned long		tos, bos;
	unsigned long		*sp;
	unsigned long		depth = 0;
	struct vm_area_struct	*stack_vma;
	struct page		*stack_page = NULL;

	if (pcstack) {
                if (unlikely(pcstack_limit < 2)) {
                        DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
                        return 0;
                }
                *pcstack++ = (uint64_t)p->pid;
		*pcstack++ = (uint64_t)p->tgid;
		pcstack_limit-=2;
	}

	/*
	 * We cannot ustack() if this task has no mm, if this task is a kernel
	 * thread, or when someone else has the mmap_sem or the page_table_lock
	 * (because find_user_vma() ultimately does a __get_user_pages() and
	 * thence a follow_page(), which can take that lock).
	 */
	if (mm == NULL || (p->flags & PF_KTHREAD) ||
	    spin_is_locked(&mm->page_table_lock))
		goto out;

	if (!down_read_trylock(&mm->mmap_sem))
		goto out;
	atomic_inc(&mm->mm_users);

#ifdef CONFIG_X86_64
	tos = current_user_stack_pointer();
#else
	tos = user_stack_pointer(current_pt_regs());
#endif
	stack_vma = find_user_vma(p, mm, NULL, (unsigned long) tos, 0);
	if (!stack_vma ||
	    stack_vma->vm_start > (unsigned long) tos)
		goto unlock_out;

#ifdef CONFIG_STACK_GROWSUP
#error This code does not yet work on STACK_GROWSUP platforms.
#endif
	bos = stack_vma->vm_end;
	if (stack_guard_page_end(stack_vma, bos))
                bos -= PAGE_SIZE;

	/*
	 * If we have a pcstack, loop as long as we are within the stack limit.
	 * Otherwise, loop until we run out of stack.
	 */
	for (sp = (unsigned long *)tos;
	     sp <= (unsigned long *)bos &&
		     ((pcstack && pcstack_limit > 0) ||
		      !pcstack);
	     sp++) {
		struct vm_area_struct	*code_vma;
		unsigned long		addr;

		/*
		 * Recheck for faultedness and pin at page boundaries.
		 */
		if (!stack_page || (((unsigned long)sp & PAGE_MASK) == 0)) {
			if (stack_page) {
				put_page(stack_page);
				stack_page = NULL;
			}

			if (!find_user_vma(p, mm, &stack_page,
					   (unsigned long) sp, 1))
				break;
		}

		DTRACE_CPUFLAG_SET(CPU_DTRACE_NOPF);
		DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
		get_user(addr, sp);
		DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);
		DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOPF);

		if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_FAULT)) {
			DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_BADADDR);
			DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_PF_TRAPPED);
			break;
		}

		code_vma = find_user_vma(p, mm, NULL, addr, 0);

		if (!code_vma || code_vma->vm_start > addr)
			continue;

		if ((addr >= tos && addr <= bos) ||
		    (code_vma->vm_flags & VM_GROWSDOWN)) {
			/* stack address - may need it for the fpstack. */
		} else if (code_vma->vm_flags & VM_EXEC) {
			if (pcstack) {
				*pcstack++ = addr;
				pcstack_limit--;
			}
			depth++;
		}
	}
	if (stack_page != NULL)
		put_page(stack_page);

unlock_out:
	atomic_dec(&mm->mm_users);
	up_read(&mm->mmap_sem);

out:
	if (pcstack)
		while (pcstack_limit--)
			*pcstack++ = 0;

	return depth;
}

void dtrace_getupcstack(uint64_t *pcstack, int pcstack_limit)
{
	dtrace_getufpstack(pcstack, NULL, pcstack_limit);
}

int dtrace_getstackdepth(dtrace_mstate_t *mstate, int aframes)
{
	uintptr_t		old = mstate->dtms_scratch_ptr;
	size_t			size;
	struct stacktrace_state	st = {
					NULL,
					NULL,
					0,
					aframes,
					STACKTRACE_KERNEL
				     };

	st.pcs = (uint64_t *)P2ROUNDUP(mstate->dtms_scratch_ptr, 8);
	size = (uintptr_t)st.pcs - mstate->dtms_scratch_ptr +
			  aframes * sizeof(uint64_t);
	if (mstate->dtms_scratch_ptr + size >
	    mstate->dtms_scratch_base + mstate->dtms_scratch_size) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_NOSCRATCH);
		return 0;
	}

	dtrace_stacktrace(&st);

	mstate->dtms_scratch_ptr = old;

	return st.depth;
}

int dtrace_getustackdepth(void)
{
	return dtrace_getufpstack(NULL, NULL, 0);
}
