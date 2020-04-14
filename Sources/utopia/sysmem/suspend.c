ScePowerHandlers *power_handlers;
SceSuspendResumeHandler suspend_handler[32]; //c52c
SceSuspendResumeHandler resume_handler[32]; //c6ac

//68e4
int ClearSuspendResumeHandlers()
{
  int i;
  for(i = 0; i < 0x1F; i++)
  {
    suspend_handler[i].func = 0;
    resume_handler[i].func = 0;
  }

  return 0;
}

//6924
int sceKernelRegisterPowerHandlers(ScePowerHandlers &handlers)
{
  power_handlers = handlers;

  return 0;
}

//6934
int sceKernelPowerLock(int unk_a0)
{
  retrun power_handlers ? power_handlers->power_lock(unk_a0) : 0;
}

//696c
int sceKernelPowerLockForUser(int unk_a0)
{
  retrun power_handlers ? power_handlers->power_lock_for_user(unk_a0) : 0;
}

//69a4
int sceKernelPowerUnlock(int unk_a0)
{
  retrun power_handlers ? power_handlers->power_unlock(unk_a0) : 0;
}

//69dc
int sceKernelPowerUnlockForUser(int unk_a0)
{
  retrun power_handlers ? power_handlers->power_unlock_for_user(unk_a0) : 0;
}

//6a14
int sceKernelPowerTick(int unk_a0)
{
  retrun power_handlers ? power_handlers->power_tick(unk_a0) : 0;
}

//6a4c
int sceKernelRegisterSuspendHandler(int num, *func, void *arg)
{
  int intr;

  if(num > 0x1F)
  {
    return -1;
  }

  intr = sceKernelCpuSuspendIntr();

  suspend_handler[num].func = func;
  suspend_handler[num].gpreg = gp;
  suspend_handler[num].arg = arg;

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//6ad0
int sceKernelRegisterResumeHandler(int num, *func, void *arg)
{
  int intr;

  if(num > 0x1F)
  {
    //6b38
    return -1;
  }

  intr = sceKernelCpuSuspendIntr();

  resume_handler[num].func = func;
  resume_handler[num].gpreg = gp;
  resume_handler[num].arg = arg;

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//6b54
int sceKernelDispatchSuspendHandlers(unk_a0)
{
  int i;

//   s3 = gp;

  for(i = 0; i < 0x20; i++)
  {
    if(suspend_handler[i].func)
    {
      //v1 = gp;
      //gp = suspend_handler[i].unk_4;
      suspend_handler[i].func(unk_a0, suspend_handler[i].arg);
    }
  }

//   v0 = gp;
//   gp = s3;

  return 0;
}

//6be0
int sceKernelDispatchResumeHandlers(unk_a0)
{
  int i;
//   s3 = gp;

  for(i = 0; i < 0x20; i++)
  {
    if(resume_handler[i].func)
    {
//       v1 = gp;
//       gp = resume_handler[i].unk_4;
      resume_handler[i].func(unk_a0, resume_handler[i].arg);
    }
  }

//   v0 = gp;
//   gp = s3;

  return 0;
}

//6c70
//setjmp
loc_00006C70(...)
{
}

//6e64
sub_00006E64(...)
{
  0x0(t8) = t0;
  0x4(t8) = t1;
  0x8(t8) = t2;
  0xC(t8) = t3;
  0x10(t8) = t4;
  0x14(t8) = t5;
  0x18(t8) = t6;
  0x1C(t8) = t7;
}

//6e8c
//longjmp
loc_00006e8c(...)
{
}

//7068
sub_00007068(...)
{
  t0 = 0x0(t8);
  t1 = 0x4(t8);
  t2 = 0x8(t8);
  t3 = 0xC(t8);
  t4 = 0x10(t8);
  t5 = 0x14(t8);
  t6 = 0x18(t8);
  t7 = 0x1C(t8);
}

//7090
void sceSuspendForKernel_67B59042()
{
  loc_00006C70(&var_C310);
}

//70a0
void sceSuspendForKernel_B2C9640B(unk_a0)
{
  loc_00006E8C(&var_C310, unk_a0);
}

