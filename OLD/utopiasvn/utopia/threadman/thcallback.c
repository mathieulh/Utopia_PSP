#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thcallback.h"

extern ThreadmanInfo gInfo;

//db4
int sceKernelRegisterThreadEventHandler(const char *name, SceUID threadid, int mask, SceKernelThreadEventHandler handler, void *common)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(name) || IS_ADDR_KERNEL(handler) || IS_ADDR_KERNEL(common)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext() || gInfo.currentThread->eventMask != 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(mask & ~0xF)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MASK;
  }

  int intr = sceKernelCpuSuspendIntr();

  ThreadInfo *thinfo;
  uidControlBlock *cb;
  if(threadid == 0xFFFFFFF8 || threadid == 0xFFFFFFF0 || threadid == 0xFFFFFFFF)
  {
    if(IS_USER_MODE && threadid != 0xFFFFFFF0)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }
  }
  else
  {
    if(threadid == 0)
    {
      if(mask != THREAD_EXIT)
      {
        sceKernelCpuResumeIntr(intr);

        RESET_K1;

        return SCE_KERNEL_ERROR_OUT_OF_RANGE;
      }

      threadid = gInfo.currentThread->UID;
    }

    if(sceKernelGetUIDcontrolBlockWithType(threadid, uidThreadType, &cb) != 0)
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

    thinfo = UID_INFO(ThreadInfo, cb, uidThreadType);
    if(!(gInfo.currentThread->attr & 0x80000000) && (thinfo->attr & 0x80000000))
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_ILLEGAL_PERM;
    }
  }

  //f30
  uidControlBlock *uid;
  int ret = sceKernelCreateUID(uidThreadEventHandlerType, name, IS_USER_MODE ? 0 : 0xFF, &uid);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  ThreadEventHandlerInfo *hinfo = UID_INFO(ThreadEventHandlerInfo, uid, uidThreadEventHandlerType);
  hinfo->thread = thinfo;
  hinfo->UID = uid->UID;
  hinfo->mask = mask;
  hinfo->user_mode = IS_USER_MODE;
  hinfo->handler = handler;
  hinfo->common = common;
  SceModule *mod = sceKernelFindModuleByAddress(handler);
  hinfo->gpreg = mod ? mod->unk_68 : gp;

  //ffc
  ADD_TO_LIST(hinfo, gInfo.eventHandlers);
  
  THREADMAN_TRACE(0x2B, 5, threadid, mask, handler, common, uid->UID);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return uid->UID;
}

//10f8
int sceKernelReleaseThreadEventHandler(int uid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext() || gInfo.currentThread->eventMask != 0)
  {
    RESET_K1;
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(intr != 0)
  {
    if(gInfo.dispatch_thread_suspended != 0)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
    }
  }

  //11ac
  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidThreadEventHandlerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_TEID;
  }

  if(!CAN_DELETE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x2C, 1, uid);

  int ret = sceKernelDeleteUID(uid);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//1260 - call event handlers
int DispatchThreadEventHandlers(int mask, ThreadInfo *thread, int intr)
{
  int ret;
  ThreadEventHandlerInfo *handler;
  int sp_4 = 0;

  gInfo.currentThread->eventMask |= mask;

  LOOP_LIST(handler, gInfo.eventHandlers)
  {
    if(!(handler->mask & mask))
    {
      continue;
    }

    if(handler->unk_10 != 0)
    {
      continue;
    }

    if(!THREAD_IS_USER_MODE(gInfo.currentThread) && THREAD_IS_USER_MODE(thread))
    {
      continue;
    }

    if((int)handler->thread == -1 || (thread->attr < 0 && gInfo.currentThread->attr < 0 && (int)handler->thread == -0x10) ||
            (int)handler->thread == -0x8 || handler->thread == thread)
    {
      uidControlBlock *cb = UIDCB_FROM_INFO(handler, gInfo.uidThreadEventHandlerType);
      gInfo.currentThread->eventHandlerCB = cb;
      handler->unk_C++;

      ret = sub_00002574(handler->user_mode, handler->handler, mask, thread->UID, handler->common, 0, handler->gpreg);
      if(ret != 0)
      {
        handler->unk_10 = 1;
      }

      gInfo.currentThread->eventHandlerCB = 0;
      handler->unk_C--;

      if(handler->unk_10 != 0 && handler->unk_C == 0)
      {
        REMOVE_FROM_LIST(handler);

        sp_4 += sub_000004C0(cb, 0);
      }
    }
  }

  if(sp_4 != 0)
  {
    gInfo.nextThread = 0;

    _ReleaseWaitThread(0, intr);
  }

  gInfo.currentThread->eventMask &= ~mask;

  return SCE_KERNEL_ERROR_OK;
}

