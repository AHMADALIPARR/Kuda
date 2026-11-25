// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// FILE: holyc/vm_core.HC
/* HolyC native x86-64 VM core - TempleOS compatible */
#define VM_ABI_VERSION 0x00010001
#define VM_MODULE_MAGIC 0x564D4D4F
#define VM_PAGE_SIZE 4096
#define VM_MAX_REGIONS 64
#define VM_MAX_CTX 32
#define VM_TRACE_RING_SIZE 4096
#define VM_CAP_RING0 (1<<0)
#define VM_CAP_MEM (1<<1)
#define VM_CAP_C11 (1<<2)
#define VM_CAP_JIT (1<<3)
#define VM_CAP_TRACE (1<<4)
#define VM_CAP_REPL (1<<5)

#define VM_OK 0
#define VM_ERR_INVAL -1
#define VM_ERR_NOMEM -2
#define VM_ERR_PERM -3
#define VM_ERR_ALIGN -4
#define VM_ERR_HASH -5
#define VM_ERR_MAGIC -6
#define VM_ERR_VERSION -7
#define VM_ERR_CAP -8
#define VM_ERR_STATE -9
#define VM_ERR_FAULT -10

#define VM_ST_INIT 0
#define VM_ST_ADMITTED 1
#define VM_ST_RUNNING 2
#define VM_ST_HALTED 3
#define VM_ST_FAULT 4

class VmRegion {
  U8 *base;
  U64 size;
  U64 used;
  U32 flags;
  U32 pad;
};

class VmModuleHdr {
  U32 magic;
  U32 version;
  U64 abi_hash;
  U64 proof_hash;
  U64 caps;
  U64 entry_off;
  U64 code_size;
  U32 n_regions;
  U32 pad;
};

class VmExecCtx {
  U64 rax, rbx, rcx, rdx;
  U64 rsi, rdi, rbp, rsp;
  U64 r8, r9, r10, r11;
  U64 r12, r13, r14, r15;
  U64 rip;
  U64 rflags;
  U32 state;
  U32 priv_ring;
  U64 caps;
  U64 pad;
};

class VmTraceRec {
  U64 tsc;
  U64 rip;
  U32 op;
  U32 ctx_id;
  U64 data0;
  U64 data1;
};

VmRegion g_regions[VM_MAX_REGIONS];
U32 g_n_regions;
VmExecCtx g_ctx[VM_MAX_CTX];
U32 g_n_ctx;
VmTraceRec g_trace[VM_TRACE_RING_SIZE];
U32 g_trace_head;
U32 g_trace_tail;
U32 g_vm_state;
U64 g_vm_caps;

U0 VmInit()
{
  MemSet(g_regions, 0, sizeof(g_regions));
  MemSet(g_ctx, 0, sizeof(g_ctx));
  MemSet(g_trace, 0, sizeof(g_trace));
  g_n_regions = 0;
  g_n_ctx = 0;
  g_trace_head = 0;
  g_trace_tail = 0;
  g_vm_state = VM_ST_INIT;
  g_vm_caps = 0;
}

U0 VmShutdown()
{
  g_vm_state = VM_ST_HALTED;
  g_n_regions = 0;
  g_n_ctx = 0;
}

I64 VmCtxCreate(U64 caps)
{
  I64 id;
  if (g_n_ctx >= VM_MAX_CTX)
    return VM_ERR_NOMEM;
  id = g_n_ctx++;
  MemSet(&g_ctx[id], 0, sizeof(VmExecCtx));
  g_ctx[id].state = VM_ST_INIT;
  g_ctx[id].caps = caps;
  g_ctx[id].priv_ring = 3;
  return id;
}

U0 VmCtxDestroy(I64 id)
{
  if (id < 0 || id >= g_n_ctx)
    return;
  MemSet(&g_ctx[id], 0, sizeof(VmExecCtx));
}

