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
 */

/*
 * Copyright 2011, 2012, 2013 Oracle, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _DT_PERF_H_
#define _DT_PERF_H_

extern void dt_perf_provide(void *, const dtrace_probedesc_t *);
extern int _dt_perf_enable(void *, dtrace_id_t, void *);
extern void _dt_perf_disable(void *, dtrace_id_t, void *);
extern void dt_perf_destroy(void *, dtrace_id_t, void *);

extern dtrace_provider_id_t	dt_perf_id;

extern int dt_perf_dev_init(void);
extern void dt_perf_dev_exit(void);

#endif /* _DT_PERF_H_ */
