# SYNOPSIS

pseudocron [-n|-p|-v] *crontab expression*

# DESCRIPTION

pseudocron: sleep(1) using a cron expression

Also see [runcron](https://github.com/msantos/runcron).

cron expressions are parsed using
[ccronexpr](https://github.com/staticlibs/ccronexpr).

* simple

    pseudocron is sleep(1) that accepts a crontab(5) expression for the
    duration and then exits. That's all it does.

    Use a supervisor like [daemontools](https://cr.yp.to/daemontools.html)
    to run the job.

* safe

    No setuid binaries or processes running as root.

	pseudocron operations are sandboxed. It cannot open files
	or sockets or signal processes. The only ways pseudocron can
	interact with other processes is via stdin/stdout/stderr and
	the process exit value.

* container friendly

    pseudocron runs as a normal user without any special privileges.
    It does not need to modify the filesystem. pseudocron is meant to
    run in an automated environment without user intervention.

    Since processes run sequentially and synchronously, jobs won't pile
    up if the run time of the task exceeds the time spec increment unless
    specifically requested to run in the background.

    pseudocron does not modify the application environment or interfere
    with stdio.

The standard crontab(5) expressions are supported. The seconds field
is optional:

				field          allowed values
				-----          --------------
				second         0-59 (optional)
				minute         0-59
				hour           0-23
				day of month   1-31
				month          1-12 (or names, see below)
				day of week    0-7 (0 or 7 is Sun, or use names)

crontab(5) aliases also work:

				string         meaning
				------         -------
				@reboot        Run once, at startup (see below).
				@yearly        Run once a year, "0 0 1 1 *".
				@annually      (same as @yearly)
				@monthly       Run once a month, "0 0 1 * *".
				@weekly        Run once a week, "0 0 * * 0".
				@daily         Run once a day, "0 0 * * *".
				@midnight      (same as @daily)
				@hourly        Run once an hour, "0 * * * *".
				@never         Never run (sleep forever)

## @reboot

Unlike _crontab_(5), `pseudocron` will run the `@reboot` alias
immediately.

The behaviour can be mimicked by setting the `PSEUDOCRON_REBOOT`
environment variable. If the variable is unset, `@reboot` is equivalent to
`* * * * * *`.

If set to any value, `@reboot` is equivalent to `@never`.

~~~ shell
#!/bin/sh

while :; do
    # runs in the next second
    pseudocron "@reboot"
    echo test
    export PSEUDOCRON_REBOOT=1
    # equivalent to sleep infinity
done
~~~

# EXAMPLES

        # parse only the crontab expression
        # every 15 minutes (5 fields)
        pseudocron -n "*/15 * * * *"

        # parse only the crontab expression and view the calculated times
        # every 15 seconds (6 fields)
        pseudocron -nvv "*/15 * * * * *"

Writing a batch job:

        #!/bin/sh
        
        set -e
        
        # Run daily at 8:15am
        pseudocron "15 8 * * *"
        echo Running job

# OPTIONS

-n, --dryrun
: Do not sleep. Can be used to verify the crontab expression.

-p, --print
:	Output the number of seconds to the next crontab time specification.

-v, --verbose
:	Output the calculated dates to stderr.

-h, --help
:	display usage

--timestamp *YY*-*MM*-*DD* *hh*-*mm*-*ss*|*@seconds*
:	Use *timestamp* for the initial start time instead of now.

--stdin
: Read crontab expression from stdin.

# BUILDING

## Quick Install

    make

## Selecting a Sandbox

    PSEUDOCRON_SANDBOX=null make clean all

## Using musl libc

    ## Using the rlimit sandbox
    PSEUDOCRON_SANDBOX=rlimit ./musl-make

    ## linux seccomp sandbox: requires kernel headers

    # clone the kernel headers somewhere
    cd /path/to/dir
    git clone https://github.com/sabotage-linux/kernel-headers.git

    # then compile
    PSEUDOCRON_INCLUDE=/path/to/dir ./musl-make clean all

## Sandbox

Setting the `PSEUDOCRON_SANDBOX` environment variable controls which
sandbox is used. The available sandboxes are:

* seccomp: linux

* pledge: openbsd

* capsicum: freebsd

* rlimit: all

* null: all

For example, to force using the rlimit sandbox:

    PSEUDOCRON_SANDBOX=rlimit make clean all

The `null` sandbox disables sandboxing. It can be used for debugging
problems with a sandbox.

    PSEUDOCRON_SANDBOX=null make clean all
    strace -o null.trace ./pseudcron ...

    PSEUDOCRON_SANDBOX=seccomp make clean all
    strace -o seccomp.trace ./pseudcron ...

# ALTERNATIVES

* [runcron](https://github.com/msantos/runcron)

* [runwhen](http://code.dogmap.org/runwhen/)

* [supercronic](https://github.com/aptible/supercronic)

* [uschedule](https://ohse.de/uwe/uschedule.html)

# SEE ALSO

_crontab_(5), _sleep_(1)
