#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thvpl.h"

//8d00
SceUID sceKernelCreateVpl(const char *name, int part, int attr, unsigned int size, struct SceKernelVplOptParam *opt)
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

  if(attr & ~0x41FF)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
  }

  if(size == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MEMSIZE;
  }

  int intr = sceKernelCpuSuspendIntr();

  PspSysmemPartitionInfo pinfo;
  pinfo.size = sizeof(PspSysmemPartitionInfo);
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

  uidControlBlock *cb;
  ret = sceKernelCreateUID(uidVplType, name, IS_USER_MODE ? 0xFF : (attr & 0xFF), &cb);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return ret;
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);

  VplInfo *vinfo = UID_INFO(VplInfo, cb, uidVplType);

  vinfo->heapAddr = sceKernelCreateHeap(part, size, IS_SET(attr, SCE_VPL_ATTR_UNK4000) ? SCE_SYSMEM_HEAP_ATTR_UNK2 : 0, name);
  if(vinfo->heapAddr <= 0)
  {
    ASSERT(sceKernelDeleteUID(cb->UID) <= 0);

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  vinfo->heapSize = sceKernelHeapTotalFreeSize(ret);
  vinfo->opt = opt;

  qinfo->attr = attr;

  THREADMAN_TRACE(0x59, 1, name);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return cb->UID;
}

//8fa0
int sceKernelDeleteVpl(SceUID uid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr(); //s2

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;
    
    return SCE_KERNEL_ERROR_UNKNOWN_VPLID;
  }
  
  if(!CAN_DELETE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x5A, 1, uid);

  ASSERT(sceKernelDeleteUID(uid) <= 0);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//90f8
int sceKernelAllocateVpl(SceUID uid, unsigned int size, void **data, unsigned int *timeout)
{
  return _AllocateVpl(uid, size, data, timeout, 0);
}

//9100
int sceKernelAllocateVplCB(SceUID uid, unsigned int size, void **data, unsigned int *timeout)
{
  return _AllocateVpl(uid, size, data, timeout, 1);
}

//9108
int _AllocateVpl(SceUID uid, unsigned int size, void **data, unsigned int *timeout, int isCB)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(data) || IS_ADDR_KERNEL(timeout))
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
  if(intr == 0 || gInfo.dispatch_thread_suspended != 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;

    return SCE_KERNEL_ERROR_CAN_NOT_WAIT;
  }

  _CheckThreadKernelStack();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VPLID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  VplInfo *vinfo = UID_INFO(VplInfo, cb, uidVplType);

  if(size == 0 || size > vinfo->heapSize)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MEMSIZE;
  }

  SceKernelSysClock clk;
  if(timeout)
  {
    sub_0000D6D8(&clk, timeout);
  }

  do
  {
    if(isCB && gInfo.currentThread->unk_58 != 0)
    {
      DispatchCallbacks(intr);
    }

    int mem = sceKernelAllocHeapMemory(vinfo->heapAddr, size);

    THREADMAN_TRACE(isCB ? 0x5C : 0x5B, 3, uid, size, mem);

    if(mem != 0)
    {
      ret = 0;
      *data = mem;
      break;
    }

    gInfo.currentThread->unk_28 = size;
    gInfo.currentThread->unk_54 = isCB;
    ret = AddThreadToWaitQueue(cb, 0x6, qinfo->attr, &clk, timeout); //s0
    if(ret == 0)
    {
      *data = gInfo.currentThread->unk_24;
    }
  } while(isCB && ret == SCE_KERNEL_ERROR_NOTIFY_CALLBACK);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//9410
int sceKernelTryAllocateVpl(SceUID uid, unsigned int size, void **data)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  sceKernelIsIntrContext();

  if(IS_USER_MODE && IS_ADDR_KERNEL(data))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VPLID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }
  
  VplInfo *vinfo = UID_INFO(VplInfo, cb, uidVplType);

  if(size == 0 || size > vinfo->heapSize)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MEMSIZE;
  }

  int mem = sceKernelAllocHeapMemory(vinfo->heapAddr, size);

  THREADMAN_TRACE(0x5D, 3, uid, size, mem);

  if(mem != 0)
  {
    *data = mem;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//95d4
int sceKernelFreeVpl(SceUID uid, void *data)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(data))
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

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VPLID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }
  
  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  VplInfo *vinfo = UID_INFO(VplInfo, cb, uidVplType);

  if(data == 0 || sceKernelFreeHeapMemory(vinfo->heapAddr) < 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK;
  }

  THREADMAN_TRACE(0x5E, 2, uid, data);

  int mem;
  if(qinfo->numWaitThreads > 0 && (mem = sceKernelAllocHeapMemory(vinfo->heapAddr, qinfo->waitThread->unk_28) != 0))
  {
    qinfo->waitThread->unk_24 = mem;

    sub_00000480(cb, 0, 0, intr);
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//97b4
int sceKernelCancelVpl(SceUID uid, int *pnum)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  sceKernelIsIntrContext();

  if(IS_USER_MODE && IS_ADDR_KERNEL(pnum))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VPLID;
  }

  if(!CAN_CANCEL_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x5F, 2, uid, pnum);

  int ret = sceKernelCallUIDObjFunction(cb, UIDFUNC_CANCEL);

  if(pnum)
  {
    *pnum = ret;
  }
  
  sceKernelCpuResumeIntr(intr);
  
  RESET_K1;
  
  return SCE_KERNEL_ERROR_OK;
}

//9908
int sceKernelReferVplStatus(SceUID uid, SceKernelVplInfo *info)
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
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VPLID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x60, 2, uid info);

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  VplInfo *vinfo = UID_INFO(VplInfo, cb, uidVplType);

  SceKernelVplInfo *buf = (SceKernelVplInfo *)gInfo.unk_6a0;
  
  memset(buf, 0, sizeof(SceKernelVplInfo));
  
  buf->size = sizeof(SceKernelVplInfo);
  
  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }
  
  buf->attr = qinfo->attr;
  buf->poolSize = vinfo->heapSize;
  buf->freeSize = sceKernelHeapTotalFreeSize(vinfo->heapAddr);
  buf->numWaitThreads = qinfo->numWaitThreads;
  
  memcpy(info, buf, MIN(buf->size, info->size));
  
  sceKernelCpuResumeIntr(intr);
  
  RESET_K1;
  
  return SCE_KERNEL_ERROR_OK;
}

UidFunction VplUidFuncTable[] = //15D58
{
  { UIDFUNC_CREATE, VplCreateUID },
  { UIDFUNC_DELETE, VplDeleteUID },
  { 0, 0 }
};

//9aec
int initVpl()
{
  int ret;

  ret = sceKernelCreateUIDtypeInherit("WaitQ", "Vpl", sizeof(VplInfo), VplUidFuncTable, 0, &uidVplType);

  ASSERT(ret <= 0);

  return SCE_KERNEL_ERROR_OK;
}

//9b78
int VplCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//9ba0
int VplDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  VplInfo *vinfo = UID_INFO(VplInfo, cb, type);

  if(vinfo->heapAddr > 0)
  {
    sceKernelDeleteHeap(vinfo->heapAddr);
  }

  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return SCE_KERNEL_ERROR_OK;
}
