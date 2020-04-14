//systable.c

SceLibraryEntry *var_75A0;
??? *var_75A4;
*var_75A8;
*var_75AC;
SceLibraryEntry var_75BC[0x1E]; //size 0x24
??? var_79F4[0x5A]; //size 0x2C

//0
int sub_00000000()
{
  var_75BC[0].next = 0;

  for(i = 0x1D; i > 0; i--)
  {
    var_75BC[i].next = &var_75BC[i - 1];
  }

  var_75A0 = &var_75BC[0x1D];

  return 0;
}

//48
SceLibraryEntry *sub_00000048()
{
  SceLibraryEntry *ret;

  if(var_75A0 == 0)
  {
    ret = sceKernelAllocHeapMemory(LoadCoreHeap(), sizeof(SceLibraryEntry));
  }
  else
  {
    ret = var_75A0;

    var_75A0 = var_75A0->next;
    ret->next = 0;
  }

  return ret;
}

//98
SceLibraryEntry *sub_00000098(SceLibraryEntry *unk_a0)
{
  SceLibraryEntry *ret;

  if(unk_a0 < var_75BC || unk_a0 >= (var_75BC + sizeof(var_75BC)))
  {
    sceKernelFreeHeapMemory(LoadCoreHeap(), unk_a0);

    ret = var_75A0;
  }
  else
  {
    ret = unk_a0;

    unk_a0->next = var_75A0;
    var_75A0 = unk_a0;
  }

  return ret;
}

//10c
int sub_0000010C()
{
  var_79F4[0].unk_0 = 0;

  for(i = 0x59; i > 0; i--)
  {
    var_79F4[i].unk_0 = &var_79F4[i - 1];
  }

  var_75A4 = &var_79F4[0x59];

  return 0;
}

//154
void *sub_00000154()
{
  void *ret;

  if(var_75A4 == 0)
  {
    ret = sceKernelAllocHeapMemory(LoadCoreHeap(), 0x2C);
  }
  else
  {
    ret = var_75A4;
    var_75A4 = var_75A4->unk_0;
  }

  return ret;
}

//1a0
void *sub_000001A0(unk_a0)
{
  void *ret;

  if(unk_a0 < var_79F4 || unk_a0 >= (var_79F4 + sizeof(var_79F4)))
  {
    sceKernelFreeHeapMemory(LoadCoreHeap(), unk_a0);

    ret = var_75A4;
  }
  else
  {
    ret = unk_a0;

    unk_a0->unk_0 = var_75A4;
    var_75A4 = unk_a0;
  }

  return ret;
}

//214
//unk_a0 = 0x2000
int SyscallTableInit(int unk_a0, void **unk_a1)
{
  s3 = unk_a1;
  s0 = unk_a0;

  SceUID heapid = LoadCoreHeap(); //a0/v0

  void *table = sceKernelAllocHeapMemory(heapid, 0x4010); //s1/v0
  if(table == 0)
  {
    Kprintf("systable.c:%s:No Heap Memory\n", __FUNCTION__);

    return SCE_KERNEL_ERROR_ERROR;
  }

  table->unk_0 = 0;
  table->unk_4 = unk_a0 * 4;
  table->unk_8 = 0x3FFC + unk_a0 * 4;
  table->unk_C = 0x4010;

  for(i = 0xFFF; i >= 0; i--)
  {
    *(table + i * 0x10) = UndefSyscall;
  }

  SysTable *unk;
  s0 = &var_75AC;
  if(var_75A8 == 0)
  {
    //we already did this above!
    //heapid = LoadCoreHeap();

    unk = (SysTable *)sceKernelAllocHeapMemory(heapid, sizeof(SysTable)); //v1/v0
  }
  else
  {
    unk = var_75A8; //v1

    var_75A8 = var_75A8->unk_0;
  }

  var_75AC = unk;

  unk->next = 0;
  unk->unk_4 = 0; //h
  unk->unk_6 = 0x1000; //h
  unk->unk_8 = 0;

  ret = sceKernelRegisterSystemCallTable(table); //s0/v0
  if(ret != 0)
  {
    Kprintf("systable.c:%s:sceKernelRegisterSystemCallTable failed: 0x%X\n", __FUNCTION__, ret);

    //again...we already did this above!
    //v0 = LoadCoreHeap(...);

    sceKernelFreeHeapMemory(heapid, table);

    return ret;
  }

  *unk_a1 = table;

  return SCE_KERNEL_ERROR_OK;
}

