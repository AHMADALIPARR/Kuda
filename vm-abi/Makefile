# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# FILE: Makefile
.PHONY: all clean ats2 c11 holyc tests

CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-stack-protector -Iinclude -Isrc
PATSOPT := patsopt
# ATS runtime home: same default as /usr/bin/patsopt's wrapper script
# (prefix=/usr, ats2-postiats-0.4.2); override with ATS_HOME=...
ATS_HOME ?= /usr/lib/ats2-postiats-0.4.2
ATS_CFLAGS := -I$(ATS_HOME) -I$(ATS_HOME)/ccomp/runtime

all: ats2 c11 tests

# patsopt 0.4.2 flag order matters: `-o <out>` must precede `-d <file>`.
# The dropped form `patsopt -IATS proofs -o <out> <file>` passed the
# .dats as a bare argument, which patsopt ignores with a warning, so it
# generated nothing and left a 0-byte <out> (verified: `patsopt -d
# <file> -cc -o <out>` is no better -- the C goes to stdout and <out>
# stays 0 bytes). `-o <out> -d <file> -cc` typechecks and writes the
# real generated C (3,461 bytes for this file).
# The generated C is compiled with -Wall -Wextra but WITHOUT the
# project's -Werror: it is machine-generated code that #includes the
# ATS 0.4.2 runtime headers, which themselves violate -Wall -Wextra
# -Werror (unused-parameter in pats_ccomp_memalloca.h,
# misleading-indentation in prelude/CATS/basics.cats, ordered pointer/
# integer comparison and unused parameters in pointer.cats,
# unused-but-set-variable in fprintf.cats), so -Werror on this file
# can never pass. No warning is suppressed (-Wno-* is not used; the
# diagnostics stay visible) and -Werror still applies to every
# hand-written source. The real verification of the proofs is patsopt
# -cc itself (typecheck + codegen), as with patscc.
ats2:
	$(PATSOPT) -o proofs/abi_contract_dats.c -d proofs/abi_contract.dats -cc
	$(CC) -std=c11 -O2 -Wall -Wextra $(ATS_CFLAGS) -c proofs/abi_contract_dats.c -o proofs/abi_contract_dats.o

c11: src/c11_substrate.o
src/c11_substrate.o: src/c11_substrate.c include/vm_abi.h
	$(CC) $(CFLAGS) -c src/c11_substrate.c -o src/c11_substrate.o

tests: tests/test_c11_abi
tests/test_c11_abi: tests/test_c11_abi.c src/c11_substrate.o include/vm_abi.h
	$(CC) $(CFLAGS) tests/test_c11_abi.c src/c11_substrate.o -o tests/test_c11_abi

clean:
	rm -f proofs/*.o proofs/*_dats.c src/*.o tests/test_c11_abi
