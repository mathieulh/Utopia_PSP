#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thsema.h"

extern ThreadmanInfo gInfo;

//3c90
SceUID sceKernelCreateSema(const char *name, SceUInt attr, int initVal, int maxVal, SceKernelSemaOptParam *option)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(name) || IS_ADDR_KERNEL(option)))
  {
    Kprintf("thsema.c:%s:sceKernelIsCalledByUser == 0x%x\n", __FUNCTION__, IS_USER_MODE);

    Kprintf("thsema.c:%s:SCE_KERNEL_ERROR_ILLEGAL_ADDR, oldstat=0x%x\n", __FUNCTION__, old_k1);

    //SCE bug: should reset K1 here?

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(attr & ~SCE_SEMA_LEGAL_ATTR)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  int ret = sceKernelCreateUID(uidSemaphoreType, name, IS_USER_MODE ? 0xFF : (attr & 0xFF), &cb);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType);
  waitq->attr = attr;

  SemaphoreInfo *sema = UID_INFO(SemaphoreInfo, cb, uidSemaphoreType);
  sema->option = option;
  sema->cur_count = initVal;
  sema->max_count = maxVal;
  sema->init_count = initVal;

  THREADMAN_TRACE(0x36, 1, name);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return cb->UID;
}

//3e68
int sceKernelDeleteSema(SceUID semaid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(semaid, uidSemaphoreType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
  }

  if(!CAN_DELETE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x37, 1, semaid);

  ASSERT(sceKernelDeleteUID(semaid) <= 0);
  
  sceKernelCpuResumeIntr(intr);

  RESET_K1;
  
  return 0;
}

