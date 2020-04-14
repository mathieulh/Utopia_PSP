var_35E4;
InterruptManCB var_3910;
var_3A7C;
var_3A80;
var_3A9C;
var_3AA0;
SceKernelSysClock var_3AA4;
var_3AAC;
var_3AB0;
var_3AB4;
var_3ACC;
InterruptInfo var_3AE4[0x43];

int module_bootstart(...)
{
  int intr = sceKernelCpuSuspendIntr();

  CTC0_R(GPREG_ZR, 14);
  CTC0_R(GPREG_ZR, 15);

  sub_000015B4(&var_3910, 0, 0x107C);

  sub_000034C4(...);

  ReleaseContextHooks(...);

  t0 = &var_3910.unk_1D4;
  a3 = -1;
  v1 = &var_3910.unk_1E0;
  a2 = &var_3910.unk_60;
  a1 = 0;
  a0 = 0x42;

  for(i = 0; i < 0x43; i++)
  {
    var_3910.unk_60[i] = &var_3910.unk_1D4[i];
    
    var_3910.unk_1D4[i].unk_10 = -1;
    var_3910.unk_1D4[i].unk_14 = -1;
  }

  t1 = 0x2000;
  var_3910.unk_1A8 = 0x2000;
  
  t9 = &var_498C;
  
  a1 = &var_4AC0;

  v0 = 0xA0000000;
  var_3910.unk_1A4 = &var_4AC0;
  t8 = t9 | v0;
  var_3910.unk_1B8 = t9;
  s0 = 0x8(t9);
  CTC0_V(t8, 12);
  CTC0_V(s0, 13);

  sceKernelRegisterExceptionHandler(0, loc_0000226C);

  sceKernelRegisterPriorityExceptionHandler(0, 3, loc_00002F90);

  sceKernelRegisterExceptionHandler(8, loc_0000316C);

  sceKernelRegisterIntrHandler(0x42, 0, loc_00002FB8, 0, 0);

  sceKernelRegisterSuspendHandler(0x1D, sub_000021D4, 0);

  sceKernelRegisterResumeHandler(0x1D, sub_00002224, 0);

  InterruptManagerForKernel_43A7BBDC(0x42, 0);

  int status;
  MFC0_V(status, COP0_SR_STATUS);
  status &= ~0xFF00;
  status |= 0x400;
  MTC0_V(status, COP0_SR_STATUS);

  sceKernelEnableIntr(0x42, status);

  MTC0_R(GPREG_ZR, COP0_SR_COUNT);

  MTC0_V(0x80000000, COP0_SR_COMPARE);

  sceKernelCpuResumeIntr(intr);

  sceKernelRegisterLibrary(&var_35E4);
  
  a0 = 0;
  v0 = MIN(v0, a0);
  
  return v0;
}

