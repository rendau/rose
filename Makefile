.PHONY = release default all clean
.DEFAULT_GOAL = default

TARGET = rose

CC = gcc
LD = ld
AR = ar
CFLAGS = -g -Wall -fPIC
IFLAGS = -I.
LFLAGS = 
LIBS = -lc -lssl -lcrypto -lpthread

HEADERS = trace.h ns.h mem.h spm.h objtypes.h obj.h \
	bool.h int.h float.h str.h chain.h \
	hmap.h crypt.h poll.h sm.h frmp.h httpp.h wsp.h \
	stz.h eh.h thm.h
CFILES = ns.c mem.c spm.c bool.c int.c float.c str.c \
	chain.c hmap.c crypt.c poll.c sm.c frmp.c httpp.c \
	wsp.c stz.c eh.c thm.c

MAKEARGS = 

ifeq ($(arch),mips)
ifdef dir
TARGET := $(TARGET)_mips
export STAGING_DIR=$(sd)
BINDIR = $(word 1, $(wildcard $(dir)/staging_dir/toolchain*/bin))
ENVDIR = $(word 1, $(wildcard $(dir)/staging_dir/target*))
CC = $(BINDIR)/mips-openwrt-linux-gcc
LD = $(BINDIR)/mips-openwrt-linux-ld
AR = $(BINDIR)/mips-openwrt-linux-ar
IFLAGS += -I$(ENVDIR)/usr/include
LFLAGS += -L$(ENVDIR)/usr/lib
MAKEARGS += arch=mips dir=$(dir)
endif
endif

ifdef pdb
	IFLAGS += -I`pg_config --includedir`
	LFLAGS += -L`pg_config --libdir`
	LIBS += -lpq
	HEADERS += pdb.h
	CFILES += pdb.c
	MAKEARGS += pdb=1
endif

TARGET_DYN = lib$(TARGET).so
TARGET_STAT = lib$(TARGET).a

OBJECTS = $(CFILES:.c=.o)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(TARGET_DYN): $(OBJECTS)
	$(LD) -shared $(LFLAGS) $^ -o $@ $(LIBS)

$(TARGET_STAT): $(OBJECTS)
	$(AR) rcs $@ $^

test: test.o $(TARGET_DYN)
	$(CC) -L. $< -o testlib -l$(TARGET)

default: $(TARGET_DYN) $(TARGET_STAT)

all: default test

release:
	make clean
	make -s $(MAKEARGS)
	rm -rf release
	mkdir release
	mkdir release/$(TARGET)
	cp $(TARGET_DYN) release/$(TARGET)/
	cp README release/$(TARGET)/
	mkdir release/$(TARGET)/include
	cp $(HEADERS) release/$(TARGET)/include

clean:
	rm -rf *.o *~ testlib
	rm -rf release

distclean:
	make clean
	rm -rf *.a *.so
