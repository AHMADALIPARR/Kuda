# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# FILE: Makefile
.PHONY: all clean ats2 c11 holyc tests

CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-stack-protector -Iinclude -Isrc
PATSOPT := patsopt
ATSFLAGS:= -IATS proofs

all: ats2 c11 tests

ats2:
	$(PATSOPT) $(ATSFLAGS) -o proofs/abi_contract_dats.c proofs/abi_contract.dats
	$(CC) $(CFLAGS) -c proofs/abi_contract_dats.c -o proofs/abi_contract_dats.o

c11: src/c11_substrate.o
src/c11_substrate.o: src/c11_substrate.c include/vm_abi.h
	$(CC) $(CFLAGS) -c src/c11_substrate.c -o src/c11_substrate.o

tests: tests/test_c11_abi
tests/test_c11_abi: tests/test_c11_abi.c src/c11_substrate.o include/vm_abi.h
	$(CC) $(CFLAGS) tests/test_c11_abi.c src/c11_substrate.o -o tests/test_c11_abi

clean:
	rm -f proofs/*.o proofs/*_dats.c src/*.o tests/test_c11_abi