U0 VmCtxSave(I64 id)
{
  if (id < 0 || id >= g_n_ctx)
    return;
  asm {
    MOV RAX, &g_ctx[id].rax
    MOV [RAX], RAX
    MOV RAX, &g_ctx[id].rbx
    MOV [RAX], RBX
    MOV RAX, &g_ctx[id].rcx
    MOV [RAX], RCX
    MOV RAX, &g_ctx[id].rdx
    MOV [RAX], RDX
    MOV RAX, &g_ctx[id].rsi
    MOV [RAX], RSI
    MOV RAX, &g_ctx[id].rdi
    MOV [RAX], RDI
    MOV RAX, &g_ctx[id].rbp
    MOV [RAX], RBP
    MOV RAX, &g_ctx[id].rsp
    MOV [RAX], RSP
    MOV RAX, &g_ctx[id].r8
    MOV [RAX], R8
    MOV RAX, &g_ctx[id].r9
    MOV [RAX], R9
    MOV RAX, &g_ctx[id].r10
    MOV [RAX], R10
    MOV RAX, &g_ctx[id].r11
    MOV [RAX], R11
    MOV RAX, &g_ctx[id].r12
    MOV [RAX], R12
    MOV RAX, &g_ctx[id].r13
    MOV [RAX], R13
    MOV RAX, &g_ctx[id].r14
    MOV [RAX], R14
    MOV RAX, &g_ctx[id].r15
    MOV [RAX], R15
  }
}

U0 VmCtxRestore(I64 id)
{
  if (id < 0 || id >= g_n_ctx)
    return;
  asm {
    MOV RAX, &g_ctx[id].rax
    MOV RAX, [RAX]
    MOV RBX, &g_ctx[id].rbx
    MOV RBX, [RBX]
    MOV RCX, &g_ctx[id].rcx
    MOV RCX, [RCX]
    MOV RDX, &g_ctx[id].rdx
    MOV RDX, [RDX]
    MOV RSI, &g_ctx[id].rsi
    MOV RSI, [RSI]
    MOV RDI, &g_ctx[id].rdi
    MOV RDI, [RDI]
    MOV RBP, &g_ctx[id].rbp
    MOV RBP, [RBP]
    MOV RSP, &g_ctx[id].rsp
    MOV RSP, [RSP]
    MOV R8, &g_ctx[id].r8
    MOV R8, [R8]
    MOV R9, &g_ctx[id].r9
    MOV R9, [R9]
    MOV R10, &g_ctx[id].r10
    MOV R10, [R10]
    MOV R11, &g_ctx[id].r11
    MOV R11, [R11]
    MOV R12, &g_ctx[id].r12
    MOV R12, [R12]
    MOV R13, &g_ctx[id].r13
    MOV R13, [R13]
    MOV R14, &g_ctx[id].r14
    MOV R14, [R14]
    MOV R15, &g_ctx[id].r15
    MOV R15, [R15]
  }
}

I64 VmKernelRegister(U8 *name, U0 (*fn)())
{
  /* Simple registration table - extend as needed */
  return VM_OK;
}

I64 VmKernelLoad(U8 *path)
{
  return VM_OK;
}

U64 VmCallC11(U0 *fn, U64 a0, U64 a1, U64 a2, U64 a3, U64 a4, U64 a5)
{
  U64 ret;
  asm {
    MOV RDI, a0
    MOV RSI, a1
    MOV RDX, a2
    MOV RCX, a3
    MOV R8, a4
    MOV R9, a5
    CALL fn
    MOV ret, RAX
  }
  return ret;
}

U0 VmDispatch(U0 **table, U32 idx)
{
  if (!table || !table[idx])
    return;
  (*table[idx])();
}

I64 VmMemAllocRegion(U8 *base, U64 size)
{
  I64 id;
  if (!base || size < VM_PAGE_SIZE || (base & 15))
    return VM_ERR_ALIGN;
  if (g_n_regions >= VM_MAX_REGIONS)
    return VM_ERR_NOMEM;
  id = g_n_regions++;
  g_regions[id].base = base;
  g_regions[id].size = size;
  g_regions[id].used = 0;
  g_regions[id].flags = 0;
  return id;
}

U0 VmMemFreeRegion(I64 id)
{
  if (id < 0 || id >= g_n_regions)
    return;
  g_regions[id].used = 0;
  g_regions[id].flags = 0;
}

