/*
 * Copyright (c) 2013-2019 by Datera, Inc.
 *
 * Licensed under the GNU General Public License 2.0.
 *
 * Some rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see < http://www.gnu.org/licenses/gpl-2.0.html>
 */

#ifndef _DSTATS_H
#define _DSTATS_H

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/time.h>

#define NSEC_PER_ns			1L
#define NSEC_PER_us			NSEC_PER_USEC
#define NSEC_PER_ms			NSEC_PER_MSEC
#define NSEC_PER_sec			NSEC_PER_SEC

/* The lo in d128 is mod D128_LO_LIMIT, not 2^64! */
#define D128_LO_LIMIT	1000000000000000000ULL

typedef struct { u64 lo; u64 hi; } d128;

static inline void d128_accumulate(d128 *accum, u64 x)
{
	u64 nhi = (accum->hi), nlo = (x + accum->lo);

	if (nlo >= D128_LO_LIMIT) {
		accum->hi = (1 + nhi);
		nlo -= D128_LO_LIMIT;
	}
	accum->lo = nlo;
}

struct _dat_stats {
	uint64_t	last;
	uint64_t	max;
	d128		sum;
};

struct dat_stats {
	spinlock_t		lock;
	uint64_t		count; /* Completed ops */
	atomic64_t		started; /* Started ops */
	struct _dat_stats	s;
};

/* composite double stats, hence the suffix _cd */
struct dat_stats_cd {
	spinlock_t		lock;
	uint64_t		count;
	atomic64_t		started;
	struct _dat_stats	s1;
	struct _dat_stats	s2;
};

/* common routines */
#define __dat_stats_clear(_stats)		\
do {						\
	(_stats)->last = 0;			\
	(_stats)->max = 0;			\
	(_stats)->sum.lo = 0;			\
	(_stats)->sum.hi = 0;			\
} while (0)

#define dat_stats_init(_stats)			\
do {						\
	spin_lock_init(&(_stats)->lock);	\
	(_stats)->count = 0;			\
	atomic64_set(&(_stats)->started, 0);	\
	__dat_stats_clear(&(_stats)->s);	\
} while (0)

void dat_stats_clear(struct dat_stats *stats);
void dat_stats_clear_with_irqlock(struct dat_stats *stats);
void dat_stats_update(struct dat_stats *stats, uint64_t size);
void dat_stats_update_with_irqlock(struct dat_stats *stats, uint64_t size);

/* time stats */
#define dat_time_stats_init dat_stats_init
#define dat_time_stats_clear dat_stats_clear
#define dat_time_stats_clear_with_irqlock dat_stats_clear_with_irqlock

static inline void dat_time_start(struct dat_stats *stats)
{
	atomic64_inc(&stats->started);
}

void dat_time_stats_update(struct dat_stats *stats, uint64_t time);
void dat_time_stats_update_with_irqlock(struct dat_stats *stats, uint64_t time);

/* composite stats - time, size */
#define dat_time_size_stats_init(_stats)		\
do {							\
	spin_lock_init(&(_stats)->lock);		\
	(_stats)->count = 0;				\
	atomic64_set(&(_stats)->started, 0);		\
	__dat_stats_clear(&(_stats)->s1);		\
	__dat_stats_clear(&(_stats)->s2);		\
} while (0)

static inline void dat_time_size_start(struct dat_stats_cd *stats)
{
	atomic64_inc(&stats->started);
}

void dat_time_size_stats_clear(struct dat_stats_cd *stats);
void dat_time_size_stats_clear_with_irqlock(struct dat_stats_cd *stats);
void dat_time_size_stats_update(struct dat_stats_cd *stats, uint64_t time,
				uint64_t size);
void dat_time_size_stats_update_with_irqlock(struct dat_stats_cd *stats,
					     uint64_t time, uint64_t size);
int dat_time_size_stats_print(struct dat_stats_cd *stats, char *str,
			      char *buf);

#endif /* _DSTATS_H */
