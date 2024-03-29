/*
 * Copyright 2018-2023 Michael Santos <michael.santos@gmail.com>
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

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ccronexpr.h"
#include "pseudocron.h"

#define PSEUDOCRON_VERSION "0.4.1"

static time_t timestamp(const char *s);
static int fields(const char *s);
static int arg_to_timespec(const char *arg, size_t arglen, char *buf,
                           size_t buflen);
static const char *alias_to_timespec(const char *alias);
static void usage(void);

extern char *__progname;

static struct pseudocron_alias {
  const char *name;
  const char *timespec;
} pseudocron_aliases[] = {{"@reboot", "* * * * * *"},
                          {"@yearly", "0 0 0 1 1 *"},
                          {"@annually", "0 0 0 1 1 *"},
                          {"@monthly", "0 0 0 1 * *"},
                          {"@weekly", "0 0 0 * * 0"},
                          {"@daily", "0 0 0 * * *"},
                          {"@midnight", "0 0 0 * * *"},
                          {"@hourly", "0 0 * * * *"},

                          {"@never", "@never"},

                          {NULL, NULL}};

enum { OPT_STDIN = 1, OPT_TIMESTAMP = 2, OPT_PRINT = 4, OPT_DRYRUN = 8 };

static const struct option long_options[] = {
    {"stdin", no_argument, NULL, OPT_STDIN},
    {"dryrun", no_argument, NULL, 'n'},
    {"print", no_argument, NULL, 'p'},
    {"timestamp", required_argument, NULL, OPT_TIMESTAMP},
    {"verbose", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

int main(int argc, char *argv[]) {
  cron_expr expr = {0};
  const char *errbuf = NULL;
  char buf[255] = {0};
  char arg[252] = {0};
  char *p;
  time_t now;
  time_t next;
  double diff;
  int opt = 0;
  int verbose = 0;
  int ch;
  int rv;

  /* initialize local time before enabling process restrictions */
  now = time(NULL);
  if (now == -1)
    err(EXIT_FAILURE, "error: time");

  (void)localtime(&now);

  if (restrict_process_init() < 0)
    err(3, "error: restrict_process_init");

  while ((ch = getopt_long(argc, argv, "hnpv", long_options, NULL)) != -1) {
    switch (ch) {
    case 'n':
      opt |= OPT_DRYRUN;
      break;

    case 'p':
      opt |= OPT_PRINT;
      break;

    case 'v':
      verbose++;
      break;

    case OPT_STDIN:
      opt |= OPT_STDIN;
      break;

    case OPT_TIMESTAMP:
      now = timestamp(optarg);
      if (now == -1)
        errx(2, "error: invalid timestamp: %s", optarg);
      break;

    case 'h':
      usage();
      exit(0);
    default:
      usage();
      exit(2);
    }
  }

  argc -= optind;
  argv += optind;

  switch (argc) {
  case 0: {
    char *nl = NULL;

    if (!(opt & OPT_STDIN)) {
      usage();
      exit(2);
    }

    if (read(0, arg, sizeof(arg) - 1) < 0)
      err(EXIT_FAILURE, "error: read failure");

    nl = strchr(arg, '\n');
    if (nl != NULL)
      *nl = '\0';
  } break;

  case 1:
    rv = snprintf(arg, sizeof(arg), "%s", argv[0]);
    if (rv < 0 || (unsigned)rv >= sizeof(arg))
      errx(EXIT_FAILURE, "error: timespec exceeds maximum length: %zu",
           sizeof(arg));
    break;

  default:
    usage();
    exit(2);
  }

  /* replace tabs with spaces */
  for (p = arg; *p != '\0'; p++)
    if (*p == '\t' || *p == '\n' || *p == '\r')
      *p = ' ';

  if (arg_to_timespec(arg, sizeof(arg), buf, sizeof(buf)) < 0)
    errx(EXIT_FAILURE, "error: invalid crontab timespec");

  if (verbose > 1)
    (void)fprintf(stderr, "crontab=%s\n", buf);

  if ((strcmp(buf, "@never") == 0) ||
      (strcmp(arg, "@reboot") == 0 && getenv("PSEUDOCRON_REBOOT"))) {
    diff = UINT32_MAX;
    goto PSEUDOCRON_SLEEP;
  }

  cron_parse_expr(buf, &expr, &errbuf);
  if (errbuf)
    errx(EXIT_FAILURE, "error: invalid crontab timespec: %s", errbuf);

  next = cron_next(&expr, now);
  if (next == -1)
    errx(EXIT_FAILURE, "error: cron_next: next scheduled interval: %s",
         errno == 0 ? "invalid timespec" : strerror(errno));

  if (verbose > 0) {
    (void)fprintf(stderr, "now[%lld]=%s", (long long)now, ctime(&now));
    (void)fprintf(stderr, "next[%lld]=%s", (long long)next, ctime(&next));
  }

  diff = difftime(next, now);
  if (diff < 0)
    errx(EXIT_FAILURE, "error: difftime: negative duration: %.f seconds", diff);

