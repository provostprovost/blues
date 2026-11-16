# Firmware Makefile for TexasBlues-1 (TB-1)
# Supports host verification, unit testing, and bridge to target build

FELUCCA_ID := FM-1_960
APP_NAME   := "TB-1"
TARGET     := build/tb1_blues.fwsc
SRCS       := src/dinghy.c

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Werror=double-promotion
INCLUDES := -Isrc -I../firmware/src -I../firmware/hal

.PHONY: all check_phase0 clean

all: check_phase0

check_phase0:
	@mkdir -p ../build
	$(CC) -DSTANDALONE_TEST $(CFLAGS) $(INCLUDES) ../tests/test_phase0.c -o ../build/test_phase0
	../build/test_phase0

clean:
	rm -rf ../build/test_phase0
