#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thevent.h"

extern ThreadmanInfo gInfo;

//4c04
int sceKernelCreateEventFlag(const char *name, int attr, int bits, SceKernelEventFlagOptParam *opt)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(name) || IS_ADDR_KERNEL(opt))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(attr & ~SCE_EVF_LEGAL_ATTR)
  {
    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  int ret = sceKernelCreateUID(uidEventFlagType, name, IS_USER_MODE ? 0xFF : (attr & 0xFF), &cb);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  qinfo->attr = attr;

  EventFlagInfo *efinfo = UID_INFO(EventFlagInfo, cb, uidEventFlagType);
  efinfo->opt = opt;
  efinfo->currentPattern = bits;
  efinfo->initialPattern = bits;

  THREADMAN_TRACE(0x3E, 1, cb->UID);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return cb->UID;
}

//4d98
int sceKernelDeleteEventFlag(int evid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(evid, uidEventFlagType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;
    
    return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
  }
  
  if(!CAN_DELETE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;

    return SCE_KERNEL_ERROR_INVALID_PERM;
  }

  THREADMAN_TRACE(0x3F, 1, evid);

  ASSERT(sceKernelDeleteUID(evid) <= 0);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//4ef0
int sceKernelSetEventFlag(SceUID evid, u32 bits)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  int isIntr = sceKernelIsIntrContext();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(evid, uidEventFlagType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);

  EventFlagInfo *evinfo = UID_INFO(EventFlagInfo, cb, uidEventFlagType);

  if(bits == 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_OK;
  }

  THREADMAN_TRACE(0x40, 2, evid, bits);

  evinfo->currentPattern |= bits;

  int s6 = 0;
  ThreadInfo *thread, *last_thread;
  LOOP_LIST(thread, qinfo->waitThread)
  {
    if(evinfo->currentPattern == 0)
    {
      break;
    }

    int match;
    if(thread->cbArg2 & PSP_EVENT_WAITOR)
    {
      match = (evinfo->currentPattern & thread->cbArg1);
    }
    else
    {
      match = ((evinfo->currentPattern & thread->cbArg1) == thread->cbArg1);
    }

    if(match)
    {
      if(thread->cbArg3 != 0)
      {
        *thread->cbArg3 = evinfo->currentPattern;
      }

      if(thread->cbArg2 & PSP_EVENT_WAITCLEAR)
      {
        evinfo->currentPattern = 0;
      }

      if(thread->cbArg2 & PSP_EVENT_WAITCLEARMATCHED)
      {
        evinfo->currentPattern &= ~thread->cbArg1;
      }

      if(sub_0000022C(thread) > 0)
      {
        last_thread = thread;
        s6++;
      }
    }
  }

  if(isIntr && s6 > 0)
  {
    gInfo.nextThread = 0;
  }
  else if(s6 == 1)
  {
    _RemoveThreadFromReadyQueue(last_thread, last_thread->currentPriority);

    _SwitchToThread(last_thread, intr);
  }
  else if(s6 > 0)
  {
    gInfo.nextThread = 0;

    _ReleaseWaitThread(0, intr);
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//5158
int sceKernelClearEventFlag(SceUID evid, u32 bits)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  sceKernelIsIntrContext();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(evid, uidEventFlagType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  EventFlagInfo *evinfo = UID_INFO(EventFlagInfo, cb, uidEventFlagType);

  THREADMAN_TRACE(0x41, 2, evid, bits);

  sceKernelCpuResumeIntr(intr);

  evinfo->currentPattern &= bits;

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//5284
int sceKernelWaitEventFlag(int evid, u32 pattern, u32 flags, u32 *outBits, unsigned int *timeout)
{
  return _WaitEventFlag(evid, pattern, flags, outBits, timeout, 0);
}

//528c
int sceKernelWaitEventFlagCB(int evid, u32 pattern, u32 flags, u32 *outBits, unsigned int *timeout)
{
  return _WaitEventFlag(evid, pattern, flags, outBits, timeout, 1);
}

//5294
int _WaitEventFlag(int evid, u32 pattern, u32 flags, u32 *outBits, unsigned int *timeout, int isCB)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(outBits) || IS_ADDR_KERNEL(timeout)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if((flags & ~0x31) || (flags & 0x30) == 0x30)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MODE;
  }
  
  if(pattern == 0)
  {
    RESET_K1;
    
    return SCE_KERNEL_ERROR_EVF_ILPAT;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  if(intr == 0 || gInfo.dispatch_thread_suspended)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  _CheckThreadKernelStack();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(evid, uidEventFlagType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
  }

  if(!CAN_UNK4_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType);
  EventFlagInfo *event = UID_INFO(EventFlagInfo, cb, uidEventFlagType);

  if(!(waitq->attr & PSP_EVENT_WAITMULTIPLE) && waitq->numWaitThreads > 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_EVF_MULTI;
  }

  THREADMAN_TRACE(isCB ? 0x43 : 0x42, 4, evid, pattern, flags, outBits);

  SceKernelSysClock clk;
  if(timeout)
  {
    sub_0000D6D8(&clk, *timeout);
  }

  do
  {
    if(timeout && gInfo.currentThread->callbackNotify != 0)
    {
      DispatchCallbacks(intr);
    }

    int match;
    if(flags & PSP_EVENT_WAITOR)
    {
      match = event->currentPattern & pattern;
    }
    else
    {
      match = ((event->currentPattern & pattern) == pattern);
    }

    if(match)
    {
      if(outBits)
      {
        *outBits = event->currentPattern;
      }

      if(flags & PSP_EVENT_WAITCLEAR)
      {
        event->currentPattern = 0;
      }

      if(flags & PSP_EVENT_WAITCLEARMATCHED)
      {
        event->currentPattern &= ~pattern;
      }

      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_OK;
    }

    gInfo.currentThread->cbArg1 = pattern;
    gInfo.currentThread->cbArg2 = flags;
    gInfo.currentThread->cbArg3 = outBits;
    gInfo.currentThread->isCallback = isCB;

    ret = AddThreadToWaitQueue(cb, 0x4, 0, &clk, timeout); //s0
  } while(isCB && ret == SCE_KERNEL_ERROR_NOTIFY_CALLBACK);

  if(ret > 0 && outBits)
  {
    *outBits = event->currentPattern;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//5680
int sceKernelPollEventFlag(int evid, u32 pattern, u32 flags, u32 *outBits)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(outBits))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if((flags & ~0x31) || (flags & 0x30) == 0x30)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MODE;
  }

  if(pattern == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_EVF_ILPAT;
  }

  int intr = sceKernelCpuSuspendIntr(); //s7

  //hmm, why do this, and then call sceKernelGetUIDcontrolBlockWithType()?
  uidControlBlock *cb = UIDCB_FROM_UID(evid);

  if(sceKernelGetUIDcontrolBlockWithType(evid, gInfo.uidEventFlagType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
  }

  if(!CAN_UNK4_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, gInfo.uidWaitQType); //v0
  EventFlagInfo *event = UID_INFO(EventFlagInfo, cb, gInfo.uidEventFlagType); //s2

  if(!(waitq->attr & PSP_EVENT_WAITMULTIPLE) && waitq->numWaitThreads > 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_EVF_MULTI;
  }

  THREADMAN_TRACE(0x44, 4, evid, pattern, flags, outBits);

  u32 match;
  if(flags & PSP_EVENT_WAITOR)
  {
    match = event->currentPattern & pattern;
  }
  else
  {
    match = ((event->currentPattern & pattern) == pattern);
  }

  if(outBits)
  {
    outBits = event->currentPattern;
  }

  if(match == 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_EVF_COND;
  }

  if(flags & PSP_EVENT_WAITCLEAR)
  {
    event->currentPattern = 0;
  }

  if(flags & PSP_EVENT_WAITCLEARMATCHED)
  {
    event->currentPattern &= ~pattern;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//58e8
int sceKernelCancelEventFlag(int evid, int pattern, int *result)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  sceKernelIsIntrContext();

  if(IS_USER_MODE && !IS_ADDR_KERNEL(result))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_INVALID_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(evid, uidEventFlagType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
  }

  if(!CAN_CANCEL_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  EventFlagInfo *evinfo = UID_INFO(EventFlagInfo, cb, uidEventFlagType);

  THREADMAN_TRACE(0x45, 3, evid, pattern, result);

  int ret = sceKernelCallUIDObjFunction(cb, UIDFUNC_CANCEL);

  evinfo->currentPattern = pattern;

  if(result)
  {
    *result = ret;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//5a64
int sceKernelReferEventFlagStatus(SceUID uid, SceKernelEventFlagInfo *status)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(IS_USER_MODE && IS_ADDR_KERNEL(status))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidEventFlagType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x46, 2, uid, status);

  SceKernelEventFlagInfo *buf = (SceKernelEventFlagInfo *)var_164a8;

  memset(buf, 0, sizeof(SceKernelEventFlagInfo));

  buf->size = sizeof(SceKernelEventFlagInfo);

  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  buf->attr = qinfo->attr;
  buf->numWaitThreads = qinfo->numWaitThreads;

  EventFlagInfo *evinfo = UID_INFO(EventFlagInfo, cb, uidEventFlagType);
  buf->initPattern = evinfo->initialPattern;
  buf->currentPattern = evinfo->currentPattern;

  memcpy(status, buf, MIN(buf->size, status->size);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

UidFunction EventFlagUidFuncTable[] = //15d00
{
  { UIDFUNC_CREATE, EventFlagCreateUID },
  { UIDFUNC_DELETE, EventFlagDeleteUID },
  { 0, 0 }
};

//5c44
int initEventFlag()
{
  int ret;

  ret = sceKernelCreateUIDtypeInherit("WaitQ", "EventFlag", sizeof(EventFlagInfo), EventFlagUidFuncTable, 0, &uidEventFlagType);

  ASSERT(ret <= 0);

  return SCE_KERNEL_ERROR_OK;
}

//5cd0
int EventFlagCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//5cf8
int EventFlagDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return SCE_KERNEL_ERROR_OK;
}
