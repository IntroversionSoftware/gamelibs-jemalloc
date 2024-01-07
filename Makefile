# -*- Makefile -*- for jemalloc

.SECONDEXPANSION:
.SUFFIXES:

ifneq ($(findstring $(MAKEFLAGS),s),s)
ifndef V
        QUIET          = @
        QUIET_CC       = @echo '   ' CC $<;
        QUIET_AR       = @echo '   ' AR $@;
        QUIET_RANLIB   = @echo '   ' RANLIB $@;
        QUIET_INSTALL  = @echo '   ' INSTALL $<;
        export V
endif
endif

LIB    = libjemalloc.a
AR    ?= ar
ARFLAGS ?= rc
CC    ?= gcc
RANLIB?= ranlib
RM    ?= rm -f

BUILD_DIR := lib
BUILD_ID  ?= default-build-id
OBJ_DIR   := $(BUILD_DIR)/$(BUILD_ID)

ifeq (,$(BUILD_ID))
$(error BUILD_ID cannot be an empty string)
endif

uname_S := $(shell uname -s)

ifneq (,$(findstring MINGW,$(uname_S)))
PLATFORM := mingw
endif
ifeq ($(uname_S),Darwin)
PLATFORM := macosx
endif
ifeq ($(uname_S),Linux)
PLATFORM := linux
endif

prefix ?= /usr/local
libdir := $(prefix)/lib
includedir := $(prefix)/include

HEADERS_PLATFORM = \
	$(wildcard $(PLATFORM)/include/jemalloc/*.h) \
	$(wildcard $(PLATFORM)/include/jemalloc/internal/*.h)
HEADERS_INTERNAL = \
	$(wildcard include/jemalloc/internal/jemalloc*.h)
SOURCES = \
	src/arena.c \
	src/background_thread.c \
	src/base.c \
	src/bin.c \
	src/bin_info.c \
	src/bitmap.c \
	src/buf_writer.c \
	src/cache_bin.c \
	src/ckh.c \
	src/counter.c \
	src/ctl.c \
	src/decay.c \
	src/div.c \
	src/ecache.c \
	src/edata.c \
	src/edata_cache.c \
	src/ehooks.c \
	src/emap.c \
	src/eset.c \
	src/exp_grow.c \
	src/extent.c \
	src/extent_dss.c \
	src/extent_mmap.c \
	src/fxp.c \
	src/hook.c \
	src/hpa.c \
	src/hpa_hooks.c \
	src/hpdata.c \
	src/inspect.c \
	src/jemalloc.c \
	src/large.c \
	src/log.c \
	src/malloc_io.c \
	src/mutex.c \
	src/nstime.c \
	src/pa.c \
	src/pa_extra.c \
	src/pac.c \
	src/pages.c \
	src/pai.c \
	src/peak_event.c \
	src/prof.c \
	src/prof_data.c \
	src/prof_log.c \
	src/prof_recent.c \
	src/prof_stats.c \
	src/prof_sys.c \
	src/psset.c \
	src/rtree.c \
	src/safety_check.c \
	src/san.c \
	src/san_bump.c \
	src/sc.c \
	src/sec.c \
	src/stats.c \
	src/sz.c \
	src/tcache.c \
	src/test_hooks.c \
	src/thread_event.c \
	src/ticker.c \
	src/tsd.c \
	src/util.c \
	src/witness.c

ifeq ($(PLATFORM),macosx)
SOURCES += \
	src/zone.c
endif

HEADERS_INST := $(patsubst $(PLATFORM)/include/%,$(includedir)/%,$(HEADERS_PLATFORM))
HEADERS_INST += $(patsubst include/%,$(includedir)/%,$(HEADERS_INTERNAL))
OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOURCES))

CFLAGS ?= -O2
CFLAGS += -DJEMALLOC_NO_PRIVATE_NAMESPACE -D_REENTRANT -I$(PLATFORM)/include -Iinclude

ifeq ($(PLATFORM),mingw)
CFLAGS += -D_WIN32
endif
ifeq ($(PLATFORM),linux)
CFLAGS += -D_GNU_SOURCE
endif

.PHONY: install

all: $(OBJ_DIR)/$(LIB)

$(includedir)/%.h: $(PLATFORM)/include/%.h | $$(@D)/.
	$(QUIET_INSTALL)cp $< $@
	@chmod 0644 $@

$(includedir)/%.h: include/%.h | $$(@D)/.
	$(QUIET_INSTALL)cp $< $@
	@chmod 0644 $@

$(libdir)/%.a: $(OBJ_DIR)/%.a | $$(@D)/.
	$(QUIET_INSTALL)cp $< $@
	@chmod 0644 $@

install: $(HEADERS_INST) $(libdir)/$(LIB)

clean:
	$(RM) -r $(OBJ_DIR)

distclean: clean
	$(RM) -r $(BUILD_DIR)

$(OBJ_DIR)/$(LIB): $(OBJECTS) | $$(@D)/.
	$(QUIET_AR)$(AR) $(ARFLAGS) $@ $^
	$(QUIET_RANLIB)$(RANLIB) $@

$(OBJ_DIR)/%.o: %.c $(OBJ_DIR)/.cflags | $$(@D)/.
	$(QUIET_CC)$(CC) $(CFLAGS) -o $@ -c $<

.PRECIOUS: $(OBJ_DIR)/. $(OBJ_DIR)%/. $(libdir)/. $(libdir)%/. $(includedir)/. $(includedir)%/.

$(OBJ_DIR)/.:
	$(QUIET)mkdir -p $@

$(OBJ_DIR)%/.:
	$(QUIET)mkdir -p $@

$(libdir)/.:
	$(QUIET)mkdir -p $@

$(libdir)%/.:
	$(QUIET)mkdir -p $@

$(includedir)/.:
	$(QUIET)mkdir -p $@

$(includedir)%/.:
	$(QUIET)mkdir -p $@

TRACK_CFLAGS = $(subst ','\'',$(CC) $(CFLAGS))

$(OBJ_DIR)/.cflags: .force-cflags | $$(@D)/.
	@FLAGS='$(TRACK_CFLAGS)'; \
    if test x"$$FLAGS" != x"`cat $(OBJ_DIR)/.cflags 2>/dev/null`" ; then \
        echo "    * rebuilding jemalloc: new build flags or prefix"; \
        echo "$$FLAGS" > $(OBJ_DIR)/.cflags; \
    fi

.PHONY: .force-cflags