U0 VmMemSeal(I64 id)
{
  if (id < 0 || id >= g_n_regions)
    return;
  g_regions[id].flags |= 1;
}

I64 VmStateTransition(I64 id, U32 next)
{
  if (id < 0 || id >= g_n_ctx)
    return VM_ERR_INVAL;
  switch (g_ctx[id].state) {
    case VM_ST_INIT:
      if (next != VM_ST_ADMITTED && next != VM_ST_FAULT)
        return VM_ERR_STATE;
      break;
    case VM_ST_ADMITTED:
      if (next != VM_ST_RUNNING && next != VM_ST_FAULT)
        return VM_ERR_STATE;
      break;
    case VM_ST_RUNNING:
      if (next != VM_ST_HALTED && next != VM_ST_FAULT)
        return VM_ERR_STATE;
      break;
    default:
      return VM_ERR_STATE;
  }
  g_ctx[id].state = next;
  return VM_OK;
}

I64 VmAdmitModule(VmModuleHdr *hdr, U8 *code, U64 code_len)
{
  U64 h;
  U32 cpl;
  if (!hdr)
    return VM_ERR_INVAL;
  if (hdr->magic != VM_MODULE_MAGIC)
    return VM_ERR_MAGIC;
  if (hdr->version != VM_ABI_VERSION)
    return VM_ERR_VERSION;
  /* Privilege ring check */
  asm {
    MOV RAX, CS
    AND RAX, 3
    MOV cpl, RAX
  }
  if (cpl != 0 && (hdr->caps & VM_CAP_RING0))
    return VM_ERR_PERM;
  /* Capability admission */
  if ((g_vm_caps & hdr->caps) != hdr->caps)
    return VM_ERR_CAP;
  /* Hash gate (simplified FNV) */
  h = 14695981039346656037;
  U8 *p = hdr;
  U64 i;
  for (i = 0; i < sizeof(VmModuleHdr) - 8; i++) {
    h ^= p[i];
    h *= 1099511628211;
  }
  if (h != hdr->abi_hash)
    return VM_ERR_HASH;
  g_vm_state = VM_ST_ADMITTED;
  return VM_OK;
}

U0 VmReplEval()
{
  U8 *line;
  while (TRUE) {
    DocPrint("vm> ");
    line = GetStr;
    if (!line) break;
    if (!StrCmp(line, "quit") || !StrCmp(line, "exit"))
      break;
    ExePutS(line);
    Free(line);
  }
}

U0 VmInspect()
{
  I64 i;
  DocPrint("VM State: %d Caps: %X\n", g_vm_state, g_vm_caps);
  DocPrint("Contexts: %d\n", g_n_ctx);
  for (i = 0; i < g_n_ctx; i++) {
    DocPrint(" ctx[%d] state=%d ring=%d caps=%X\n",
      i, g_ctx[i].state, g_ctx[i].priv_ring, g_ctx[i].caps);
  }
  DocPrint("Regions: %d\n", g_n_regions);
  for (i = 0; i < g_n_regions; i++) {
    DocPrint(" reg[%d] base=%X size=%X used=%X flags=%X\n",
      i, g_regions[i].base, g_regions[i].size,
      g_regions[i].used, g_regions[i].flags);
  }
}

U0 VmTraceEmit(U32 op, U32 ctx_id, U64 d0, U64 d1)
{
  U32 next = (g_trace_head + 1) % VM_TRACE_RING_SIZE;
  if (next == g_trace_tail)
    g_trace_tail = (g_trace_tail + 1) % VM_TRACE_RING_SIZE;
  g_trace[g_trace_head].tsc = GetTSC;
  g_trace[g_trace_head].rip = 0;
  g_trace[g_trace_head].op = op;
  g_trace[g_trace_head].ctx_id = ctx_id;
  g_trace[g_trace_head].data0 = d0;
  g_trace[g_trace_head].data1 = d1;
  g_trace_head = next;
}

I64 VmFaultRecover(I64 id, I32 err)
{
  if (id < 0 || id >= g_n_ctx)
    return VM_ERR_INVAL;
  g_ctx[id].state = VM_ST_FAULT;
  VmCtxRestore(id);
  return err;
}
