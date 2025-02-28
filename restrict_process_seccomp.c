/* Copyright 2018-2025 Michael Santos <michael.santos@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pseudocron.h"
#ifdef RESTRICT_PROCESS_seccomp
#include <errno.h>
#include <stddef.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

/* macros from openssh-7.2/sandbox-seccomp-filter.c */

/* Linux seccomp_filter sandbox */
#define SECCOMP_FILTER_FAIL SECCOMP_RET_KILL

/* Use a signal handler to emit violations when debugging */
#ifdef SANDBOX_SECCOMP_FILTER_DEBUG
#undef SECCOMP_FILTER_FAIL
#define SECCOMP_FILTER_FAIL SECCOMP_RET_TRAP
#endif /* SANDBOX_SECCOMP_FILTER_DEBUG */

/* Simple helpers to avoid manual errors (but larger BPF programs). */
#define SC_DENY(_nr, _errno)                                                   \
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_##_nr, 0, 1),                       \
      BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ERRNO | (_errno))
#define SC_ALLOW(_nr)                                                          \
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_##_nr, 0, 1),                       \
      BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW)
#define SC_ALLOW_ARG(_nr, _arg_nr, _arg_val)                                   \
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_##_nr, 0,                           \
           4), /* load first syscall argument */                               \
      BPF_STMT(BPF_LD + BPF_W + BPF_ABS,                                       \
               offsetof(struct seccomp_data, args[(_arg_nr)])),                \
      BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, (_arg_val), 0, 1),                   \
      BPF_STMT(BPF_RET + BPF_K,                                                \
               SECCOMP_RET_ALLOW), /* reload syscall number; all rules expect  \
                                      it in accumulator */                     \
      BPF_STMT(BPF_LD + BPF_W + BPF_ABS, offsetof(struct seccomp_data, nr))

/*
 * http://outflux.net/teach-seccomp/
 * https://github.com/gebi/teach-seccomp
 *
 */
#define syscall_nr (offsetof(struct seccomp_data, nr))
#define arch_nr (offsetof(struct seccomp_data, arch))

#if defined(__i386__)
#define SECCOMP_AUDIT_ARCH AUDIT_ARCH_I386
#elif defined(__x86_64__)
#define SECCOMP_AUDIT_ARCH AUDIT_ARCH_X86_64
#elif defined(__arm__)
#define SECCOMP_AUDIT_ARCH AUDIT_ARCH_ARM
#elif defined(__aarch64__)
#define SECCOMP_AUDIT_ARCH AUDIT_ARCH_AARCH64
#else
#warning "seccomp: unsupported platform"
#define SECCOMP_AUDIT_ARCH 0
#endif

int restrict_process_init(void) {
  struct sock_filter filter[] = {
      /* Ensure the syscall arch convention is as expected. */
      BPF_STMT(BPF_LD + BPF_W + BPF_ABS, offsetof(struct seccomp_data, arch)),
      BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, SECCOMP_AUDIT_ARCH, 1, 0),
      BPF_STMT(BPF_RET + BPF_K, SECCOMP_FILTER_FAIL),
      /* Load the syscall number for checking. */
      BPF_STMT(BPF_LD + BPF_W + BPF_ABS, offsetof(struct seccomp_data, nr)),

  /* Syscalls to non-fatally deny */

  /* Syscalls to allow */
#ifdef __NR_brk
      SC_ALLOW(brk),
#endif
#ifdef __NR_exit_group
      SC_ALLOW(exit_group),
#endif

#ifdef __NR_nanosleep
      SC_ALLOW(nanosleep),
#endif
#ifdef __NR_clock_getres
      SC_ALLOW(clock_getres),
#endif
#ifdef __NR_clock_gettime
      SC_ALLOW(clock_gettime),
#endif
#ifdef __NR_clock_gettime64
      SC_ALLOW(clock_gettime64),
#endif
#ifdef __NR_gettimeofday
      SC_ALLOW(gettimeofday),
#endif

  /* /etc/localtime */
#ifdef __NR_fstat
      SC_ALLOW(fstat),
#endif
#ifdef __NR_fstat64
      SC_ALLOW(fstat64),
#endif
#ifdef __NR_stat
      SC_ALLOW(stat),
#endif
#ifdef __NR_stat64
      SC_ALLOW(stat64),
#endif
#ifdef __NR_newfstatat
      SC_ALLOW(newfstatat),
#endif

  /* stdio */
#ifdef __NR_read
      SC_ALLOW(read),
#endif
#ifdef __NR_readv
      SC_ALLOW(readv),
#endif
#ifdef __NR_write
      SC_ALLOW(write),
#endif
#ifdef __NR_writev
      SC_ALLOW(writev),
#endif

  /* sigwinch */
#ifdef __NR_ioctl
      SC_ALLOW(ioctl),
#endif
#ifdef __NR_restart_syscall
      SC_ALLOW(restart_syscall),
#endif

#ifdef __NR_mmap
      SC_ALLOW(mmap),
#endif
#ifdef __NR_mprotect
      SC_ALLOW(mprotect),
#endif
#ifdef __NR_munmap
      SC_ALLOW(munmap),
#endif

      /* Default deny */
      BPF_STMT(BPF_RET + BPF_K, SECCOMP_FILTER_FAIL)};

  struct sock_fprog prog = {
      .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
      .filter = filter,
  };

  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0)
    return -1;

  return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
}
#endif