//198
sceKernelRegisterIntrHandler(...)
{
  int ret = 0; //s2
  fp = unk_a1;
  s6 = unk_a2;
  s3 = unk_a0;
  s0 = unk_t0;
  sp_0 = unk_a3;
  
  if(sceKernelIsIntrContext())
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(unk_a0 > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  if(unk_t0 != 0 && unk_t0->unk_0 != 0xC)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRPARAM;
  }

  int intr = sceKernelCpuSuspendIntr(); //sp_4

  if(unk_t0 != 0 && unk_t0->unk_4 > 0 && var_3910.unk_1BC == 0)
  {
    ret = sceKernelCreateHeap(SYSMEM_PARTITION_KERNEL, 0x100, 0x1, "interruptman_heap");
    if(ret < 0)
    {
      sceKernelCpuResumeIntr(intr);

      return ret;
    }

    if(ret > 0)
    {
      var_3910.unk_1BC = ret;
    }
  }

  ??? *unk = var_3AE4 + unk_a0 * 0x38;
  if(unk->unk_0 != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_FOUND_HANDLER;
  }

  if(unk_a2 >= 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  unk->unk_28 = 0;
  unk->unk_2C = 0;

  void *mem;
  if(unk_t0 != 0 && unk_t0->unk_4 > 0)
  {
    u32 *mem = sceKernelAllocHeapMemory(var_3910.unk_1BC, unk_t0->unk_4 * 15 * 4); //s5
    if(mem != 0)
    {
      sub_000015B4(mem, 0, unk_t0->unk_4 * 15 * 4);

      t8 = unk_t0->unk_4 * 4;
      v1 = mem + t8;
      v0 = unk_t0->unk_4;
      a0 = s5;

      for(i = unk_t0->unk_4, j = mem + unk_t0->unk_4 * 4; i > 0; i--, j += 0x38)
      {
        mem[i] = j;
      }
    }

    unk->unk_28 = mem;
    if(mem == 0)
    {
      sceKernelCpuResumeIntr(intr);

      return SCE_KERNEL_ERROR_NO_MEMORY;
    }

    unk->unk_2C = unk_t0->unk_8;
  }

  if(ret != 0)
  {
    return ret;
  }

  sceKernelSuspendIntr(); //why??

  fp = unk_a1 ? 0x2 : ret;
  unk->unk_0 = unk_a2 | fp;
  u32 *mod = (u32 *)sceKernelFindModuleByAddress(unk_a2);
  unk->unk_4 = mod ? mod[0x68 / 4] : 0;
  unk->unk_8 = unk_a3;
  unk->unk_30 = unk_t0 ? unk_t0->unk_4 : 0; //b -- unk_t0->unk_4 is a word above, and a byte here??

  a1 = unk->unk_30; //w
  a2 = a1 & ~0x100;
  a1 = a2 & ~0x1000;
  unk->unk_30 = a1;
  if(s3 < 0x40)
  {
    unk->unk_30 |= 0x600;
  }
  else
  {
    unk->unk_30 &= ~0x1700;
  }

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//400
InterruptManagerForKernel_15894D0B(...)
{
  s3 = unk_a2;
  s1 = unk_a1;
  s0 = unk_a0;

  if(sceKernelIsIntrContext())
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(unk_a0 > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr(); //s4
  
  ??? *unk = &var_3AE4 + unk_a0 * 0x38; //a3
  if(unk->unk_0 == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }
  
  if(unk_a1 < 0 || unk_a1 >= unk->unk_30) //b
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  t0 = unk->unk_28 + unk_a1 * 4;
  t1 = 0 < s3;
  a2 = 0x0(t0);
  t9 = -0x2001;
  a0 = 0x30(a2);
  //ins        $a0, $t1, 11, 1
  0x30(a2) = a0;
  t8 = unk->unk_30;
  s3 = t8 & t9;
  unk->unk_30 &= ~0x2000;
  v1 = unk->unk_30; //b
  t0 = 0;
  if(v1 != 0)
  {
    t4 = 0x28(a3);
    a2 = v1;
    v1 = a1 + t4;
    t3 = 0x0(v1);
    a1 = 0x30(t3);
    //ext        $v0, $a1, 11, 1
    t0++;
    do
    {
      if(v0 != 0)
      {
        unk->unk_30 |= 0x2000;
        break;
      }

      v1 = t0 < a2;
      t0++;
    } while(v1 != 0)
  }
  
  t5 = unk->unk_30;
  //ext        $a3, $t5, 13, 1
  if(a3 != 0)
  {
    sceKernelSetIntrLevel(unk_a0, 2);
  }

  sceKernelCpuResumeIntr(intr);
  
  return 0;
}

//578
int sceKernelSetIntrLevel(int intrcode, int level)
{
  s1 = unk_a0;
  s2 = unk_a1;
  
  if(intrcode >= 0x40)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }
  
  v1 = a1 - 1;
  a0 = v1 < 3;
  if(level < 1 || level > 3)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRLEVEL;
  }
  
  int intr = sceKernelCpuSuspendIntr(); //s3

  s0 = &var_3AE4 + intrcode * 0x38;

  if(s0.unk_0 == 0 || (s0.unk_0 & 0x3) == 0x3)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }
  
  t9 = s0.unk_30;
  a0 = s2 ^ 2;
  //ext        $t7, $t9, 13, 1
  t6 = t7 & t8;
  if(t6 != 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    return SCE_KERNEL_ERROR_ILLEGAL_INTRLEVEL;
  }
  
  if(sub_000016FC(intrcode) == 0)
  {
    a3 = s0.unk_30;
    //ins        $a3, $s2, 9, 2
    s0.unk_30 = a3;
  }
  else
  {
    a2 = s0.unk_30;
    //ext        $v1, $a2, 9, 2
    a1 = v1 < s2;
    if(a1 != 0)
    {
      sub_000015E4(intrcode);
    }

    s1 = s0.unk_30;
    //ins        $s1, $s2, 9, 2
    s0.unk_30 = s1;
    
    sub_0000165C(intrcode);
    
    sub_00001818();
  }
  
  sceKernelCpuResumeIntr(intr);
  
  return SCE_KERNEL_ERROR_OK;
}

