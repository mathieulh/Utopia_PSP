#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thfpl.h"

//9c1c
int sceKernelCreateFpl(const char *name, int part, int attr, unsigned int blockSize,
                         unsigned int numBlocks, SceKernelFplOptParam *opt)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(name))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_OUT_OF_RANGE;
  }

  if(attr & ~SCE_FPL_LEGAL_ATTR)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_OUT_OF_RANGE;
  }

  if(blockSize <= 0 || numBlocks <= 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MEMSIZE;
  }

  int intr = sceKernelCpuSuspendIntr();

  PartitionInfo pinfo;
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

  void *heapmem = sceKernelAllocHeapMemory(heapuid, numBlocks * sizeof(int));
  if(heapmem == 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  int actualBlockSize = ((blockSize + 0x3) & ~0x3);
  SceUID partmem = sceKernelAllocPartitionMemory(part, name, IS_SET(attr, SCE_FPL_ATTR_UNK4000) ? SCE_SYSMEM_HEAP_ATTR_UNK1 : 0, actualBlockSize * numBlocks, 0);
  if(partmem <= 0)
  {
    sceKernelFreeHeapMemory(heapuid, heapmem);

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  void *addr = sceKernelGetBlockHeadAddr(partmem);

  uidControlBlock *cb;
  SceUID uid = sceKernelCreateUID(gInfo.uidFplType, name, IS_USER_MODE ? 0xFF : (attr & 0xFF), &cb);
  if(uid != 0)
  {
    sceKernelFreeHeapMemory(heapuid, heapmem);

    sceKernelFreePartitionMemory(partmem);

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return uid;
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  qinfo->attr = attr;

  FplInfo *fplinfo = UID_INFO(FplInfo, cb, gInfo.uidFplType);
  fplinfo->opt = opt;
  fplinfo->addr = addr;
  fplinfo->firstFreeBlock = 0;
  fplinfo->heapaddr = heapmem;
  fplinfo->blockSize = blockSize;
  fplinfo->numBlocks = numBlocks;
  fplinfo->actualBlockSize = actualBlockSize;

  int i;
  for(i = 0; i < numBlocks; i++)
  {
    _FreeFplBlock(fplinfo, i);
  }

  THREADMAN_TRACE(0x61, 1, cb->UID);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return cb->UID;
}

//9f24
int sceKernelDeleteFpl(SceUID uid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidFplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_FPLID;
  }

  if(!CAN_DELETE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x62, 1, uid);

  int ret = sceKernelDeleteUID(uid);
  ASSERT(ret <= 0);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//a07c
int sceKernelAllocateFpl(SceUID uid, void **data, unsigned int *timeout)
{
  return _AllocateFpl(uid, data, timeout, 0);
}

//a084
int sceKernelAllocateFplCB(SceUID uid, void **data, unsigned int *timeout)
{
  return _AllocateFpl(uid, data, timeout, 1);
}

//a08c
int _AllocateFpl(SceUID uid, void **data, unsigned int *timeout, int isCB)
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
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidFplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_FPLID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  FplInfo *fplinfo = UID_INFO(FplInfo, cb, gInfo.uidFplType);

  SceKernelSysClock clk;
  if(timeout != 0)
  {
    sub_0000D6D8(&clk, *timeout);
  }

  do
  {
    if(isCallback && gInfo.currentThread->callbackNotify != 0)
    {
      DispatchCallbacks(intr);
    }

    if(fplinfo->firstFreeBlock != 0)
    {
      *data = fplinfo->addr + fplinfo->actual_size * _AllocateFplBlock(fplinfo);

      THREADMAN_TRACE(isCB ? 0x64 : 0x63, 2, uid, *data);

      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return SCE_KERNEL_ERROR_OK;
    }

    gInfo.currentThread->isCallback = isCB;

    ret = AddThreadToWaitQueue(cb, 0x7, qinfo->attr, &clk, timeout);
    if(ret == 0)
    {
      *data = gInfo.currentThread->cbArg1;
    }
  } while(isCB && ret == SCE_KERNEL_ERROR_NOTIFY_CALLBACK);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//a360
int sceKernelTryAllocateFpl(SceUID uid, void **data)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  sceKernelIsIntrContext();

  if(IS_USER_MODE && (IS_ADDR_KERNEL(data)))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidFplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_FPLID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  FplInfo *fplinfo = UID_INFO(FplInfo, cb, gInfo.uidFplType);

  if(fplinfo->firstFreeBlock == 0)
  {
    THREADMAN_TRACE(0x65, 2, uid, 0);

    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  *data = fplinfo->addr + fplinfo->actualBlockSize * _AllocateFplBlock(fplinfo);

  THREADMAN_TRACE(0x65, 2, uid, unk);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//a520
int sceKernelFreeFpl(SceUID uid, void **data)
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
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidFplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_FPLID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);

  FplInfo *fplinfo = UID_INFO(FplInfo, cb, gInfo.uidFplType);

  if(data == 0 || (data < fplinfo->addr) || (data > (fplinfo->addr + (fplinfo->actualBlockSize * fplinfo->numBlocks)))
           || data != (fplinfo->addr + (fplinfo->actualBlockSize * ((data - fplinfo->addr) / fplinfo->actualBlockSize))))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK;
  }

  THREADMAN_TRACE(0x66, 2, uid, data);

  if(qinfo->numWaitThreads <= 0)
  {
    _FreeFplBlock(fplinfo, (data - fplinfo->addr) / fplinfo->actualBlockSize);
  }
  else
  {
    qinfo->waitThread->cbArg1 = data;

    sub_00000480(cb, 0, 0, intr);
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//a740
int sceKernelCancelFpl(SceUID uid, int *pnum)
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
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidFplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_FPLID;
  }

  if(!CAN_CANCEL_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x67, 2, uid, pnum);

  //the FPL type does not have a cancel function in 1.50, so this does nothing!
  int ret = sceKernelCallUIDObjFunction(cb, UIDFUNC_CANCEL);

  if(pnum)
  {
    *pnum = ret;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//a894
int sceKernelReferFplStatus(SceUID uid, SceKernelFplInfo *fplout)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(IS_USER_MODE && IS_ADDR_KERNEL(fplout))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidFplType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_FPLID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x68, 2, uid, fplinfo);

  WaitQInfo *qinfo = UID_INFO(WaitQInfo, cb, uidWaitQType);
  FplInfo *fplinfo = UID_INFO(FplInfo, cb, gInfo.uidFplType);

  SceKernelFplInfo *buf = (SceKernelFplInfo *)var_164a8;

  memset(buf, 0, sizeof(SceKernelFplInfo));

  buf->size = sizeof(SceKernelFplInfo);

  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }

  buf->attr = qinfo->attr;
  buf->blockSize = fplinfo->blockSize;
  buf->numBlocks = fplinfo->numBlocks;
  buf->freeBlocks = fplinfo->freeBlocks;
  buf->numWaitThreads = qinfo->numWaitThreads;

  memcpy(fplout, buf, MIN(buf->size, fplout->size));

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//aa7c
void _FreeFplBlock(FplInfo *fplinfo, int i)
{
  fplinfo->free++;

  u32 *old_unk8 = fplinfo->firstFreeBlock;
  if(old_unk8 == 0)
  {
    fplinfo->firstFreeBlock = fpinfo->heapaddr[i] = &fplinfo->heapaddr[i];

    return;
  }

  u32 old = *old_unk8;
  fplinfo->firstFreeBlock = &fplinfo->heapaddr[i];
  fplinfo->heapaddr[i] = old;
  *old_unk8 = &fplinfo->heapaddr[i];
}

