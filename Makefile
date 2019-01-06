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
              -pie -fPIE \
              -fno-strict-aliasing
    LDFLAGS += -Wl,-z,relro,-z,now -Wl,-z,noexecstack
	  PSEUDOCRON_SANDBOX ?= seccomp
else ifeq ($(UNAME_SYS), OpenBSD)
    CFLAGS ?= -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
              -Wformat -Werror=format-security \
              -pie -fPIE \
              -fno-strict-aliasing
    LDFLAGS += -Wno-missing-braces -Wl,-z,relro,-z,now -Wl,-z,noexecstack
    PSEUDOCRON_SANDBOX ?= pledge
else ifeq ($(UNAME_SYS), FreeBSD)
    CFLAGS ?= -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
              -Wformat -Werror=format-security \
              -pie -fPIE \
              -fno-strict-aliasing
    LDFLAGS += -Wno-missing-braces -Wl,-z,relro,-z,now -Wl,-z,noexecstack
    PSEUDOCRON_SANDBOX ?= capsicum
else ifeq ($(UNAME_SYS), Darwin)
    CFLAGS ?= -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
              -Wformat -Werror=format-security \
              -pie -fPIE \
              -fno-strict-aliasing
    LDFLAGS += -Wno-missing-braces
endif

RM ?= rm

PSEUDOCRON_SANDBOX ?= rlimit
PSEUDOCRON_CFLAGS ?= -g -Wall -fwrapv -pedantic

CFLAGS += $(PSEUDOCRON_CFLAGS) \
          -DCRON_USE_LOCAL_TIME \
          -DPSEUDOCRON_SANDBOX=\"$(PSEUDOCRON_SANDBOX)\" \
          -DPSEUDOCRON_SANDBOX_$(PSEUDOCRON_SANDBOX)

LDFLAGS += $(PSEUDOCRON_LDFLAGS)

all: $(PROG)

$(PROG):
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LDFLAGS)

clean:
	-@$(RM) $(PROG)

test: $(PROG)
	@PATH=.:$(PATH) bats test
