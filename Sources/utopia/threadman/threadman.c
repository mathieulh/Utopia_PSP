#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"

ThreadmanInfo gInfo;

//e8d4
int module_bootstart(int size, SysMemConfig *config) __attribute__((alias("ThreadManInit")));

int ThreadManInit(int size, void *config)
{
  int intr, ret;
  int sp = GET_SP;

  intr = sceKernelCpuSuspendIntr();

  memset(&gInfo, 0, sizeof(ThreadmanInfo));

  gInfo.cpuID = GetCPUID();

  initWaitQ();

  initThread();

  initCallback();

  initSema();

  initEventFlag();

  initMbx();

  initMsgPipe();

  initVpl();

  initFpl();

  initAlarm();

  CLEAR_LIST(gInfo.sleepingThreads);
  CLEAR_LIST(gInfo.delayedThreads);
  CLEAR_LIST(gInfo.stoppedThreads);
  CLEAR_LIST(gInfo.suspendedThreads);
  CLEAR_LIST(gInfo.deadThreads);

  for(i = MAX_THREAD_PRIORITY; i >= 0; i--)
  {
    CLEAR_LIST(gInfo.readyThreads[i]);
  }

  gInfo.vfpuContext = 0;
  gInfo.unk_420 = -1;
  gInfo.cleanup.next = 0;
  gInfo.cleanup.used = 0;
  gInfo.cpuProfilerMode = CPU_PROFILER_NONE;

  CLEAR_LIST(gInfo.eventHandlers);

  if(KDebugForKernel_24C32559(0x17))
  {
    *((u32 *)0xBC000030) |= 0x300;

    Kprintf("CPU Profiler enabled: ");

    ret = KDebugForKernel_24C32559(0x16);
    Kprintf(ret ? "Thread Mode\n" : "Global Mode\n");
    gInfo.cpuProfilerMode = ret ? CPU_PROFILER_THREAD : CPU_PROFILER_GLOBAL;
  }

  heapuid = sceKernelCreateHeap(0x1, 0x800, 0x1, "SceThreadmanHeap");
  ASSERT(heapuid > 0);

  uidControlBlock *uid;
  ret = sceKernelCreateUID(gInfo.uidThreadType, "SceThreadmanIdle", 0, &uid);
  ASSERT(ret <= 0);

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, uid, gInfo.uidThreadType);
  thinfo->UID = uid->UID;
  thinfo->stackSize = THREAD_KSTACKSIZE;

  mem = sceKernelAllocPartitionMemory(SYSMEM_PARTITION_KERNEL, "stack:SceThreadmanIdle", PSP_SMEM_High, THREAD_KSTACKSIZE, 0);
  ASSERT(mem > 0);

  SceThreadStack *stack = (SceThreadStack *)sceKernelGetBlockHeadAddr(mem);
  stack->threadid = thinfo->UID;

  thinfo->currentPriority = MAX_THREAD_PRIORITY;
  thinfo->status = PSP_THREAD_READY;
  thinfo->originalPriority = MAX_THREAD_PRIORITY;
  thinfo->attr = 0;
  thinfo->initAttr = 0;
  thinfo->gpreg = GET_GP;
  thinfo->entry = IdleThread;
  thinfo->stack = stack;

  SceKernelHeapOptParam opt;
  opt.size = sizeof(SceKernelHeapOptParam);
  opt.alignment = 0x40;
  thinfo->threadContext = sceKernelAllocHeapMemoryWithOption(heapuid, sizeof(struct SceThreadContext), &opt);

  ResetThreadContext(thinfo->threadContext);
  thinfo->threadContext->type = 0x4;
  thinfo->threadContext->gpr[GPREG_RA - 1] = sub_000136F4;
  thinfo->threadContext->gpr[GPREG_SP - 1] = thinfo->stack + thinfo->stackSize;
  thinfo->threadContext->gpr[GPREG_FP - 1] = thinfo->stack + thinfo->stackSize;
  thinfo->threadContext->EPC = thinfo->entry;
  thinfo->threadContext->gpr[GPREG_GP - 1] = thinfo->gpreg;
  thinfo->threadContext->SR = 0x20000003;

  gInfo.thIdle = thinfo;

  _AddThreadToReadyQueue(thinfo);

  ASSERT(sceKernelCreateUID(gInfo.uidThreadType, "SceThreadmanCleaner", 0, &uid) <= 0);

  thinfo = UID_INFO(ThreadInfo, uid, gInfo.uidThreadType);
  thinfo->UID = uid->UID;
  thinfo->stackSize = THREAD_KSTACKSIZE;

  mem = sceKernelAllocPartitionMemory(SYSMEM_PARTITION_KERNEL, "stack:SceThreadmanCleaner", PSP_SMEM_High, THREAD_KSTACKSIZE, 0);
  ASSERT(mem > 0);

  stack = (SceThreadStack *)sceKernelGetBlockHeadAddr(mem);
  stack->threadid = thinfo->UID;

  thinfo->originalPriority = 1;
  thinfo->currentPriority = 1;
  thinfo->attr = 0;
  thinfo->initAttr = 0;
  thinfo->status = PSP_THREAD_WAITING;
  thinfo->gpreg = GET_GP;
  thinfo->entry = CleanerThread;
  thinfo->stack = stack;

  opt.size = sizeof(SceKernelHeapOptParam);
  opt.alignment = 0x40;
  thinfo->threadContext = sceKernelAllocHeapMemoryWithOption(heapuid, sizeof(struct SceThreadContext), &opt);

  ResetThreadContext(thinfo->threadContext);

  thinfo->threadContext->type = 0x4;
  thinfo->threadContext->gpr[GPREG_RA - 1] = sub_000136F4;
  thinfo->threadContext->gpr[GPREG_SP - 1] = thinfo->stack + thinfo->stackSize;
  thinfo->threadContext->gpr[GPREG_FP - 1] = thinfo->stack + thinfo->stackSize;
  thinfo->threadContext->EPC = thinfo->entry;
  thinfo->threadContext->gpr[GPREG_GP - 1] = thinfo->gpreg;
  thinfo->threadContext->SR = 0x20000003;

  gInfo.thCleaner = thinfo;
  gInfo.doCleanup = 0;

  ASSERT(sceKernelCreateUID(gInfo.uidThreadType, "SceThreadmanInit", 0, &uid) <= 0);

  thinfo = UID_INFO(ThreadInfo, uid, gInfo.uidThreadType);
  thinfo->UID = uid->UID;

  SceUID partitionid, blockid;
  sceKernelQueryMemoryInfo(sp, &partitionid, &blockid);

  thinfo->stack = (SceThreadStack *)sceKernelGetBlockHeadAddr(blockid);
  thinfo->stackSize = sceKernelQueryBlockSize(sp);

  thinfo->stack->threadid = thinfo->UID;

  thinfo->originalPriority = 0x20;
  thinfo->currentPriority = 0x1;
  thinfo->status = PSP_THREAD_RUNNING;
  thinfo->gpreg = GET_GP;
  thinfo->attr = 0;
  thinfo->initAttr = 0;

  opt.size = sizeof(SceKernelHeapOptParam);
  opt.alignment = 0x40;
  thinfo->threadContext = sceKernelAllocHeapMemoryWithOption(heapuid, sizeof(struct SceThreadContext), &opt);

  ResetThreadContext(thinfo->threadContext);

  gInfo.currentThread = thinfo;
  gInfo.nextThread = thinfo;

  _CTC0(thinfo->threadContext, COP0_CR_GPR_SP_KERNEL);
  _CTC0(0, COP0_CR_GPR_SP_USER);
  _CTC0(&thinfo->scContext, COP0_CR_CURRENT_TCB);

  pspSdkSetK1(0);

  RegisterContextHooks(loc_00000B24, _SwitchThread);

  InitSystemTimer();

  sceKernelRegisterIntrHandler(0x41, 0, loc_0000EFD0, 0, 0);

  sceKernelEnableIntr(0x41);

  gInfo.unk_698 = sceKernelQuerySystemCall(_sceKernelExitThread);

  gInfo.unk_69C = sceKernelQuerySystemCall(_sceKernelReturnFromCallback);

  gInfo.systemStatusFlag = sceKernelCreateEventFlag("SceThreadmanSysStat", SCE_EVF_ATTR_MULTI, 0, 0);
  ASSERT(gInfo.systemStatusFlag > 0);

  sceKernelSetBootCallbackLevel(sub_000136C0, 3, 0);

  sceKernelCpuResumeIntr(intr);

  sceKernelCpuEnableIntr();

  return 0;
}

//efd8
int CleanerThread(SceSize args, void *argp)
{
  int i;
  uidControlBlock *cb;

  while(1 == 1)
  {
    int intr = sceKernelCpuSuspendIntr();

    ThreadCleanup *clean, *next_clean;
    for(clean = &gInfo.cleanup; clean; clean = next_clean)
    {
      next_clean = clean->next;

      for(i = 0; i < clean->used; i++)
      {
        if(sceKernelGetUIDcontrolBlockWithType(clean->uid[i], gInfo.uidThreadType, &cb) == 0)
        {
          ThreadInfo *thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

          _ClearAllThreadStacks(thread);
        }

        sceKernelDeleteUID(clean->uid[i]);
      }

      clean->used = 0;
      clean->next = 0;
      if(clean != &gInfo.cleanup)
      {
        sceKernelFreeHeapMemory(gInfo.heapuid, clean);
      }
    }

    gInfo.currentThread->status = PSP_THREAD_WAITING;

    gInfo.nextThread = 0;

    _ReleaseWaitThread(1, 1);
  }
}

//f0dc
int _AddThreadToCleanupQueue(SceUID uid)
{
  int i;

  ThreadCleanup *cleanup;
  for(cleanup = &gInfo.cleanup; cleanup->used > 7; cleanup = cleanup->next)
  {
    if(cleanup->used > 0) //duh?
    {
      for(i = 0; i < cleanup->used; i++)
      {
        if(cleanup->uid[i] == uid)
        {
          return SCE_KERNEL_ERROR_UNKNOWN_THID;
        }
      }
    }

    if(cleanup->next == 0)
    {
      cleanup->next = sceKernelAllocHeapMemory(gInfo.heapuid, sizeof(ThreadCleanup));
      if(cleanup->next == 0)
      {
        return SCE_KERNEL_ERROR_NO_MEMORY;
      }

      cleanup->next->next = 0;
      cleanup->next->used = 0;
    }
  }

  for(i = 0; i < cleanup->used; i++)
  {
    if(cleanup->uid[i] == uid)
    {
      return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }
  }

  cleanup->uid[cleanup->used++] = uid;

  gInfo.nextThread = 0;

  gInfo.doCleanup = 1;

  return SCE_KERNEL_ERROR_OK;
}

//f200
/* - this seems to be exactly the same as sub_1308c
void sub_0000F200(ThreadInfo *thread)
{
  ADD_TO_LIST(thread, gInfo.readyThreads[thread->currentPriority]);

  SET_THREAD_PRIORITY_FLAG(thread->currentPriority);
}
*/

