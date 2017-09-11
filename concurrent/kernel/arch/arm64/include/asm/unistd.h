/*
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef CONFIG_COMPAT
#define __ARCH_WANT_COMPAT_STAT64
#define __ARCH_WANT_SYS_GETHOSTNAME
#define __ARCH_WANT_SYS_PAUSE
#define __ARCH_WANT_SYS_GETPGRP
#define __ARCH_WANT_SYS_LLSEEK
#define __ARCH_WANT_SYS_NICE
#define __ARCH_WANT_SYS_SIGPENDING
#define __ARCH_WANT_SYS_SIGPROCMASK
#define __ARCH_WANT_COMPAT_SYS_SENDFILE
#define __ARCH_WANT_SYS_FORK
#define __ARCH_WANT_SYS_VFORK

/*
 * Compat syscall numbers used by the AArch64 kernel.
 */
#define __NR_compat_restart_syscall	0
#define __NR_compat_exit		1
#define __NR_compat_read		3
#define __NR_compat_write		4
#define __NR_compat_sigreturn		119
#define __NR_compat_rt_sigreturn	173

/*
 * The following SVCs are ARM private.
 */
#define __ARM_NR_COMPAT_BASE		0x0f0000
#define __ARM_NR_compat_cacheflush	(__ARM_NR_COMPAT_BASE+2)
#define __ARM_NR_compat_set_tls		(__ARM_NR_COMPAT_BASE+5)

#define __NR_compat_syscalls		384
#endif

#define __ARCH_WANT_SYS_CLONE
#include <uapi/asm/unistd.h>

/*
 * New system calls for light intensity synchronization
 */

#define __NR_set_light_intensity (__NR_arch_specific_syscall + 0) /* 244 */
__SYSCALL(__NR_set_light_intensity, sys_set_light_intensity)

#define __NR_get_light_intensity (__NR_arch_specific_syscall + 1) /* 245 */
__SYSCALL(__NR_get_light_intensity, sys_get_light_intensity)

#define __NR_light_evt_create (__NR_arch_specific_syscall + 2) /* 246 */
__SYSCALL(__NR_light_evt_create, sys_light_evt_create)

#define __NR_light_evt_wait (__NR_arch_specific_syscall + 3) /* 247 */
__SYSCALL(__NR_light_evt_wait, sys_light_evt_wait)

#define __NR_light_evt_signal (__NR_arch_specific_syscall + 4) /* 248 */
__SYSCALL(__NR_light_evt_signal, sys_light_evt_signal)

#define __NR_light_evt_destroy (__NR_arch_specific_syscall + 5) /* 249 */
__SYSCALL(__NR_light_evt_destroy, sys_light_evt_destroy)