PSEUDOCRON_SLEEP:
  if (opt & OPT_PRINT)
    (void)printf("%.f\n", diff);

  if (!(opt & OPT_DRYRUN)) {
    unsigned int sleepfor = diff > UINT32_MAX ? UINT32_MAX : (unsigned int)diff;
    while (sleepfor > 0)
      sleepfor = sleep(sleepfor);
  }

  if (verbose > 1) {
    now = time(NULL);
    if (now == -1)
      err(EXIT_FAILURE, "error: time");
    (void)fprintf(stderr, "exit[%lld]=%s", (long long)now, ctime(&now));
  }

  return 0;
}

static int fields(const char *s) {
  int n = 0;
  const char *p = s;
  int field = 0;

  for (; *p != '\0'; p++) {
    if (*p != ' ') {
      if (!field) {
        n++;
        field = 1;
      }
    } else {
      field = 0;
    }
  }

  return n;
}

static int arg_to_timespec(const char *arg, size_t arglen, char *buf,
                           size_t buflen) {
  const char *timespec;
  int n;
  int rv;

  n = fields(arg);

  switch (n) {
  case 1:
    timespec = alias_to_timespec(arg);

    if (timespec == NULL)
      return -1;

    rv = snprintf(buf, buflen, "%s", timespec);
    break;

  case 5:
    rv = snprintf(buf, buflen, "0 %s", arg);
    break;

  case 6:
  default:
    rv = snprintf(buf, buflen, "%s", arg);
    break;
  }

  return (rv < 0 || (unsigned)rv >= buflen) ? -1 : 0;
}

static time_t timestamp(const char *s) {
  struct tm tm = {0};

  switch (s[0]) {
  case '@':
    if (strptime(s + 1, "%s", &tm) == NULL)
      return -1;

    break;

  default:
    if (strptime(s, "%Y-%m-%d %T", &tm) == NULL)
      return -1;

    break;
  }

  tm.tm_isdst = -1;

  return mktime(&tm);
}

static const char *alias_to_timespec(const char *name) {
  struct pseudocron_alias *ap;

  for (ap = pseudocron_aliases; ap->name != NULL; ap++) {
    if (strcmp(name, ap->name) == 0) {
      return ap->timespec;
    }
  }

  return NULL;
}

static void usage() {
  (void)fprintf(stderr,
                "%s: [OPTION] <CRONTAB EXPRESSION>\n"
                "version: %s (using %s mode process restrition)\n\n"
                "-n, --dryrun           do nothing\n"
                "-p, --print            output seconds to next timespec\n"
                "-v, --verbose          verbose mode\n"
                "    --timestamp <YY-MM-DD hh-mm-ss|@epoch>\n"
                "                       provide an initial time\n"
                "    --stdin            read crontab from stdin\n",
                __progname, PSEUDOCRON_VERSION, RESTRICT_PROCESS);
}
