PACKAGE := vmfs-tools
VERSION := 0.0.0

CC := gcc
CFLAGS := -Wall -O2 -g -D_FILE_OFFSET_BITS=64 $(EXTRA_CFLAGS)
LDFLAGS := $(shell pkg-config --libs uuid)
SRC := $(wildcard *.c)
HEADERS := $(wildcard *.h)
OBJS := $(SRC:%.c=%.o)
PROGRAMS := debugvmfs vmfs-fuse
prefix := /usr

all: $(PROGRAMS)

vmfs-fuse: LDFLAGS+=$(shell pkg-config --libs fuse)
vmfs-fuse.o: CFLAGS+=$(shell pkg-config --cflags fuse)

define program_template
$(1): $(filter-out $(foreach program,$(filter-out $(1), $(PROGRAMS)),$(program).o), $(OBJS))
endef
$(foreach program, $(PROGRAMS), $(eval $(call program_template, $(program))))

$(OBJS): %.o: %.c $(HEADERS)

$(PROGRAMS):
	$(CC) -o $@ $^ $(LDFLAGS)

clean: CLEAN := $(filter $(PROGRAMS) $(OBJS) $(PACKAGE)-%.tar.gz,$(wildcard *))
clean:
	$(if $(CLEAN),rm $(CLEAN))

ALL_DIST := $(SRC) $(HEADERS) $(MAKEFILE_LIST) LICENSE README
DIST_DIR := $(PACKAGE)-$(VERSION)
dist: $(ALL_DIST)
	@rm -rf "$(DIST_DIR)"
	@mkdir "$(DIST_DIR)"
	cp -p $(ALL_DIST) $(DIST_DIR)
	tar -zcf "$(DIST_DIR).tar.gz" "$(DIST_DIR)"
	@rm -rf "$(DIST_DIR)"

install:
	install -d -m 0755 $(DESTDIR)$(prefix)/sbin
	install -m 0755 $(PROGRAMS) $(DESTDIR)$(prefix)/sbin

.PHONY: all clean dist install
