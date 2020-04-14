#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thempipe.h"
#include "util.h"

//6bf0
SceUID sceKernelCreateMsgPipe(const char *name, int part, int attr, int size, void *opt)
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

  if(attr & ~SCE_MPP_LEGAL_ATTR)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
  }

  int intr = sceKernelCpuSuspendIntr(); //s7

  SceKernelPartitionInfo pinfo;
  pinfo.size = sizeof(SceKernelPartitionInfo);
  int ret = sceKernelQueryMemoryPartitionInfo(part, &pinfo);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  if(IS_USER_MODE && (pinfo.attr & 0x3) != 0x3)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  SceUID partmem = 0;
  int addr = 0;
  if(size != 0)
  {
    partmem = sceKernelAllocPartitionMemory(part, "threadman mpipe", IS_SET(attr, 1 << 14) ? PSP_SMEM_High : PSP_SMEM_Low, size, 0);
    if(partmem <= 0)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_NO_MEMORY;
    }

    addr = sceKernelGetBlockHeadAddr(partmem);
  }

  uidControlBlock *cb;
  ret = sceKernelCreateUID(uidMsgPipeType, name, IS_USER_MODE ? 0xFF : (attr & 0xFF), &cb);
  if(ret != 0)
  {
    if(partmem > 0)
    {
      sceKernelFreePartitionMemory(partmem);
    }

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  qinfo->attr = attr;

  MsgPipeInfo *mpipeinfo = UID_INFO(MsgPipeInfo, cb, uidMsgPipeType);
  mpipeinfo->opt = opt;
  mpipeinfo->mode = 0;
  mpipeinfo->buf = addr;
  mpipeinfo->sendptr = addr;
  mpipeinfo->recvptr = addr;
  mpipeinfo->bufsize = size;
  mpipeinfo->used = 0;

  THREADMAN_TRACE(0x4F, 1, cb->UID);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return cb->UID;
}

//6e60
int sceKernelDeleteMsgPipe(SceUID uid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr(); //s2

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidMsgPipeType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MPPID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x10))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x50, 1, uid);

  ASSERT(sceKernelDeleteUID(uid) <= 0);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//6fb8
int sceKernelSendMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout)
{
  return _SendMsgPipe(uid, message, size, unk1, unk2, timeout, 0);
}

//6fc0
int sceKernelSendMsgPipeCB(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout)
{
  return _SendMsgPipe(uid, message, size, unk1, unk2, timeout, 1);
}

//6fc8
int _SendMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout, int isCB)
{
  int left, len, ret;
  ThreadInfo *thread;

  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(message) || IS_ADDR_KERNEL(unk2) || IS_ADDR_KERNEL(timeout)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(unk1 & 0xFFFFFFFE)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr(0; //sp_20
  if(intr == 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  if(gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  _CheckThreadKernelStack();

  uidControlBlock *cb; //a2/sp_10
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidMsgPipeType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MPPID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x2))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType); //s4
  MsgPipeInfo *mpipe = UID_INFO(MsgPipeInfo, cb, uidMsgPipeType); //s6

  if(mpipe->bufsize != 0 && mpipe->bufsize < size)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_SIZE;
  }

  THREADMAN_TRACE(isCB ? 0x52 : 0x51, 4, uid, message, size, unk1);

  SceKernelSysClock clk;
  if(timeout != 0)
  {
    sub_0000D6D8(&clk, *timeout);
  }

  //7210
  while(1 == 1)
  {
    if(isCB && gInfo.currentThread->callbackNotify)
    {
      DispatchCallbacks(intr);
    }

    //7228
    if((waitq->attr & 0x100) && wait->unk_4 == 0 && waitq->waitThread->currentPriority > gInfo.currentThread->currentPriority)
    {
      for(thread = waitq->waitThread; mpipe->mode == 2 && waitq->numWaitThreads > 0; thread = thread->next)
      {
        len = MIN(thread->cbArg2, left); //s1

        //73dc
        memcpy(thread->cbArg1, message, len);

        thread->cbArg1 += len;
        thread->cbArg2 -= len;

        message += len;
        left -= len;

        if(thread->cbArg4 != 0)
        {
          *thread->cbArg4 = thread->cbArg3 - thread->cbArg2;
        }

        //7420
        if(thread->cbArg5 == 1 || thread->cbArg2 == 0)
        {
          waitq->unk_4 = 0;

          sp_24 += sub_0000022C(s7);
        }
        else
        {
          waitq->unk_4 = 1;
        }
      }

      //7240
      if(left != 0)
      {
        len = MIN(mpipe->bufsize - mpipe->used, left);

        //7264
        if(unk1 != 1 || len <= 0)
        {
          if(unk1 >= 1 || len != left)
          {
            goto loc_000072a4;
            //->72a8
          }
        }

        //728c
        sub_00008890(mpipe, message, len);

        message += len;
        left -= len;
      }
    }

    //72a4
    //72a8
    if(unk2 != 0)
    {
      *unk2 = size - left;
    }

    //72bc
    if(unk1 != 1 || size != left)
    {
      if(unk1 > 0 || left < 1)
      {
        break;
        //->7378
      }
    }

    //72ec
    mpipe->mode = 1;

    gInfo.currentThread->cbArg1 = message;
    gInfo.currentThread->cbArg2 = left;
    gInfo.currentThread->cbArg3 = size;
    gInfo.currentThread->cbArg4 = unk2;
    gInfo.currentThread->cbArg5 = unk1;
    gInfo.currentThread->isCallback = isCB;

    waitq->mode = (size != left); //???

    ret = AddThreadToWaitQueue(cb, 0x8, waitq->attr & 0x100, &clk, timeout);

    if(waitq->numWaitThreads == 0)
    {
      mpipe->mode = 0;
    }

    if(!isCB || ret != SCE_KERNEL_ERROR_NOTIFY_CALLBACK)
    {
      break;
    }
  }

  //7378
  if(sp_24 != 0)
  {
    gInfo.nextThread = 0;

    _ReleaseWaitThread(0, intr);
  }

  //7384
  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//7568
int sceKernelTrySendMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2)
{
  ThreadInfo *thread;
  int ret, sp_10 = 0, orig_size = size;

  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(message) || IS_ADDR_KERNEL(unk2))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(unk1 & ~0x1)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MODE;
  }

  int isIntr = sceKernelIsIntrContext(); //sp_C

  int intr = sceKernelCpuSuspendIntr(); //sp_8

  uidControlBlock *cb; //sp_0/a2
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidMsgPipeType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MPPID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x2))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType); //s4
  MsgPipeInfo *mpipe = UID_INFO(MsgPipeInfo, cb, uidMsgPipeType); //s6

  if(mpipe->bufsize != 0 && mpipe->bufsize < size)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_SIZE;
  }

  THREADMAN_TRACE(0x53, 4, uid, message, size, unk1);

  if(mpipe->mode == 1)
  {
    if((waitq->attr & 0x100) || waitq->unk_4 == 0 || waitq->waitThread->currentPriority <= gInfo.currentThread->currentPriority)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_MPP_FULL;
    }
  }

  //76f0
  if(unk1 == 0)
  {
    int free = mpipe->bufsize - mpipe->used;

    if(mpipe->mode == 2)
    {
      //78f4
      LOOP_LIST(thread, waitq->waitThread)
      {
        free += thread->cbArg2;
      }
    }

    if(free < size)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_MPP_FULL;
    }
  }

  //7718
  for(thread = waitq->waitThread; mpipe->mode == 2 && waitq->numWaitThreads > 0; thread = thread->next)
  {
    len = MIN(mpipe->bufsize, size);

    //7854
    memcpy(thread->cbArg1, message, len);

    thread->cbArg1 += len;
    thread->cbArg2 -= len;

    message += len;
    size -= len;
    if(thread->cbArg4 != 0)
    {
      thread->cbArg3 -= (thread->cbArg2 - len);
    }

    //7898
    if(thread->cbArg5 == 1 || thread->cbArg2 == 0)
    {
      waitq->unk_4 = 0;

      sp_10 += sub_0000022C(thread);
    }
    else
    {
      waitq->unk_4 = 1;
    }
  }

  //7720
  if(size != 0)
  {
    len = MIN((mpipe->bufsize - mpipe->used), size); //s3

    if(unk1 != 1 || len <= 0)
    {
      if(unk1 == 0 && len == size)
      {
        sub_00008890(mpipe, message, len);

        size -= len;
      }
    }
  }

  //7784
  if(unk2 != 0)
  {
    *unk2 = orig_size - size;
  }

  //77a4
  if((unk1 == 1 && size == orig_size) || (unk1 < 1 && size > 0))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_MPP_FULL;
  }

  if(sp_10 != 0)
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