//f258
int _SwitchThread(SceThreadContext *context)
{
  if(gInfo.dispatch_thread_suspended)
  {
    return context;
  }

  if(gInfo.currentThread->thread_profiler_info)
  {
    memcpy(&gInfo.currentThread->thread_profiler_info->info, THREAD_PROFILER_INFO, sizeof(SceProfilerInfo));
  }

  if(gInfo.currentThread->status != PSP_THREAD_KILLED)
  {
    if(gInfo.currentThread->stack->threadid != gInfo.currentThread->UID)
    {
      Kprintf("\nThread (thid=0x%x) stack overflow\n Stack = 0x%x, Stack size = 0x%x, SP = 0x%x\n",
                        gInfo.currentThread->UID, gInfo.currentThread->stack, gInfo.currentThread->stackSize, gInfo.currentThread->threadContext->gpr[GPREG_SP - 1]);
      _KillThread((uidControlBlock *)(gInfo.currentThread - gInfo.uidThreadType->size * sizeof(int));
    }

    if(THREAD_IS_USER_MODE(gInfo.currentThread))
    {
      if(gInfo.currentThread->stack->threadid != gInfo.currentThread->UID)
      {
        Kprintf("\nThread (thid=0x%x) stack overflow\n Stack = 0x%x, Stack size = 0x%x, SP = 0x%x\n",
                          gInfo.currentThread->UID, gInfo.currentThread->kStack, gInfo.currentThread->kStackSize, gInfo.currentThread->threadContext->gpr[GPREG_SP - 1]);
        _KillThread((uidControlBlock *)(gInfo.currentThread - gInfo.uidThreadType->size * sizeof(int));
      }

      int user_mode = gInfo.currentThread->threadContext->SR & 0x18;
      if((user_mode && (gInfo.currentThread->threadContext->gpr[GPREG_SP - 1] < gInfo.currentThread->stack))
              || gInfo.currentThread->threadContext->gpr[GPREG_SP - 1] < gInfo.currentThread->kStack)
      {
        Kprintf("\nThread (thid=0x%x) stack overflow\n Stack = 0x%x, Stack size = 0x%x, SP = 0x%x\n",
                          gInfo.currentThread->UID, user_mode ? gInfo.currentThread->stack : gInfo.currentThread->kStack,
                          user_mode ? gInfo.currentThread->stackSize : gInfo.currentThread->kStackSize,
                          gInfo.currentThread->threadContext->gpr[GPREG_SP - 1]);
        _KillThread(UIDCB_FROM_INFO(gInfo.currentThread, gInfo.uidThreadType));
      }
    }
  }

  //f348
  if(gInfo.nextThread == 0)
  {
    _GetNextExecutableThread();
  }

  if(gInfo.nextThread == gInfo.currentThread)
  {
    gInfo.unk_680++;
  }
  else
  {
    v0v1 = sub_0000B490();

    t7 = &gInfo.unk_428;
    t9 = gInfo.unk_428.hi;
    t1 = gInfo.unk_428.low;
    t6 = s0 + 0x64;
    a1 = gInfo.unk_428.low;
    t2 = gInfo.unk_428.hi;
    a0 = 0;
    s5 = t9 << 0;
    a3 = 0;
    t9 = a0 + t1;
    t8 = 0;
    t3 = t9 < t1;
    t4 = t2 << 0;
    a2 = t5 + a3;
    t2 = t8 + a1;
    t5 = a2 + t3;
    a0 = t4 + a3;
    t3 = t2 < a1;
    a1 = a0 + t3;
    t8 = v0 < t9;
    t3 = v1 - t5;
    a0 = v0 - t9;
    t5 = t2 + a0;
    t4 = t3 - t8;
    t9 = t5 - a0;
    t8 = a1 + t4;
    t0 = t8 + t9;
    t3 = v1 >> 0;
    t2 = t0 >> 0;
    gInfo.unk_428.hi = t3;
    gInfo.unk_428.low = v0;

    gInfo.currentThread->runClocks.hi = t2;
    gInfo.currentThread->runClocks.low = t5;

    gInfo.threadSwitchCount++;
    *(gInfo.currentThread->pCount)++;
  }

  THREADMAN_TRACE(0x1, 4, gInfo.currentThread->UID, gInfo.currentThread->currentPriority, gInfo.nextThread->UID, gInfo.nextThread->currentPriority);

  gInfo.currentThread = gInfo.nextThread;
  if(gInfo.currentThread->thread_profiler_info != 0)
  {
    memcpy(THREAD_PROFILER_INFO, &gInfo.currentThread->thread_profiler_info->info, sizeof(SceProfilerInfo));

    SYNC;

    _CTC0(gInfo.currentThread->thread_profiler_info, COP0_CR_PROFILER_BASE);
  }

  if(gInfo.currentThread->threadContext->SR & 0x40000000)
  {
    _SwitchVFPUContext(gInfo.currentThread);
  }

  _CTC0(gInfo.currentThread->threadContext, COP0_CR_GPR_SP_KERNEL); //strange?
  _CTC0(gInfo.currentThread->kStackPointer, COP0_CR_GPR_SP_USER);
  _CTC0(&gInfo.currentThread->scContext, COP0_CR_CURRENT_TCB);

  return gInfo.currentThread->threadContext;
}

//f5b4
void _SwitchVFPUContext(ThreadInfo *thread)
{
  if(gInfo.vfpuContext == thread->vfpuContext)
  {
    return;
  }

  if(gInfo.unk_420 != THREAD_IS_USER_MODE(thread) ? -1 : 0)
  {
    sub_00000D84();

    if(!THREAD_IS_USER_MODE(thread))
    {
      *((u32 *)0xBC000044) |= 0x20;
    }
    else
    {
      *((u32 *)0xBC000044) &= ~0x20;
    }

    SYNC;

    gInfo.unk_420 = THREAD_IS_USER_MODE(thread) ? -1 : 0;
  }

  if(gInfo.vfpuContext != 0)
  {
    SaveVFPUContext(gInfo.vfpuContext);
  }

  gInfo.vfpuContext = thread->vfpuContext;
  LoadVFPUContext(thread->vfpuContext);

  gInfo.vfpuSwitchCount++;
}

//f690
SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int currentPriority,
                             int stackSize, SceUInt attr, SceKernelThreadOptParam *option);
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(name) || IS_ADDR_KERNEL(entry) || IS_ADDR_KERNEL(option))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext() || gInfo.currentThread->eventMask != 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(attr & ~THREAD_LEGAL_ATTR)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_OUT_OF_RANGE;
  }

  if(IS_USER_MODE)
  {
    COPY_BITS(attr, gInfo.currentThread->attr, PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_USBWLAN | PSP_THREAD_ATTR_VSH);
  }

  if(currentPriority >= MAX_THREAD_PRIORITY - 1)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
  }

  if(IS_SET(attr, PSP_THREAD_ATTR_USER))
  {
    //grant all permissions on the thread
    attr |= UID_GRANT_ALL;

    //priorities 120 and above are reserved for kernel mode threads
    if(currentPriority > MAX_THREAD_PRIORITY - 8)
    {
      RESET_K1;

      return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
    }
  }

  //check alignment of address of entry func
  if(entry % 0x4)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ENTRY;
  }

  if(stackSize < MIN_THREAD_STACKSIZE)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE;
  }

  SceKernelThreadOptParam *opt = (SceKernelThreadOptParam *)gInfo.unk_6B8;
  memset(opt, 0, sizeof(SceKernelThreadOptParam));

  if(option != 0)
  {
    memcpy(opt, option, MIN(option->size, sizeof(SceKernelThreadOptParam)));
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  int ret = sceKernelCreateUID(gInfo.uidThreadType, name, IS_USER_MODE ? UID_GRANT_ALL : (attr & UID_LEGAL_ATTR), &cb);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  thinfo->currentPriority = currentPriority;
  thinfo->option = option;
  thinfo->attr = attr;
  thinfo->initAttr = attr;
  thinfo->entry = entry;
  thinfo->originalPriority = currentPriority;

  SceModule *mod = sceKernelFindModuleByAddress(entry);
  thinfo->gpreg = mod ? mod->unk_68 : GET_GP;

  strcpy(gInfo.unk_6C0, "stack:");

  strncpy(gInfo.unk_6C0 + 6, name, 0x1F - 6);

  gInfo.unk_6C0[0x1F] = 0;

  int partitionid = opt->partitionid;
  if(partitionid == 0)
  {
    partitionid = THREAD_IS_USER_MODE(thinfo) ? SYSMEM_PARTITION_USER : SYSMEM_PARTITION_KERNEL;
  }

  PspSysmemPartitionInfo *partinfo = (PspSysmemPartitionInfo *)gInfo.unk_6A0;
  partinfo->size = sizeof(PspSysmemPartitionInfo);
  ret = sceKernelQueryMemoryPartitionInfo(partitionid, partinfo);
  if(ret != 0)
  {
    ASSERT(sceKernelDeleteUID(thinfo->UID) <= 0);

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  if(THREAD_IS_USER_MODE(thinfo))
  {
    if((partinfo->attr & 0x3) != 0x3)
    {
      ASSERT(sceKernelDeleteUID(thinfo->UID) <= 0);
      
      sceKernelCpuResumeIntr(intr);
      
      return SCE_KERNEL_ERROR_ILLEGAL_PERM;
    }
    else
    {
      t8 = 0; //wtf is this about??
    }
  }
  else
  {
    t8 = 0x10000;
  }

  //s1 = t8 + 0x64C8; t8 is either 0x10000 (with s1 then = gInfo.unk_6C0) or 0 (with s1 then = 0x64C8???)
  int mem = sceKernelAllocPartitionMemory(partitionid, gInfo.unk_6C0, PSP_SMEM_High, (stackSize + 0xFF) % 0x100, 0);
  if(mem <= 0)
  {
    ASSERT(sceKernelDeleteUID(thinfo->UID) <= 0);

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  thinfo->stackSize = (stackSize + 0xFF) % 0x100;
  thinfo->stack = (SceThreadStack *)sceKernelGetBlockHeadAddr(mem);
  if(THREAD_IS_USER_MODE(thinfo)) //is this right???
  {
    strcpy(gInfo.unk_6C0, "kstack:");

    strncpy(gInfo.unk_6C0 + 7, name, 0x1F - 7);

    gInfo.unk_6C0[0x1F] = 0;

    mem = sceKernelAllocPartitionMemory(SYSMEM_PARTITION_KERNEL, gInfo.unk_6C0, PSP_SMEM_High, THREAD_KSTACKSIZE, 0);
    if(mem <= 0)
    {
      ASSERT(sceKernelDeleteUID(thinfo->UID) <= 0);

      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_NO_MEMORY;
    }

    thinfo->kStackSize = THREAD_KSTACKSIZE;
    thinfo->kStack = (SceThreadStack *)sceKernelGetBlockHeadAddr(mem);
    if(gInfo.cpuProfilerMode == CPU_PROFILER_THREAD)
    {
      thinfo->thread_profiler_info = (SceThreadProfilerInfo *)sceKernelAllocHeapMemory(gInfo.heapuid, sizeof(SceThreadProfilerInfo));
      if(thinfo->thread_profiler_info == 0)
      {
        ASSERT(sceKernelDeleteUID(thinfo->UID) <= 0);

        sceKernelCpuResumeIntr(intr);

        RESET_K1;

        return SCE_KERNEL_ERROR_NO_MEMORY;
      }

      memset(thinfo->thread_profiler_info, 0, sizeof(SceThreadProfilerInfo));

      thinfo->thread_profiler_info->unk_0 = 0xBC400000;
      thinfo->thread_profiler_info->unk_4 = gInfo.cpuProfilerMode;
      thinfo->thread_profiler_info->unk_8 = gInfo.cpuProfilerMode;
    }
  }

  //f96c
  SceKernelHeapOptParam *opt = (SceKernelHeapOptParam *)gInfo.unk_6B0;
  opt->size = sizeof(SceKernelHeapOptParam);
  opt->alignment = 0x40;
  thinfo->threadContext = sceKernelAllocHeapMemoryWithOption(gInfo.heapuid, sizeof(struct SceThreadContext), opt);
  if(thinfo->threadContext == 0)
  {
    ASSERT(sceKernelDeleteUID(thinfo->UID) <= 0);

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  v1 = 0; //again, wtf is going on here?
  if(THREAD_CAN_ACCESS_VFPU(thinfo))
  {
    opt->size = sizeof(SceKernelHeapOptParam);
    opt->alignment = 0x40;
    thinfo->vfpuContext = sceKernelAllocHeapMemoryWithOption(heapuid, 0x240, opt);
    if(thinfo->vfpuContext == 0)
    {
      ASSERT(sceKernelDeleteUID(thinfo->UID) <= 0);

      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_NO_MEMORY;
    }

    v1 = 0x10000;
  }

  //a3 = v1 + 0x642C; v1 is either 0x10000 (with a3 then = gInfo.stoppedThreads) or 0 (with a3 then = 0x642C???)
  ADD_TO_LIST(thinfo, gInfo.stoppedThreads);

  thinfo->exitStatus = SCE_KERNEL_ERROR_DORMANT;
  thinfo->status = PSP_THREAD_STOPPED;

  sceKernelCpuResumeIntr(intr);

  if(!THREAD_NO_FILLSTACK(thinfo))
  {
    memset(thinfo->stack, 0xFF, thinfo->stackSize);

    if(THREAD_IS_USER_MODE(thinfo)) //is this right???
    {
      memset(thinfo->kStack, 0xFF, thinfo->kStackSize);
    }
  }

  intr = sceKernelCpuSuspendIntr();

  THREADMAN_TRACE(0x10, 2, entry, thinfo->UID);

  DispatchThreadEventHandlers(THREAD_CREATE, thinfo, intr);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return thinfo->UID;
}

//fd88
int sceKernelDeleteThread(SceUID thid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext() || gInfo.currentThread->eventMask)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(thid == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(IS_USER_MODE && !CAN_DELETE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  if(thinfo == gInfo.thIdle || thinfo == gInfo.thCleaner)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(thinfo->status != PSP_THREAD_STOPPED && thinfo->status != PSP_THREAD_KILLED)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NOT_DORMANT;
  }

  if(thinfo->eventMask)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  THREADMAN_TRACE(0x11, 1, thid);

  DispatchThreadEventHandlers(THREAD_DELETE, thinfo, intr);

  thinfo->status = 0;

  REMOVE_FROM_LIST(thinfo);

  _ClearAllThreadStacks(thinfo, intr);

  ASSERT(sceKernelDeleteUID(thinfo->UID) <= 0);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//ffa4
void ResetThreadContext(struct SceThreadContext *context)
{
  int i;

  context->type = 0xDEADBEEF;

  for(i = GPREG_RA - 1; i >= 0; i--)
  {
    context->gpr[i] = 0xDEADBEEF;
  }

  context->gpr[GPREG_K0 - 1] = 0;
  context->gpr[GPREG_K1 - 1] = 0;

  for(i = 0x1F; i >= 0; i--)
  {
    context->fpr[i] = 0x7F800001;
  }

  context->fc31 = 0xE00;
  context->hi = 0xDEADBEEF;
  context->lo = 0xDEADBEEF;
  context->SR = 0;
  context->EPC = 0;
  context->field_114 = 0;
  context->field_118 = 0;
}

//10018
int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(argp))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext() || gInfo.currentThread->eventMask)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(thid == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  if(thinfo->status != PSP_THREAD_STOPPED)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NOT_DORMANT;
  }

  _FreeThreadSyscallStacks(thinfo);

  if(thinfo->eventMask)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  THREADMAN_TRACE(0x12, 3, thid, arglen, argp);

  u32 *stack_top = (u32 *)(thinfo->stack + (thinfo->stackSize - thinfo->stackSize % 4));
  if(THREAD_IS_USER_MODE(thinfo))
  {
    stack_top -= 0x100;
    thinfo->unk_8C = stack_top;

    memset(stack_top, 0, 0x40);
  }

  thinfo->stack->threadid = thinfo->UID;
  if(THREAD_IS_USER_MODE(thinfo))
  {
    thinfo->kStack->threadid = thinfo->UID;
  }

  memset(thinfo->ktls, 0, sizeof(ktls));

  //set up the args for the thread's entry func, and put them on the stack
  if(arglen > 0 && argp > 0)
  {
    stack_top -= (arglen + 0xF) & ~0xF;
    memcpy(stack_top, argp, arglen);

    thinfo->arglen = arglen;
    thinfo->argp = argp;
  }
  else
  {
    thinfo->arglen = 0;
    thinfo->argp = 0;
  }

  if(THREAD_IS_USER_MODE(thinfo))
  {
    stack_top -= 0x40;

    stack_top[0] = MAKE_SYSCALL(gInfo.unk_698);
    stack_top[1] = stack_top[2] = stack_top[3] = 0;
    _CACHE(0x1B, stack_top);
    _CACHE(0x8, stack_top);
  }

  thinfo->waitType = 0;
  thinfo->attr = thinfo->initAttr;
  thinfo->currentPriority = thinfo->originalPriority;
  thinfo->waitType = 0;
  thinfo->wakeupCount = 0;
  thinfo->stackPointer = stack_top;

  ResetThreadContext(thinfo->threadContext);
  thinfo->threadContext->type = 0x4;

  if(THREAD_IS_USER_MODE(thinfo))
  {
    thinfo->kStackPointer = thinfo->kStack + thinfo->kStackSize;
  }
  else
  {
    thinfo->kStackPointer = 0;
  }

  thinfo->threadContext->gpreg[GPREG_GP - 1] = thinfo->gpreg;
  thinfo->threadContext->gpreg[GPREG_SP - 1] = stack_top;
  thinfo->threadContext->gpreg[GPREG_FP - 1] = stack_top;
  thinfo->threadContext->EPC = _StartThread;
  thinfo->threadContext->SR = 0x20000003;

  thinfo->exitStatus = SCE_KERNEL_ERROR_NOT_DORMANT;

  REMOVE_FROM_LIST(thinfo);

  if(gInfo.unk_734 == 0 || (!THREAD_IS_USER_MODE(thinfo) && !IS_SET(thinfo->attr, PSP_THREAD_ATTR_UNK08000000)))
  {
    if(thinfo->currentPriority >= gInfo.currentThread->currentPriority || gInfo.dispatch_thread_suspended)
    {
      thinfo->status = PSP_THREAD_READY;

      _AddThreadToReadyQueue(thinfo);
    }
    else
    {
      if(intr == 0)
      {
        thinfo->status = PSP_THREAD_READY;

        _AddThreadToReadyQueue(thinfo);

        gInfo.nextThread = 0;
      }
      else
      {
        ADD_TO_LIST(gInfo.currentThread, gInfo.readyThreads[gInfo.currentThread->currentPriority]);

        gInfo.currentThread->status = PSP_THREAD_READY;

        SET_THREAD_PRIORITY_FLAG(gInfo.currentThread->currentPriority);

        thinfo->status = PSP_THREAD_RUNNING;

        gInfo.nextThread = thinfo;
      }

      _ReleaseWaitThread(0, intr);
    }
  }
  else
  {
    ADD_TO_LIST(thinfo, gInfo.suspendedThreads);

    thinfo->status = PSP_THREAD_SUSPEND;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//104a8
int sceKernelExitThread(int status)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext() || gInfo.currentThread->eventMask)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(status < 0)
  {
    status = SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT;
  }

  gInfo.currentThread->exitStatus = status;

  while(1 == 1)
  {
    THREADMAN_TRACE(0x13, 1, status);
  
    doExitDeleteThread(0, gInfo.currentThread->exitStatus);
  }
}

//10558
int sceKernelExitDeleteThread(int status)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext() || gInfo.currentThread->eventMask)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(status < 0)
  {
    status = SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT;
  }

  gInfo.currentThread->exitStatus = status;

  while(1 == 1)
  {
    THREADMAN_TRACE(0x14, 1, status);

    doExitDeleteThread(1, gInfo.currentThread->exitStatus);
  }
}

