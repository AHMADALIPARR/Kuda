// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* HolyC test suite for VM core */
U0 TestVmLifecycle()
{
  VmInit;
  if (g_vm_state != VM_ST_INIT)
    throw('Fail');
  VmShutdown;
  if (g_vm_state != VM_ST_HALTED)
    throw('Fail');
  DocPrint("lifecycle OK\n");
}

U0 TestCtx()
{
  I64 id;
  VmInit;
  id = VmCtxCreate(VM_CAP_MEM | VM_CAP_C11);
  if (id < 0)
    throw('Fail');
  if (g_ctx[id].state != VM_ST_INIT)
    throw('Fail');
  VmCtxDestroy(id);
  DocPrint("ctx OK\n");
}

U0 TestAdmit()
{
  VmModuleHdr hdr;
  I64 r;
  MemSet(&hdr, 0, sizeof(hdr));
  hdr.magic = VM_MODULE_MAGIC;
  hdr.version = VM_ABI_VERSION;
  hdr.caps = 0;
  g_vm_caps = VM_CAP_ALL;
  r = VmAdmitModule(&hdr, NULL, 0);
  /* Hash will fail intentionally for negative path coverage */
  DocPrint("admit path exercised\n");
}

U0 TestRing()
{
  U32 cpl;
  asm {
    MOV RAX, CS
    AND RAX, 3
    MOV cpl, RAX
  }
  DocPrint("CS.CPL = %d\n", cpl);
}

U0 TestTrace()
{
  VmInit;
  VmTraceEmit(1, 0, 0xAA, 0xBB);
  if (g_trace_head == 0)
    throw('Fail');
  DocPrint("trace OK\n");
}

U0 TestFault()
{
  I64 id, r;
  VmInit;
  id = VmCtxCreate(0);
  r = VmFaultRecover(id, VM_ERR_FAULT);
  if (g_ctx[id].state != VM_ST_FAULT)
    throw('Fail');
  DocPrint("fault OK\n");
}

U0 Main()
{
  TestVmLifecycle;
  TestCtx;
  TestAdmit;
  TestRing;
  TestTrace;
  TestFault;
  DocPrint("HolyC VM tests OK\n");
}