//1470
int sceKernelReferThreadEventHandlerStatus(SceUID uid, SceKernelThreadEventHandlerInfo *info)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(IS_USER_MODE && IS_ADDR_KERNEL(ehinfo))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidThreadEventHandlerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_TEID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x2D, 2, uid, info);

  ThreadEventHandlerInfo *tehinfo = UID_INFO(ThreadEventHandlerInfo, cb, uidThreadEventHandlerType);

  SceKernelThreadEventHandlerInfo *buf = (SceKernelThreadEventHandlerInfo *)var_164a8;

  memset(info, 0, sizeof(SceKernelThreadEventHandlerInfo));

  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }

  if(tehinfo->thread == -0x8)
  {
    buf->threadId = (SceUID)tehinfo->thread;
  }
  else if(tehinfo->thread == ((int)tehinfo->thread >= -0x7) ? -0x1 : -0x10)
  {
    buf->threadId = (SceUID)tehinfo->thread;
  }
  else
  {
    buf->threadId = tehinfo->thread->UID;
  }

  buf->common = tehinfo->common;
  buf->mask = tehinfo->mask;
  buf->handler = tehinfo->handler;

  memcpy(info, tehinfo, MIN(buf->size, info->size));

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//1678
int sceKernelCreateCallback(const char *name, SceKernelCallbackFunction func, void *arg)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(func))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(!IS_USER_MODE && !IS_ADDR_KERNEL(func))
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

  uidControlBlock *uid;
  int ret = sceKernelCreateUID(gInfo.uidCallbackType, name, IS_USER_MODE ? 0 : 0xFF, &uid);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  CallbackInfo *cbinfo = UID_INFO(CallbackInfo, uid, gInfo.uidCallbackType);
  cbinfo->uid = uid->UID;
  cbinfo->thread = gInfo.currentThread;
  cbinfo->notifyCount = 0;
  cbinfo->notifyArg = 0;
  cbinfo->user_mode = IS_USER_MODE;
  cbinfo->func = func;
  cbinfo->arg = arg;

  SceModule *mod = sceKernelFindModuleByAddress(func);
  cbinfo->gpreg = (mod == 0) ? gp : mod->unk_68;

  ADD_TO_LIST(cbinfo, gInfo.currentThread->callbacks);

  THREADMAN_TRACE(0x82, 3, func, arg, uid->UID);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return uid->UID;
}