//10608
int sceKernelTerminateThread(SceUID thid)
{
  SET_K1_SRL16;

  if(thid == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x15, 1, cb->UID);

  int ret = doTerminateDeleteThread(cb, 0, intr);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//10728
int sceKernelTerminateDeleteThread(SceUID thid)
{
  SET_K1_SRL16;

  if(thid == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x16, 1, cb->UID);

  int ret = doTerminateDeleteThread(cb, 1, intr);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//10848
void _StartThread()
{
  int intr = sceKernelCpuSuspendIntr();

  DispatchThreadEventHandlers(THREAD_START, gInfo.currentThread, intr);

  sub_0000FFA4(gInfo.currentThread->threadContext);

  gInfo.currentThread->threadContext->type = 4;

  gInfo.currentThread->threadContext->gpr[GPR_A0 - 1] = ginfo.currentThread->arglen;
  gInfo.currentThread->threadContext->gpr[GPR_A1 - 1] = gInfo.currentThread->argp;
  gInfo.currentThread->threadContext->gpr[GPR_K0 - 1] = gInfo.currentThread->unk_8C;
  gInfo.currentThread->threadContext->gpr[GPR_GP - 1] = gInfo.currentThread->gpreg;
  gInfo.currentThread->threadContext->gpr[GPR_SP - 1] = gInfo.currentThread->stackPointer;
  gInfo.currentThread->threadContext->gpr[GPR_FP - 1] = gInfo.currentThread->stackPointer;
  gInfo.currentThread->threadContext->gpr[GPR_RA - 1] = THREAD_IS_USER_MODE(gInfo.currentThread) ? gInfo.currentThread->stackPointer : sub_000136F4;
  gInfo.currentThread->threadContext->SR = THREAD_IS_USER_MODE(gInfo.currentThread) ? 0x20000013 : 0x20000003;
  gInfo.currentThread->threadContext->EPC = gInfo.currentThread->entry;

  //reset the VFPU context for the thread, if required
  if(gInfo.currentThread->vfpuContext != 0)
  {
    for(i = 0x7F; i >= 0; i--)
    {
      gInfo.currentThread->vfpuContext->unk_0[i] = 0x7F800001;
    }

    gInfo.currentThread->vfpuContext->unk_200 = 0xE4;
    gInfo.currentThread->vfpuContext->unk_204 = 0xE4;
    gInfo.currentThread->vfpuContext->unk_208 = 0;
    gInfo.currentThread->vfpuContext->unk_20C = 0x3F;
    gInfo.currentThread->vfpuContext->unk_210 = 0;
    gInfo.currentThread->vfpuContext->unk_220 = 0x3F800001;
    gInfo.currentThread->vfpuContext->unk_224 = 0x3F800002;
    gInfo.currentThread->vfpuContext->unk_228 = 0x3F800004;
    gInfo.currentThread->vfpuContext->unk_22C = 0x3F800008;
    gInfo.currentThread->vfpuContext->unk_230 = 0x3F800000;
    gInfo.currentThread->vfpuContext->unk_234 = 0x3F800000;
    gInfo.currentThread->vfpuContext->unk_238 = 0x3F800000;
    gInfo.currentThread->vfpuContext->unk_23C = 0x3F800000;
    
    if(gInfo.currentThread->vfpuContext == gInfo.vfpuContext)
    {
      gInfo.vfpuContext = 0;
    }
  }

  if(THREAD_CAN_ACCESS_VFPU(gInfo.currentThread))
  {
    gInfo.currentThread->threadContext->SR |= 0x40000000;

    _SwitchVFPUContext(gInfo.currentThread);
  }

  ReturnToThread(gInfo.currentThread->threadContext);
}

//109dc
int sceKernelSuspendDispatchThread()
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();
  if(intr == 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CPUDI;
  }

  THREADMAN_TRACE(0x17, 0);

  int ret = 0;
  if(gInfo.dispatch_thread_suspended == 0)
  {
    gInfo.dispatch_thread_suspended = 1;
    
    ret = 1;
  }

  sceKernelCpuResumeIntr(intr);
  
  RESET_K1;

  return ret;
}

//10ac0
int sceKernelResumeDispatchThread(int unk_a0)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();
  if(intr == 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CPUDI;
  }

  THREADMAN_TRACE(0x18, 0);

  if(unk_a0 != 0)
  {
    if(gInfo.dispatch_thread_suspended == 0)
    {
      sceKernelCpuResumeIntr(intr);
      
      RESET_K1;

      return SCE_KERNEL_ERROR_OK;
    }
    else if(gInfo.dispatch_thread_suspended == 1)
    {
      //nothing to do here
    }
    else if(gInfo.dispatch_thread_suspended == 2)
    {
      gInfo.dispatch_thread_suspended = 0;

      ADD_TO_LIST(gInfo.currentThread, gInfo.suspendedThreads);

      gInfo.currentThread->status = PSP_THREAD_SUSPEND;
    }
    else if(gInfo.dispatch_thread_suspended == 3)
    {
      gInfo.dispatch_thread_suspended = 0;

      doExitDeleteThread(0, SCE_KERNEL_ERROR_THREAD_TERMINATED);
      //execution will not return from doExitDeleteThread - how do we do that?
    }

    gInfo.nextThread = 0;

    _ReleaseWaitThread(0, intr);

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_OK;
  }

  if(gInfo.dispatch_thread_suspended == 0)
  {
    gInfo.dispatch_thread_suspended = 1;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//10c34
int sceKernelChangeCurrentThreadAttr(u32 clear_bits, u32 set_bits)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext() || gInfo.currentThread->eventMask)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if((clear_bits & ~PSP_THREAD_ATTR_VFPU) || (set_bits & ~PSP_THREAD_ATTR_VFPU))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
  }

  THREADMAN_TRACE(0x19, 2, clear_bits, set_bits);

  int intr = sceKernelCpuSuspendIntr();

  gInfo.currentThread->attr &= ~clear_bits;
  gInfo.currentThread->attr |= set_bits;

  int status = _MFC0(COP0_SR_STATUS);
  if(!THREAD_CAN_ACCESS_VFPU(gInfo.currentThread) || (status & 0x40000000))
  {
    //10e50
    if(!THREAD_CAN_ACCESS_VFPU(gInfo.currentThread) && (status & 0x40000000))
    {
      gInfo.currentThread->threadContext->SR &= ~0x40000000;
      if(gInfo.currentThread->scContext != 0)
      {
        gInfo.currentThread->scContext->status &= ~0x40000000; //???
      }

      _MTC0(status & ~0x40000000, COP0_SR_STATUS);
    }
  }
  else
  {
    if(gInfo.currentThread->vfpuContext == 0)
    {
      //10d84
      SceKernelHeapOptParam opt;
      opt.size = sizeof(SceKernelHeapOptParam);
      opt.alignment = 0x40;
      gInfo.currentThread->vfpuContext = sceKernelAllocHeapMemoryWithOption(gInfo.heapuid, 0x240, opt);
      if(gInfo.currentThread->vfpuContext == 0)
      {
        gInfo.currentThread->attr &= ~PSP_THREAD_ATTR_VFPU;

        sceKernelCpuResumeIntr(intr);

        RESET_K1;

        return SCE_KERNEL_ERROR_NO_MEMORY;
      }
    }

    //10d30
    gInfo.currentThread->threadContext->SR |= 0x40000000;

    if(gInfo.currentThread->scContext != 0)
    {
      gInfo.currentThread->scContext->status |= 0x40000000;
    }

    _MTC0(status | 0x40000000, COP0_SR_STATUS);

    _SwitchVFPUContext(gInfo.currentThread);
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//10ed8
int sceKernelChangeThreadPriority(SceUID thid, int priority)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  int isIntrContext = sceKernelIsIntrContext();

  if(thid == 0 && isIntrContext)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  if(thinfo->status == PSP_THREAD_STOPPED || thinfo->status == PSP_THREAD_KILLED)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_DORMANT;
  }

  if(priority == 0)
  {
    priority = gInfo.currentThread->currentPriority;
  }

  if((THREAD_IS_USER_MODE(thinfo) && priority > MAX_THREAD_PRIORITY - 8)
          || (!THREAD_IS_USER_MODE(thinfo) && priority >= MAX_THREAD_PRIORITY - 1))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
  }

  THREADMAN_TRACE(0x1A, 2, thid, priority);

  if(thinfo == gInfo.currentThread)
  {
    if(gInfo.dispatch_thread_suspended)
    {
      thinfo->currentPriority = priority;
    }
    else
    {
      if(isIntr)
      {
        thinfo->status = PSP_THREAD_READY;
        thinfo->currentPriority = priority;
        _AddThreadToReadyQueue(thinfo);
        gInfo.nextThread = 0;
      }
      else
      {
        int temp_priority, i, bit;
        for(i = 0; i < 4; i++)
        {
          if(gInfo.priority_mask[i] != 0)
          {
            for(bit = 0; bit < 31; bit++)
            {
              if(IS_SET(priority_mask[i], 1 << bit)
              {
                break;
              }
            }
            temp_priority = i * 32 + bit;
            break;
          }
        }

        if(i == 4)
        {
          temp_priority = MAX_THREAD_PRIORITY + 1;
        }

        if(temp_priority <= priority)
        {
          thinfo->currentPriority = priority;

          if(intr != 0)
          {
            thinfo->status = PSP_THREAD_READY;
            _AddThreadToReadyQueue(thinfo);
          }

          gInfo.nextThread = 0;

          _ReleaseWaitThread(0, intr);
        }
      }
    }
  }
  else
  {
    if(thinfo->status == PSP_THREAD_READY)
    {
      if(thinfo->next == thinfo->prev && thinfo->next != thinfo)
      {
        CLEAR_THREAD_PRIORITY_FLAG(thinfo->currentPriority);
      }

      thinfo->currentPriority = priority;

      REMOVE_FROM_LIST(thinfo);

      if(isIntr != 0 || gInfo.dispatch_thread_suspended)
      {
        _AddThreadToReadyQueue(thinfo);
        gInfo.nextThread = 0;
      }
      else
      {
        if(priority >= gInfo.currentThread)
        {
          thinfo->status = PSP_THREAD_READY;
          _AddThreadToReadyQueue(thinfo);
        }
        else
        {
          if(intr == 0)
          {
            thinfo->status = PSP_THREAD_READY;
            _AddThreadToReadyQueue(thinfo);
            gInfo.nextThread = 0;
          }
          else
          {
            gInfo.currentThread->status = PSP_THREAD_READY;

            ADD_TO_LIST(gInfo.currentThread, gInfo.readyThreads[gInfo.currentThread->currentPriority]);

            SET_THREAD_PRIORITY_FLAG(gInfo.currentThread->currentPriority);

            gInfo.nextThread = thinfo;

            thinfo->status = PSP_THREAD_RUNNING;

            _ReleaseWaitThread(0, intr);
          }
        }
      }
    }
    else if((thinfo->status == PSP_THREAD_WAITING || thinfo->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
            && thinfo->waitType != 0)
    {
      WaitQInfo *waitq = UID_INFO(WaitQInfo, thinfo->waitType, gInfo.uidWaitQType);
      if(waitq->attr & 0x100)
      {
        REMOVE_FROM_LIST(thinfo);

        sub_000001B4(thinfo);

        if(sub_000005F4(thinfo->waitType) != 0 && !gInfo.dispatch_thread_suspended)
        {
          gInfo.nextThread = 0;
          if(!isIntr)
          {
            _ReleaseWaitThread(0, intr);
          }
        }
      }
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//11330
int sceKernelRotateThreadReadyQueue(int priority)
{
  int i, bit;

  SET_K1_SRL16;

  int isIntrContext = sceKernelIsIntrContext();

  if(isIntrContext && priority == 0)
  {
    for(i = 0; i < 4; i++)
    {
      if(gInfo.priority_mask[i] != 0)
      {
        for(bit = 0; bit < 31; bit++)
        {
          if(IS_SET(priority_mask[i], 1 << bit)
          {
            break;
          }
        }
        priority = i * 32 + bit;
        break;
      }
    }

    if(i == 4)
    {
      priority = MAX_THREAD_PRIORITY + 1;
    }
  }

  //is this right???
  if((IS_USER_MODE && priority > 0 && priority <= MAX_THREAD_PRIORITY - 8) || (!IS_USER_MODE && priority >= MAX_THREAD_PRIORITY))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
  }

  int intr = sceKernelCpuSuspendIntr(intr);

  THREADMAN_TRACE(0x1B, 1, priority);

  if(!isIntrContext && priority == 0)
  {
    priority = gInfo.currentThread->currentPriority;
  }

  if(gInfo.readyThreads[priority].next != &gInfo.readyThreads[priority])
  {
    if(priority == gInfo.currentThread->currentPriority && !gInfo.dispatch_thread_suspended)
    {
      if(isIntrContext)
      {
        gInfo.currentThread->status = PSP_THREAD_READY;

        _AddThreadToReadyQueue(gInfo.currentThread);

        gInfo.nextThread = 0;
      }
      else if(intr != 0)
      {
        gInfo.currentThread->status = PSP_THREAD_READY;

        _AddThreadToReadyQueue(gInfo.currentThread);

        gInfo.nextThread = 0;

        _ReleaseWaitThread(0, intr);
      }
      else
      {
        gInfo.nextThread = 0;
      }
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//11538
int sceKernelReleaseWaitThread(SceUID thid)
{
  SET_K1_SRL16;

  int isIntr = sceKernelIsIntrContext();

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  if(thread == gInfo.currentThread)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  if(thread->status != PSP_THREAD_WAITING && thread->status != (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NOT_WAIT;
  }

  if(thread->waitType && !CAN_WRITE(thread->waitType))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x1C, 1, thid);

  int ret = sub_0000022C(thread);

  thread->callbackStatus = SCE_KERNEL_ERROR_RELEASE_WAIT;

  if(thread->waitType)
  {
    ret += sub_000005F4(thread->waitType);
  }

  if(ret != 0)
  {
    gInfo.nextThread = 0;

    if(!isIntr)
    {
      _ReleaseWaitThread(0, intr);
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//11738
int sceKernelGetThreadExitStatus(SceUID thid)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);
  int ret = thinfo->exitStatus;

  THREADMAN_TRACE(0x1F, 2, thid, ret);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//11848
int sceKernelGetSyscallRA()
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext() || gInfo.currentThread->scContext == 0)
  {
    RESET_K1;

    return 0;
  }

  THREADMAN_TRACE(0x27, 1, gInfo.currentThread->UID);

  RESET_K1;

  return gInfo.currentThread->scContext->ra;
}

//118e8
int sceKernelExtendKernelStack(int size, void (*cb)(void *), void *arg)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  if(IS_ADDR_KERNEL(cb))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(size < MIN_THREAD_STACKSIZE)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE;
  }

  int size = (size + 0xFF) & ~0xFF;
  int mem = sceKernelAllocPartitionMemory(SYSMEM_PARTITION_KERNEL, "threadman exstack", PSP_SMEM_High, size, 0);
  if(mem <= 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  SceThreadStack *stack = (SceThreadStack *)sceKernelGetBlockHeadAddr(mem);

  if(!THREAD_NO_FILLSTACK(gInfo.currentThread))
  {
    memset(stack, 0xFF, size);
  }

  stack->threadid = gInfo.currentThread->UID;

  THREADMAN_TRACE(0x28, 2, cb, arg);

  int intr = sceKernelCpuSuspendIntr(intr);

  SceSyscallStackInfo scStackInfo;
  scStackInfo.next = gInfo.currentThread->scStackInfo; //sp_0
  gInfo.currentThread->scStackInfo = &scStackInfo;
  if(THREAD_IS_USER_MODE(gInfo.currentThread))
  {
    scStackInfo.stack = gInfo.currentThread->kStack;
    scStackInfo.stackSize = gInfo.currentThread->kStackSize;

    gInfo.currentThread->kStack = stack;
    gInfo.currentThread->kStackSize = size;
  }
  else
  {
    scStackInfo.stack = gInfo.currentThread->stack;
    scStackInfo.stackSize = gInfo.currentThread->stackSize;

    gInfo.currentThread->stack = stack;
    gInfo.currentThread->stackSize = size;
  }

  int old_sp = SET_SP(stack + size);

  sceKernelCpuResumeIntr(intr);

  int ret = cb(arg);

  int intr = sceKernelCpuSuspendIntr();

  SET_SP(old_sp);

  if(THREAD_IS_USER_MODE(gInfo.currentThread))
  {
    gInfo.currentThread->kStack = scStackInfo.stack;
    gInfo.currentThread->kStackSize = scStackInfo.stackSize;
  }
  else
  {
    gInfo.currentThread->stack = scStackInfo.stack;
    gInfo.currentThread->stackSize = scStackInfo.stackSize;
  }

  gInfo.currentThread->scStackInfo = scStackInfo.next;

  sceKernelCpuResumeIntr(intr);

  if(THREAD_CLEAR_STACK(gInfo.currentThread))
  {
    memset(stack, 0, size);
  }

  sceKernelFreePartitionMemory(mem);

  RESET_K1;

  return ret;
}

//11b68
void _ClearAllThreadStacks(ThreadInfo *thinfo, int intr)
{
  if(THREAD_CLEAR_STACK(thinfo))
  {
    sceKernelCpuResumeIntr(intr);

    memset(thinfo->stack, 0, thinfo->stackSize);

    if(THREAD_IS_USER_MODE(thinfo))
    {
      memset(thinfo->kStack, 0, thinfo->kStackSize);
    }

    SceSyscallStackInfo *context;
    for(context = thinfo->scStackInfo; context; context = context->next)
    {
      memset(context->stack, 0, context->stackSize);
    }

    sceKernelCpuSuspendIntr();
  }
}

//11c0c
void _FreeThreadSyscallStacks(ThreadInfo *thinfo)
{
  int addr;

  while(thinfo->scStackInfo)
  {
    if(THREAD_IS_USER_MODE(thinfo))
    {
      addr = (int)thinfo->kStack;
      thinfo->kStack = thinfo->scStackInfo->stack;
      thinfo->kStackSize = thinfo->scStackInfo->stackSize;
    }
    else
    {
      addr = (int)thinfo->stack;
      thinfo->stack = thinfo->scStackInfo->stack;
      thinfo->stackSize = thinfo->scStackInfo->stackSize;
    }

    thinfo->scStackInfo = thinfo->scStackInfo->next;

    SceUID partitionid, blockid;
    sceKernelQueryMemoryInfo(addr, &partitionid, &blockid);

    sceKernelFreePartitionMemory(blockid);
  }
}

//11c90
int sceKernelCheckThreadStack()
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_OK;
  }

  THREADMAN_TRACE(0x20, 0);

  int ret;
  if(THREAD_IS_USER_MODE(gInfo.currentThread))
  {
    ret = gInfo.currentThread->scContext->sp - gInfo.currentThread->stack;
    if(ret < 0x40)
    {
      int intr = sceKernelCpuSuspendIntr();

      Kprintf("\nThread (thid=0x%x) stack overflow\n Stack = 0x%x, Stack size = 0x%x, SP = 0x%x\n",
              gInfo.currentThread->UID, gInfo.currentThread->stack, gInfo.currentThread->stackSize,
              gInfo.currentThread->scContext->sp);

      _KillThread(UIDCB_FROM_INFO(gInfo.currentThread, gInfo.uidThreadType));

      _ReleaseWaitThread(1, 1);
    }

    RESET_K1;

    return ret;
  }
  else
  {
    RESET_K1;

    //why do we check user mode again here?
    if(THREAD_IS_USER_MODE(gInfo.currentThread) || gInfo.currentThread->stack != 0)
    {
      ret = GET_SP - THREAD_IS_USER_MODE(gInfo.currentThread) ? gInfo.currentThread->kStack : gInfo.currentThread->stack;
      if(ret < 0x40)
      {
        int intr = sceKernelCpuSuspendIntr();

        SaveThreadContext(gInfo.currentThread->threadContext);

        Kprintf("\nThread (thid=0x%x) stack overflow\n Stack = 0x%x, Stack size = 0x%x, SP = 0x%x\n",
                gInfo.currentThread->UID, THREAD_IS_USER_MODE(gInfo.currentThread) ? gInfo.currentThread->kStack : gInfo.currentThread->stack,
                THREAD_IS_USER_MODE(gInfo.currentThread) ? gInfo.currentThread->kStackSize : gInfo.currentThread->stackSize,
                gInfo.currentThread->threadContext->gpr[GPREG_SP - 1]);

        _KillThread(UIDCB_FROM_INFO(gInfo.currentThread, gInfo.uidThreadType);

        _ReleaseWaitThread(1, 1);
      }

      return ret;
    }
    else
    {
      return 0;
    }
  }
}

//11e60
int sceKernelGetThreadStackFreeSize(SceUID thid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  
  if(thid == 0)
  {
    thid = gInfo.currentThread->UID;
  }
  
  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  THREADMAN_TRACE(0x21, 1, thid);

  SceThreadStack *stack = thread->stack;
  int size = thread->stackSize;

  sceKernelCpuResumeIntr(intr);

  int ret = 0, i;

  for(i = 0; i < (size - 0x10) / sizeof(int); i++)
  {
    if(stack->value[i] != -1)
    {
      ret = i * sizeof(int);
      break;
    }
  }

  RESET_K1;

  return size;
}

//11fd4
int sceKernelGetThreadKernelStackFreeSize(SceUID thid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  
  thid = (thid == 0) ? gInfo.currentThread->UID : thid;

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  THREADMAN_TRACE(0x2A, 1, thid);

  SceThreadStack *stack = THREAD_IS_USER_MODE(thread) ? thread->kStack : thread->stack;
  int size = THREAD_IS_USER_MODE(thread) ? thread->kStackSize : thread->stackSize;

  sceKernelCpuResumeIntr(intr);

  int ret = 0;
  for(i = 0; i < (size - 0x10) / sizeof(int); i++)
  {
    if(stack->value[i] != -1)
    {
      ret = i * sizeof(int);
      break;
    }
  }

  RESET_K1;

  return ret;
}

//1215c
int sceKernelReferThreadStatus(SceUID thid, SceKernelThreadInfo *info)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  int isIntrContext = sceKernelIsIntrContext();

  if(IS_USER_MODE && IS_ADDR_KERNEL(info))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(thid == 0)
  {
    if(isIntrContext)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
    }

    thid = gInfo.currentThread->UID;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;
    
    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }
  
  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);
  
  THREADMAN_TRACE(0x22, 2, thid, info);
  
  SceKernelThreadInfo *buf = (SceKernelThreadInfo *)gInfo.unk_6A0;
  
  memset(buf, 0, sizeof(SceKernelThreadInfo));
  
  buf->size = sizeof(SceKernelThreadInfo);
  
  if(thread->name)
  {
    strncpy(buf->name, thread->name, 0x1F);
  }
  
  buf->attr = thread->attr;
  buf->entry = thread->entry;
  buf->status = thread->status;
  buf->stackSize = thread->stackSize;
  buf->stack = thread->stack;
  buf->initPriority = thread->initPriority;
  buf->currentPriority = thread->currentPriority;
  buf->gpReg = thread->gpReg;
  if(thread->status == PSP_THREAD_WAITING || thread->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
  {
    buf->waitType = thread->waitType;
    buf->waitId = thread->waitTypeCB->UID;
  }
  buf->wakeupCount = thread->wakeupCount;
  buf->exitStatus = thread->exitStatus;

  t6 = gInfo.currentThread;
  v0 = 0;
  fp = &buf->runClocks;
  s2 = thread->runClocks.hi;
  a3 = thread->runClocks.low;
  t9 = s2 << 0;
  s2 = v0 + a3;
  t8 = s2 < a3;
  t7 = t9 + s3;
  s3 = t7 + t8;
  if(thread == gInfo.currentThread)
  {
    v0v1 = sub_0000B490();

    t0 = gInfo.unk_428.hi;
    a3 = gInfo.unk_428.low;

    a1 = 0;
    t8 = t0 << 0;
    t5 = a1 + a3;
    t9 = 0;
    t7 = t5 < a3;
    t6 = t8 + t9;
    t1 = t6 + t7;
    a0 = v0 - t5;
    t4 = v0 < t5;
    t3 = v1 - t1;
    t2 = t3 - t4;
    s2 = s2 + a0;
    t0 = s2 < a0;
    a2 = s3 + t2;
    s3 = a2 + t0;
  }

  v1 = s3 >> 0;
  buf->runClocks.hi = v1;
  buf->runClocks.low = s2;
  
  buf->intrPreemptCount = thread->intrPreemptCount;
  buf->threadPreemptCount = thread->threadPreemptCount;
  buf->releaseCount = thread->releaseCount;
  
  memcpy(info, buf, MIN(info->size, buf->size));
  
  sceKernelCpuResumeIntr(intr);
  
  RESET_K1;
  
  return SCE_KERNEL_ERROR_OK;
}

//12474
int sceKernelReferThreadRunStatus(SceUID thid, SceKernelThreadRunStatus *status)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(status))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(thid == 0)
  {
    thid = gInfo.currentThread->UID;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  THREADMAN_TRACE(0x23, 2, thid, status);

  memset(gInfo.var_6A0, 0, sizeof(SceKernelThreadRunStatus));

  SceKernelThreadRunStatus *buf = (SceKernelThreadRunStatus *)gInfo.var_6A0;
  buf->size = sizeof(SceKernelThreadRunStatus);
  buf->status = thread->status;
  buf->currentPriority = thread->currentPriority;
  if(buf->status == PSP_THREAD_WAITING || buf->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
  {
    buf->waitType = thread->waitType;
    if(thread->waitTypeCB)
    {
      buf->waitId = thread->waitTypeCB->UID;
    }
  }

  buf->wakeupCount = thread->wakeupCount;
  if(thread == gInfo.currentThread)
  {
    v0v1 = sub_0000B490();

    t8 = gInfo.unk_428.hi;
    t6 = gInfo.unk_428.low;
    t7 = 0;
    t5 = t8 << 0;
    t2 = t7 + t6;
    a3 = 0;
    t4 = t2 < t6;
    t3 = t5 + a3;
    t1 = t3 + t4;
    a0 = v0 - t2;
    t0 = v0 < t2;
    a1 = v1 - t1;
    t9 = a1 - t0;
    s0 = s0 + a0;
    t8 = s0 < a0;
    v0 = s1 + t9;
    s1 = v0 + t8;
  }

  a2 = s1 >> 0;
  buf->runClocks.hi = a2;
  buf->runClocks.low = s0;
  
  buf->intrPreemptCount = thread->intrPreemptCount;
  buf->threadPreemptCount = thread->threadPreemptCount;
  buf->releaseCount = thread->releaseCount;
  
  memcpy(status, buf, MIN(buf->size, status->size));
  
  sceKernelCpuResumeIntr(intr);
  
  RESET_K1;
  
  return SCE_KERNEL_ERROR_OK;
}

//1272c
int sceKernelReferThreadKernelStatus(int thid, SceKernelThreadKInfo *info)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();
  
  int isIntr = sceKernelIsIntrContext();
  
  if(IS_USER_MODE && IS_ADDR_KERNEL(info))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(thid == 0)
  {
    if(isIntr)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_ILLEGAL_THID;
    }

    thid = gInfo.currentThread->UID;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, gInfo.uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x24, 2, thid, info);

  SceKernelThreadKInfo *buf = (SceKernelThreadKInfo *)gInfo.unk_6A0;

  _ReferThreadKernelStatus(cb, buf);

  memcpy(info, gInfo.unk_6A0, MIN(gInfo.unk_6A0->size, info->size);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;
  
  return SCE_KERNEL_ERROR_OK;
}

//128c4
int sceKernelReferSystemStatus(SceKernelSystemStatus *status)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(IS_USER_MODE && IS_ADDR_KERNEL(status))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  THREADMAN_TRACE(0x89, 1, status);

  SceKernelSystemStatus *buf = (SceKernelSystemStatus *)gInfo.unk_6A0;

  _ReferSystemStatus(buf, intr);

  memcpy(status, buf, MIN(status->size, buf->size));

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//129c8
int sceKernelGetThreadmanIdList(enum SceKernelIdListType type, SceUID *readbuf, int readbufsize, int *idcount)
{
  int count = 0, offset = 0;

  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(readbuf) || IS_ADDR_KERNEL(idcount))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr();

  THREADMAN_TRACE(0x8A, 4, type, readbuf, readbufsize, idcount);

  if(type >= 0x3F)
  {
    LinkedList *list;
    switch(type)
    {
      case 0x40:
      {
        list = &gInfo.sleepingThreads;
        break;
      }

      case 0x41:
      {
        list = &gInfo.delayedThreads;
        break;
      }

      case 0x42:
      {
        list = &gInfo.suspendedThreads;
        break;
      }
      
      case 0x43:
      {
        list = &gInfo.stoppedThreads;
        break;
      }

      default:
      {
        if(sceKernelGetUIDcontrolBlock(type, &typecb) != 0 || SysMemForKernel_82D3CEE3(typecb, uidWaitQType) == 0)
        {
          sceKernelCpuResumeIntr(intr);

          RESET_K1;

          return SCE_KERNEL_ERROR_ILLEGAL_TYPE;
        }

        if(!CAN_READ_UID(typecb))
        {
          sceKernelCpuResumeIntr(intr);

          RESET_K1;

          return SCE_KERNEL_ERROR_ILLEGAL_PERM;
        }

        WaitQInfo *waitq = UID_INFO(WaitQInfo, typecb, gInfo.uidWaitQType); //v0
        list = &waitq->waitThread;
      }
    }

    ThreadInfo *thread;
    LOOP_LIST(thread, *list)
    {
      uidControlBlock *cb = UIDCB_FROM_INFO(thread, gInfo.uidThreadType);

      if(!IS_USER_MODE || cb->attribute != 0)
      {
        count++;
        if(offset < readbufsize)
        {
          readbuf[offset++] = cb->UID;
        }
      }
    }
  }
  else
  {
    uidControlBlock *typecb;
    switch(type)
    {
      case 1:
      {
        typecb = gInfo.uidThreadType;
        break;
      }

      case 2:
      {
        typecb = gInfo.uidSemaphoreType;
        break;
      }

      case 3:
      {
        typecb = gInfo.uidEventFlagType;
        break;
      }

      case 4:
      {
        typecb = gInfo.uidMboxType;
        break;
      }

      case 5:
      {
        typecb = gInfo.uidVplType;
        break;
      }

      case 6:
      {
        typecb = gInfo.uidFplType;
        break;
      }

      case 7:
      {
        typecb = gInfo.uidMsgPipeType;
        break;
      }

      case 8:
      {
        typecb = gInfo.uidCallbackType;
        break;
      }

      case 9:
      {
        typecb = gInfo.uidThreadEventHandlerType;
        break;
      }

      case 10:
      {
        typecb = gInfo.uidAlarmType;
        break;
      }

      case 11:
      {
        typecb = gInfo.uidVTimerType;
        break;
      }

      default:
      {
        sceKernelCpuResumeIntr(intr);

        RESET_K1;

        return SCE_KERNEL_ERROR_ILLEGAL_TYPE;
      }
    }

    uidControlBlock *cb;
    ThreadInfo *thread;
    for(cb = typecb; cb != typecb->parent; cb = cb->parent)
    {
      if(typecb != gInfo.uidThreadType || (thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType)) != gInfo.thIdle && thread != gInfo.thCleaner))
      {
        if(!IS_USER_MODE || cb->attr != 0)
        {
          count++;
          if(offset < readbufsize)
          {
            readbuf[offset++] = cb->UID;
          }
        }
      }
    }
  }

  sceKernelCpuResumeIntr();

  if(idcount != 0)
  {
    *idcount = count;
  }
}

