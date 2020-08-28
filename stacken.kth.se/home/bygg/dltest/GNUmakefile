# Wrapper to pick up environment when we use GNU make.

SYS = $(shell uname)

LDADD=

ifeq ($(SYS), Linux)
LDADD= -ldl
else ifeq ($(SYS), SunOS)
LDADD= -ldl
endif

# Now do common stuff.

include Makefile