//1858
int sceKernelDeleteCallback(SceUID uid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext() || gInfo.currentThread->callbackCB != 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(intr != 0 && gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidCallbackType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_CBID;
  }

  if(!CAN_DELETE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x83, 1, uid);

  int ret = sceKernelDeleteUID(cb);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//19b0
int sceKernelNotifyCallback(SceUID uid, int arg2)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  int is_intr_ctxt = sceKernelIsIntrContext();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidCallbackType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_CBID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  CallbackInfo *callback = UID_INFO(CallbackInfo, cb, gInfo.uidCallbackType);
  
  THREADMAN_TRACE(0x84, 2, uid, arg2);

  callback->thread->callbackNotify = 0x1;
  callback->notifyArg = arg2;
  callback->notifyCount++;
  if(callback->thread->isCallback != 0)
  {
    if(callback->thread->status == PSP_THREAD_WAITING || callback->thread->status == (PSP_THREAD_WAITING | PSP_THREAD_SUSPEND))
    {
      int s0 = sub_0000022C(callback->thread);

      callback->thread->callbackStatus = SCE_KERNEL_ERROR_NOTIFY_CALLBACK;
      if(callback->thread->waitType != 0)
      {
        s0 += sub_000005F4(callback->thread->waitType);
      }

      if(s0 != 0)
      {
        gInfo.nextThread = 0;
        if(!is_intr_ctxt)
        {
          _ReleaseWaitThread(0, intr);
        }
      }
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//1b7c
int sceKernelCancelCallback(SceUID uid)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidCallbackType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_CBID;
  }

  if(!CAN_CANCEL_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  CallbackInfo *callback = UID_INFO(CallbackInfo, cb, gInfo.uidCallbackType);
  
  THREADMAN_TRACE(0x85, 1, uid);

  callback->notifyArg = 0;
  callback->notifyCount = 0;

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//1c90
int sceKernelGetCallbackCount(SceUID uid)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidCallbackType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_CBID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  CallbackInfo *callback = UID_INFO(CallbackInfo, cb, gInfo.uidCallbackType);

  int ret = callback->notifyCount;

  THREADMAN_TRACE(0x86, 2, cb, ret);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//1da0
int sceKernelCheckCallback()
{
  int ret = 0;

  SET_K1_SRL16;

  if(sceKernelIsIntrContext() || gInfo.currentThread->callbackCB != 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(gInfo.currentThread->callbackNotify != 0)
  {
    ret = (DispatchCallbacks(intr) > 0);
  }

  THREADMAN_TRACE(0x87, 1, ret);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//1e8c - call callbacks
int DispatchCallbacks(int intr)
{
  gInfo.currentThread->callbackNotify = 0;

  int fp = 0, s6 = 0;
  CallbackInfo *callback, *next_callback;
  uidControlBlock *cb;
  LOOP_LIST_SAFE(callback, next_callback, gInfo.currentThread->callbacks)
  {
    cb = UIDCB_FROM_INFO(callback, gInfo.uidCallbackType);
    if(callback->notifyCount != 0 && callback->unk_10 == 0)
    {
      int notifyCount = callback->notifyCount;
      callback->notifyCount = 0;

      void *notifyArg = callback->notifyArg;
      callback->notifyArg = 0;

      s6++;

      callback->unk_C++;

      gInfo.currentThread->callbackCB = cb;

      ret = sub_00002574(callback->user_mode, callback->func, notifyCount, notifyArg, callback->arg, 0, callback->gpreg);
      if(ret != 0)
      {
        callback->unk_10 = 1;
      }

      gInfo.currentThread->callbackCB = 0;

      callback->unk_C--;

      if(callback->unk_10 != 0 && callback->unk_C == 0)
      {
        REMOVE_FROM_LIST_SAFE(callback, next_callback);

        fp += sub_000004C0(cb, 0);
      }
    }
  }

  if(fp != 0)
  {
    gInfo.nextThread = 0;

    _ReleaseWaitThread(0, intr);
  }

  return s6;
}

//1ff4
int sceKernelReferCallbackStatus(SceUID uid, SceKernelCallbackInfo *status)
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
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidCallbackType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_CBID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x88, 2, uid, status);

  CallbackInfo *cbinfo = UID_INFO(CallbackInfo, cb, gInfo.uidCallbackType);

  SceKernelCallbackInfo *buf = (SceKernelCallbackInfo *)var_164a8;

  memset(buf, 0, sizeof(SceKernelCallbackInfo));

  buf->size = sizeof(SceKernelCallbackInfo);

  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }

  buf->threadId =cbinfo->thread->UID;
  buf->callback = cbinfo->func;
  buf->notifyCount = cbinfo->notifyCount;
  buf->common = cbinfo->arg;
  buf->notifyArg = cbinfo->notifyArg;

  memcpy(status, buf, MIN(buf->size, status->size));

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//21d4
int _sceKernelReturnFromCallback()
{
  int intr = sceKernelCpuSuspendIntr();

  if(gInfo.currentThread->callbackContext == 0)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  gInfo.currentThread->callbackStatus = gInfo.currentThread->scContext->unk[1];

  ReturnToThread(gInfo.currentThread->callbackContext);

  return SCE_KERNEL_ERROR_OK;
}

//2228
void sub_00002228(ThreadInfo *thread)
{
  if(thread->eventHandlerCB == 0)
  {
    return;
  }

  ThreadEventHandlerInfo *ehinfo = UID_INFO(ThreadEventHandlerInfo, thread->eventHandlerCB, uidThreadEventHandlerType);

  ehinfo->unk_C--;

  if(ehinfo->unk_10 != 0)
  {
    if(ehinfo->unk_C == 0)
    {
      REMOVE_FROM_LIST(ehinfo);

      sub_000004C0(thread->eventHandlerCB, 0);
    }
  }

  thread->eventHandlerCB = 0;
  thread->eventMask = 0;
}

//22b4
int sub_000022B4(int mask, ThreadInfo *thread)
{
  ThreadEventHandlerInfo handler;
  LOOP_LIST(handler, gInfo.eventHandlers)
  {
    if(!(handler->mask & mask))
    {
      continue;
    }

    if(handler->unk_10 != 0)
    {
      continue;
    }

    if(!(gInfo.currentThread->attr & 0x80000000) && (thread->attr & 0x80000000))
    {
      continue;
    }

    if((int)handler->thread == -1 || (int)handler->thread == -8 ||
          ((thread->attr & 0x80000000) && (gInfo.currentThread->attr & 0x80000000) && (int)handler->thread == -0x10))
    {
      return 1;
    }
  }

  return 0;
}

//237c
void sub_0000237C(ThreadInfo *thread)
{
  CallbackInfo *callback, *next_callback;

  LOOP_LIST_SAFE(callback, next_callback, thread->callbacks)
  {
    callback->unk_C = 0;

    sceKernelDeleteUID(callback->UID);
  }

  thread->callbackNotify = 0;
}

//23d4
void sub_000023D4(ThreadInfo *thread)
{
  if(thread->callbackCB == 0)
  {
    return;
  }

  CallbackInfo *cbinfo = UID_INFO(CallbackInfo, thread->callbackCB, gInfo.uidCallbackType);

  cbinfo->unk_C--;
  if(cbinfo->unk_10 != 0)
  {
    if(cbinfo->unk_C == 0)
    {
      REMOVE_FROM_LIST(cbinfo);

      sub_000004C0(thread->callbackCB, 0);
    }
  }

  thread->callbackCB = 0;
}

UidFunction ThreadEventHandlerUidFuncTable[] = //var_15CB0
{
  { UIDFUNC_CREATE, ThreadEventHandlerCreateUID },
  { UIDFUNC_DELETE, ThreadEventHandlerDeleteUID },
  { 0, 0 }
};

UidFunction CallbackUidFuncTable[] = //var_15CC8
{
  { UIDFUNC_CREATE, CallbackCreateUID },
  { UIDFUNC_DELETE, CallbackDeleteUID },
  { 0, 0 }
};

//245c
int initCallback()
{
  int ret;

  ASSERT(sceKernelCreateUIDtypeInherit("WaitQ", "ThreadEventHandler", sizeof(ThreadEventHandlerInfo),
          ThreadEventHandlerUidFuncTable, 0, &uidThreadEventHandlerType) <= 0);

  ASSERT(sceKernelCreateUIDtypeInherit("WaitQ", "Callback", sizeof(CallbackInfo), CallbackUidFuncTable, 0, &gInfo.uidCallbackType) <= 0);

  return SCE_KERNEL_ERROR_OK;
}

//2574
int sub_00002574(int user_mode, int EPC, int a0, int a1, int a2, int a3, int gp)
{
  struct SceThreadContext context;

  int tmp1 = gInfo.currentThread->kStackPointer;
  int tmp2 = gInfo.currentThread->scContext;
  int tmp3 = gInfo.currentThread->callbackContext;

  _CheckThreadKernelStack();

  if(SaveThreadContext(&context) != 0)
  {
    gInfo.currentThread->kStackPointer = tmp1;
    gInfo.currentThread->scContext = tmp2;
    gInfo.currentThread->callbackContext = tmp3;

    _CTC0(tmp1, COP0_CR_GPR_SP_USER);

    return gInfo.currentThread->callbackStatus;
  }

  gInfo.currentThread->callbackContext = &context;

  void *stack;
  if(!user_mode)
  {
    stack = context.gpr[GPREG_SP - 1];
  }
  else
  {
    if(gInfo.currentThread->scContext == 0)
    {
      stack = gInfo.currentThread->stackPointer;
    }
    else
    {
      stack = gInfo.currentThread->scContext->sp;
    }

    //265c
    stack -= 0x40;

    ((u32 *)stack)[0] = MAKE_SYSCALL(gInfo.unk_69C);
    ((u32 *)stack)[1] = 0;
    ((u32 *)stack)[2] = 0;
    ((u32 *)stack)[3] = 0;
    _CACHE(0x1B, stack);
    _CACHE(0x8, stack);
  }

  //clear the thread's context
  ResetThreadContext(gInfo.currentThread->threadContext);

  gInfo.currentThread->threadContext->type = 0x4;
  gInfo.currentThread->threadContext->gpr[GPREG_A0 - 1] = a0;
  gInfo.currentThread->threadContext->gpr[GPREG_A1 - 1] = a1;
  gInfo.currentThread->threadContext->gpr[GPREG_A2 - 1] = a2;
  gInfo.currentThread->threadContext->gpr[GPREG_A3 - 1] = a3;
  gInfo.currentThread->threadContext->gpr[GPREG_GP - 1] = gp;
  gInfo.currentThread->threadContext->gpr[GPREG_SP - 1] = stack;
  gInfo.currentThread->threadContext->gpr[GPREG_FP - 1] = stack;

  if(!user_mode)
  {
    gInfo.currentThread->threadContext->gpr[GPREG_RA - 1] = sub_000029B8;
  }
  else
  {
    gInfo.currentThread->threadContext->gpr[GPREG_RA - 1] = stack;
  }

  gInfo.currentThread->threadContext->SR = 0x20000003;
  if(user_mode)
  {
    gInfo.currentThread->threadContext->SR = 0x20000013;
  }

  gInfo.currentThread->threadContext->EPC = EPC;

  if(!user_mode)
  {
    gInfo.currentThread->kStackPointer = 0;
  }
  else
  {
    gInfo.currentThread->kStackPointer = gInfo.currentThread->callbackContext->gpr[GPREG_SP - 1];
  }

  _CTC0(gInfo.currentThread->kStackPointer, COP0_CR_GPR_SP_USER);

  ReturnToThread(gInfo.currentThread->threadContext);

  return SCE_KERNEL_ERROR_OK;
}

//2738
int ThreadEventHandlerCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//2760
int ThreadEventHandlerDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int ret;
  int intr = sceKernelCpuSuspendIntr();

  ThreadEventHandlerInfo *thinfo = UID_INFO(ThreadEventHandlerInfo, cb, type);
  if(thinfo->unk_10 != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_UNKNOWN_CBID;
  }

  thinfo->unk_10 = 1;

  while(thinfo->unk_C > 0)
  {
    _CheckThreadKernelStack();

    ret = AddThreadToWaitQueue(cb, 0xA, 0, 0, 0);
    if(ret != 0)
    {
      sceKernelCpuResumeIntr(intr);

      return ret;
    }
  }

  REMOVE_FROM_LIST(thinfo);

  sceKernelCpuResumeIntr(intr);

  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return SCE_KERNEL_ERROR_OK;
}

//2878
int CallbackCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//28a0
int CallbackDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int ret;
  int intr = sceKernelCpuSuspendIntr();

  CallbackInfo *cbinfo = UID_INFO(CallbackInfo, cb, type);
  if(cbinfo->unk_10 != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_UNKNOWN_CBID;
  }

  cbinfo->unk_10 = 1;

  while(cbinfo->unk_C > 0)
  {
    _CheckThreadKernelStack();

    ret = AddThreadToWaitQueue(cb, 0xB, 0, 0, 0);
    if(ret != 0)
    {
      sceKernelCpuResumeIntr(intr);

      return ret;
    }
  }

  REMOVE_FROM_LIST(cbinfo);

  sceKernelCpuResumeIntr(intr);

  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return SCE_KERNEL_ERROR_OK;
}

//29b8
int sub_000029B8()
{
  int intr = sceKernelCpuSuspendIntr();

  if(gInfo.currentThread->callbackContext == 0)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  gInfo.currentThread->callbackStatus = v0; //not return from sceKernelCpuSuspendIntr(), but the value of v0 passed in to the function!?

  ReturnToThread(gInfo.currentThread->callbackContext);

  return SCE_KERNEL_ERROR_OK;
}