//12d68
enum SceKernelIdListType sceKernelGetThreadmanIdType(SceUID uid)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlock(uid, &cb) != 0 || (IS_USER_MODE && cb->attr == 0))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT;
  }

  THREADMAN_TRACE(0x8B, 1, uid);

  if(SysMemForKernel_82D3CEE3(cb, gInfo.uidThreadType))
  {
    ret = SCE_KERNEL_TMID_Thread;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidSemaphoreType))
  {
    ret = SCE_KERNEL_TMID_Semaphore;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidEventFlagType))
  {
    ret = SCE_KERNEL_TMID_EventFlag;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidMboxType))
  {
    ret = SCE_KERNEL_TMID_Mbox;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidVplType))
  {
    ret = SCE_KERNEL_TMID_Vpl;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidFplType))
  {
    ret = SCE_KERNEL_TMID_Fpl;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidMsgPipeType))
  {
    ret = SCE_KERNEL_TMID_Mpipe;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidCallbackType))
  {
    ret = SCE_KERNEL_TMID_Callback;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidThreadEventHandlerType))
  {
    ret = SCE_KERNEL_TMID_ThreadEventHandler;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidAlarmType))
  {
    ret = SCE_KERNEL_TMID_Alarm;
  }
  else if(SysMemForKernel_82D3CEE3(cb, uidVTimerType))
  {
    ret = SCE_KERNEL_TMID_VTimer;
  }
  else
  {
    ret = SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

UidFunction ThreadUidFuncTable[] = //var_15DE8
{
  { UIDFUNC_CREATE, ThreadCreateUID },
  { UIDFUNC_DELETE, ThreadDeleteUID },
  { 0, 0 }
};

//12f30
int initThread()
{
  int ret;

  ret = sceKernelCreateUIDtypeInherit("WaitQ", "Thread", sizeof(ThreadInfo), ThreadUidFuncTable, 0, &gInfo.uidThreadType);

  ASSERT(ret <= 0);

  return SCE_KERNEL_ERROR_OK;
}

//12fbc
int sceKernelSetThreadTraceFunction(void (*func)(int type, int ra, int nargs, ...), ThreadmanInfo *tminfo)
{
  gInfo.trace = func;

  if(tminfo != 0)
  {
    *tminfo = &gInfo;
  }

  return SCE_KERNEL_ERROR_OK;
}

//12fd8
void loc_00012fd8(unk_a0, unk_a1, unk_a2, unk_a3, unk_t0, unk_t1, unk_t2, unk_t3)
{
  int intr = sceKernelCpuSuspendIntr();

  //this is very weird!
  if(gInfo.trace)
  {
    gInfo.trace(0, 0, unk_a1 + 1, unk_a0, unk_a2, unk_a3, unk_t0, unk_t1);
  }

  sceKernelCpuResumeIntr(intr);
}

//1308c
void _AddThreadToReadyQueue(ThreadInfo *thread)
{
  ADD_TO_LIST(thread, gInfo.readyThreads[thread->currentPriority]);

  SET_THREAD_PRIORITY_FLAG(thread->currentPriority);
}

//130e4
void _RemoveThreadFromReadyQueue(ThreadInfo *thread, int priority)
{
  ThreadInfo *next = thread->next, *prev = thread->prev;

  if(thread->next == thread->prev && thread->next != thread)
  {
    CLEAR_THREAD_PRIORITY_FLAG(priority);
  }

  REMOVE_FROM_LIST(thread);
}

//1313c
void _SwitchToThread(ThreadInfo *thread, int intr)
{
  if(thread->currentPriority >= gInfo.currentThread->currentPriority || gInfo.dispatch_thread_suspended)
  {
    thread->status = PSP_THREAD_READY;

    _AddThreadToReadyQueue(thread);

    return;
  }

  if(intr != 0)
  {
    ADD_TO_LIST(gInfo.currentThread, gInfo.readyThreads[gInfo.currentThread->currentPriority]);
    SET_THREAD_PRIORITY_FLAG(gInfo.currentThread->currentPriority);
    gInfo.currentThread->status = PSP_THREAD_READY;

    thread->status = PSP_THREAD_RUNNING;
    gInfo.nextThread = thread;
  }
  else
  {
    _AddThreadToReadyQueue(thread);

    thread->status = PSP_THREAD_READY;
    gInfo.nextThread = 0;
  }

  _ReleaseWaitThread(0, intr);
}

//1326c
void _sceKernelExitThread()
{
  sceKernelExitThread(gInfo.currentThread->scContext->unk[1]);
}

//13294
int sceKernelGetThreadId()
{
  SET_K1_SRL16;

  THREADMAN_TRACE(0x1D, 0);

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  RESET_K1;

  return gInfo.currentThread->UID;
}

//13318
int sceKernelGetThreadCurrentPriority()
{
  SET_K1_SRL16;

  THREADMAN_TRACE(0x1E, 1, gInfo.currentThread->currentPriority);

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  RESET_K1;

  return gInfo.currentThread->currentPriority;
}

//1339c
int sceKernelGetUserLevel()
{
  SET_K1_SRL16;

  THREADMAN_TRACE(0x25, 1, (gInfo.currentThread->attr >> 28) ^ 0x8);

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  RESET_K1;

  return (gInfo.currentThread->attr >> 28 ^ 0x8);
}

//13430
int sceKernelIsUserModeThread()
{
  SET_K1_SRL16;

  THREADMAN_TRACE(0x26, 1, THREAD_IS_USER_MODE(gInfo.currentThread));

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  RESET_K1;

  return (THREAD_IS_USER_MODE(gInfo.currentThread));
}

//134bc
int _CheckThreadKernelStack()
{
  if(!THREAD_IS_USER_MODE(gInfo.currentThread) && gInfo.currentThread->stack == 0)
  {
    return 0;
  }

  int bottom;
  if(THREAD_IS_USER_MODE(gInfo.currentThread))
  {
    bottom = gInfo.currentThread->kStack;
  }
  else
  {
    bottom = gInfo.currentThread->stack;
  }

  ret = _GET_GPREG(GPREG_SP) - bottom;
  if(ret >= 0x40)
  {
    return ret;
  }

  sceKernelCpuSuspendIntr();

  SaveThreadContext(gInfo.currentThread->threadContext);

  Kprintf("\nThread (thid=0x%x) stack overflow\n Stack = 0x%x, Stack size = 0x%x, SP = 0x%x\n",
          gInfo.currentThread->UID, bottom,
          gInfo.currentThread->attr < 0 ? gInfo.currentThread->kStackSize : gInfo.currentThread->stackSize,
          gInfo.currentThread->threadContext->gpr[GPREG_FP - 1]);

  _KillThread(UIDCB_FROM_INFO(gInfo.currentThread, Info.uidThreadType));

  _ReleaseWaitThread(1, 1);

  return ret;
}

//135a8
int sceKernelCheckThreadKernelStack()
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return 0;
  }

  THREADMAN_TRACE(0x29, 0);

  RESET_K1;

  return _CheckThreadKernelStack();
}

