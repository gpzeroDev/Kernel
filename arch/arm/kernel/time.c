/*
 *  linux/arch/arm/kernel/time.c
 *
 *  Copyright (C) 1991, 1992, 1995  Linus Torvalds
 *  Modifications for ARM (C) 1994-2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains the ARM-specific time handling details:
 *  reading the RTC at bootup, etc...
 *
 *  1994-07-02  Alan Modra
 *              fixed set_rtc_mmss, fixed time.year for >= 2000, new mktime
 *  1998-12-20  Updated NTP code according to technical memorandum Jan '96
 *              "A Kernel Model for Precision Timekeeping" by Dave Mills
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/timex.h>
#include <linux/errno.h>
#include <linux/profile.h>
#include <linux/sysdev.h>
#include <linux/timer.h>
#include <linux/irq.h>

#include <linux/mc146818rtc.h>

#include <asm/leds.h>
#include <asm/thread_info.h>
#include <asm/stacktrace.h>
#include <asm/mach/time.h>

/*
 * Our system timer.
 */
struct sys_timer *system_timer;

#if defined(CONFIG_RTC_DRV_CMOS) || defined(CONFIG_RTC_DRV_CMOS_MODULE)
/* this needs a better home */
DEFINE_SPINLOCK(rtc_lock);

#ifdef CONFIG_RTC_DRV_CMOS_MODULE
EXPORT_SYMBOL(rtc_lock);
#endif
#endif	/* pc-style 'CMOS' RTC support */

/* change this if you have some constant time drift */
#define USECS_PER_JIFFY	(1000000/HZ)

#ifdef CONFIG_SMP
unsigned long profile_pc(struct pt_regs *regs)
{
	struct stackframe frame;

	if (!in_lock_functions(regs->ARM_pc))
		return regs->ARM_pc;

	frame.fp = regs->ARM_fp;
	frame.sp = regs->ARM_sp;
	frame.lr = regs->ARM_lr;
	frame.pc = regs->ARM_pc;
	do {
		int ret = unwind_frame(&frame);
		if (ret < 0)
			return 0;
	} while (in_lock_functions(frame.pc));

	return frame.pc;
}
EXPORT_SYMBOL(profile_pc);
#endif

/*
 * hook for setting the RTC's idea of the current time.
 */
int (*set_rtc)(void);

#ifndef CONFIG_GENERIC_TIME
static unsigned long dummy_gettimeoffset(void)
{
	return 0;
}
#endif

static unsigned long next_rtc_update;

/*
 * If we have an externally synchronized linux clock, then update
 * CMOS clock accordingly every ~11 minutes.  set_rtc() has to be
 * called as close as possible to 500 ms before the new second
 * starts.
 */
static inline void do_set_rtc(void)
{
	if (!ntp_synced() || set_rtc == NULL)
		return;

	if (next_rtc_update &&
	    time_before((unsigned long)xtime.tv_sec, next_rtc_update))
		return;

	if (xtime.tv_nsec < 500000000 - ((unsigned) tick_nsec >> 1) &&
	    xtime.tv_nsec >= 500000000 + ((unsigned) tick_nsec >> 1))
		return;

	if (set_rtc())
		/*
		 * rtc update failed.  Try again in 60s
		 */
		next_rtc_update = xtime.tv_sec + 60;
	else
		next_rtc_update = xtime.tv_sec + 660;
}

#ifdef CONFIG_LEDS_TIMER
static inline void do_leds(void)
{
	static unsigned int count = HZ/2;

	if (--count == 0) {
		count = HZ/2;
		leds_event(led_timer);
	}
}
#else
#define	do_leds()
#endif

#ifndef CONFIG_GENERIC_TIME
void do_gettimeofday(struct timeval *tv)
{
	unsigned long flags;
	unsigned long seq;
	unsigned long usec, sec;

	do {
		seq = read_seqbegin_irqsave(&xtime_lock, flags);
		usec = system_timer->offset();
		sec = xtime.tv_sec;
		usec += xtime.tv_nsec / 1000;
	} while (read_seqretry_irqrestore(&xtime_lock, seq, flags));

	/* usec may have gone up a lot: be safe */
	while (usec >= 1000000) {
		usec -= 1000000;
		sec++;
	}

	tv->tv_sec = sec;
	tv->tv_usec = usec;
}

