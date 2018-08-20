/* Copyright 2018 Michael Santos <michael.santos@gmail.com>
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
#ifdef PSEUDOCRON_SANDBOX_capsicum
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/capability.h>

#include <termios.h>

#include <errno.h>

    int
sandbox_init()
{
    struct rlimit rl = {0};
    cap_rights_t policy_read;
    cap_rights_t policy_write;

    const unsigned long cmds[] = { TIOCGETA, TIOCGWINSZ };

    if (setrlimit(RLIMIT_NPROC, &rl) < 0)
      return -1;

    (void)cap_rights_init(&policy_read, CAP_READ);
    (void)cap_rights_init(&policy_write, CAP_WRITE, CAP_FSTAT, CAP_IOCTL);

    /* stdin */
    if (cap_rights_limit(STDIN_FILENO, &policy_read) < 0)
        return -1;

    /* stdout */
    if (cap_rights_limit(STDOUT_FILENO, &policy_write) < 0)
        return -1;

    if (cap_ioctls_limit(STDOUT_FILENO, cmds, sizeof(cmds)) < 0)
        return -1;

    /* stderr */
    if (cap_rights_limit(STDERR_FILENO, &policy_write) < 0)
        return -1;

    if (cap_ioctls_limit(STDERR_FILENO, cmds, sizeof(cmds)) < 0)
        return -1;

    return cap_enter();
}
#endif
