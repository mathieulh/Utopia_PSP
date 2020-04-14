#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thutils.h"
#include "util.h"

//0
//for some reason, the values passed for waitType do not tie up with the SceKernelIdListType enumeration...
//in general, it seems to have 1 added, although that is not always the case (callback/event handler)
int AddThreadToWaitQueue(uidControlBlock *waitTypeCB, int waitType, int attr, SceKernelSysClock *clk, int *timeout)
{
  int ret;

  gInfo.currentThread->timeoutId = 0;

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, waitTypeCB, gInfo.uidWaitQType); //s1
  if(timeout != 0)
  {
    ret = sub_0000D994(clk, sub_00000698, gInfo.currentThread);
    if(ret < 0)
    {
      return ret;
    }

    gInfo.currentThread->timeoutId = ret;
    gInfo.currentThread->timeout = timeout;
  }

  gInfo.currentThread->status = PSP_THREAD_WAITING;
  gInfo.currentThread->waitType = waitType;
  gInfo.currentThread->callbackStatus = 0;
  gInfo.currentThread->waitTypeCB = waitTypeCB;
  qinfo->numWaitThreads++;
  if(!(attr & SCE_WAITQ_ATTR_UNK100))
  {
    ADD_TO_LIST(gInfo.currentThread, qinfo->waitThread);
  }
  else
  {
    LOOP_LIST(thread, qinfo->waitThread)
    {
      if(thread->currentPriority > gInfo.currentThread->currentPriority)
      {
        break;
      }
    }

    if(qinfo->unk_4 != 0 && thread->prev == &qinfo->waitThread)
    {
      thread = thread->next;
    }

    INSERT_IN_LIST_BEFORE(gInfo.currentThread, thread);
  }

  gInfo.nextThread = 0;

  _ReleaseWaitThread(1, 1);

  return gInfo.currentThread->callbackStatus;
}

//17c
void sub_0000017C(LinkedList *list, int waitType)
{
  ADD_TO_LIST(gInfo.currentThread, list);

  gInfo.currentThread->status = PSP_THREAD_WAITING;
  gInfo.currentThread->waitType = waitType;
  gInfo.currentThread->waitTypeCB = 0;
  gInfo.currentThread->callbackStatus = 0;
}

//1b4
void sub_000001B4(ThreadInfo *thinfo, uidControlBlock *cb)
{
  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);

  ThreadInfo *thread;
  LOOP_LIST(thread, qinfo->waitThread)
  {
    if(thread->currentPriority > thinfo->currentPriority)
    {
      break;
    }
  }

  //1fc
  if(qinfo->unk_4 != 0 && thread->prev == &qinfo->waitThread)
  {
    thread = thread->next;
  }

  //20c
  INSERT_IN_LIST_BEFORE(thinfo, thread);
}

//22c
int sub_0000022c(ThreadInfo *thinfo)
{
  if(thinfo->status == PSP_THREAD_KILLED)
  {
    return 0;
  }

  thinfo->isCallback = 0;
  if(thinfo->timeoutId != 0)
  {
    //2f4
    int ret = sub_0000DAF4(thinfo->timeoutId, thinfo->timeout);

    ASSERT(ret < 1);

    thinfo->timeoutId = 0;
    thinfo->timeout = 0;
  }

  //25c
  if(thinfo->waitTypeCB != 0)
  {
    sceKernelCallUIDObjFunction(thinfo->waitTypeCB, 0x5BD49380);
  }

  //26c
  REMOVE_FROM_LIST(thinfo);

  if(thinfo->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
  {
    ADD_TO_LIST(thinfo, gInfo.suspendedThreads);

    thinfo->status = PSP_THREAD_SUSPEND;

    return 0;
  }
  else
  {
    thinfo->status = PSP_THREAD_READY;
    _AddThreadToReadyQueue(thinfo);

    return 1;
  }
}

//344
void sub_00000344(ThreadInfo *thinfo, int isIntrContext, int intr)
{
  if(thinfo->status == PSP_THREAD_KILLED)
  {
    return;
  }

  thinfo->isCallback = 0;
  if(thinfo->timeoutId != 0)
  {
    int ret = sub_0000DAF4(thinfo->timeoutId, thinfo->timeout);

    ASSERT(ret <= 0);

    thinfo->timeoutId = 0;
    thinfo->timeout = 0;
  }

  if(thinfo->waitTypeCB != 0)
  {
    sceKernelCallUIDObjFunction(thinfo->waitTypeCB, 0x5BD49380);
  }

  REMOVE_FROM_LIST(thinfo);

  if(thinfo->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
  {
    ADD_TO_LIST(thinfo, gInfo.suspendedThreads);

    thinfo->status = PSP_THREAD_SUSPEND;
  }
  else
  {
    thinfo->status = PSP_THREAD_READY;

    if(isIntrContext != 0)
    {
      _AddThreadToReadyQueue(thinfo);

      gInfo.nextThread = 0;
    }
    else
    {
      _SwitchToThread(thinfo, intr);
    }
  }
}

//480
void sub_00000480(uidControlBlock *cb, int status, int isIntrContext, int intr)
{
  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);

  qinfo->waitThread.next->callbackStatus = status;

  sub_00000344(qinfo->waitThread.next, isIntrContext, intr);
}

//4c0
int sub_000004C0(uidControlBlock *cb, int status)
{
  int i = 0;
  ThreadInfo *thread;
  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);

  LOOP_LIST(thread, qinfo->waitThread)
  {
    sub_0000022C(thread);

    thread->callbackStatus = status;

    i++;
  }

  return i;
}