//79d0
int sceKernelReceiveMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout)
{
  return _ReceiveMsgPipe(uid, message, size, unk1, unk2, timeout, 0);
}

//79d8
int sceKernelReceiveMsgPipeCB(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout)
{
  return _ReceiveMsgPipe(uid, message, size, unk1, unk2, timeout, 1);
}

//79e0
int _ReceiveMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout, int isCB)
{
  int orig_size = size, len;

  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(message) || IS_ADDR_KERNEL(unk2) || IS_ADDR_KERNEL(timeout)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(unk1 & ~0x1)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MODE;
  }
  
  if(sceKernelIsIntrContext())
  {
    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr(); //sp_20
  if(intr == 0 || gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;
    
    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  _CheckThreadKernelStack();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidMsgPipeType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MPPID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x4))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }
  
  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType); //s6
  MsgPipeInfo *mpipe = UID_INFO(MsgPipeInfo, cb, uidMsgPipeType); //s4

  if(mpipe->bufsize != 0 && mpipe->bufsize < size)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_SIZE;
  }

  THREADMAN_TRACE(isCB ? 0x55 : 0x54, 4, uid, message, size, unk1);

  SceKernelSysClock clk;
  if(timeout)
  {
    sub_0000D6D8(&clk, *timeout);
  }

  do
  {
    if(isCB && gInfo.currentThread->callbackNotify != 0)
    {
      DispatchCallbacks(intr);
    }

    int sp_28 = 0;
    if(mpipe->mode != 2 || (waitq->attr & 0x1000) || waitq->unk_4 == 0 || waitq->waitThread->currentPriority > gInfo.currentThread->currentPriority)
    {
      if(mpipe->used != 0)
      {
        len = MIN(mpipe->used, size);

        sub_00008968(mpipe, message, len);

        message += len;
        size -= len;
      }

      if(mpipe->mode == 1 && waitq->numWaitThreads > 0)
      {
        for(thread = waitq->waitThread; mpipe->mode == 1 && waitq->numWaitThreads > 0; thread = thread->next)
        {
          if(size != 0)
          {
            len = MIN(thread->cbArg2, size);
          }

          memcpy(message, thread->cbArg1, len);

          message += len;
          size -= len;

          thread->cbArg1 += len;
          thread->cbArg2 -= len;

          len = MIN(mpipe->bufsize - mpipe->used, thread->cbArg2);

          if((thread->cbArg5 == 1 && len > 0) || (thread->cbArg5 == 0 && thread->cbArg2 == len))
          {
            sub_00008890(mpipe, thread->cbArg1, len);

            thread->cbArg2 -= len;
          }

          if(thread->cbArg4 != 0)
          {
            *thread->cbArg4 = thread->cbArg3 - thread->cbArg2;
          }

          if((thread->cbArg5 == 1 && a2) || a2 == 0)
          {
            waitq->unk_4 = 0;

            sp_28 += sub_0000022C(thread);
          }
          else
          {
            waitq->unk_4 = 1;

            break;
          }
        }
      }
    }

    //7c84
    if(unk2)
    {
      *unk2 = orig_size - size;
    }

    if((unk1 == 1 && size == orig_size) || (unk1 <= 0 && size > 0))
    {
      mpipe->mode = 2;

      gInfo.currentThread->cbArg1 = message;

      sp_28 = 0;
      gInfo.currentThread->cbArg2 = size;
      gInfo.currentThread->cbArg3 = orig_size;
      gInfo.currentThread->cbArg4 = unk2;
      gInfo.currentThread->cbArg5 = unk1;
      waitq->unk_4 = (size != orig_size);
      gInfo.currentThread->isCallback = isCB;
      ret = AddThreadToWaitQueue(cb, 0x8, IS_SET(waitq->attr, 1 << 12) * 0x100, &clk, timeout);
      if(waitq->numWaitThreads == 0)
      {
        mpipe->mode = 0;
      }
    }
  } while(isCB && ret == SCE_KERNEL_ERROR_NOTIFY_CALLBACK)

  //7d5c
  if(sp_28 != 0)
  {
    gInfo.nextThread = 0;

    _ReleaseWaitThread(0, intr);
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//7fd8
int sceKernelTryReceiveMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2)
{
  ThreadInfo *thread;
  int sp_14 = 0, orig_size = size;

  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(message) || IS_ADDR_KERNEL(unk2))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(unk1 & ~0x1)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MODE;
  }

  int isIntr = sceKernelIsIntrContext(); //sp_10;

  int intr = sceKernelCpuSuspendIntr(); //sp_C

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidMsgPipeType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MPPID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x4))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType); //s6
  MsgPipeInfo *mpipe = UID_INFO(MsgPipeInfo, cb, uidMsgPipeType); //s5

  if(mpipe->bufsize != 0 && mpipe->bufsize < size)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_SIZE;
  }

  THREADMAN_TRACE(0x56, 4, uid, message, size, unk1);

  if(mpipe->mode == 2)
  {
    if(!(waitq->attr & 0x1000) || waitq->unk_4 != 0 || waitq->waitThread->currentPriority <= gInfo.currentThread->currentPriority)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_MPP_EMPTY;
    }
  }

  //8168
  int used;
  if(unk1 == 0)
  {
    used = mpipe->used;

    if(mpipe->mode == 1)
    {
      //83c4
      LOOP_LIST(thread, waitq->waitThread)
      {
        used += thread->cbArg2;
      }
    }

    if(used < size)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_MPP_EMPTY;
    }
  }

  int len;
  if(used == 0)
  {
    len = MIN(used, size);

    sub_00008968(mpipe, message, len);
  }

  //81c4
  for(thread = waitq->waitThread; mpipe->mode == 1 && waitq->numWaitThreads > 0; thread = thread->next)
  {
    if(size != 0)
    {
      len = MIN(thread->cbArg2, size);

      memcpy(message, thread->cbArg1, len);

      thread->cbArg1 += len;
      thread->cbArg2 -= len;

      message += len;
      size -= len;
    }

    len = MIN(mpipe->bufsize - mpipe->used, thread->cbArg2);

    if((thread->cbArg5 == 1 && len > 0) || (thread->cbArg5 == 0 && len == thread->cbArg1))
    {
      //83a4
      sub_00008890(mpipe, thread->cbArg1, len);

      thread->cbArg2 -= len;
    }

    //8324/8
    if(thread->cbArg4 != 0)
    {
      *thread->cbArg4 = thread->cbArg3 - thread->cbArg2;
    }
    
    if((thread->cbArg5 == 1 && a2) || thread->cbArg2 == 0)
    {
      waitq->unk_4 = 0;

      sp_14 += sub_0000022C(thread);
    }
    else
    {
      waitq->unk_4 = 1;
    }
  }

  //81cc
  if(unk2 != 0)
  {
    *unk2 = orig_size - size;
  }

  if((unk1 == 1 && size == orig_size) || (unk1 < 1 && size > 0))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_MPP_EMPTY;
  }

  if(sp_14 != 0)
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

