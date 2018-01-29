.PHONY: all clean test

PROG=   pseudocron
SRCS=   pseudocron.c \
        ccronexpr.c \
        sandbox_null.c \
        sandbox_rlimit.c \
        sandbox_pledge.c \
        sandbox_capsicum.c \
        sandbox_seccomp.c

UNAME_SYS := $(shell uname -s)
ifeq ($(UNAME_SYS), Linux)
    CFLAGS ?= -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
              -Wformat -Werror=format-security \
              -fno-strict-aliasing
		PSEUDOCRON_SANDBOX ?= seccomp
else ifeq ($(UNAME_SYS), OpenBSD)
    CFLAGS ?= -DHAVE_STRTONUM \
              -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
              -Wformat -Werror=format-security \
              -fno-strict-aliasing
		PSEUDOCRON_SANDBOX ?= pledge
else ifeq ($(UNAME_SYS), FreeBSD)
    CFLAGS ?= -DHAVE_STRTONUM \
              -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
              -Wformat -Werror=format-security \
              -fno-strict-aliasing
		PSEUDOCRON_SANDBOX ?= capsicum
endif

RM ?= rm

PSEUDOCRON_SANDBOX ?= rlimit
PSEUDOCRON_CFLAGS ?= -g -Wall -fwrapv

CFLAGS += $(PSEUDOCRON_CFLAGS) \
			-DCRON_USE_LOCAL_TIME \
		  -DPSEUDOCRON_SANDBOX=\"$(PSEUDOCRON_SANDBOX)\" \
		 	-DPSEUDOCRON_SANDBOX_$(PSEUDOCRON_SANDBOX)

LDFLAGS += $(PSEUDOCRON_LDFLAGS) -Wl,-z,relro,-z,now

all: $(PROG)

$(PROG):
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LDFLAGS)

clean:
	-@$(RM) $(PROG)

test: $(PROG)
	@PATH=.:$(PATH) bats test