//544
void sub_00000544(ThreadInfo *thinfo)
{
  thinfo->isCallback = 0;

  if(thinfo->timeoutId != 0)
  {
    int ret = sub_0000DAF4(thinfo->timeoutId, thinfo->timeout);

    ASSERT(ret <= 0);

    thinfo->timeoutId = 0;
    thinfo->timeout = 0;
  }

  if(thinfo->waitTypeCB != 0)
  {
    sceKernelCallUIDObjFunction(thinfo->waitTypeCB, 0x5BD49380);
  }
}

//5f4
int sub_000005F4(uidControlBlock *cb)
{
  return sceKernelCallUIDObjFunction(cb, 0x85FE2B4C);
}

UidFunction WaitQUidFuncTable[] = //15c80
{
  { 0x5BD49380, loc_00000a18 },
  { 0x85FE2B4C, loc_00000a10 },
  { UIDFUNC_CANCEL, WaitQCancel },
  { UIDFUNC_CREATE, WaitQCreateUID },
  { UIDFUNC_DELETE, WaitQDeleteUID },
  { 0, 0 }
};

//614
int initWaitQ()
{
  int ret;

  ret = sceKernelCreateUIDtype("WaitQ", sizeof(WaitQInfo), WaitQUidFuncTable, 0, &uidWaitQType);

  ASSERT(ret <= 0, "initWaitQ");

  return 0;
}

//698
void sub_00000698(ThreadInfo *thinfo)
{
  int ret;

  int intr = sceKernelCpuSuspendIntr();

  thinfo->timeoutId = 0;
  if(thinfo->timeout != 0)
  {
    thinfo->*timeout = 0;
  }

  thinfo->timeout = 0;

  if(thinfo->status != PSP_THREAD_KILLED)
  {
    thinfo->isCallback = 0;
    if(thinfo->timeoutId != 0) //err...it's set to 0 above!?
    {
      int ret = sub_0000DAF4(thinfo->timeoutId, 0);

      ASSERT(ret <= 0);

      thinfo->timeoutId = 0;
      thinfo->timeout = 0;
    }

    if(thinfo->waitTypeCB != 0)
    {
      sceKernelCallUIDObjFunction(thinfo->waitTypeCB, 0x5BD49380);
    }

    REMOVE_FROM_LIST(thinfo);

    if(thinfo->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
    {
      ADD_TO_LIST(thinfo, gInfo.suspendedThreads);

      thinfo->status = PSP_THREAD_SUSPEND;

      ret = 0;
    }
    else
    {
      thinfo->status = PSP_THREAD_READY;

      _AddThreadToReadyQueue(thinfo);

      ret = 1;
    }
  }

  thinfo->callbackStatus = SCE_KERNEL_ERROR_WAIT_TIMEOUT;

  if(thinfo->waitTypeCB != 0)
  {
    sceKernelCallUIDObjFunction(thinfo->waitTypeCB, 0x85FE2B4C);
  }

  if(ret != 0)
  {
    gInfo.nextThread = 0;
  }

  sceKernelCpuResumeIntr(intr);
}

//810
int WaitQCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  qinfo->attr = 0;
  qinfo->unk_4 = 0;
  qinfo->numWaitThreads = 0;
  CLEAR_LIST(qinfo->waitThread);

  return cb->UID;
}

//868
int WaitQDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int i = 0;
  ThreadInfo *thread;

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, uidWaitQType, uidWaitQType);

  LOOP_LIST(thread, qinfo->waitThread)
  {
    sub_0000022C(thread);

    thread->callbackStatus = SCE_KERNEL_ERROR_WAIT_DELETE;

    i++;
  }

  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  if(i != 0)
  {
    gInfo.nextThread = 0;

    _ReleaseWaitThread(0, 0);
  }

  return 0;
}

//95c
int WaitQCancel(uidControlBlock *cb)
{
  int i;
  ThreadInfo *thread;

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);

  for(i = 0, thread = qinfo->waitThread.next; thread != &qinfo->waitThread; i++, thread = thread->next)
  {
    sub_0000022C(thread);

    thread->callbackStatus = SCE_KERNEL_ERROR_WAIT_CANCEL;

    i++;
  }

  if(i != 0)
  {
    gInfo.nextThread = 0;

    _ReleaseWaitThread(0, 0);
  }

  return i;
}

//a10
int loc_00000a10(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  return 0;
}

//a18
int loc_00000a18(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, type);

  qinfo->numWaitThreads--;

  return 0;
}

//a38
void _ReleaseWaitThread(int isRelease, int unk_a1)
{
  gInfo.currentThread->pCount = isRelease ? &gInfo.currentThread->releaseCount : &gInfo.currentThread->threadPreemptCount;

  if(unk_a1 == 0)
  {
    _MTC0(_MFC0(COP0_SR_CAUSE) | 0x200, COP0_SR_CAUSE);

    sceKernelEnableIntr(0x41);

    return;
  }

  int profbase = _CFC0(COP0_CR_PROFILER_BASE);
  if(profbase != 0)
  {
    if(profbase->unk_8++ == 0)
    {
      profbase->unk_4 = profbase->unk_0->unk_0;
      profbase->unk_0->unk_0 = 0;

      _SYNC();
    }
  }

  if(SaveThreadContext(gInfo.currentThread->threadContext) == 0)
  {
    ReturnToThread(_SwitchThread(gInfo.currentThread->threadContext));
  }

  gInfo.currentThread->isCallback = 0;

  if(profbase != 0)
  {
    if(profbase->unk_8 != 0)
    {
      if(--profbase->unk_8 == 0)
      {
        profbase->unk_0->unk_0 = profbase->unk_4;

        _SYNC();
      }
    }
  }
}