//84a4
int sceKernelCancelMsgPipe(SceUID uid, int *psend, int *precv)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr(); //s5

  sceKernelIsIntrContext();

  if(IS_USER_MODE && (IS_ADDR_KERNEL(psend) || IS_ADDR_KERNEL(precv))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidMsgPipeType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MPPID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x8))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  MsgPipeInfo *mpipe = UID_INFO(MsgPipeInfo, mpipe, uidMsgPipeType); //s0

  THREADMAN_TRACE(0x57, 3, uid, psend, precv);

  int ret = sceKernelCallUIDObjFunction(cb, UIDFUNC_CANCEL);

  if(psend != 0)
  {
    *psend = (mpipe->mode == 1) ? ret : 0;
  }

  if(precv != 0)
  {
    *precv = (mpipe->mode == 2) ? ret : 0;
  }

  mpipe->mode = 0;
  mpipe->sendptr = mpipe->buf;
  mpipe->recvptr = mpipe->buf;
  mpipe->used = 0;

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//8680
int sceKernelReferMsgPipeStatus(SceUID uid, SceKernelMppInfo *info)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr(); //s7

  if(IS_USER_MODE && IS_ADDR_KERNEL(info))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidMsgPipeType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MPPID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x1))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x58, 2, uid, info);

  SceKernelMppInfo *buf = (SceKernelMppInfo *)var_164a8;

  memset(buf, 0, sizeof(SceKernelMppInfo));

  buf->size = sizeof(SceKernelMppInfo);

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  MsgPipeInfo *mpinfo = UID_INFO(MsgPipeInfo, cb, uidMsgPipeType);

  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }

  buf->attr = qinfo->attr;
  if(mpinfo->mode == 0x1)
  {
    buf->numSendWaitThreads = qinfo->numWaitThreads;
  }
  else if(mpinfo->mode == 0x2)
  {
    buf->numReceiveWaitThreads = qinfo->numWaitThreads;
  }

  buf->bufSize = mpinfo->bufsize;
  buf->freeSize = mpinfo->bufsize - mpinfo->used;

  memcpy(info, buf, MIN(buf->size, info->size));

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//8890
void sub_00008890(MsgPipeInfo *mpipe, void *message, int size)
{
  if(mpipe->sendptr >= mpipe->recvptr)
  {
    int len = MIN(size, mpipe->bufsize - (mpipe->sendptr - mpipe->buf);

    memcpy(mpipe->sendptr, message, len);

    size -= len;
    message += len;

    mpipe->used += len;

    if(mpipe->sendptr + len < (mpipe->buf + mpipe->bufsize))
    {
      mpipe->sendptr += len;
    }
    else
    {
      mpipe->sendptr = mpipe->buf;
    }
  }

  memcpy(mpipe->sendptr, message, size);

  mpipe->used += size;
  mpipe->sendptr += size;
}

//8968
void sub_00008968(MsgPipeInfo *mpipe, void *message, int size)
{
  if(mpipe->recvptr >= mpipe->sendptr)
  {
    int len = MIN(size, mpipe->bufsize - (mpipe->recvptr - mpipe->buf));

    memcpy(message, mpipe->recvptr, len);

    size -= len;
    message += len;

    mpipe->used -= len;

    if((mpipe->recvptr + len) < (mpipe->buf + mpipe->bufsize))
    {
      mpipe->recvptr += len;
    }
    else
    {
      mpipe->recvptr = mpipe->buf;
    }
  }

  memcpy(message, mpipe->recvptr, size);

  mpipe->used -= size;
  mpipe->recvptr += size;
}

UidFunction MsgPipeUidFuncTable[] = //15D30
{
  { 0x5BD49380, sub_00008cb0 },
  { 0x85FE2B4C, sub_00008b88 },
  { UIDFUNC_CREATE, MsgPipeCreateUID },
  { UIDFUNC_DELETE, MsgPipeDeleteUID },
  { 0, 0 }
};

//8a44
int initMsgPipe()
{
  int ret;

  ret = sceKernelCreateUIDtypeInherit("WaitQ", "MsgPipe", sizeof(MsgPipeInfo), MsgPipeUidFuncTable, 0, &uidMsgPipeType);

  ASSERT(ret <= 0);

  return 0;
}

//8ad0
int MsgPipeCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);
  
  return cb->UID;
}

//8af8
int MsgPipeDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  //surely the buffer should be freed here, if one was allocated?

  return 0;
}

//8b88
int sub_00008B88(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int readyThreads = 0;

  WaitQInfo *waitq = UID_INFO(WaitQInfo, cb, uidWaitQType); //a2
  MsgPipeInfo *mpipe = UID_INFO(MsgPipeInfo, cb, uidMsgPipeType); //s2

  if(waitq->numWaitThreads <= 0 || mpipe->mode == 1)
  {
    return 0;
  }

  ThreadInfo *thread;
  LOOP_LIST(thread, waitq->waitThread)
  {
    if(!(thread->cbArg5 == 1 && mpipe->bufsize - mpipe->used > 0) && (thread->cbArg5 != 0 || thread->cbArg2 != mpipe->bufsize - mpipe->used))
    {
      break;
    }

    sub_00008890(mpipe, thread->cbArg1, mpipe->bufsize - mpipe->used);

    thread->cbArg2 -= mpipe->bufsize - mpipe->used;
    if(thread->cbArg4 != 0)
    {
      *thread->cbArg4 = thread->cbArg3 - mpipe->bufsize - mpipe->used;
    }

    readyThreads += sub_0000022C(thread);
  }

  return s3;
}