//368
int AllocSysTable(short numEntries)
{
  SysTable *table;
  for(table = var_75AC; table; table = table->next)
  {
    if(!(table->unk_4 & 0x1))
    {
      if(table->unk_6 == numEntries)
      {
        table->unk_4 |= 0x1;
        return table->unk_8;
      }

      if(table->unk_6 > numEntries)
      {
        SysTable *new_table;
        if(var_75A8 != 0)
        {
          new_table = var_75A8;
          var_75A8 = var_75A8->unk_0;
        }
        else
        {
          new_table = (SysTable *)sceKernelAllocHeapMemory(LoadCoreHeap(), sizeof(SysTable));
        }

        new_table->next = table->next;
        new_table->unk_4 = 0;
        new_table->unk_6 = table->unk_6 - numEntries;
        new_table->unk_8 = table->unk_8 + numEntries;

        table->unk_4 |= 0x1;
        table->unk_6 = numEntries;
        table->next = new_table;

        return table->unk_8;
      }
    }
  }

  Kprintf("systable.c:%s:Automatic SyscallTable grow needed\n", __FUNCTION__);

  return SCE_KERNEL_ERROR_ERROR;
}

//480
FreeSysTable(...)
{
  a2 = unk_a0;
  s0 = var_75AC;
  s1 = 0;

  for(s0 = var_75AC; s0; s0 = s0->next)
  {
    if(var_75AC->unk_8 == unk_a0)
    {
      if(!(s0->unk_4 & 0x1))
      {
        Kprintf("systable.c:%s:try to Release unused table: %d\n", __FUNCTION__, unk_a0);
        
        return 0;
      }
      
      //why??
      s0->unk_4 &= ~0x1;

      if(s1 != 0 && s1->unk_4 == 0)
      {
        s1->unk_6 += s0->unk_6;
        s1->next = s0->next;
        if(s0 < 0x896C || s0 >= 0x896C + 0x2D0)
        {
          sceKernelFreeHeapMemory(LoadCoreHeap(), s0);
        }
        else
        {
          s0->next = var_75A8;
          var_75A8 = s0;
        }

        s0 = s1;
      }
    }

    s1 = s0->next;
    if(s1 == 0 || s1->unk_4 != 0)
    {
      return s0;
    }
    
    s0->unk_6 += s1->unk_6;
    s0->next = s1->next;
    if(s1 < 0x896C || s1 >= 0x896C + 0x2D0)
    {
      sceKernelFreeHeapMemory(LoadCoreHeap(), s1);
    }
    else
    {
      s1->next = var_75A8;
      var_75A8 = s1;
    }

    return s0;
  }

  Kprintf("systable.c:%s:Cannot found index %d\n", _+FUNCTION__, unk_a0);

  return 0;
}

//650
int loc_00000650(unk_a0)
{
  if(unk_a0 < 0x896C || unk_a0 >= 0x896C + 0x2D0)
  {
    sceKernelFreeHeapMemory(LoadCoreHeap(), unk_a0);
  }
  else
  {
    unk_a0->next = var_75A8;
    var_75A8 = unk_a0;
  }

  return var_75A8;
}

//6c4
int UndefSyscall()
{
  Kprintf("systable.c:%s: is called.\n", __FUNCTION__);

  return SCE_KERNEL_ERROR_ERROR;
}

//6f4
sub_000006F4(reglibin *lib, stubin *stub)
{
  sub_0000072C(lib, stub, 0);
}

//710
sub_00000710(reglibin *lib, stubin *stub)
{
  sub_0000072C(lib, stub, 1);
}

typedef struct _unkobj1
{
  int unk_0;
  int unk_4;
  char unk_8;
  char unk_9;
  short unk_A;
  int unk_C;
  int unk_14;
} unkobj1;

//72c
int sub_0000072C(reglibin *lib, stubin *stub, int unk_a2)
{
  if(stubin->num_vars == 0 || lib->num_vars < stub->num_vars)
  {
    return SCE_KERNEL_ERROR_ERROR;
  }

  if(lib->num_vars == 0 || stub->len < 6)
  {
    return SCE_KERNEL_ERROR_OK;
  }

  //so why does it loop twice, and not do anything the first time?
  for(s5 = 0; s5 < 2; s5++)
  {
    if(stub->num_vars > 0)
    {
      for(s3 = 0; s3 < stub->num_vars; s3++)
      {
        if(lib->num_vars > 0)
        {
          for(a0 = 0; a0 < lib->num_vars; a0++)
          {
            if(lib->entry_table[lib->num_funcs + a0] == stub->var[s3].unk_4)
            {
              break;
            }
          }
        }

        if(a0 == lib->num_vars)
        {
          return SCE_KERNEL_ERROR_ERROR;
        }

        if(s5 == 1)
        {
          sub_00000898(stub->var[s3].unk_0, lib->entry_table[lib->num_funcs * 2 + lib->num_vars + stub->var[s3].unk_0], unk_a2);
        }
      }
    }
  }

  return SCE_KERNEL_ERROR_OK;
}

//898
sub_00000898(...)
{
}