//aabc
int _AllocateFplBlock(FplInfo *fplinfo)
{
  a1 = fplinfo->unk_8;

  if(fplinfo->unk_8 == 0)
  {
    return -1;
  }

  fplinfo->free--;

  if(fplinfo->unk_8 != fplinfo->unk_8->unk_0)
  {
    fplinfo->unk_8->unk_0 = fplinfo->unk_8->unk_0->unk_0;
  }
  else
  {
    a1 = 0;
  }

  fplinfo->unk_8 = a1; //???
  fplinfo->unk_8->unk_0->unk_0 = 0;

  return (fplinfo->unk_8->unk_0 - fplinfo->heapaddr) / 4;
}

UidFunction FplUidFuncTable[] = //15d70
{
  { UIDFUNC_CREATE, FplCreateUID },
  { UIDFUNC_DELETE, FplDeleteUID },
  { 0, 0 }
};

//ab0c
int initFpl()
{
  int ret;

  ret = sceKernelCreateUIDtypeInherit("WaitQ", "Fpl", sizeof(FplInfo), FplUidFuncTable, 0, &gInfo.uidFplType);

  ASSERT(ret <= 0);

  return SCE_KERNEL_ERROR_OK;
}

//ab98
int FplCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//abc0
int FplDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  FplInfo *fplinfo = UID_INFO(FplInfo, cb, type);

  sceKernelFreeHeapMemory(heapuid, fplinfo->heapaddr);

  SceUID partition, block;
  sceKernelQueryMemoryInfo(fplinfo->addr, &partition, &block);

  sceKernelFreePartitionMemory(block);

  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return SCE_KERNEL_ERROR_OK;
}