EXPORT_SYMBOL(do_gettimeofday);

int do_settimeofday(struct timespec *tv)
{
	time_t wtm_sec, sec = tv->tv_sec;
	long wtm_nsec, nsec = tv->tv_nsec;

	if ((unsigned long)tv->tv_nsec >= NSEC_PER_SEC)
		return -EINVAL;

	write_seqlock_irq(&xtime_lock);
	/*
	 * This is revolting. We need to set "xtime" correctly. However, the
	 * value in this location is the value at the most recent update of
	 * wall time.  Discover what correction gettimeofday() would have
	 * done, and then undo it!
	 */
	nsec -= system_timer->offset() * NSEC_PER_USEC;

	wtm_sec  = wall_to_monotonic.tv_sec + (xtime.tv_sec - sec);
	wtm_nsec = wall_to_monotonic.tv_nsec + (xtime.tv_nsec - nsec);

	set_normalized_timespec(&xtime, sec, nsec);
	set_normalized_timespec(&wall_to_monotonic, wtm_sec, wtm_nsec);

	ntp_clear();
	write_sequnlock_irq(&xtime_lock);
	clock_was_set();
	return 0;
}

EXPORT_SYMBOL(do_settimeofday);
#endif /* !CONFIG_GENERIC_TIME */

/**
 * save_time_delta - Save the offset between system time and RTC time
 * @delta: pointer to timespec to store delta
 * @rtc: pointer to timespec for current RTC time
 *
 * Return a delta between the system time and the RTC time, such
 * that system time can be restored later with restore_time_delta()
 */
void save_time_delta(struct timespec *delta, struct timespec *rtc)
{
	set_normalized_timespec(delta,
				xtime.tv_sec - rtc->tv_sec,
				xtime.tv_nsec - rtc->tv_nsec);
}
EXPORT_SYMBOL(save_time_delta);

/**
 * restore_time_delta - Restore the current system time
 * @delta: delta returned by save_time_delta()
 * @rtc: pointer to timespec for current RTC time
 */
void restore_time_delta(struct timespec *delta, struct timespec *rtc)
{
	struct timespec ts;

	set_normalized_timespec(&ts,
				delta->tv_sec + rtc->tv_sec,
				delta->tv_nsec + rtc->tv_nsec);

	do_settimeofday(&ts);
}
EXPORT_SYMBOL(restore_time_delta);

#ifndef CONFIG_GENERIC_CLOCKEVENTS
/*
 * Kernel system timer support.
 */
void timer_tick(void)
{
	profile_tick(CPU_PROFILING);
	do_leds();
	do_set_rtc();
	write_seqlock(&xtime_lock);
	do_timer(1);
	write_sequnlock(&xtime_lock);
#ifndef CONFIG_SMP
	update_process_times(user_mode(get_irq_regs()));
#endif
}
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_GENERIC_CLOCKEVENTS)
static int timer_suspend(struct sys_device *dev, pm_message_t state)
{
	struct sys_timer *timer = container_of(dev, struct sys_timer, dev);

	if (timer->suspend != NULL)
		timer->suspend();

	return 0;
}

static int timer_resume(struct sys_device *dev)
{
	struct sys_timer *timer = container_of(dev, struct sys_timer, dev);

	if (timer->resume != NULL)
		timer->resume();

	return 0;
}
#else
#define timer_suspend NULL
#define timer_resume NULL
#endif

static struct sysdev_class timer_sysclass = {
	.name		= "timer",
	.suspend	= timer_suspend,
	.resume		= timer_resume,
};

static int __init timer_init_sysfs(void)
{
	int ret = sysdev_class_register(&timer_sysclass);
	if (ret == 0) {
		system_timer->dev.cls = &timer_sysclass;
		ret = sysdev_register(&system_timer->dev);
	}

	return ret;
}

device_initcall(timer_init_sysfs);

void __init time_init(void)
{
#ifndef CONFIG_GENERIC_TIME
	if (system_timer->offset == NULL)
		system_timer->offset = dummy_gettimeoffset;
#endif
	system_timer->init();
}

