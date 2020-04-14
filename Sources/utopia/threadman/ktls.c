#include "../common/common.h"

#include "../sdk/include/threadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "ktls.h"

extern ThreadmanInfo gInfo;

//e360
int sceKernelAllocateKTLS(SceSize size, int (*cb)(unsigned int *size, void *arg), void *arg)
{
  if(sceKernelIsIntrContext())
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  int i;
  for(i = 0; i < 0x10; i++)
  {
    if(!gInfo.ktls[i].allocated)
    {
      gInfo.ktls[i].used = 0;
      gInfo.ktls[i].size = size;
      gInfo.ktls[i].func = cb;
      gInfo.ktls[i].arg = arg;

      SceModule *mod = sceKernelFindModuleByAddress(cb);
      gInfo.ktls[i].gpreg = mod ? mod->unk_68 : _GET_GPREG(GPREG_GP);

      gInfo.ktls[i].allocated = 1;

      THREADMAN_TRACE(0x8C, 4, id, cb, arg, i);

      sceKernelCpuResumeIntr(intr);

      RESET_K1;

      return s1;
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_KTLS_FULL;
}

//e4b4
int sceKernelFreeKTLS(int id)
{
  if(sceKernelIsIntrContext())
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(id >= 0x10)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_KTLSID;
  }

  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  THREADMAN_TRACE(0x8D, 1, id);

  if(!gInfo.ktls[id].allocated)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_KTLSID;
  }

  if(gInfo.ktls[id].used)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_KTLS_BUSY;
  }

  gInfo.ktls[id].allocated = 0;

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//e5d4
void *sceKernelGetThreadKTLS(int id, SceUID thid, int mode);
{
  if(sceKernelIsIntrContext())
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(id >= 0x10)
  {
    return 0;
  }

  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(thid == 0)
  {
    thid = gInfo.currentThread->UID;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(thid, uidThreadType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return 0;
  }

  ThreadInfo *thread = UID_INFO(ThreadInfo, cb, uidThreadType);
  if(!gInfo.ktls[id].allocated)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return 0;
  }

  int ret = thread->ktls[id];
  if(ret == 0)
  {
    if(mode != 0)
    {
      ret = sceKernelAllocHeapMemory(gInfo.heapuid, gInfo.ktls[id].size);

      THREADMAN_TRACE(0x8F, 4, id, thid, mode, thread->ktls[id]);

      if(ret != 0)
      {
        memset(ret, 0, gInfo.ktls[id].size);

        thread->ktls[id] = ret;

        gInfo.ktls[id].used++;
      }
    }
  }
  else
  {
    THREADMAN_TRACE(0x8F, 4, id, thid, mode, thread->ktls[id]);
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//e7a8
int sub_0000E7A8(ThreadInfo *thread)
{
  int ret = 0;

  for(i = 0xF; i >= 0; i--)
  {
    if(thread->ktls[i] != 0)
    {
      if(thread == gInfo.currentThread || !gInfo.ktls[i].func)
      {
        if(gInfo.ktls[i].func)
        {
          int old_gp = gp;

          gp = gInfo.ktls[i].gpreg;

          gInfo.ktls[i].func(thread->ktls[i], gInfo.ktls[i].arg);

          gp = old_gp;
        }

        sceKernelFreeHeapMemory(gInfo.heapuid, thread->ktls[i]);

        int intr = sceKernelCpuSuspendIntr();

        thread->ktls[i] = 0;

        gInfo.ktls[i].used--;

        sceKernelCpuResumeIntr(intr);
      }
      else
      {
        ret = 1;
      }
    }
  }

  return ret;
}

//e8b4
void *sceKernelGetKTLS(int id)
{
  return sceKernelGetThreadKTLS(id, 0, 1);
}
