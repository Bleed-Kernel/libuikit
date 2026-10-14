LIB_TARGET_64  = libuikit.a
DEMO_TARGET_64 = uikit-demo

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S)$(UNAME_M),Darwinarm64)
CC 	:= x86_64-elf-gcc
AR 	:= x86_64-elf-ar
else
CC 	:= gcc
AR 	:= ar
endif

ELF_BIN_DIR_64 = bin
ELF_OBJ_DIR_64 = $(ELF_BIN_DIR_64)/obj

LIB_DIR = sysroot/lib
LIBC = $(LIB_DIR)/blibc.a
CRT0 = $(LIB_DIR)/start.o

BLIBC_REPO = https://github.com/Bleed-Kernel/blibc.git
BLIBC_DIR  ?= external/blibc

CRIME_REPO = https://github.com/Bleed-Kernel/Crime-Scene.git
CRIME_DIR  ?= $(if $(wildcard ../Crime-Scene/Makefile),../Crime-Scene,external/crime-scene)
CRIME_OUT  = libcrime
LIBCRIME   = $(CRIME_OUT)/libcrime.a

COMMON_CFLAGS = \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-Wall -Wextra \
	-O2 \
	-nostdlib \
	-no-pie \
	-m64 \
	-Iinclude \
	-I$(CRIME_OUT)/include \
	-Isysroot/include \
	-Isysroot/include/bleed \
	-nostdinc

LDFLAGS = \
	-static \
	-nostdlib \
	-no-pie \
	-m64 \
	-L$(LIB_DIR)

SRCS      = $(shell find src -name '*.c')
DEMO_SRCS = $(shell find examples -name '*.c')

LIB_OBJS_64  = $(patsubst %.c,$(ELF_OBJ_DIR_64)/%.o,$(SRCS))
DEMO_OBJS_64 = $(patsubst %.c,$(ELF_OBJ_DIR_64)/%.o,$(DEMO_SRCS))

LIBUIKIT = $(ELF_BIN_DIR_64)/$(LIB_TARGET_64)

all: $(LIBUIKIT) $(DEMO_TARGET_64)

$(LIBUIKIT): blibc $(LIB_OBJS_64)
	@mkdir -p $(ELF_BIN_DIR_64)
	rm -f $@
	$(AR) rcs $@ $(LIB_OBJS_64)

$(DEMO_TARGET_64): blibc crime $(DEMO_OBJS_64) $(LIBUIKIT)
	$(CC) $(LDFLAGS) \
		-o $(ELF_BIN_DIR_64)/$(DEMO_TARGET_64) \
		$(CRT0) \
		$(DEMO_OBJS_64) \
		$(LIBUIKIT) \
		$(LIBCRIME) \
		$(LIBC)

$(ELF_OBJ_DIR_64)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(COMMON_CFLAGS) -MMD -MP -c $< -o $@

-include $(LIB_OBJS_64:.o=.d) $(DEMO_OBJS_64:.o=.d)

$(LIB_OBJS_64) $(DEMO_OBJS_64): | blibc crime

blibc: $(LIBC)

$(LIBC):
	@echo "[BLIBC] Preparing blibc"
	@if [ ! -d "$(BLIBC_DIR)" ]; then \
		git clone $(BLIBC_REPO) $(BLIBC_DIR); \
	fi
	$(MAKE) -C $(BLIBC_DIR)

	@echo "[BLIBC] Syncing sysroot"
	@mkdir -p sysroot
	@cp -r $(BLIBC_DIR)/sysroot/* sysroot/

crime: blibc
	@echo "[CRIME] Preparing crime-scene"
	@if [ ! -d "$(CRIME_DIR)" ]; then \
		git clone $(CRIME_REPO) $(CRIME_DIR); \
	elif [ "$(CRIME_DIR)" = "external/crime-scene" ]; then \
		git -C $(CRIME_DIR) pull --ff-only || echo "[CRIME] couldnt pull, building whats there"; \
	fi
	@$(MAKE) -C $(CRIME_DIR) bin/libcrime.a BLIBC_DIR=$(abspath $(BLIBC_DIR))
	@echo "[CRIME] Syncing $(CRIME_OUT)"
	@rm -rf $(CRIME_OUT)/include
	@mkdir -p $(CRIME_OUT)/include/crime
	@cp -p $(CRIME_DIR)/include/crime/*.h $(CRIME_OUT)/include/crime/
	@cmp -s $(CRIME_DIR)/bin/libcrime.a $(LIBCRIME) || cp -p $(CRIME_DIR)/bin/libcrime.a $(LIBCRIME)

build: all

clean:
	rm -rf bin

distclean:
	rm -rf bin sysroot external $(CRIME_OUT)

.PHONY: all build clean distclean blibc crime $(DEMO_TARGET_64)
