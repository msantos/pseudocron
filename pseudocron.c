/*
 * Copyright 2018 Michael Santos <michael.santos@gmail.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <getopt.h>
#include <time.h>
#include <string.h>

#include "ccronexpr.h"
#include "pseudocron.h"

#define PSEUDOCRON_VERSION "0.1.0"

static time_t timestamp(const char *s);
static int fields(const char *s);
static int arg_to_timespec(const char *arg, size_t arglen,
    char *buf, size_t buflen);
static const char *alias_to_timespec(const char *alias);
static void usage();

extern char *__progname;

static struct pseudocron_alias {
  const char *name;
  const char *timespec;
} pseudocron_aliases[] = {
  {"@reboot",   "0 * * * * *"},
  {"@yearly",   "0 0 0 1 1 *"},
  {"@annually", "0 0 0 1 1 *"},
  {"@monthly",  "0 0 0 1 * *"},
  {"@weekly",   "0 0 0 * * 0"},
  {"@daily",    "0 0 0 * * *"},
  {"@midnight", "0 0 0 * * *"},
  {"@hourly",   "0 0 * * * *"},
  {NULL,        NULL}
};

enum {
  OPT_STDIN = 1,
  OPT_TIMESTAMP = 2,
  OPT_PRINT = 4,
  OPT_DRYRUN = 8
};

static const struct option long_options[] =
{
  {"stdin",       no_argument,        NULL, OPT_STDIN},
  {"dryrun",      no_argument,        NULL, 'n'},
  {"print",       no_argument,        NULL, 'p'},
  {"timestamp",   required_argument,  NULL, OPT_TIMESTAMP},
  {"verbose",     no_argument,        NULL, 'v'},
  {"help",        no_argument,        NULL, 'h'},
  {NULL,          0,                  NULL, 0}
};

  int
main(int argc, char *argv[])
{
  cron_expr expr = {0};
  const char *errbuf = NULL;
  char buf[255] = {0};
  char arg[252] = {0};
  time_t now;
  time_t next;
  double diff;
  int opt = 0;
  int verbose = 0;
  int ch;
  int rv;

  /* initialize local time before entering sandbox */
  now = time(NULL);
  if (now == -1)
    err(EXIT_FAILURE, "time");

  (void)localtime(&now);

  if (sandbox_init() < 0)
    err(3, "sandbox_init");

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
          errx(EXIT_FAILURE, "invalid timestamp: %s", optarg);
        break;

      case 'h':
      default:
        usage();
    }
  }

  argc -= optind;
  argv += optind;

  switch (argc) {
    case 0: {
      char *nl = NULL;

      if (!(opt & OPT_STDIN))
        usage();

      if (read(0, arg, sizeof(arg)-1) < 0)
        err(EXIT_FAILURE, "read failure");

      nl = strchr(arg, '\n');
      if (nl != NULL)
        *nl = '\0';
      }
      break;

    case 1:
      rv = snprintf(arg, sizeof(arg), "%s", argv[0]);
      if (rv < 0 || rv >= sizeof(arg))
        errx(EXIT_FAILURE, "timespec exceeds maximum length: %zu",
            sizeof(arg));
      break;

    default:
      usage();
      break;
  }

  if (arg_to_timespec(arg, sizeof(arg), buf, sizeof(buf)) < 0)
    errx(EXIT_FAILURE, "invalid crontab timespec");

  if (verbose > 1)
    (void)fprintf(stderr, "crontab=%s\n", buf);

  cron_parse_expr(buf, &expr, &errbuf);
  if (errbuf)
    errx(EXIT_FAILURE, "invalid crontab timespec: %s", errbuf);

  next = cron_next(&expr, now);
  if (next == -1)
    err(EXIT_FAILURE, "time");

  if (verbose > 0) {
    (void)fprintf(stderr, "now[%ld]=%s", now, ctime(&now));
    (void)fprintf(stderr, "next[%ld]=%s", next, ctime(&next));
  }

  diff = difftime(next, now);
  if (diff < 0)
    errx(EXIT_FAILURE, "difftime: %.f", diff);

  if (opt & OPT_PRINT)
    (void)printf("%.f\n", diff);

  if (!(opt & OPT_DRYRUN)) {
    int sleepfor = diff;
    while (sleepfor > 0)
      sleepfor = sleep(sleepfor);
  }

  if (verbose > 1) {
    now = time(NULL);
    if (now == -1)
      err(EXIT_FAILURE, "time");
    (void)fprintf(stderr, "exit[%ld]=%s", now, ctime(&now));
  }

  return 0;
}

  static int
fields(const char *s)
{
  int n = 0;
  const char *p = s;
  int field = 0;

  for (; *p != '\0'; p++) {
    if (*p != ' ') {
      if (!field) {
        n++;
        field = 1;
      }
    }
    else {
      field = 0;
    }
  }

  return n;
}

  static int
arg_to_timespec(const char *arg, size_t arglen, char *buf, size_t buflen)
{
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

  return (rv < 0 || rv >= buflen) ? -1 : 0;
}

  static time_t
timestamp(const char *s)
{
  struct tm tm = {0};

  if (strptime(s, "%Y-%m-%d %T", &tm) == NULL)
    return -1;

  return mktime(&tm);
}

  static const char *
alias_to_timespec(const char *name)
{
  struct pseudocron_alias *ap = NULL;

  for (ap = pseudocron_aliases; ap->name != NULL; ap++) {
    if (strcmp(name, ap->name) == 0) {
      return ap->timespec;
    }
  }

  return NULL;
}

  static void
usage()
{
  errx(EXIT_FAILURE, "[OPTION] <CRONTAB EXPRESSION>\n"
    "version: %s (using %s sandbox)\n\n"
    "-n, --dryrun           do nothing\n"
    "-p, --print            output seconds to next timespec\n"
    "-v, --verbose          verbose mode\n"
    "    --timestamp <YY-MM-DD hh-mm-ss>\n"
    "                       provide an initial time\n"
    "    --stdin            read crontab from stdin\n",
    PSEUDOCRON_VERSION,
    PSEUDOCRON_SANDBOX);
}
