# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# FILE: Makefile
.PHONY: all clean ats2 c11 ir backend tests

CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -Isrc -Iir
PATSOPT := patsopt
ATS_HOME ?= /usr/lib/ats2-postiats-0.4.2
ATS_CFLAGS := -I$(ATS_HOME) -I$(ATS_HOME)/ccomp/runtime

all: ats2 c11 ir backend tests

ats2:
	$(PATSOPT) -o proofs/gpu_ir_contract_dats.c -d proofs/gpu_ir_contract.dats -cc
	$(CC) -std=c11 -O2 -Wall -Wextra $(ATS_CFLAGS) -c proofs/gpu_ir_contract_dats.c -o proofs/gpu_ir_contract_dats.o

c11: src/occam_frontend.o src/gpu_ir.o src/instr_select.o
src/occam_frontend.o: src/occam_frontend.c include/occam.h include/gpu_ir.h
	$(CC) $(CFLAGS) -c src/occam_frontend.c -o src/occam_frontend.o
src/gpu_ir.o: src/gpu_ir.c include/gpu_ir.h
	$(CC) $(CFLAGS) -c src/gpu_ir.c -o src/gpu_ir.o
src/instr_select.o: src/instr_select.c include/gpu_ir.h include/backend.h
	$(CC) $(CFLAGS) -c src/instr_select.c -o src/instr_select.o

ir: src/gpu_ir.o

backend: backend/ptx_emit.o backend/amdgpu_emit.o backend/spirv_emit.o
backend/ptx_emit.o: backend/ptx_emit.c include/backend.h include/gpu_ir.h
	$(CC) $(CFLAGS) -c backend/ptx_emit.c -o backend/ptx_emit.o
backend/amdgpu_emit.o: backend/amdgpu_emit.c include/backend.h include/gpu_ir.h
	$(CC) $(CFLAGS) -c backend/amdgpu_emit.c -o backend/amdgpu_emit.o
backend/spirv_emit.o: backend/spirv_emit.c include/backend.h include/gpu_ir.h
	$(CC) $(CFLAGS) -c backend/spirv_emit.c -o backend/spirv_emit.o

tests: tests/test_gpu_ir
tests/test_gpu_ir: tests/test_gpu_ir.c src/gpu_ir.o src/occam_frontend.o src/instr_select.o \
                   backend/ptx_emit.o backend/amdgpu_emit.o backend/spirv_emit.o
	$(CC) $(CFLAGS) tests/test_gpu_ir.c src/gpu_ir.o src/occam_frontend.o src/instr_select.o \
       backend/ptx_emit.o backend/amdgpu_emit.o backend/spirv_emit.o -o tests/test_gpu_ir

clean:
	rm -f proofs/*.o proofs/*_dats.c src/*.o backend/*.o tests/test_gpu_ir
