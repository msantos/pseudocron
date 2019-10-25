/* Copyright 2018-2019 Michael Santos <michael.santos@gmail.com>
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
#ifdef RESTRICT_PROCESS_capsicum
#include <sys/capability.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>

int restrict_process_init() {
  struct rlimit rl = {0};
  cap_rights_t policy_read;
  cap_rights_t policy_write;

  if (setrlimit(RLIMIT_NPROC, &rl) < 0)
    return -1;

  (void)cap_rights_init(&policy_read, CAP_READ);
  (void)cap_rights_init(&policy_write, CAP_WRITE, CAP_FSTAT);

  if (cap_rights_limit(STDIN_FILENO, &policy_read) < 0)
    return -1;

  if (cap_rights_limit(STDOUT_FILENO, &policy_write) < 0)
    return -1;

  if (cap_rights_limit(STDERR_FILENO, &policy_write) < 0)
    return -1;

  return cap_enter();
}
#endif