//13628
int sceKernelGetSystemStatusFlag()
{
  return gInfo.systemStatusFlag;
}

//13634
int sceKernelReferThreadProfiler()
{
  if(sceKernelIsIntrContext() || gInfo.cpuProfilerMode != CPU_PROFILER_THREAD)
  {
    return 0;
  }

  return (gInfo.currentThread->thread_profiler_info != 0) ? (0x40000000 | (0xBC400000 & 0x3FFFFFFF)) : 0;
}

//13694
int sceKernelReferGlobalProfiler()
{
  return (gInfo.cpuProfilerMode == CPU_PROFILER_GLOBAL) ? (0x40000000 | (0xBC400000 & 0x3FFFFFFF)) : 0;
}

//136c0
int sub_000136C0()
{
  sceKernelExitDeleteThread(0);

  return SCE_KERNEL_ERROR_OK;
}

//136e0
int IdleThread(SceSize args, void *argp)
{
// IdleThread:		; Refs: 0x0000EAC0
// 	0x000136E0: 0x4002C800 '...@' - mfc0       $v0, EBase
//
// loc_000136E4:		; Refs: 0x000136EC
// 	0x000136E4: 0xBC4A0000 '..J.' - cache      0xA, 0x0($v0)
// 	0x000136E8: 0x70000000 '...p' - halt
// 	0x000136EC: 0x08004DB9 '.M..' - j          loc_000136E4
// 	0x000136F0: 0x00000000 '....' - nop
}

