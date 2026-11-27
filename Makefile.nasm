# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
BACKEND_OBJ := $(AS_DIR)/as_callfunc_x64_nasm.o
$(BACKEND_OBJ): $(AS_DIR)/as_callfunc_x64_nasm.asm
	$(NASM) -f elf64 -o $@ $<
