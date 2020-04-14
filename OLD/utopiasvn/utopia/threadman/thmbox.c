#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thmbox.h"

extern ThreadmanInfo gInfo;

//5d18
int sceKernelCreateMbx(const char *name, unsigned int attr, SceKernelMbxOptParam *opt)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(name) || IS_ADDR_KERNEL(opt)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(attr & ~SCE_MBX_LEGAL_ATTR)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  int ret = sceKernelCreateUID(uidMboxType, name, IS_USER_MODE ? 0xFF : (attr & 0xFF), &cb);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  WaitQInfo *qinfo = t7 = UID_INFO(WaitQInfo, cb, uidWaitQType);
  qinfo->attr = attr;

  MboxInfo *mbinfo = s0 = UID_INFO(MboxInfo, cb, uidMboxType);
  mbinfo->option = opt;

  THREADMAN_TRACE(0x47, 1, mbinfo);
  
  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//5e98
int sceKernelDeleteMbx(SceUID mbxid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(mbxid, uidMboxType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MBXID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x10))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x48, 1, mbxid);

  ASSERT(sceKernelDeleteUID(mbxid) <= 0);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//5ff0
int sceKernelSendMbx(SceUID mbxid, void *message)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  int isIntr = sceKernelIsIntrContext();

  if(IS_USER_MODE && IS_ADDR_KERNEL(message))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(mbxid, uidMboxType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_MBXID;
  }

  if(IS_USER_MODE && !(cb->attr & 0x2))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x49, 2, mbxid, message);

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);

  MboxInfo *mbxinfo = UID_INFO(MboxInfo, cb, uidMboxType);

  if(qinfo->numWaitThreads > 0)
  {
    if(qinfo->waitThread->unk_24 != 0)
    {
      qinfo->waitThread->unk_24->unk_0 = message;
    }

    sub_00000480(cb, 0, isIntr, intr);
  }
  else
  {
    if(sub_00006ADC(mbxinfo, qinfo, message) != 0)
    {
      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//61a4
int sceKernelReceiveMbx(SceUID mbxid, void **pmessage, unsigned int *timeout)
{
  return _ReceiveMbx(mbxid, pmessage, timeout, 0);
}

//61ac
int sceKernelReceiveMbxCB(SceUID mbxid, void **pmessage, unsigned int *timeout)
{
  return _ReceiveMbx(mbxid, pmessage, timeout, 1);
}

//61b4
int _ReceiveMbx(SceUID mbxid, void **pmessage, unsigned int *timeout, int isCB)
{
}

//64e4
int sceKernelPollMbx(SceUID mbxid, void **pmessage)
{
}

//6714
int sceKernelCancelReceiveMbx(SceUID mbxid, int *pnum)
{
}

//6868
int sceKernelReferMbxStatus(SceUID mbxid, SceKernelMbxInfo *info)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(IS_USER_MODE && IS_ADDR_KERNEL(info))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(mbxid, uidMboxType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;
    
    return SCE_KERNEL_ERROR_UNKNOWN_MBXID;
  }
  
  if(IS_USER_MODE && !(cb->attr & 0x1))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x4E, 2, mbxid, info);

  SceKernelMbxInfo *buf = (SceKernelMbxInfo *)var_164a8;

  memset(buf, 0, sizeof(SceKernelMbxInfo));

  buf->size = sizeof(SceKernelMbxInfo);

  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  buf->attr = qinfo->unk_0;
  buf->numWaitThreads = qinfo->unk_8;

  MboxInfo *mbxinfo = UID_INFO(MboxInfo, cb, uidMboxType);
  buf->numMessages = mbxinfo->unk_4;
  if(mbxinfo->unk_8)
  {
    buf->firstMessage = mbxinfo->unk_8->unk_0;
  }

  memcpy(info, buf, MIN(buf->size, info->size));

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

UidFunction MboxUidFuncTable[] = //15d18
{
  { UIDFUNC_CREATE, MboxCreateUID },
  { UIDFUNC_DELETE, MboxDeleteUID },
  { 0, 0 }
};

//6a50
int initMbx()
{
  int ret;

  ret = sceKernelCreateUIDtypeInherit("WaitQ", var_15720 /* wrong? */, sizeof(MboxInfo), MboxUidFuncTable, 0, &uidMboxType);

  ASSERT(ret <= 0);

  return 0;
}

//6adc
sub_00006ADC(...)
{
  a3 = 0x4(a0);
  t1 = 0x8(a0);
  v1 = a3 + 0x1;
  0x4(a0) = v1;
  if(t1 != 0)
  {
    t0 = 0x0(a1);
    v1 = t0 & 0x400;
    a3 = t1;
    if(v1 == 0)
    {
      a1 = 0x0(t1);
      if(IS_USER_MODE && IS_ADDR_KERNEL(0x0(t1)))
      {
        return -1;
      }
      
      0x0(a2) = a1;
      v0 = 0x8(a0);
      0x0(v0) = a2;
      0x8(a0) = a2;
      
      return 0;
    }

    //6b44
    while(1 == 1)
    {
      a1 = 0x0(a3);
      t0 = a1;
      if(IS_USER_MODE && IS_ADDR_KERNEL(0x0(a3)))
      {
        return -1;
      }
  
      t7 = 0x4(t0); //b
      t6 = 0x4(a2); //b
      t5 = t6 < t7;
      if(t5 != 0)
      {
        break;
      }
  
      a3 = t0;
      if(t0 == t1)
      {
        0x8(a0) = a2;
        a1 = 0x0(t0);
        break;
      }
    }
    
    if(IS_USER_MODE && IS_ADDR_KERNEL(a1))
    {
      return -1;
    }
    
    0x0(a2) = a1;
    0x0(a3) = a2;

    return 0;
  }
  else
  {
    0x8(a0) = a2;
    0x0(a2) = a2;

    return 0;
  }
}

//6ba8
int MboxCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//6bd0
int MboxDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return 0;
}

