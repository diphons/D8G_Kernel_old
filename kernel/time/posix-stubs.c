/*
 * Dummy stubs used when CONFIG_POSIX_TIMERS=n
 *
 * Created by:  Nicolas Pitre, July 2016
 * Copyright:   (C) 2016 Linaro Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/syscalls.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/posix-timers.h>

asmlinkage long sys_ni_posix_timers(void)
{
	pr_err_once("process %d (%s) attempted a POSIX timer syscall "
		    "while CONFIG_POSIX_TIMERS is not set\n",
		    current->pid, current->comm);
	return -ENOSYS;
}

#define SYS_NI(name)  SYSCALL_ALIAS(sys_##name, sys_ni_posix_timers)
#define COMPAT_SYS_NI(name)  SYSCALL_ALIAS(compat_sys_##name, sys_ni_posix_timers)

SYS_NI(timer_create);
SYS_NI(timer_gettime);
SYS_NI(timer_getoverrun);
SYS_NI(timer_settime);
SYS_NI(timer_delete);
SYS_NI(clock_adjtime);
SYS_NI(getitimer);
SYS_NI(setitimer);
#ifdef __ARCH_WANT_SYS_ALARM
SYS_NI(alarm);
#endif
COMPAT_SYS_NI(clock_adjtime);

/*
 * We preserve minimal support for CLOCK_REALTIME and CLOCK_MONOTONIC
 * as it is easy to remain compatible with little code. CLOCK_BOOTTIME
 * is also included for convenience as at least systemd uses it.
 */

SYSCALL_DEFINE2(clock_settime, const clockid_t, which_clock,
		const struct timespec __user *, tp)
{
	struct timespec new_tp;

	if (which_clock != CLOCK_REALTIME)
		return -EINVAL;
	if (copy_from_user(&new_tp, tp, sizeof (*tp)))
		return -EFAULT;
	return do_sys_settimeofday(&new_tp, NULL);
}

SYSCALL_DEFINE2(clock_gettime, const clockid_t, which_clock,
		struct timespec __user *,tp)
{
	struct timespec kernel_tp;

	switch (which_clock) {
	case CLOCK_REALTIME: ktime_get_real_ts(&kernel_tp); break;
	case CLOCK_MONOTONIC: ktime_get_ts(&kernel_tp); break;
	case CLOCK_BOOTTIME: get_monotonic_boottime(&kernel_tp); break;
	default: return -EINVAL;
	}
	if (copy_to_user(tp, &kernel_tp, sizeof (kernel_tp)))
		return -EFAULT;
	return 0;
}

SYSCALL_DEFINE2(clock_getres, const clockid_t, which_clock, struct timespec __user *, tp)
{
	struct timespec rtn_tp = {
		.tv_sec = 0,
		.tv_nsec = hrtimer_resolution,
	};

	switch (which_clock) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
	case CLOCK_BOOTTIME:
		if (copy_to_user(tp, &rtn_tp, sizeof(rtn_tp)))
			return -EFAULT;
		return 0;
	default:
		return -EINVAL;
	}
}

SYSCALL_DEFINE4(clock_nanosleep, const clockid_t, which_clock, int, flags,
		const struct timespec __user *, rqtp,
		struct timespec __user *, rmtp)
{
	struct timespec t;

	switch (which_clock) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
	case CLOCK_BOOTTIME:
		if (copy_from_user(&t, rqtp, sizeof (struct timespec)))
			return -EFAULT;
		if (!timespec_valid(&t))
			return -EINVAL;
		current->restart_block.fn = do_no_restart_syscall;
		return hrtimer_nanosleep(&t, rmtp, flags & TIMER_ABSTIME ?
					 HRTIMER_MODE_ABS : HRTIMER_MODE_REL,
					 which_clock);
	default:
		return -EINVAL;
	}
}

#ifdef CONFIG_COMPAT
long clock_nanosleep_restart(struct restart_block *restart_block)
{
	return hrtimer_nanosleep_restart(restart_block);
}
#endif