//136f4
int sub_000136F4()
{
  return sceKernelExitThread(_GET_GPREG(GPREG_V0));
}

//13710
void _GetNextExecutableThread()
{
  int i, bit, priority;

  gInfo.nextThread = gInfo.currentThread;
  if(gInfo.doCleanup == 0)
  {
    if(gInfo.currentThread->status == PSP_THREAD_RUNNING)
    {
      for(i = 0; i < (MAX_THREAD_PRIORITY + 1) / 32; i++)
      {
        if(priority_mask[i] != 0)
        {
          for(bit = 0; bit < 31; bit++)
          {
            if(IS_SET(priority_mask[i], 1 << bit)
            {
              break;
            }
          }
          priority = i * 32 + bit;
          break;
        }
      }

      if(i == (MAX_THREAD_PRIORITY + 1) / 32)
      {
        priority = MAX_THREAD_PRIORITY + 1;
      }

      if(priority > MAX_THREAD_PRIORITY)
      {
        Kprintf("Panic: not found ready Thread\n");
        return;
      }

      if(priority >= gInfo.currentThread->currentPriority)
      {
        return;
      }

      //make current thread ready
      SET_THREAD_PRIORITY_FLAG(gInfo.currentThread->currentPriority);
      ADD_TO_LIST(gInfo.currentThread, gInfo.readyThreads[gInfo.currentThread->currentPriority]);
      gInfo.currentThread->status = PSP_THREAD_READY;

      //set next thread to run
      if(gInfo.readyThreads[priority].next->next == gInfo.readyThreads[priority].next->prev
              && gInfo.readyThreads[priority].next->next != gInfo.readyThreads[priority].next)
      {
        CLEAR_THREAD_PRIORITY_FLAG(priority);
      }
      REMOVE_FROM_LIST(gInfo.readyThreads[priority].next);
      gInfo.nextThread = gInfo.readyThreads[priority].next;
      gInfo.readyThreads[priority].next->status = PSP_THREAD_RUNNING;
    }
    else
    {
      for(i = 0; i < (MAX_THREAD_PRIORITY + 1) / 32; i++)
      {
        if(gInfo.priority_mask[i] != 0)
        {
          for(bit = 0; bit < 31; bit++)
          {
            if(IS_SET(priority_mask[i], 1 << bit)
            {
              break;
            }
          }
          priority = i * 32 + bit;
          break;
        }
      }

      if(i == (MAX_THREAD_PRIORITY + 1) / 32)
      {
        priority = MAX_THREAD_PRIORITY + 1;
      }

      if(priority > MAX_THREAD_PRIORITY)
      {
        Kprintf("Panic: not found executable Thread\n");
        return;
      }

      if(gInfo.readyThreads[priority].next->next == gInfo.readyThreads[priority].next->prev
              && gInfo.readyThreads[priority].next->next != gInfo.readyThreads[priority].next)
      {
        CLEAR_THREAD_PRIORITY_FLAG(priority);
      }
      REMOVE_FROM_LIST(gInfo.readyThreads[priority].next);
      gInfo.nextThread = gInfo.readyThreads[priority].next;
      gInfo.readyThreads[priority].next->status = PSP_THREAD_RUNNING;
    }
  }
  else
  {
    gInfo.doCleanup = 0;

    //set the cleaner thread to run next
    gInfo.thCleaner->status = PSP_THREAD_RUNNING;
    gInfo.nextThread = gInfo.thCleaner;

    if(gInfo.currentThread->status != PSP_THREAD_RUNNING)
    {
      return;
    }

    //make current thread ready
    gInfo.currentThread->status = PSP_THREAD_READY;
    ADD_TO_LIST(gInfo.currentThread, gInfo.readyThreads[gInfo.currentThread->currentPriority / 32]);
    SET_THREAD_PRIORITY_FLAG(gInfo.currentThread->currentPriority);
  }
}

//139a0
void _KillThread(uidControlBlock *cb)
{
  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);
  if(gInfo.dispatch_thread_suspended)
  {
    gInfo.dispatch_thread_suspended = 0;
    Kprintf("thread_is_dead: Thread dispatch is disabled!\n");
  }

  sub_000004C0(cb, SCE_KERNEL_ERROR_THREAD_TERMINATED);

  sub_00002228(thinfo);

  sub_000023D4(thinfo);

  sub_0000E7A8(thinfo);

  sub_0000237C(thinfo);

  ADD_TO_LIST(thinfo, gInfo.deadThreads);
  thinfo->exitStatus = SCE_KERNEL_ERROR_DORMANT;
  thinfo->status = PSP_THREAD_KILLED;

  gInfo.nextThread = 0;

  Kprintf("Thread \"%s\" (thid=0x%x) is DEAD\n\n", cb->name ? cb->name : var_15b68, thinfo->UID);

  s0 = 0;
  s2 = 0x3;
  t8 = s0 & 0x3;

  //13a84
  int i;
  for(i = 0; i < 32; i++)
  {
    if(!(i % 4))
    {
      Kprintf("$%02d: ", i);
    }

    t3 = thinfo->threadContext;
    //13a94
    t2 = s0 << 2;
    t1 = t2 + t3;
    Kprintf("0x%08x ", thinfo->threadContext->gpr[i]);

    //wtf? 0 < s0 < 0x20, so it will never be signed
    t0 = i >> 31; //sra
    a2 = t0 >> 30;
    a1 = i + a2;
    v0 = a1 >> 2; //sra
    a0 = v0 << 2;
    t9 = s0 - a0;
    if(i - (v0 << 2) == 3)
    {
      Kprintf("\n");
    }
  }
}

//13b34
void doExitDeleteThread(int unk_a0, int unk_a1)
{
  int intr = sceKernelCpuSuspendIntr();

  if(gInfo.dispatch_thread_suspended)
  {
    Kprintf("doExitDeleteThread: Thread dispatch is disabled!\n");
    gInfo.dispatch_thread_suspended = 0;
  }

  uidControlBlock *cb = UIDCB_FROM_INFO(gInfo.currentThread, gInfo.uidThreadType);
  if(unk_a0 != 0)
  {
    gInfo.currentThread->eventMask |= THREAD_DELETE;
  }

  DispatchThreadEventHandlers(THREAD_EXIT, gInfo.currentThread, intr);

  sub_0000E7A8(gInfo.currentThread);

  sub_000004C0(cb, unk_a1);

  sub_0000237C(gInfo.currentThread);

  if(gInfo.currentThread->eventMask & THREAD_DELETE)
  {
    DispatchThreadEventHandlers(THREAD_DELETE, gInfo.currentThread, intr);

    gInfo.currentThread->status = PSP_THREAD_DORMANT;

    _AddThreadToCleanupQueue(gInfo.currentThread->UID);
  }
  else
  {
    ADD_TO_LIST(gInfo.currentThread, gInfo.stoppedThreads);

    gInfo.currentThread->status = PSP_THREAD_STOPPED;

    gInfo.nextThread = 0;

    _ReleaseWaitThread(1, 1);

    while(1 == 1)
    {
      Kprintf("panic ! Thread DORMANT !\n");
      //break 0x1
    }
  }
}

//13c70
int doTerminateDeleteThread(uidControlBlock *cb, int del, int intr)
{
  int ret = SCE_KERNEL_ERROR_OK;

  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  int isIntr = sceKernelIsIntrContext();
  if(del > 0 && isIntr)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(!isIntr)
  {
    if(gInfo.currentThread->eventMask != 0)
    {
      return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
    }

    if(thread == gInfo.currentThread)
    {
      return SCE_KERNEL_ERROR_ILLEGAL_THID;
    }
  }

  if(thread->status == PSP_THREAD_DORMANT)
  {
    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(thread->status == PSP_THREAD_STOPPED || thread->status == PSP_THREAD_KILLED)
  {
    if(isIntr && thread == gInfo.currentThread && gInfo.dispatch_thread_suspended)
    {
      gInfo.dispatch_thread_suspended = 3;

      return SCE_KERNEL_ERROR_OK;
    }

    if(thread->eventMask & (THREAD_EXIT | THREAD_DELETE))
    {
      if(del == 0)
      {
        return SCE_KERNEL_ERROR_DORMANT;
      }

      //err...and the point of this is??
      thread->eventMask |= THREAD_DELETE;

      return SCE_KERNEL_ERROR_OK;
    }

    thread->exitStatus = SCE_KERNEL_ERROR_THREAD_TERMINATED;
    if(gInfo.dispatch_thread_suspended)
    {
      gInfo.dispatch_thread_suspended = 0;
      Kprintf("doTerminateDeleteThread: Thread dispatch is disabled!\n");
    }

    sub_00002228(thread);

    sub_000023D4(thread);

    ret = sub_0000E7A8(thread);
    if(sub_0000E7A8(thread) == 0 && sub_000022B4(0x4, thread) == 0)
    {
      sub_000004C0(cb, SCE_KERNEL_ERROR_THREAD_TERMINATED);

      sub_0000237C(thread);

      if(!isIntr || thread != gInfo.currentThread)
      {
        if(thread->status == PSP_THREAD_READY)
        {
          if(thread->next == thread->prev && thread->next != thread)
          {
            CLEAR_THREAD_PRIORITY_FLAG(thread->currentPriority);
          }

          REMOVE_FROM_LIST(thread);
        }
        else
        {
          REMOVE_FROM_LIST(thread);

          if(thread->status == PSP_THREAD_WAITING || thread->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
          {
            sub_00000544(thread);

            if(thread->waitType != 0)
            {
              sub_000005F4(thread->waitType);
            }
          }
        }
      }

      ADD_TO_LIST(thread, gInfo.stoppedThreads);

      thread->status = PSP_THREAD_STOPPED;
    }
    else
    {
      if(thread->status > PSP_THREAD_READY)
      {
        REMOVE_FROM_LIST(thread);

        if(thread->status == PSP_THREAD_WAITING || thread->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
        {
          sub_00000544(thread);

          if(thread->waitType != 0)
          {
            sub_000005F4(thread->waitType);
          }
        }
      }

      sub_0000FFA4(thread->threadContext);

      thread->threadContext->type = 0x4;
      thread->threadContext->gpr[GPREG_A1 - 1] = SCE_KERNEL_ERROR_THREAD_TERMINATED;
      thread->threadContext->gpr[GPREG_A0 - 1] = 0;
      thread->threadContext->gpr[GPREG_SP - 1] = THREAD_IS_USER_MODE(thread) ? thread->kStackPointer : thread->stackPointer;
      thread->threadContext->gpr[GPREG_RA - 1] = sub_000136F4;
      thread->threadContext->gpr[GPREG_FP - 1] = thread->threadContext->gpr[GPREG_SP - 1];
      thread->threadContext->EPC = doExitDeleteThread;
      thread->threadContext->gpr[GPREG_GP - 1] = thread->gpreg;
      thread->threadContext->status = 0x20000003;
      if(thread->status != PSP_THREAD_READY)
      {
        thread->status = PSP_THREAD_READY;
        _AddThreadToReadyQueue(thread);

        gInfo.nextThread = 0;
      }

      if(!isIntr)
      {
        ret = AddThreadToWaitQueue(cb, 0x9, 0, 0, 0);
        if(ret != SCE_KERNEL_ERROR_THREAD_TERMINATED)
        {
          ret = SCE_KERNEL_ERROR_OK;
        }
      }
    }

    if(del == 0)
    {
      return ret;
    }

    DispatchThreadEventHandlers(THREAD_DELETE, thread, intr);

    thread->status = PSP_THREAD_DORMANT;

    REMOVE_FROM_LIST(thread);

    _ClearAllThreadStacks(thread, intr);

    ASSERT(sceKernelDeleteUID(thread->UID) <= 0);

    return ret;
  }
  else
  {
    if(del == 0)
    {
      return SCE_KERNEL_ERROR_DORMANT;
    }

    DispatchThreadEventHandlers(THREAD_DELETE, thread, intr);

    thread->status = 0;

    REMOVE_FROM_LIST(thread);

    _ClearAllThreadStacks(thread, intr);

    ASSERT(sceKernelDeleteUID(thread->UID) <= 0);

    return SCE_KERNEL_ERROR_OK;
  }
}

//1415c
void _ReferThreadKernelStatus(uidControlBlock *cb, SceKernelThreadKInfo *status)
{
  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType;

  memset(status, 0, sizeof(SceKernelThreadKInfo));

  status->size = sizeof(SceKernelThreadKInfo);
  if(cb->name)
  {
    strncpy(status->name, cb->name, 31);
  }

  status->attr = thread->attr;
  status->entry = thread->entry;
  status->status = thread->status;
  status->stackSize = thread->stackSize;
  status->stack = thread->stack;
  status->kstackSize = thread->kStackSize;
  status->args = thread->arglen;
  status->kstack = thread->kStack;
  status->gpReg = thread->gpreg;
  status->initPriority = thread->initPriority;
  status->argp = thread->argp;
  status->currentPriority = thread->currentPriority;
  if(status->status == PSP_THREAD_WAITING || status->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
  {
    status->waitType = thread->waitType;
    if(thread->waitTypeCB != 0)
    {
      status->waitId = thread->waitTypeCB->UID;
    }
    else
    {
      status->waitId = 0; //unnecessary!
    }
  }

  status->wakeupCount = thread->wakeupCount;

  s3 = &gInfo;
  t5 = 0;
  t3 = 0;
  s4 = thread->runClocks.hi;
  a1 = thread->runClocks.low;
  s0 = &status->runClocks;
  t4 = s4 << 0;
  s4 = t5 + a1;
  t2 = s4 < a1;
  a0 = t4 + t3;
  s5 = a0 + t2;

  if(thread == gInfo.currentThread)
  {
    v0v1 = sub_0000B490();

    t7 = gInfo.unk_428.hi;
    a1 = gInfo.unk_428.low;
    t6 = 0;
    t5 = t7 << 0;
    t2 = t6 + a1;
    a3 = 0;
    t4 = t2 < a1;
    t3 = t5 + a3;
    t1 = t3 + t4;
    t0 = v0 - t2;
    a0 = v0 < t2;
    a2 = v1 - t1;
    t9 = a2 - a0;
    s4 = s4 + t0;
    t8 = s4 < t0;
    t7 = s5 + t9;
    s5 = t7 + t8;
  }

  t9 = s5 << 0;
  status->runClocks.hi = t9;
  status->runClocks.low = s4;
  status->thContext = thread->threadContext;
  status->intrPreemptCount = thread->intrPreemptCount;
  status->vfpuContext = thread->vfpuContext;
  status->threadPreemptCount = thread->threadPreemptCount;
  status->releaseCount = thread->releaseCount;
  if(thread->vfpuContext != 0 && gInfo.vfpuContext == thread->vfpuContext)
  {
    SaveVFPUContext(gInfo.vfpuContext);
    gInfo.vfpuContext = 0;
  }

  if(thread->scContext != 0)
  {
    status->retAddr = thread->scContext->ra;
  }
  else
  {
    status->retAddr = 0;
  }

  status->scContext = thread->scContext;
  status->thread_profiler_info = &thread->thread_profiler_info->info;
}

//14398
void _ReferSystemStatus(SceKernelSystemStatus *status, int intr)
{
  memset(status, 0, sizeof(SceKernelSystemStatus));

  status->size = sizeof(SceKernelSystemStatus);

  if(sceKernelIsIntrContext())
  {
    status->status = 4;
  }
  else if(intr == 0)
  {
    status->status = 3;
  }
  else if(gInfo.dispatch_thread_suspended == 0)
  {
    status->status = 0;
  }
  else
  {
    status->status = 1;
  }

  status->idleClocks.low = gInfo.thIdle->runClocks.low;
  status->idleClocks.hi = gInfo.thIdle->runClocks.hi;
  status->comesOutOfIdleCount = gInfo.thIdle->intrPreemptCount;
  status->threadSwitchCount = gInfo.threadSwitchCount;
  status->vfpuSwitchCount = gInfo.vfpuSwitchCount;
}

//1443c
int ThreadCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);

  memset(thinfo, 0, sizeof(ThreadInfo));

  thinfo->next = thinfo;
  thinfo->prev = thinfo;
  thinfo->UID = cb->UID;
  thinfo->status = 0;
  CLEAR_LIST(thinfo->callbacks);

  return cb->UID;
}

//144b4
int ThreadDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int intr = sceKernelCpuSuspendIntr(); //s3

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, gInfo.uidThreadType);
  if(thinfo->status != 0)
  {
    int ret = doTerminateDeleteThread(cb, 1, intr);
    if(ret != 0)
    {
      //14628
      sceKernelCpuResumeIntr(intr);

      return ret;
    }
  }

  _FreeThreadSyscallStacks(thinfo);

  SceUID partitionid, blockid;
  if(thinfo->stack != 0)
  {
    sceKernelQueryMemoryInfo(thinfo->stack, &partitionid, &blockid);

    sceKernelFreePartitionMemory(blockid);
  }

  //14534
  if(THREAD_IS_USER_MODE(thinfo))
  {
    //145d0
    if(thinfo->kStack != 0)
    {
      sceKernelQueryMemoryInfo(thinfo->kStack, &partitionid, &blockid);

      sceKernelFreePartitionMemory(blockid);
    }

    //145dc
    if(thinfo->thread_profiler_info != 0)
    {
      sceKernelFreeHeapMemory(heapuid, thinfo->thread_profiler_info);
    }
  }

  //14540
  if(thinfo->threadContext != 0)
  {
    //145c0
    sceKernelFreeHeapMemory(heapuid, thinfo->threadContext);
  }

  //1454c
  if(thinfo->vfpuContext != 0)
  {
    if(gInfo.vfpuContext == thinfo->vfpuContext)
    {
      gInfo.vfpuContext = 0;
    }

    sub_00000D84();

    sceKernelFreeHeapMemory(heapuid, thinfo->vfpuContext);
  }

  //14578
  sceKernelCpuResumeIntr(intr);

  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return SCE_KERNEL_ERROR_OK;
}