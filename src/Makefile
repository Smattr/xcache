# Basic build system. Some options:
#  V=1 - enable build debugging output
#  ARCH=... - build for a different architecture

VERSION_MAJOR = 0
VERSION_MINOR = 1

OUTPUTS = xcache libxcache.a libxcache.so libhookgetenv.so xcache-tests

ifneq (${V},)
  Q :=
  CFLAGS += -ggdb
else
  Q := @
  CFLAGS += -O2 -DNDEBUG
endif

CFLAGS  += -std=gnu1x -W -Wall -Wextra -fPIC -DXCACHE_PIPE=\"XCACHE_PIPE\" \
    -DVERSION_MAJOR=0 -DVERSION_MINOR=1

# Source files.
SOURCES_LIB = cache.c collection/list.c comm-protocol.c db.c depset.c \
    collection/dict.c environ.c fingerprint.c log.c ptrace-wrapper.c \
    collection/set.c tee.c trace.c util/abspath.c util/aprintf.c util/cp.c \
    util/du.c util/filehash.c util/fileiter.c util/get.c util/mkdirp.c \
    util/reduce.c
SOURCES_EXE = main.c translate-syscall.c
SOURCES_LIBGETENV = comm-protocol.c hook-getenv.c
SOURCES_TEST = test.c

# Hangle building for different architectures.
HOST_ARCH = $(shell uname -m)
ARCH ?= ${HOST_ARCH}
$(info [INFO] Configuring build for ${ARCH} architecture)
ifeq ($(wildcard ${PWD}/arch/${ARCH}),)
  $(error Your chosen architecture, ${ARCH}, is not implemented)
endif
CFLAGS += -I${PWD}/arch/${ARCH}

# GLib dependencies.
CFLAGS_GLIB  += $(shell pkg-config --cflags glib-2.0)
LDFLAGS_GLIB += $(shell pkg-config --libs glib-2.0)
ifeq (${CFLAGS_GLIB},)
  $(warning Failed to find GLib dependencies)
endif
CFLAGS  += ${CFLAGS_GLIB}
LDFLAGS += ${LDFLAGS_GLIB}

# Other dependencies.
LDFLAGS += -lpthread -ldl -lsqlite3 -lssl -lcrypto

.PHONY: default
default: ${OUTPUTS}

xcache: libxcache.so $(foreach v,${SOURCES_EXE},$(patsubst %.c,%.o,${v}))
	@echo " [LD] $(notdir $@)"
	${Q}${CC} -o $@ $(foreach v,${SOURCES_EXE},$(patsubst %.c,%.o,${v})) \
      ${LDFLAGS} -L. -lxcache

libxcache.a: $(foreach v,${SOURCES_LIB},$(patsubst %.c,%.o,${v}))
	@echo " [AR] $(notdir $@)"
	${Q}ar rcs $@ $^

libxcache.so: $(foreach v,${SOURCES_LIB},$(patsubst %.c,%.o,${v}))
	@echo " [LD] $(notdir $@)"
	${Q}${CC} -shared -o $@ $^ ${LDFLAGS}

libhookgetenv.so: $(foreach v,${SOURCES_LIBGETENV},$(patsubst %.c,%.o,${v}))
	@echo " [LD] $(notdir $@)"
	${Q}${CC} -shared -o $@ $^

xcache-tests: $(foreach v,${SOURCES_TEST} ${SOURCES_LIB},$(patsubst %.c,%-testing.o,${v}))
	@echo " [LD] $(notdir $@)"
	${Q}${CC} -o $@ $^ ${LDFLAGS}

%.o: %.c
	@echo " [CC] $(notdir $@)"
	${Q}${CC} ${CFLAGS} -c -o $@ $<

%-testing.o: %.c
	@echo " [CC] $(notdir $@)"
	${Q}${CC} ${CFLAGS} -DXCACHE_TEST=1 -c -o $@ $<

.PHONY: clean
clean:
	@echo " [CLEAN] ${OUTPUTS} *.o"
	${Q}rm -rf ${OUTPUTS} *.o collection/*.o util/*.o

# Disable built in rules. WTF have they done for me lately?
.SUFFIXES:
