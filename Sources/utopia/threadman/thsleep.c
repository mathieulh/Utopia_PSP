#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thsleep.h"

//2a0c
int sceKernelSleepThread()
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();
  if(intr == 0 || gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    Kprintf("thsleep.c:%s:SCE_KERNEL_ERROR_CAN_NOT_WAIT\n", __FUNCTION__);

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  THREADMAN_TRACE(0x2E, 0);

  _CheckThreadKernelStack();

  gInfo.currentThread->callbackStatus = 0;
  if(gInfo.currentThread->wakeupCount == 0)
  {
    sub_0000017C(&gInfo.sleepingThreads, 1);

    gInfo.nextThread = 0;

    _ReleaseWaitThread(1, 1);
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return gInfo.currentThread->callbackStatus;
}

//2b48
int sceKernelSleepThreadCB()
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();
  if(intr == 0 || gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  THREADMAN_TRACE(0x2F, 0);

  _CheckThreadKernelStack();

  while(1 == 1)
  {
    if(gInfo.currentThread->callbackNotify != 0)
    {
      DispatchCallbacks(intr, gInfo.currentThread->callbackNotify);
    }

    //a2 = gInfo.currentThread->wakeupCount;
    gInfo.currentThread->callbackStatus = 0;
    if(gInfo.currentThread->wakeupCount == 0)
    {
      sub_0000017C(&gInfo.sleepingThreads, 1);

      gInfo.currentThread->isCallback = 1;

      gInfo.nextThread = 0;

      _ReleaseWaitThread(1, 1);
    }
    else
    {
      gInfo.currentThread->wakeupCount--;
    }

    if(gInfo.currentThread->callbackStatus != SCE_KERNEL_ERROR_NOTIFY_CALLBACK)
    {
      break;
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return gInfo.currentThread->callbackStatus;
}

//2cac
int sceKernelWakeupThread(SceUID thid)
{
  SET_K1_SRL16;

  if(thid == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  int isIntrContext = sceKernelIsIntrContext();

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x2))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, uidThreadType);
  if(!isIntrContext && thinfo == gInfo.currentThread)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  if(thinfo->status == PSP_THREAD_STOPPED || thinfo->status == PSP_THREAD_KILLED)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_DORMANT;
  }

  THREADMAN_TRACE(0x30, 1, thid);

  if((thinfo->status & PSP_THREAD_SUSPEND) && thinfo->waitType == SCE_KERNEL_TMID_Thread)
  {
    sub_00000344(thinfo, isIntrContext, intr);
  }
  else
  {
    thinfo->wakeupCount++;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//2e7c
int sceKernelCancelWakeupThread(SceUID thid)
{
  SET_K1_SRL16;

  int isIntrContext = sceKernelIsIntrContext();

  int intr = sceKernelCpuSuspendIntr();

  if(thid == 0)
  {
    if(isIntrContext)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_ILLEGAL_THID;
    }

    thid = gInfo.currentThread->UID;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x8))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, uidThreadType);

  THREADMAN_TRACE(0x31, 1, thid);

  int ret = thinfo->wakeupCount;
  thinfo->wakeupCount = 0;

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//2fc4
int sceKernelSuspendThread(SceUID thid)
{
  SET_K1_SRL16;

  int isIntrContext = sceKernelIsIntrContext();

  int intr = sceKernelCpuSuspendIntr();
  
  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }
  
  if(IS_USER_MODE && !(cb->attr & 0x2))
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }
  
  ThreadInfo *thinfo = UID_INFO(ThreadInfo, cb, uidThreadType);
  
  if(thinfo == gInfo.currentThread)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  THREADMAN_TRACE(0x32, 1, thid);

  if(thinfo->status > PSP_THREAD_KILLED)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_DORMANT;
  }

  int ret;
  switch(thinfo->status)
  {
    case PSP_THREAD_RUNNING:
    {
      if(gInfo.dispatch_thread_suspended)
      {
        if(gInfo.dispatch_thread_suspended != 0x3)
        {
          gInfo.dispatch_thread_suspended = 0x2;
        }

        ret = 0;
        break;
      }
      //fall through to case 2
    }

    case PSP_THREAD_READY:
    {
      if(isIntrContext)
      {
        if(thinfo == gInfo.currentThread)
        {
          gInfo.nextThread = 0;
        }
        else
        {
          _RemoveThreadFromReadyQueue(thinfo, thinfo->currentPriority);
        }

        ADD_TO_LIST(thinfo, gInfo.suspendedThreads);

        thinfo->status = PSP_THREAD_SUSPEND;

        ret = 0;
      }
      break;
    }

    case PSP_THREAD_WAITING:
    {
      thinfo->status |= PSP_THREAD_SUSPEND;
      ret = 0;
      break;
    }

    case PSP_THREAD_SUSPEND:
    case PSP_THREAD_WAITING | PSP_THREAD_SUSPEND:
    {
      ret = SCE_KERNEL_ERROR_SUSPEND;
      break;
    }

    default:
    {
      ret = SCE_KERNEL_ERROR_DORMANT;
      break;
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//31e0
int sceKernelResumeThread(SceUID thid)
{
  SET_K1_SRL16;

  if(thid == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  int isIntr = sceKernelIsIntrContext();

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x2))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, uidThreadType);

  if(!isIntr && thread == gInfo.currentThread)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  THREADMAN_TRACE(0x33, 1, thid);

  if(thread->status > 0x20)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NOT_SUSPEND;
  }

  int ret;
  switch(thread->status)
  {
    case PSP_THREAD_RUNNING:
    {
      if(gInfo.dispatch_thread_suspended == 2)
      {
        gInfo.dispatch_thread_suspended = 1;
        ret = 0;
      }
      else
      {
        ret = SCE_KERNEL_ERROR_NOT_SUSPEND;
      }
      break;
    }

    case PSP_THREAD_SUSPEND:
    {
      thread->status = PSP_THREAD_READY;
      
      REMOVE_FROM_LIST(thread);

      if(!isIntr)
      {
        _SwitchToThread(thread, intr);
      }
      else
      {
        _AddThreadToReadyQueue(thread);

        gInfo.nextThread = 0;
      }

      ret = 0;
      break;
    }

    case (PSP_THREAD_SUSPEND | PSP_THREAD_WAITING):
    {
      thread->status = PSP_THREAD_WAITING;

      ret = 0;
      break;
    }

    default:
    {
      ret = SCE_KERNEL_ERROR_NOT_SUSPEND;
      break;
    }
  }
  
  sceKernelCpuResumeIntr(intr);
  
  RESET_K1;
  
  return ret;
}

//33dc
int sceKernelSuspendAllUserThreads()
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr(); //s6

  uidControlBlock *cb;
  for(cb = uidThreadType->parent; cb; cb = cb->parent)
  {
    ThreadInfo *thread = UID_INFO(ThreadInfo, cb, uidThreadType);
    
    if((thread->attr & 0x80000000) || (thread->attr & 0x8000000))
    {
      ASSERT(thread != gInfo.currentThread);
      
      if(thread->currentPriority <= PSP_THREAD_KILLED)
      {
        switch(thread->currentPriority)
        {
          case PSP_THREAD_READY:
          case (PSP_THREAD_READY | PSP_THREAD_WAITING):
          {
            _RemoveThreadFromReadyQueue(thread, thread->currentPriority);

            ADD_TO_LIST(thread, gInfo.suspendedThreads);

            thread->status = PSP_THREAD_SUSPEND;
            break;
          }

          case PSP_THREAD_WAITING:
          case PSP_THREAD_SUSPEND:
          {
            thread->status = (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND);
            break;
          }
        }
      }
    }
  }

  gInfo.unk_734 = 1;

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//3570
int sceKernelWaitThreadEnd(SceUID thid, SceUInt *timeout)
{
  return _WaitThreadEnd(thid, timeout, 0);
}

//3578
int sceKernelWaitThreadEndCB(SceUID thid, SceUInt *timeout)
{
  return _WaitThreadEnd(thid, timeout, 1);
}

//3580
int _WaitThreadEnd(SceUID thid, SceUInt *timeout, int isCB)
{
  SET_K1_SRL16;
  
  if(IS_USER_MODE && IS_ADDR_KERNEL(timeout))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(thid == 0)
  {
    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  
  if(intr == 0 || gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  _CheckThreadKernelStack();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_THID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x4))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }
  
  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, uidThreadType); //a1
  if(a1 == gInfo.currentThread)
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_THID;
  }
  
  if(thread->status == PSP_THREAD_KILLED || thread->status == PSP_THREAD_STOPPED)
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;
    
    return thread->exitStatus;
  }

  THREADMAN_TRACE(isCB ? 0x35 : 0x34, 1, thid);

  SceKernelSysClock clk;
  if(timeout)
  {
    sub_0000D6D8(&clk, *timeout);
  }

  int ret;
  do
  {
    if(isCB && gInfo.callbackNotify != 0)
    {
      DispatchCallbacks(intr);
    }

    gInfo.isCallback = isCB;

    ret = AddThreadToWaitQueue(cb, 0x9, 0, &clk, timeout);
  } while(isCB && ret == SCE_KERNEL_ERROR_NOTIFY_CALLBACK);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//37f8
int sceKernelDelayThread(SceUInt delay)
{
  return _DelayThread(delay, 0);
}

//3800
int sceKernelDelayThreadCB(SceUInt delay)
{
  return _DelayThread(delay, 1);
}

//3808
int _DelayThread(SceUInt delay, int isCB)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr(); //s4

  if(intr == 0 || gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  _CheckThreadKernelStack();

  THREADMAN_TRACE(isCB ? 0x6D : 0x6C, 1, delay);

  SceKernelSysClock clk;
  sub_0000D6D8(&clk, delay);

  do
  {
    if(isCB && gInfo.callbackNotify != 0)
    {
      DispatchCallbacks(intr);
    }

    gInfo.currentThread->timeout = 0;
  
    int ret = sub_0000D994(&clk, sub_00003C3C, gInfo.currentThread);
    if(ret < 0)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return (ret & -(ret != SCE_KERNEL_ERROR_WAIT_TIMEOUT));
    }

    gInfo.currentThread->timeoutId = ret;

    sub_0000017C(&gInfo.delayedThreads, 2);

    _ReleaseWaitThread(1, 1);

  } while(isCB && gInfo.currentThread->callbackStatus == SCE_KERNEL_ERROR_NOTIFY_CALLBACK);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return gInfo.currentThread->callbackStatus;
}

//3a00
int sceKernelDelaySysClockThread(SceKernelSysClock *delay)
{
  return _DelaySysClockThread(delay, 0);
}

//3a08
int sceKernelDelaySysClockThreadCB(SceKernelSysClock *delay)
{
  return _DelaySysClockThread(delay, 1);
}

//3a10
int _DelaySysClockThread(SceKernelSysClock *delay, int isCB)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(IS_USER_MODE && IS_ADDR_KERNEL(delay))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr(); //s5
  if(intr == 0 || gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  _CheckThreadKernelStack();

  THREADMAN_TRACE(isCB ? 0x6F : 0x6E, 2, delay, isCB);

  sub_0000D730(&sp_0, delay);

  do
  {
    if(isCB && gInfo.currentThread->callbackNotify != 0)
    {
      DispatchCallbacks(intr);
    }

    gInfo.currentThread->timeout = 0;

    int ret = sub_0000D994(&sp_0, sub_00003C3C, gInfo.currentThread);
    if(ret < 0)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return (ret & -(ret != SCE_KERNEL_ERROR_WAIT_TIMEOUT));
    }

    gInfo.currentThread->timeoutId = ret;

    sub_0000017C(&gInfo.delayedThreads, 2);

    gInfo.isCallback = isCB;

    gInfo.nextThread = 0;

    _ReleaseWaitThread(1, 1);
  } while(isCB && gInfo.currentThread->callbackStatus == SCE_KERNEL_ERROR_NOTIFY_CALLBACK);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return gInfo.currentThread->callbackStatus;
}

//3c3c
void sub_00003C3C(ThreadInfo *thread)
{
  int intr = sceKernelCpuSuspendIntr();

  thread->timeoutId = 0;

  if(sub_0000022C(thread) > 0)
  {
    gInfo.nextThread = 0;
  }

  sceKernelCpuResumeIntr(intr);
}