//3fc0
int sceKernelSignalSema(SceUID semaid, int signal)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  int isIntrContext = sceKernelIsIntrContext();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(semaid, uidSemaphoreType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType);

  SemaphoreInfo *sema = UID_INFO(SemaphoreInfo, cb, uidSemaphoreType);

  THREADMAN_TRACE(0x38, 2, semaid, signal);

  ThreadInfo *thread;
  int newsig = sema->cur_count + signal;
  LOOP_LIST(thread, waitq->waitThread)
  {
    newsig -= (int)thread->cbArg1;
  }

  if(newsig > sema->max_count)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_SEMA_OVF;
  }

  sema->cur_count += signal;

  int count = 0;
  ThreadInfo *last_thread = 0;
  LOOP_LIST(thread, waitq->waitThread)
  {
    if((int)thread->cbArg1 > sema->cur_count)
    {
      break;
    }

    sema->cur_count -= (int)thread->cbArg1;

    if(sub_0000022C(thread) > 0)
    {
      last_thread = thread;
      count++;
    }
  }

  if(count == 1)
  {
    _RemoveThreadFromReadyQueue(last_thread, last_thread->currentPriority);

    _SwitchToThread(last_thread, intr);
  }
  else if(count > 1)
  {
    gInfo.nextThread = 0;

    if(!isIntrContext)
    {
      _ReleaseWaitThread(0, intr);
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//4200
int sceKernelWaitSema(SceUID semaid, int signal, SceUInt *timeout)
{
  return _WaitSema(semaid, signal, timeout, 0);
}

//4208
int sceKernelWaitSemaCB(SceUID semaid, int signal, SceUInt *timeout)
{
  return _WaitSema(semaid, signal, timeout, 1);
}

//4210
int _WaitSema(SceUID semaid, int signal, SceUInt *timeout, int isCB)
{
  int ret;

  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(timeout))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(signal <= 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
  }

  int intr = sceKernelCpuSuspendIntr(); //s5

  if(intr == 0 || gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  _CheckThreadKernelStack();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(semaid, uidSemaphoreType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
  }

  if(!CAN_UNK4_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType);

  SemaphoreInfo *sema = UID_INFO(SemaphoreInfo, cb, uidSemaphoreType);

  if(signal > sema->max_count)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
  }

  THREADMAN_TRACE(isCB ? 0x3A : 0x39, 2, semaid, signal);

  SceKernelSysClock clk;
  if(timeout)
  {
    sub_0000D6D8(&clk, *timeout);
  }

  int sp_18 = 0;
  do
  {
    if(isCB && gInfo.currentThread->callbackNotify)
    {
      DispatchCallbacks(intr);
    }

    if(waitq->numWaitThreads <= 0 || !(waitq->attr & SCE_SEMA_ATTR_UNK100) || gInfo.currentThread->currentPriority < waitq->waitThread.next->currentPriority)
    {
      if(!(waitq->attr & SCE_SEMA_ATTR_UNK100))
      {
        sp_18 = 1;
      }

      if(sp_18 != 0 || sema->cur_count < signal)
      {
        gInfo.currentThread->cbArg1 = signal;
      }
      else
      {
        ret = 0;
        sema->cur_count -= signal;

        break;
      }
    }
    else
    {
      sp_18 = 1;
      gInfo.currentThread->cbArg1 = signal;
    }

    gInfo.currentThread->isCallback = isCB;

    ret = AddThreadToWaitQueue(cb, 0x3, waitq->attr, &clk, timeout);
  } while(isCB && ret == SCE_KERNEL_ERROR_NOTIFY_CALLBACK)

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//4528
int sceKernelPollSema(SceUID semaid, int signal)
{
  SET_K1_SRL16;

  if(signal <= 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(semaid, uidSemaphoreType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
  }

  if(!CAN_UNK4_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType);

  SemaphoreInfo *sema = UID_INFO(SemaphoreInfo, cb, uidSemaphoreType);

  if(signal > sema->max_count)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
  }

  THREADMAN_TRACE(0x3B, 2, semaid, signal);

  if((waitq->numWaitThreads > 0 && (!(waitq->attr & SCE_SEMA_ATTR_UNK100) || gInfo.currentThread->currentPriority >= waitq->waitThread->currentPriority))
                   || sema->cur_count < signal)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_SEMA_ZERO;
  }

  sema->cur_count -= signal;

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//46e4
int sceKernelCancelSema(int semaid, int signal, int *result)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  sceKernelIsIntrContext();

  if(IS_USER_MODE && IS_ADDR_KERNEL(result))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(semaid, uidSemaphoreType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
  }

  if(!CAN_CANCEL_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  SemaphoreInfo *sema = UID_INFO(SemaphoreInfo, cb, uidSemaphoreType);
  if(signal > sema->max_count)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
  }

  THREADMAN_TRACE(0x3C, 3, semaid, signal, result);

  int ret = sceKernelCallUIDObjFunction(cb, UIDFUNC_CANCEL);

  if(signal < 0)
  {
    signal = sema->init_count;
  }

  sema->cur_count = signal;
  if(result != 0)
  {
    *result = ret;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//4898
int sceKernelReferSemaStatus(SceUID semaid, SceKernelSemaInfo *info)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();
  
  if(IS_USER_MODE && IS_KERNEL_ADDR(info))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(semaid, uidSemaphoreType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
  }
  
  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x3D, 2, semaid, info);

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType);

  SemaphoreInfo *sema = UID_INFO(SemaphoreInfo, cb, uidSemaphoreType);

  SceKernelSemaInfo *buf = (SceKernelSemaInfo *)gInfo.unk_6A0;

  memset(buf, 0, sizeof(SceKernelSemaInfo));

  buf->size = sizeof(SceKernelSemaInfo);

  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }

  buf->attr = waitq->attr;
  buf->numWaitThreads = waitq->numWaitThreads;
  buf->currentCount = sema->cur_count;
  buf->maxCount = sema->max_count;
  buf->initCount = sema->init_count;

  memcpy(info, buf, MIN(info->size, buf->size));

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

UidFunction SemaphoreUidFuncTable[] = //15CE0
{
  { 0x85FE2B4C, sub_00004b54 },
  { UIDFUNC_CREATE, SemaphoreCreateUID },
  { UIDFUNC_DELETE, SemaphoreDeleteUID },
  { 0, 0 }
};

//4a80
int initSema()
{
  int ret;

  ret = sceKernelCreateUIDtypeInherit("WaitQ", "Semaphore", sizeof(SemaphoreInfo), SemaphoreUidFuncTable, 0, &uidSemaphoreType);
}

//4b0c
int SemaphoreCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//4b34
int SemaphoreDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);
  
  return 0;
}

//4b54
int sub_00004B54(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType);

  SemaphoreInfo *sema = UID_INFO(SemaphoreInfo, cb, uidSemaphoreType);

  if(waitq->numWaitThreads <= 0)
  {
    return 0;
  }

  int count = 0;
  ThreadInfo *thread;
  LOOP_LIST(thread, qInfo->waitThread)
  {
    if(thread->cbArg1 <= sema->cur_count)
    {
      break;
    }

    sema->cur_count -= thread->cbArg1;

    if(sub_0000022C(thread) > 0)
    {
      count++;
    }
  }

  return count;
}

