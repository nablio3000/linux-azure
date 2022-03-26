#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <target/dstats.h>

/* These are generic routines for dat_stats
 * for time specific operations, use the *time_stats routines
 */
void dat_stats_clear(struct dat_stats *stats)
{
	spin_lock(&stats->lock);
	stats->count = 0;
	atomic64_set(&stats->started, 0);
	__dat_stats_clear(&stats->s);
	spin_unlock(&stats->lock);
}
EXPORT_SYMBOL(dat_stats_clear);

void dat_stats_clear_with_irqlock(struct dat_stats *stats)
{
	unsigned long flags;

	spin_lock_irqsave(&stats->lock, flags);
	stats->count = 0;
	atomic64_set(&stats->started, 0);
	__dat_stats_clear(&stats->s);
	spin_unlock_irqrestore(&stats->lock, flags);
}
EXPORT_SYMBOL(dat_stats_clear_with_irqlock);

static void __dat_stats_update(struct _dat_stats *stats, uint64_t size)
{
	stats->last = size;
	stats->max = max(stats->max, size);

	d128_accumulate(&stats->sum, size);
}

void dat_stats_update(struct dat_stats *stats, uint64_t size)
{
	spin_lock(&stats->lock);
	stats->count++;
	__dat_stats_update(&stats->s, size);
	spin_unlock(&stats->lock);
}
EXPORT_SYMBOL(dat_stats_update);

void dat_stats_update_with_irqlock(struct dat_stats *stats, uint64_t size)
{
	unsigned long flags;

	spin_lock_irqsave(&stats->lock, flags);
	stats->count++;
	__dat_stats_update(&stats->s, size);
	spin_unlock_irqrestore(&stats->lock, flags);
}
EXPORT_SYMBOL(dat_stats_update_with_irqlock);

/* time stats */

static uint64_t dat_time_get_duration(uint64_t start_time)
{
	uint64_t now = local_clock(), duration;

	duration	= (time_after64(now, start_time)
			   ? (now - start_time) : 0);
	return duration;
}

static void __dat_time_stats_update(struct _dat_stats *stats,
				    uint64_t duration)
{
	stats->last = duration;
	stats->max = max(stats->max, duration);

	d128_accumulate(&stats->sum, duration);
}

void dat_time_stats_update(struct dat_stats *stats, uint64_t start_time)
{
	uint64_t duration = dat_time_get_duration(start_time);

	spin_lock(&stats->lock);
	stats->count++;
	__dat_time_stats_update(&stats->s, duration);
	spin_unlock(&stats->lock);
}
EXPORT_SYMBOL(dat_time_stats_update);

void dat_time_stats_update_with_irqlock(struct dat_stats *stats,
					uint64_t start_time)
{
	uint64_t duration = dat_time_get_duration(start_time);
	unsigned long flags;

	spin_lock_irqsave(&stats->lock, flags);
	stats->count++;
	__dat_time_stats_update(&stats->s, duration);
	spin_unlock_irqrestore(&stats->lock, flags);
}
EXPORT_SYMBOL(dat_time_stats_update_with_irqlock);

/* composite stats - time, size */
void dat_time_size_stats_clear(struct dat_stats_cd *stats)
{
	spin_lock(&stats->lock);
	stats->count = 0;
	atomic64_set(&stats->started, 0);
	__dat_stats_clear(&stats->s1);
	__dat_stats_clear(&stats->s2);
	spin_unlock(&stats->lock);
}
EXPORT_SYMBOL(dat_time_size_stats_clear);

void dat_time_size_stats_clear_with_irqlock(struct dat_stats_cd *stats)
{
	unsigned long flags;

	spin_lock_irqsave(&stats->lock, flags);
	stats->count = 0;
	atomic64_set(&stats->started, 0);
	__dat_stats_clear(&stats->s1);
	__dat_stats_clear(&stats->s2);
	spin_unlock_irqrestore(&stats->lock, flags);
}
EXPORT_SYMBOL(dat_time_size_stats_clear_with_irqlock);

void dat_time_size_stats_update(struct dat_stats_cd *stats, uint64_t time,
				uint64_t size)
{
	uint64_t duration = dat_time_get_duration(time);

	spin_lock(&stats->lock);
	stats->count++;
	__dat_time_stats_update(&stats->s1, duration);
	__dat_stats_update(&stats->s2, size);
	spin_unlock(&stats->lock);
}
EXPORT_SYMBOL(dat_time_size_stats_update);

void dat_time_size_stats_update_with_irqlock(struct dat_stats_cd *stats,
					     uint64_t time, uint64_t size)
{
	uint64_t duration = dat_time_get_duration(time);
	unsigned long flags;

	spin_lock_irqsave(&stats->lock, flags);
	stats->count++;
	__dat_time_stats_update(&stats->s1, duration);
	__dat_stats_update(&stats->s2, size);
	spin_unlock_irqrestore(&stats->lock, flags);
}
EXPORT_SYMBOL(dat_time_size_stats_update_with_irqlock);

int dat_time_size_stats_print(struct dat_stats_cd *stats, char *str, char *buf)
{
	int ret;
	unsigned long flags;
	struct _dat_stats st[2];
	uint64_t started, count;

	spin_lock_irqsave(&stats->lock, flags);
	count = stats->count;
	started = atomic64_read(&stats->started);
	st[0] = stats->s1;
	st[1] = stats->s2;
	spin_unlock_irqrestore(&stats->lock, flags);

	ret = snprintf(buf, PAGE_SIZE,
		       "%s time statistics:\n"
		       "Started = %llu\n"
		       "Completed = %llu\n"
		       "Last duration ns = %llu\n"
		       "Maximum duration ns = %llu\n"
		       "Sum duration = { %llu : %llu } ns\n\n"
		       "%s size statistics:\n"
		       "Last bytes = %llu\n"
		       "Maximum bytes = %llu\n"
		       "Sum = { %llu : %llu } bytes\n",
		       str,
		       started,
		       count,
		       st[0].last,
		       st[0].max,
		       st[0].sum.hi,
		       st[0].sum.lo,
		       str,
		       st[1].last,
		       st[1].max,
		       st[1].sum.hi,
		       st[1].sum.lo);

	return ret;
}
EXPORT_SYMBOL(dat_time_size_stats_print);
