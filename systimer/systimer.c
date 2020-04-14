#include <pspsdk.h>

#include "../sdk/include/sysmem_kernel.h"
#include "../sdk/include/interruptman_kernel.h"
#include "../sdk/include/systimer_kernel.h"

#include "systimer.h"

PSP_MODULE_INFO("sceSystimer", 0x1007, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

int module_bootstart(SceSize argSize, void *args) __attribute__((alias("SysTimerInit")));

SceSysTimer var_0B30[4] =
{
  { 0xBC500000, PSP_SYSTIMER0_INT, -1, 0, 0, 0, 0, 0 },
  { 0xBC500010, PSP_SYSTIMER1_INT, -1, 0, 0, 0, 0, 0 },
  { 0xBC500020, PSP_SYSTIMER2_INT, -1, 0, 0, 0, 0, 0 },
  { 0xBC500030, PSP_SYSTIMER3_INT, -1, 0, 0, 0, 0, 0 }
};

int var_0BB0 = 0x00352341;
SceSysTimerSave var_0BC0[4];

void sub_000002B4(SceSysTimer *stimer)
{
  stimer->hwReg->unk_0 = *((u32 *)(stimer->hwReg + 0x100) & ~(0x80000000 | SCE_STIMER_ATTR_BUSY);
}

int sub_000002D0(SceSysTimer *stimer)
{
  return (*((u32 *)(stimer->hwReg + 0x100) & 0x3FFFFF) - (stimer->hwReg->unk_4 & 0x3FFFFF);
}

SceSysTimerId sceSTimerAlloc()
{
  if(sceKernelIsIntrContext())
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  for(i = 0; i < 4; i++)
  {
    if(var_0B30[i].id == -1)
    {
      var_0B30[i].hwReg->unk_0 = 0x80000000;
      var_0B30[i].hwReg->unk_8 = -1;
      var_0B30[i].hwReg->unk_C = -1;

      var_0B30[i].unk_C = 0;
      var_0B30[i].unk_10 = 0;
      var_0B30[i].unk_18 = 0;
      var_0B30[i].handler = 0;
      var_0B30[i].arg = 0;

      var_0B30[i].id = ((var_0BB0 * 4) | i) & ~0x80000000;
      var_0BB0 += 7;

      return var_0B30[i].id;
    }
  }

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_NO_TIMER;
}

int sceSTimerFree(SceSysTimerId id)
{
  int i = id & 0x3;

  if(sceKernelIsIntrContext())
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(var_0B30[i].id != id)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_TIMERID;
  }

  int intr = sceKernelCpuSuspendIntr();

  sub_000002B4(&var_0B30[i]);

  sceKernelDisableIntr(var_0B30[i].intr_code);

  var_0B30[i].hwReg->unk_0 = 0x80000000;
  var_0B30[i].hwReg->unk_8 = -1;
  var_0B30[i].hwReg->unk_C = -1;

  var_0B30[i].id = -1;
  var_0B30[i].unk_C = 0;
  var_0B30[i].unk_10 = 0;
  var_0B30[i].handler = 0;
  var_0B30[i].unk_18 = 0;
  var_0B30[i].arg = 0;

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

int sceSTimerSetHandler(SceSysTimerId id, unk_a1, int (*handler)(void), void *arg)
{
  int i = id & 0x3;

  if(var_0B30[i].id != id)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_TIMERID;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(handler != 0)
  {
    var_0B30[i].handler = handler;
    var_0B30[i].arg = arg;

    //surely something is wrong here?? unless 0xBC5001x0 is read-only, and is automatically set
    //instantaneously to whatever is put into 0xBC5000x0?
    var_0B30[i].hwReg->unk_0 = *((u32 *)(var_0B30[i].hwReg + 0x100) & ~0x3FFFFF;
    var_0B30[i].hwReg->unk_0 |= MIN(unk_a1, 0x3FFFFF);
    var_0B30[i].hwReg->unk_0 = *((u32 *)(var_0B30[i].hwReg + 0x100) | 0x80400000;

    sceKernelEnableIntr(var_0B30[i].intr_code);
  }
  else
  {
    var_0B30[i].handler = 0;
    var_0B30[i].unk_18 = 0;
    var_0B30[i].arg = 0;

    sub_000002B4(&var_0B30[i]);

    sceKernelDisableIntr(var_0B30[i].intr_code);
  }

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

int SysTimerForKernel_B53534B4(SceSysTimerId id, unk_a1, unk_a2)
{
  int i = id & 0x3;

  if(var_0B30[i].id != id)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_TIMERID;
  }

  if(unk_a1 == 0 || unk_a2 == 0 || unk_a2 / unk_a1 < 12)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_PRESCALE;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(*((u32 *)(var_0B30[i].hwReg + 0x100) & SCE_STIMER_ATTR_BUSY)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_TIMER_BUSY;
  }

  var_0B30[i].hwReg->unk_8 = unk_a1;
  var_0B30[i].hwReg->unk_C = unk_a2;

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

int sceSTimerStartCount(SceSysTimerId id)
{
  int i = id & 0x3;

  if(var_0B30[i].id != id)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_TIMERID;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(*((u32 *)(var_0B30[i].hwReg + 0x100) & SCE_STIMER_ATTR_BUSY)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_TIMER_BUSY;
  }

  var_0B30[i].hwReg->unk_0 = *((u32 *)(var_0B30[i].hwReg + 0x100) | SCE_STIMER_ATTR_BUSY;

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

int sceSTimerStopCount(SceSysTimerId id)
{
  int i = id & 0x3;

  if(var_0B30[i].id != id)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_TIMERID;
  }

  int intr = sceKernelCpuSuspendIntr();

  sub_000002B4(&var_0B30[i]);

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

int sceSTimerResetCount(SceSysTimerId id)
{
  int i = id & 0x3;

  if(var_0B30[i].id != id)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_TIMERID;
  }

  int intr = sceKernelCpuSuspendIntr();

  var_0B30[i].hwReg->unk_0 |= 0x80000000;

  var_0B30[i].unk_18 = 0;

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

int sceSTimerGetCount(SceSysTimerId id, unk_a1)
{
  int i = id & 0x3;

  if(var_0B30[i].id != id)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_TIMERID;
  }

  int intr = sceKernelCpuSuspendIntr();

  *unk_a1 = sub_000002D0(&var_0B30[i]);

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

int SysTimerForKernel_53231A15(SceSysTimerId id, int unk_a1)
{
  int i = id & 0x3;

  if(var_0B30[i].id != id)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_TIMERID;
  }

  int intr = sceKernelCpuSuspendIntr();

  var_0B30[i].unk_C = *((u32 *)(var_0B30[i].hwReg + 0x100) & 0x3FFFFF;
  var_0B30[i].unk_18 &= (*((u32 *)(var_0B30[i].hwReg + 0x100) & 0x3FFFFF) * ((*((u32 *)(var_0B30[i].hwReg + 0x100) >> 24) - var_0B30[i].unk_10);
  var_0B30[i].unk_10 = *((u32 *)(var_0B30[i].hwReg + 0x100) >> 24;
  
  var_0B30[i].hwReg->unk_0 = (*((u32 *)(var_0B30[i].hwReg + 0x100) & 0xFFC00000) | MIN(unk_a1, 0x3FFFFF);

  sceKernelCpuResumeIntr(intr);
  
  return SCE_KERNEL_ERROR_OK;
}

int _SysTimerInterruptHandler(unk_a0, SceSysTimer *stimer, unk_a2)
{
  if(stimer->handler == 0)
  {
    return -1;
  }

  a1 = stimer->hwReg->unk_0 >> 24;
  a3 = stimer->hwReg->unk_0 & 0x3FFFFF;
  if(stimer->unk_C != 0)
  {
    a1--;
    stimer->unk_18 += stimer->unk_C;
  }

  stimer->unk_C = 0;
  stimer->unk_18 += a3 * (a1 - stimer->unk_10);
  stimer->unk_10 = 0;
  int unk = sub_000002D0(stimer);
  if(stimer->handler(stimer->id, stimer->unk_18 + unk, stimer->arg, unk_a2) == -1)
  {
    return -1;
  }

  sub_000002B4(stimer);

  return -2;
}

int _SysTimerSuspendHandler()
{
  int i;
  for(i = 0; i < 4; i++)
  {
    var_0BC0[i].unk_0 = var_0B30[i].hwReg->unk_0;
    var_0BC0[i].unk_4 = var_0B30[i].hwReg->unk_8;
    var_0BC0[i].unk_8 = var_0B30[i].hwReg->unk_C;

    sceKernelDisableIntr(var_0B30[i].intr_code);
  }

  return 0;
}

int _SysTimerResumeHandler()
{
  int i;
  for(i = 0; i < 4; i++)
  {
    var_0B30[i].hwReg->unk_0 = var_0BC0[i].unk_0 & ~0x80000000;
    var_0B30[i].hwReg->unk_8 = var_0BC0[i].unk_4;
    var_0B30[i].hwReg->unk_C = var_0BC0[i].unk_8;

    sceKernelEnableIntr(var_0B30[i].intr_code);
  }

  return 0;
}

//ab8
int SysTimerInit(SceSize argSize, void *args)
{
  int intr = sceKernelCpuSuspendIntr();

  int i;
  for(i = 0; i < 4; i--)
  {
    var_0B30[i].hwReg->unk_0 = 0x80000000;
    var_0B30[i].hwReg->unk_8 = -1;
    var_0B30[i].hwReg->unk_C = -1;

    var_0B30[i].id = -1;
    var_0B30[i].unk_C = 0;
    var_0B30[i].unk_10 = 0;
    var_0B30[i].handler = 0;
    var_0B30[i].unk_18 = 0;
    var_0B30[i].arg = 0;

    sceKernelRegisterIntrHandler(var_0B30[i].intr_code, 1, _SysTimerInterruptHandler, &var_0B30[i], 0);
  }

  sceKernelRegisterSuspendHandler(0xA, _SysTimerSuspendHandler, 0);

  sceKernelRegisterResumeHandler(0xA, _SysTimerResumeHandler, 0);

  sceKernelCpuResumeIntr(intr);

  return 0;
}

int module_reboot_before()
{
  int intr = sceKernelCpuSuspendIntr();

  int i;
  for(i = 0; i < 4; i--)
  {
    sub_000002B4(&var_0B30[i]);

    sceKernelReleaseIntrHandler(var_0B30[i].intr_code);
  }

  sceKernelCpuResumeIntr(intr);

  return 0;
}