int InterruptManagerForKernel_43A7BBDC(int intrcode, unk_a1)
{
  s0 = unk_a0;
  s1 = unk_a1;

  if(unk_a0 > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  
  a0 = &var_3Ae4 + intrcode * 0x38;
  a1 = v0;
  if(unk_a1 == 0)
  {
    a2.unk_30 |= 0x100;
  }
  else
  {
    a2.unk_30 &= ~0x100;
  }
  
  sceKernelCpuResumeIntr(intr);
  
  return SCE_KERNEL_ERROR_OK;
}

int sceKernelEnableIntr(int intrcode)
{
  s0 = unk_a0;
  s1 = 0;
  
  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr(); //s2
  
  //a0 = var_3AE4[intrcode]
  if(var_3AE4[intrcode].unk_0 == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }

  int flag;
  if(intrcode < 0x40)
  {
    sub_0000165C(intrcode);

    sub_00001818();
  }
  else if(intrcode == 0x40)
  {
    flag = 0x100;
  }
  else if(intrcode == 0x41)
  {
    flag = 0x200;
  }
  else
  {
    flag = 0x8000;
  }

  int status;
  MFC0_V(status, COP0_SR_STATUS);
  status |= (flag & 0xFF00);
  MTC0_V(status, COP0_SR_STATUS);

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

int sceKernelSuspendIntr(int intrcode, int *unk_a1)
{
  int ret = SCE_KERNEL_ERROR_OK, status;

  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  
  int flag;
  if(intrcode == 0x40)
  {
    flag = 0x100;
  }
  else if(intrcode == 0x41)
  {
    flag = 0x200;
  }
  else if(intrcode == 0x42)
  {
    flag = 0x8000;
  }

  if(var_3AE4[intrcode].unk_0 != 0 && unk_a1 != 0)
  {
    if(intrcode < 0x40)
    {
      *unk_a1 = sub_000016FC(intrcode);
    }
    else
    {
      MFC0_V(status, COP0_SR_STATUS);
      *unk_a1 = (status & 0xFF00) & flag;
    }
  }
  else
  {
    ret = SCE_KERNEL_ERROR_NOTFOUND_HANDLER;

    if(unk_a1)
    {
      *unk_a1 = 0;
    }
  }

  if(intrcode < 0x40)
  {
    sub_00001764(intrcode);
  }
  else
  {
    MFC0_V(status, COP0_SR_STATUS);
    status &= ~(flag & 0xFF00);
    MTC0_V(status, COP0_SR_STATUS);
  }

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//960
int sceKernelResumeIntr(int intrcode, unk_a1)
{
  int ret = SCE_KERNEL_ERROR_OK, status;

  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr();

  int flag;
  if(intrcode == 0x40)
  {
    flag = 0x100;
  }
  else if(intrcode == 0x41)
  {
    flag = 0x200;
  }
  else if(intrcode == 0x42)
  {
    flag = 0x8000;
  }

  if(var_3AE4[intrcode].unk_0 != 0)
  {
    if(intrcode < 0x40)
    {
      if(unk_a1 == 0)
      {
        sub_00001764(intrcode);
      }
      else
      {
        sub_0000165C(intrcode);

        sub_00001818();
      }
    }
    else
    {
      if(unk_a1 == 0)
      {
        MFC0_V(status, COP0_SR_STATUS);
        status &= ~(flag & 0xFF00);
        MTC0_V(status, COP0_SR_STATUS);
      }
      else
      {
        MFC0_V(status, COP0_SR_STATUS);
        status |= (flag & 0xFF00);
        MTC0_V(status, COP0_SR_STATUS);
      }
    }
  }
  else
  {
    ret = SCE_KERNEL_ERROR_NOTFOUND_HANDLER;

    if(intrcode < 0x40)
    {
      sub_00001764(intrcode);
    }
    else
    {
      MFC0_V(status, COP0_SR_STATUS);
      status &= ~(flag & 0xFF00);
      MTC0_V(status, COP0_SR_STATUS);
    }
  }

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//a90
void ReleaseContextHooks()
{
  int intr = sceKernelCpuSuspendIntr();

  var_3AAC = loc_00002F80;
  var_3AB0 = loc_00002F88;

  sceKernelCpuResumeIntr(intr);
}

//ad4
sceKernelRegisterDebuggerIntrHandler(...)
{
}

//c3c
sceKernelReleaseDebuggerIntrHandler(...)
{
}

//d84
sceKernelCallSubIntrHandler(...)
{

}

//e94
int sceKernelGetUserIntrStack()
{
  u32 *stack;

  if(var_3910.unk_1B4 != 0)
  {
    CFC0_V(stack, COP0_CR_GPR_SP_KERNEL);
    return stack[0x74 / 4];
  }
  else
  {
    return var_3910.unk_1AC + var_3910.unk_1B0;
  }
}

//ec4
sceKernelCallUserIntrHandler(...)
{
}

//fe0
sceKernelRegisterSubIntrHandler(...)
{
}

//1204
sceKernelReleaseSubIntrHandler(...)
{
}

//1374
int sceKernelSuspendSubIntr(int intrcode, int unk_a1, int unk_a2)
{
  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(unk_a1 < 0 || unk_a1 >= var_3AE4[intrcode].unk_30) //b)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  if(var_3AE4[intrcode].unk_0 == 0 || var_3AE4[intrcode].unk_2C == 0 || var_3AE4[intrcode].unk_2C->unk_20 == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }

  int ret = var_3AE4[intrcode].unk_2C->unk_20(intrcode, unk_a1, unk_a2);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//1458
QueryIntrHandlerInfo(...)
{
}

//15b4 - memset
sub_000015B4(...)
{
}

//15e4
sub_000015E4(...)
{
}

//165c
sub_0000165C(...)
{
}

//16fc
sub_000016FC(...)
{
}

//1764
sub_00001764(...)
{
}

//1818
void sub_00001818()
{
  if(!sceKernelIsIntrContext())
  {
    sub_00003518(var_3910.unk_18, var_3910.unk_1C);
  }
}

//1854
sceKernelSetPrimarySyscallHandler(...)
{
}

//1924
void sceKernelCpuEnableIntr()
{
  MTIC_V(1, 0);
}

//1934
int QueryInterruptManCB(...)
{
  return &var_3910;
}

//1940
void loc_000001940(void *unk_a0)
{
  if(unk_a0 != 0)
  {
    sceKernelFreeHeapMemory(var_3ACC, unk_a0);
  }
}

//1968
int sceKernelReleaseIntrHandler(int intrcode)
{
  if(sceKernelIsIntrContext())
  {
    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  
  if(var_3AE4[intrcode].unk_0 == 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }
  
  sceKernelSuspendIntr(intrcode, 0);
  
  if(var_3AE4[intrcode].unk_28 != 0)
  {
    sceKernelFreeHeapMemory(var_3ACC, var_3AE4[intrcode].unk_28);
  }
  
  var_3AE4[intrcode].unk_0 = 0;
  var_3AE4[intrcode].unk_28 = 0;
  var_3AE4[intrcode].unk_2C = 0;

  sceKernelCpuResumeIntr(intr);
  
  return SCE_KERNEL_ERROR_OK;
}

//1a44
sceKernelIsInterruptOccurred(...)
{

}

//1ae4
void sceKernelDisableIntr(int intrcode)
{
  sceKernelSuspendIntr(intrcode, 0);
}

//1b00
void RegisterContextHooks(unk_a1, unk_a2)
{
  int intr = sceKernelCpuSuspendIntr();

  var_3AAC = unk_a1;
  var_3AB0 = unk_a0;

  sceKernelCpuResumeIntr(intr);
}

//1b48
int UnSupportIntr(intrcode)
{
  if(intrcode >= 0x40)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr();

  sceKernelSuspendIntr(intrcode, 0);

  var_3AE4[intrcode].unk_0 = 3;
  var_3AE4[intrcode].unk_30 &= ~0x600;

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

//1bd8
int SupportIntr(int intrcode)
{
  if(intrcode >= 0x40)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr();

  sceKernelSuspendIntr(intrcode, 0);

  var_3AE4[intrcode].unk_0 = 0;
  var_3AE4[intrcode].unk_30 |= 0x600;

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

//1c60
int sceKernelEnableSubIntr(int intrcode, int unk_a1)
{
  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(unk_a1 < 0 || unk_a1 >= var_3AE4[intrcode].unk_30) //b)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  if(var_3AE4[intrcode].unk_0 == 0 || var_3AE4[intrcode].unk_2C == 0 || var_3AE4[intrcode].unk_2C->unk_18 == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }

  int ret = var_3AE4[intrcode].unk_2C->unk_18(intrcode, unk_a1);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//1d2c
int sceKernelDisableSubIntr(int intrcode, int unk_a1)
{
  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(unk_a1 < 0 || unk_a1 >= var_3AE4[intrcode].unk_30) //b
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  if(var_3AE4[intrcode].unk_0 == 0 || var_3AE4[intrcode].unk_2C == 0 || var_3AE4[intrcode].unk_2C->unk_1C == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }

  int ret = var_3AE4[intrcode].unk_2C->unk_1C(intrcode, unk_a1);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//1df8
int sceKernelResumeSubIntr(int intrcode, unk_a1, unk_a2)
{
  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(unk_a1 < 0 || unk_a1 >= var_3AE4[intrcode].unk_30) //b
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  if(var_3AE4[intrcode].unk_0 == 0 || var_3AE4[intrcode].unk_2C == 0 || var_3AE4[intrcode].unk_2C->unk_24 == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }

  int ret = var_3AE4[intrcode].unk_2C->unk_24(intrcode, unk_a1, unk_a2);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//1ed4
int sceKernelIsSubInterruptOccurred(int intrcode)
{
  if(intrcode > 0x42)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(unk_a1 < 0 || unk_a1 >= var_3AE4[intrcode].unk_30) //b
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_INTRCODE;
  }

  if(var_3AE4[intrcode].unk_0 == 0 || var_3AE4[intrcode].unk_2C == 0 || var_3AE4[intrcode].unk_2C->unk_28 == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_NOTFOUND_HANDLER;
  }

  int ret = var_3AE4[intrcode].unk_2C->unk_28(intrcode, unk_a1);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//1fa0
int sceKernelRegisterUserSpaceIntrStack(int unk_a0, int unk_a1)
{
  int intr = sceKernelCpuSuspendIntr();

  if(var_3910.unk_1AC != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ALREADY_STACK_SET;
  }

  if(unk_a0 < 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    return SCE_KERNEL_ERROR_ILLEGAL_STACK_ADDRESS;
  }
  
  var_3910.unk_1AC = unk_a0;
  var_3910.unk_1B0 = unk_a1;
  
  sceKernelCpuResumeIntr(intr);
  
  return SCE_KERNEL_ERROR_OK;
}

//2018
int sceKernelGetCpuClockCounter()
{
  int count;

  MFC0_V(count, COP0_SR_COUNT);

  return count;
}

//2024
sceKernelGetCpuClockCounterWide(...)
{
}

//2090
int _sceKernelGetCpuClockCounterLow()
{
  return var_3AA4.low;
}

//209c
int _sceKernelGetCpuClockCounterHigh()
{
  return var_3AA4.hi;
}

//20a8
sceKernelRegisterSystemCallTable(SceSysCallTable *table)
{
  s0 = unk_a0;

  if(table->unk_0 != 0)
  {
    return SCE_KERNEL_ERROR_SYCALLTABLE_USED;
  }
  
  if(table->unk_8 < table->unk_4 || (table->unk_C - 0x14) < (table->unk_8 - table->unk_4))
  {
    return SCE_KERNEL_ERROR_ILLEGAL_SYSCALLTABLE;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  
  a1 = var_3AC8;
  a2 = a1->unk_0;
  t2 = a1->unk_4;
  v1 = a2;
  if(t2 != 0)
  {
    a1 = v1;
    while(1 == 1)
    {
      v1 = v1->unk_0;
      t3 = v1->unk_4;
      if(t3 == 0)
      {
        break;
      }
      
      a1 = v1;
    }
    
    a2 = v1;
  }
  
  table->unk_0 = a2;
  a1->unk_0 = table;
  
  sceKernelCpuResumeIntr(intr);
  
  return SCE_KERNEL_ERROR_OK;
}

//215c
sceKernelQuerySystemCall(...)
{
}

//21d4
sub_000021D4(...)
{
}

//2224
sub_00002224(...)
{
}

//2a18
ReturnToThread(...)
{
}

//2ec8
int sceKernelIsIntrContext()
{
  return var_3A7C;
}

//2ed4
int InterruptManagerForKernel_53991063()
{
  return var_3A9C;
}

//2ee0
int sceKernelGetInterruptExitCount()
{
  return var_3AA0;
}

//2eec
SaveThreadContext(...)
{
}

//2f80
int loc_00002f80(int unk_a0)
{
  return unk_a0;
}

//2f88
int loc_00002f88()
{
  return 0;
}

//2f90
loc_00002f90(...)
{
  int tmp1, tmp2;

  NOP;
  NOP;

  CFC0_V(tmp1, COP0_CR_EPC);
  MTC0_V(tmp1, COP0_SR_EPC);

  CFC0_V(tmp1, COP0_CR_STATUS);
  MTC0_V(tmp1, COP0_SR_STATUS);

  CFC0_R(GPREG_V0, COP0_CR_GPR_V0);
  CFC0_R(GPREG_V1, COP0_CR_GPR_V1);

  ERET;
}

//2fb8
int loc_00002fb8()
{
  int compare;

  MFC0_V(compare, COP0_SR_COMPARE);
  MTC0_V(compare + 0x80000000, COP0_SR_COMPARE);

 if(compare == 0)
 {
   var_3AA4.unk_4++;
 }

 MFC0_V(var_3AA4.unk_0, COP0_SR_COUNT);

 return -1;
}

//2ff0
sub_00002FF0(...)
{
}

//30cc
int sceKernelCpuSuspendIntr()
{
  int intr;

  MFIC_V(intr, 0);
  MTIC_R(GPREG_ZR, 0);
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  return intr;
}

//30f8
void sceKernelCpuResumeIntr(int intr)
{
  MTIC_V(intr, 0);
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
}

//3120
void sceKernelCpuResumeIntrWithSync(int unk_a0)
{
  SYNC;
  NOP;
  SYNC;
  NOP;
  MTIC_V(unk_a0, 0);
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
}

//3158
int loc_00003158(int unk_a0)
{
  return (unk_a0 <= 0);
}

//3160
int loc_00003160()
{
  int ret;

  MFIC_V(ret, 0);

  return ret;
}

//316c
loc_0000316C(...)
{
}

//34c4
void sub_000034C4()
{
  *((u32 *)0xBC300008) = 0xF;
  *((u32 *)0xBC300018) = 0;
}

//34d8
SceUInt64 sub_000034D8()
{
  v0 = *((u32 *)0xBC300000);
  v0 -= (v0 % 0x10);
  v1 = *((u32 *)0xBC300010);

  return v0v1;
}

//34f0
SceUInt64 sub_000034F0()
{
  v0 = *((u32 *)0xBC300004);
  v0 -= (v0 % 0x10);
  v1 = *((u32 *)0xBC300014);

  return v0v1;
}

//3508
SceUInt64 sub_00003508()
{
  v0 = *((u32 *)0xBC300008);
  v1 = *((u32 *)0xBC300018);

  return v0v1;
}

//3518
void sub_00003518(int unk_a0, int unk_a1)
{
  *((u32 *)0xBC300008) = unk_a0 | 0xF;
  *((u32 *)0xBC300018) = unk_a1;

  SYNC;
}

//3534
void sub_00003534(int unk_a0)
{
  if(unk_a0 < 0x1E || unk_a0 > 0x1F)
  {
    return;
  }

  *((u32 *)0xBC300000) = 1 << unk_a0;

  SYNC;
}
