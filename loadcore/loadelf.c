//loadelf.c
//28b4
CheckElfSection(...)
{
}

//2ae8
sub_00002AE8(...)
{
}

//2c9c
int sceKernelCheckExecFile(u32 *buf, SceLoadCoreExecFileInfo *execFileInfo)
{
  int s2 = 0;

  ASSERT_MSG(execFileInfo != 0, "loadelf.c:%s:Illegal parameter\n", __FUNCTION__);

  //skip ~SCE header
  if(buf[0x0 / 4] == 0x4543537E && buf[0x4 / 4] != 0)
  {
    s2 = 1;
    buf += sizeof(SceHeader); //s0
  }

  int ok = 1;
  if(!(execFileInfo->unk_4 & 0x2))
  {
    ret = sub_00004210(buf, execFileInfo);
    if(ret != 0)
    {
      return ret;
    }

    if(execFileInfo->unk_10 != 0 && execFileInfo->unk_48 == 0)
    {
      ok = 0;
    }
    else if(execFileInfo->unk_5A & 0x1)
    {
      buf = execFileInfo->unk_24; //s0
      execFileInfo->unk_54 = 1;
      execFileInfo->unk_1C = execFileInfo->unk_24;
    }
  }

  //2cf0
  if(ok)
  {
    ElfHeader *elf = (ElfHeader *)buf;
    ElfProgramHeader *ph = (ElfProgramHeader *)(buf + elf->phoff);

    execFileInfo->unk_24 = -1;
    execFileInfo->unk_30 = 0;
    if(elf->unk_4 == 0x101)
    {
      if(elf->unk_6 == 0x8)
      {
        if(elf->type == 0x2)
        {
          execFileInfo->unk_20 = 0x3;
        }
        else if(elf->type == ELF_TYPE_PRX || ((elf->type + 0x80) & 0xFFFF) < 0x2)
        {
          execFileInfo->unk_20 = 0x1;
        }

        for(a0 = 0; a0 < elf->phnum; a0++)
        {
          if(ph[a0].type == 1)
          {
            break;
          }
        }

        if(a0 < elf->phnum && ph[a0].type == 1)
        {
          execFileInfo->unk_44 = (ph[a0].paddr & 0x80000000) ? 0 : 1; //wrong way round??
          if(ph[a0].paddr == ph[a0].vaddr)
          {
            execFileInfo->unk_4C = 0;
          }
          else
          {
            execFileInfo->unk_4C = ph[a0].paddr & ~0x80000000;
          }
        }

        //2dfc
        execFileInfo->unk_24 = ph[a0].vaddr;

        int maxoffset = 0;
        if(a0 < elf->phnum)
        {
          for(; a0 < elf->phnum; a0++)
          {
            execFileInfo->unk_24 = MIN(execFileInfo->unk_24, ph[a0].vaddr;
            maxoffset = MAX(maxoffset, ph[a0].vaddr + ph[a0].memsz);
          }
        }

        //2e20
        execFileInfo->unk_30 = maxoffset - execFileInfo->unk_24;
      }
    }
  }

  //2d1c
  if((!(execFileInfo->unk_4 & 0x1) || execFileInfo->unk_10 != 0) && s2 == 1)
  {
    execFileInfo->unk_10 += sizeof(SceHeader);
  }

  return SCE_KERNEL_ERROR_OK;
}

//2f48
int sceKernelProbeExecutableObject(void *buf, SceLoadCoreExecFileInfo *execFileInfo)
{
  s1 = execFileInfo;
  s0 = buf;

  a0 = 0;
  if(buf->unk_0 == 0x4543537E) //~SCE
  {
    a0 = buf->unk_4;
  }

  if(a0 != 0)
  {
    buf += 0x40;
  }

  a0 = execFileInfo->unk_48;
  if(execFileInfo->unk_48 == 0)
  {
    ret = sceKernelCheckExecFile(buf, execFileInfo);
    if(ret < 0)
    {
      return ret;
    }
  }

  switch(execFileInfo->unk_20 + 1)
  {
    case 2:
    case 3:
    {
      //2fb8
      sub_00002AE8(buf, execFileInfo);

      execFileInfo->unk_50 = buf + execFileInfo->unk_4C;
      execFileInfo->unk_58 |= *(buf + execFileInfo->unk_4C); //hw

      int cpuid;
      MFC0_V(cpuid, COP0_SR_CPUID);
      if(cpuid != 0)
      {
        return SCE_KERNEL_ERROR_OK;
      }

      if(execFileInfo->unk_58 & 0x1000)
      {
        Kprintf("Cannot load plain kernel module\n");
        execFileInfo->unk_20 = -1;
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      if(execFileInfo->unk_58 & 0x800)
      {
        Kprintf("Cannot load plain VSH module\n");
        execFileInfo->unk_20 = -1;
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      // >= 0x122, < 0x143, < 0x140, >= 0x132 -> ok (132 - 13f)
      // >= 0x122, >= 0x143, != 0x210, != 0x220 ->ok (143 - ... !210,220)
      // < 0x122, < 0x120, >= 0x22, != 0x30, != 0x50 ->ok (22 - 11f !30,50)
      // < 0x122, < 0x120, < 0x22, < 0x20, != 0x11 ->ok (0 - 1f !11)
      //ok => return SCE_KERNEL_ERROR_OK;
      switch(execFileInfo->unk_8)
      {
      }

      //3044/8
      Kprintf("Cannot load plain USER module ");
      Kprintf(" in API 0x%08x\n", execFileInfo->unk_8);
      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
    }

    case 4:
    {
      //3104
      CheckElfSection(buf, execFileInfo);

      return SCE_KERNEL_ERROR_OK;
    }

    default:
    {
      return SCE_KERNEL_ERROR_OK;
    }
  }
}

//3138
loadrelelf_new(...)
{
}

//3598
sceKernelApplyElfRelSection(...)
{
}

//37cc
sceKernelApplyPspRelSection(...)
{
}

//39f4
int sub_000039F4(unk_a0)
{
  var_75B4 = unk_a0;

  return 0;
}

//3a04
int sceKernelLoadExecutableObject(u32 *buf, SceLoadCoreExecFileInfo *execFileInfo)
{
  if((buf[0] == 0x4543537E) ? buf[1] : 0)
  {
    buf += 0x40;
  }

  switch(execFileInfo->unk_20 + 1)
  {
    case 2:
    case 3:
    {
      loadrelelf((ElfHeader *)buf, execFileInfo);

      return 0;
    }

    case 4:
    {
      loadelf((ElfHeader *)buf, execFileInfo);

      return 0;
    }

    default:
    {
      Kprintf("loadelf.c:%s:Unsupported type\n", __FUNCTION__);

      return -1;
    }
  }
}

//3aa4
loadelf(ElfHeader *elf, SceLoadCoreExecFileInfo *execFileInfo)
{
  SceUID memuid;
  u32 *mem;
  int i, j, minaddr, maxaddr;

  ElfProgramHeader *ph = (ElfProgramHeader *)(elf + elf->phoff);

  for(i = elf->phnum; i > 0; i--)
  {
    if(ph[elf->phnum - i].type == 1)
    {
      break;
    }
  }

  execFileInfo->segmentaddr[0] = execFileInfo->unk_24;
  execFileInfo->segmentsize[0] = ph[elf->phnum - a1]->filesz;
  j = 1;
  if(i > 1 && ph[elf->phnum - i + 1].type == 1)
  {
    for(; j < i && ph[elf->phnum - i + j].type == 1; j++)
    {
      execFileInfo->segmentaddr[j] = execFileInfo->unk_24 + ph[elf->phnum - i + j].vaddr;
      execFileInfo->segmentsize[j] = ph[elf->phnum - i + j].filesz;
    }
  }

  execFileInfo->unk_84 = j;
  if(execFileInfo->unk_54 != 0)
  {
    memuid = sceKernelAllocPartitionMemory(execFileInfo->unk_40, "SceLoadelfOverlap", 0x1, sizeof(ElfHeader) + elf->phnum * sizeof(ElfProgramHeader), 0);
    if(memuid < 0)
    {
      Kprintf("loadelf.c:%s:Cannot allocate memory: res=0x%08x\n", "loadelf", memuid);

      return memuid;
    }

    mem = (u32 *)sceKernelGetBlockHeadAddr(memuid);
    if(mem != elf)
    {
      if(mem < elf)
      {
        sceKernelMemmove(mem, elf, sizeof(ElfHeader));
      }
      else
      {
        for(j = sizeof(ElfHeader) / sizeof(int) - 1; j >=0; j--)
        {
          mem[j] = ((u32 *)elf)[j];
        }
      }
    }

    //will this add correctly?
    if(mem + sizeof(ElfHeader) != ph[elf->phnum - i])
    {
      if(mem + sizeof(ElfHeader) < ph[elf->phnum - i])
      {
        sceKernelMemmove(mem + sizeof(ElfHeader), ph[elf->phnum - i], elf->phnum * sizeof(ElfProgramHeader));
      }
      else
      {
        for(j = elf->phnum * sizeof(ElfProgramHeader) / sizeof(int) - 1; j >=0; j--)
        {
          mem[sizeof(ElfHeader) / sizeof(int) + j] = ((u32 *)(elf + sizeof(ElfHeader))[j];
        }
      }
    }

    elf = (ElfHeader *)mem;
    ph = (ElfProgramHeader *)(elf + elf->phoff);
  }
  else
  {
    minaddr = -1;
    maxaddr = 0;
    for(j = 0; j < elf->phnum; j++)
    {
      if(ph[j].type == 1)
      {
        if(ph[j].vaddr > 0x80000000)
        {
          Kprintf("Cannot load kernel mode ELF\n");

          return SCE_KERNEL_ERROR_ERROR;
        }

        if(minaddr == -1)
        {
          minaddr = ph[j].vaddr;
        }

        maxaddr = ph[j].vaddr + ph[j].memsz;
      }
    }

    execFileInfo->unk_A8 = sceKernelAllocPartitionMemory(execFileInfo->unk_40, "SceLoadElfBlock", 0x2, (((maxaddr + 0xFF) / 0x100) * 0x100) - (minaddr & 0xFFFFFF00), (minaddr & 0xFFFFFF00));
    if(execFileInfo->unk_A8 < 0)
    {
      Kprintf("loadelf.c:%s:Cannot allocate memory: res=0x%08x\n", __FUNCTION__, execFileInfo->unk_A8);

      return execFileInfo->unk_A8;
    }

    //why do this if we don't keep the return value?
    sceKernelGetBlockHeadAddr(execFileInfo->unk_A8);
  }

  for(j = 0; j < elf->phnum; j++)
  {
    if(ph[j].type == 1)
    {
      if(ph[j].vaddr != elf + ph[j].offset)
      {
        if(ph[j].vaddr < elf + ph[j].offset)
        {
          sceKernelMemmove(ph[j].vaddr, elf + ph[j].offset, ph[j].filesz);
        }
        else
        {
          for(i = ph[j].filesz / sizeof(int) - 1;; i >=0; i--)
          {
            ((u32 *)(ph[j].vaddr)[i] = ((u32 *)(elf + ph[j].offset)[i];
          }
        }
      }

      if(ph[j].filesz < ph[j].memsz)
      {
        sceKernelMemset32(ph[j].vaddr + ph[j].filesz, 0, ph[j].memsz - ph[j].filesz);
      }
    }
  }

  if(mem != 0)
  {
    sceKernelFreePartitionMemory(memuid);
  }

  return SCE_KERNEL_ERROR_OK;
}

//3e78
loadrelelf(u32 *buf, SceLoadCoreExecFileInfo *execFileInfo)
{
  a2 = var_7580;
  s7 = 0;
  s5 = buf;
  s0 = execFileInfo;
  sp_40 = 0;

  if(var_7580)
  {
    loadrelelf_new(buf, execFileInfo);
    return;
  }

  v0 = buf[0x1C / 4];
  s3 = buf;
  fp = 0;
  s1 = 0;
  if(v0 != 0)
  {
    s1 = buf + v0;
  }

  v0 = buf[0x20 / 4];
  if(v0 != 0)
  {
    fp = buf + v0;
  }

  v0 = execFileInfo->unk_28;
  a0 = -1;
  s4 = execFileInfo->unk_24;
  if(v0 != a0)
  {
    a1 = v0 + s4;
    execFileInfo->unk_28 = a1;
  }

  a3 = execFileInfo->unk_2C;
  v1 = execFileInfo->unk_70;
  a2 = a3 + s4;
  execFileInfo->unk_2C = a2;
  if(v1 != a0)
  {
    t0 = v1 + s4;
    execFileInfo->unk_70 = t0;
  }
  
  v0 = execFileInfo->unk_78;
  a0 = v0 + s4;
  if(v0 != a0)
  {
    execFileInfo->unk_78 = a0;
  }
  
  t2 = execFileInfo->unk_20;
  v0 = t2 + 1;
  switch(execFileInfo->unk_20 + 1)
  {
    case 2:
    case 3:
    {
      //3f54
      a2 = s1[0x30 / 4];
      v1 = s1[0x34 / 4];
      s2 = a2 < v1;
      a0 = a2;
      if(s2 == 0)
      {
        t0 = execFileInfo->unk_24;
      }
      else
      {
        t0 = execFileInfo->unk_24;
        s7 = v1 - a2;
        s6 = t0 + a2;
        sp_40 = s6;
      }
      
      t8 = s5 < t0;
      v1 = t0 + a0;
      t9 = s5 < v1;
      if(t8 == 0 || t9 != 0)
      {
        s3 = sp < s5;
        if(sp != s5)
        {
          a0 = s5 + 0x34;
          if(s3 != 0)
          {
            //4198
          }

          a3 = s5 < a0;
          a1 = sp + 0x34;
          if(a3 != 0)
          {
            //3fb0
            while(1 == 1)
            {
              a0 -= 4;
              v0 = 0x0(a0);
              a1 -= 4;
              a2 = s5 < a0;
              0x0(a1) = v0;
              if(a2 == 0)
              {
                break;
              }
            }
            
            a2 = s1[0x30 / 4];
          }
        }
        
        //3fcc
        s3 = sp;
        t1 = s1[0x24 / 4];
      }
      
      //3fd4
      a0 = execFileInfo->unk_4C;
      a1 = t0 - t1;
      s1 = a1 + a0;
      a1 = s5 + t1;
      execFileInfo->unk_50 = s1;
      if(t0 != a1)
      {
        s0 = t0 < a1;
        t4 = a2 / 4;
        if(s0 != 0)
        {
          a0 = t0;
          SysMemForKernel_1C4B1713(...);
          a1 = buf[0x30 / 4]; //hw
        }
        else
        {
          t3 = t4 * 4;
          a3 = a1 + t3;
          t2 = a1 < a3;
          a0 = t0 + t3;
          if(t2 != 0)
          {
            while(1 == 1)
            {
              a3 -= 4;
              t5 = 0x0(a3);
              a0 -= 4;
              t0 = a1 < a3;
              0x0(a0) = t5;
              if(t0 == 0)
              {
                break;
              }
            }
          }
        }
      }

      //4024
      a1 = 0x30(s3); //hw
      s1 = 1;
      v1 = a1 & 0xFFFF;
      t9 = s1 < v1;
      a3 = 0x70000000;
      if(t9 != 0)
      {
        s2 = 0x700000A0;
        s0 = fp + 0x28;
        s6 = 9;
        while(1 == 1)
        {
          a0 = 0x4(s0);
          v0 = a0 ^ 0x9;
          t3 = a0 ^ s2;
          t1 = 0 < v0;
          t2 = 0 < t3;
          a2 = t1 & t2;
          if(a2 == 0)
          {
            //40cc
            t0 = -0x20(s0);
            t4 = t0 & 0x2;
            if(t4 != 0)
            {
              if(a0 == s2)
              {
                t9 = 0x24(s0);
                //413c
                v1 = 0x14(s0);
                t8 = 0x10(s0);
                //divu       $v1, $t9
                a3 = v1 / t9;
                t7 = 0x1C(s0);
                a2 = t8 + s5;
                t6 = t7 * 4;
                t5 = t6 + t7;
                a0 = t5 * 8;
                a1 = a0 + fp;
                t1 = 0x14(a1);
                t0 = 0xC(a1);
                a0 = s4;
                a1 = s4;
                sceKernelApplyPspRelSection(...);
                a0 = 0x4(s0);
              }

              if(a0 == s6)
              {
                a2 = 0x24(s0);
                //40f0
                a1 = 0x14(s0);
                t0 = 0x10(s0);
                //divu a1, a2;
                a3 = a1 / a2;
                v0 = 0x1C(s0);
                a2 = t0 + s5;
                a0 = s4;
                t4 = v0 * 4;
                t3 = t4 + v0;
                t1 = t3 * 8;
                t2 = t1 + fp;
                t1 = 0x14(t2);
                t0 = 0xC(t2);
                a1 = s4;
                sceKernelApplyElfRelSection(...);
              }

              a1 = 0x30(s3); //hw
            }
          }

          //4068
          s1++;
          a0 = a1 & 0xFFFF;
          a3 = s1 < a0;
          s0 += 0x28;
          if(a3 == 0)
          {
            break;
          }
        }
      }

      //407c
      a0 = sp_40;
      if(s7 != 0)
      {
        a2 = s7;
        a1 = 0;
        SysMemForKernel_2F808748(...);
      }

      return 0;
    }

    default:
    {
      //41b8
      ASSERT_MSG(1 == 2, "loadelf.c:%s:Unsupported type\n", __FUNCTION__);
      break;
    }
  }
}

//4210
int sub_00004210(u32 *buf, SceLoadCoreExecFileInfo *execFileInfo)
{
  v1 = buf & 0x3F;
  s1 = buf;
  s0 = execFileInfo;

  a0 = 0;
  execFileInfo->unk_48 = 0;
  if(buf % 0x40)
  {
    return SCE_KERNEL_ERROR_OK;
  }

  a1 = buf[0x0 / 4];
  if(a1 != 0x5053507E) //~PSP
  {
    execFileInfo->unk_10 = 0;
    return SCE_KERNEL_ERROR_OK;
  }

  PspHeader *hdr = (PspHeader *)buf;

  execFileInfo->unk_10 = buf[0x2C / 4];
  execFileInfo->unk_58 = buf[0x4 / 4] & 0xFFFF;
  execFileInfo->unk_5C = buf[0x28 / 4];
  execFileInfo->unk_10 = buf[0x2C / 4]; //why again??
  execFileInfo->unk_5A = buf[0x4 / 4] >> 16;
  execFileInfo->unk_28 = buf[0x30 / 4];
  if(buf[0x34 / 4] < 0)
  {
    execFileInfo->unk_44 = 0;
    execFileInfo->unk_4C = buf[0x34 / 4] & ~0x80000000;
  }
  else
  {
    execFileInfo->unk_44 = 1;
    execFileInfo->unk_4C = buf[0x34 / 4];
  }

  execFileInfo->unk_3C = buf[0x38 / 4];
  if(execFileInfo->unk_4 & 0x1)
  {
    execFileInfo->unk_20 = (execFileInfo->unk_5A & 0x2) ? 3 : 1;
    //42d8
    execFileInfo->unk_30 = 0;
    execFileInfo->unk_24 = buf[0x44 / 4];
    a3 = 0x27(buf); //b
    //seb a2, a3
    t1 = 0;
    if(a2 > 0)
    {
      t0 = buf[0x44 / 4];
      a1 = buf;
      a0 = buf[0x44 / 4];
      while(1 == 1)
      {
        t3 = a0 < t0;
        t1++;
        if(t3 != 0)
        {
          execFileInfo->unk_24 = a0;
          t0 = a0;
          a3 = 0x27(buf); //b
          a0 = 0x44(a1);
        }

        t5 = 0x54(a1);
        //seb v0, a3
        a2 = t1 < v0;
        v0 = a0 + t5;
        t4 = t2 < v0;
        a1 += 4;
        if(t4 != 0)
        {
          t2 = v0;
        }

        if(a2 == 0)
        {
          break;
        }

        a0 = 0x44(a1);
      }
    }
    else
    {
      t0 = execFileInfo->unk_24;
    }

    //434c
    s1 = t2 - t0;
    a0 = 0;
    execFileInfo->unk_30 = s1;
  }
  else
  {
    //4364
    if(!(execFileInfo->unk_5A & 0x1) || execFileInfo->unk_24 != 0)
    {
      a0 = buf;
      a1 = execFileInfo;
      //440c
      a2 = &sp_0;
      v1 = v0 = sub_00004490(...);
      if(v0 < 0)
      {
        return v0;
      }

      if(v0 == SCE_KERNEL_ERROR_LIBRARY_NOT_YET_LINKED)
      {
        a1 = 0;
        //4444
        v0 = var_75B4;
        if(v0 == 0)
        {
          return 0;
        }

        v0 = v0(buf, execFileInfo->unk_10, &sp_0);
        if(v0 != 0)
        {
          execFileInfo->unk_48 = 0;
          return v0;
        }

        execFileInfo->unk_48 = 1;
        return v0;
      }
      else if(v0 == 0)
      {
        return SCE_KERNEL_ERROR_OK;
      }
      else
      {
        return SCE_KERNEL_ERROR_LIBRARY_NOT_YET_LINKED;
      }
    }
    else
    {
      execFileInfo->unk_20 = (execFileInfo->unk_5A & 0x2) ? 3 : 1;
      execFileInfo->unk_30 = 0;
      execFileInfo->unk_24 = buf[0x44 / 4];
      a3 = 0x27(buf); //b
      //seb t6, a3
      t2 = 0;
      t1 = 0;
      if(t6 > 0)
      {
        t0 = v1; //buf[0x44 / 4]
        a1 = s1;
        a0 = 0x44(a1);
        while(1 == 1)
        {
          t1++;
          execFileInfo->unk_24 = MIN(execFileInfo->unk_24, 0x44(a1));

          a1 += 4;
          t2 = MAX(t2, 0x44(a1) + 0x54(a1));

          if(t1 >= 0x27(buf))
          {
            break;
          }

          a0 = 0x44(a1);
        }
      }
      else
      {
        t0 = execFileInfo->unk_24;
      }

      //434C
      s1 = t2 - t0;
      a0 = 0;
      execFileInfo->unk_30 = s1;
    }
  }

  //4250
  return a0;
}

//4490
int sub_00004490(u32 *buf, SceLoadCoreExecFileInfo *execFileInfo, void *unk_a2)
{
  int ret = 0;

  a0 = 0x120;
  v1 = execFileInfo->unk_8;
  if(v1 == a0)
  {
    t5 = buf[0x4 / 4] & 0xFFFF;
    v1 = t5 & 0x1E00;
    if(v1 != 0)
    {
      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
    }

    a1 = 0x7C(buf); //b
    v1 = a1 & 0xFF;
    v0 = 9;
    if(v1 != v0)
    {
      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
    }

    //->4530
  }
  else
  {
    a0 = v1 < 0x121;
    t8 = 0x140;
    if(a0 == 0)
    {
      //48ec
      t9 = v1 < 0x141;
      if(v1 == t8)
      {
        //4a84
        t9 = buf[0x4 / 4] & 0xFFFF;
        t6 = 0x800;
        t7 = t9 & 0x1E00;
        if(t7 != t6)
        {
          return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
        }
        
        a1 = 0x7C(buf); //b
        v0 = 0xC;
        v1 = a1 & 0xFF;
        if(v1 != v0)
        {
          return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
        }
      }
      else
      {
        t0 = 0x142;
        if(t9 == 0)
        {
          //49e0
          t1 = v1 < 0x142;
          if(v1 == t0)
          {
            //4a58
            t3 = buf[0x4 / 4] & 0xFFFF;
            t2 = t3 & 0x1E00;
            if(t2 != 0)
            {
              return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
            }
            
            a1 = 0x7C(buf); //b
            v1 = a1 & 0xFF;
            v0 = 0xE;
            if(v1 != v0)
            {
              return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
            }
          }
          else
          {
            if(t1 != 0)
            {
              a3 = buf[0x4 / 4] & 0xFFFF;
              //4a30
              a1 = a3 & 0x1E00;
              if(a1 != 0)
              {
                return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
              }
              
              a1 = 0x7C(buf); //b
              v1 = a1 & 0xFF;
              v0 = 0xD;
              if(v1 != v0)
              {
                return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
              }
            }
            else
            {
              t2 = 0x210;
              t3 = 0x220;
              if(v1 == t2)
              {
                //4a0c
                a0 = buf[0x4 / 4] & 0xFFFF;
                v1 = a0 & 0x1E00;
                if(v1 == 0)
                {
                  a1 = 0x7C(buf); //b
                  //47b8
                  v1 = a1 & 0xFF;
                  if(v1 != 0)
                  {
                    v0 = 0x4;
                    if(v1 != v0)
                    {
                      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                    }
                  }
                }
                else
                {
                  t6 = 0x800;
                  if(v1 == t6)
                  {
                    a1 = 0x7C(buf); //b
                    //4800
                    v0 = 0x3;
                    if(v1 != v0)
                    {
                      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                    }
                  }
                  else
                  {
                    return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                  }
                }
              }
              else
              {
                if(v1 != t3)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
                else
                {
                  //4990
                  t7 = 0x800;
                  v0 = t9 & 0x1E00;
                  if(v0 != t7)
                  {
                    return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                  }
                  else
                  {
                    a1 = 0x7C(buf); //b
                    v1 = a1 & 0xFF;
                    v0 = 0x3;
                    if(v1 != v0)
                    {
                      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                    }
                  }
                }
              }
            }
          }
        }
        else
        {
          a0 = 0x122;
          a1 = v1 < 0x122;
          if(v1 == a0)
          {
            //49ac
            t7 = buf[0x4 / 4] & 0xFFFF;
            t6 = t7 & 0x1E00;
            if(t6 != 0)
            {
              return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
            }
            
            a1 = 0x7C(buf); //b
            v1 = a1 & 0xFF;
            if(v1 != 0)
            {
              v0 = 0x9;
              if(v1 != v0)
              {
                return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
              }
            }
          }
          else
          {
            if(a1 != 0)
            {
              t9 = buf[0x4 / 4] & 0xFFFF;
              //4990
              t7 = 0x800;
              v0 = t9 & 0x1E00;
              if(v0 != t7)
              {
                return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
              }

              a1 = 0x7C(buf);
              v1 = a1 & 0xFF;
              v0 = 0x3;
              if(v1 != v0)
              {
                return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
              }
            }
            else
            {
              a2 = 0x130;
              a3 = 0x131;
              if(v1 == a2)
              {
                //4960
                a0 = buf[0x4 / 4] & 0xFFFF;
                t9 = 0x400;
                a1 = a0 & 0x1E00;
                if(a1 != t9)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
                
                a1 = 0x7C(buf); //b
                v1 = a1 & 0xFF;
                v0 = 0xA;
                if(v1 != v0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
              }
              else
              {
                if(v1 == a3)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
                else
                {
                  t3 = buf[0x4 / 4] & 0xFFFF;
                  t0 = 0x400;
                  t1 = t3 & 0x1E00;
                  if(t1 != t0)
                  {
                    return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                  }

                  a1 = 0x7C(buf); //b
                  t5 = a1 & 0xFF;
                  v1 = t5 - 0xA;
                  t4 = v1 < 0x2;
                  if(t4 == 0)
                  {
                    return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                  }
                }
              }
            }
          }
        }
      }
    }
    else
    {
      a1 = 0x21;
      a2 = v1 < 0x22;
      if(v1 == a1)
      {
        //48c0
        a1 = buf[0x4 / 4] & 0xFFFF;
        t9 = 0x800;
        a0 = a1 & 0x1E00;
        a2 = a0 < 0x801;
        if(a0 == t9)
        {
          a1 = 0x7C(buf); //b
          v1 = a1 & 0xFF;
          v0 = 0x3;
          if(v1 != v0)
          {
            return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
          }
        }
        else
        {
          a3 = 0x1000;
          if(a2 == 0)
          {
            //47dc
            if(a0 != a3)
            {
              return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
            }
            
            a1 = 0x7C(buf); //b
            v1 = a1 & 0xFF;
            v0 = 0x1;
            if(v1 != v0)
            {
              return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
            }
          }
          else
          {
            if(a0 != 0)
            {
              return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
            }

            a1 = 0x7C(buf); //b
            v1 = a1 & 0xFF;
            v0 = 0x4;
            if(v1 != v0)
            {
              return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
            }
          }
        }
      }
      else
      {
        t3 = 0x50;
        if(a2 == 0)
        {
          //4860
          t4 = v1 < 0x51;
          if(v1 == t3)
          {
            //48c0
            a1 = buf[0x4 / 4] & 0xFFFF;
            t9 = 0x800;
            a0 = a1 & 0x1E00;
            a2 = a0 < 0x801;
            if(a0 == t9)
            {
              //47fc
              a1 = 0x7C(buf); //b
              v1 = a1 & 0xFF;
              v0 = 0x3;
              if(v1 != v0)
              {
                return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
              }
            }
            else
            {
              a3 = 0x1000;
              if(a2 == 0)
              {
                //47dc
                if(a0 != a3)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
                
                a1 = 0x7C(buf); //b
                v1 = a1 & 0xFF;
                v0 = 0x1;
                if(v1 != v0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
              }
              else
              {
                if(a0 != 0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }

                a1 = 0x7C(buf); //b
                //483c
                v1 = a1 & 0xFF;
                v0 = 0x4;
                if(v1 != v0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
              }
            }
          }
          else
          {
            t6 = 0x100;
            if(t4 == 0)
            {
              //4884
              t7 = 0x110;
              if(v1 != t6)
              {
                if(v1 != t7)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }

                t2 = buf[0x4 / 4] & 0xFFFF;
                t1 = t2 & 0x1E00;
                if(t1 != 0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }

                a1 = 0x7C(buf); //b
                if(a1 != 0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
              }
            }
            else
            {
              t5 = 0x30;
              if(v1 == t5)
              {
                return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
              }

              t6 = buf[0x4 / 4] & 0xFFFF;
              v1 = t6 & 0x1E00;
              if(v1 != 0)
              {
                t7 = 0x1000;
                if(v1 != t7)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }

                a1 = 0x7C(buf); //b
                v1 = a1 & 0xFF;
                v0 = 0x1;
                if(v1 != v0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
              }
              else
              {
                //483c
                v1 = a1 & 0xFF;
                v0 = 0x4;
                if(v1 != v0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
              }
            }
          }
        }
        else
        {
          a3 = 0x10;
          t0 = v1 < 0x11;
          if(v1 == a3)
          {
            //4848
            t4 = buf[0x4 / 4] & 0xFFFF;
            v1 = t4 & 0x1E00;
            if(v1 == 0)
            {
              //47b8
              v1 = a1 & 0xFF;
              if(v1 != 0)
              {
                v0 = 0x4;
                if(v1 != v0)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
              }
            }
          }
          else
          {
            t1 = 0x11;
            if(t0 == 0)
            {
              //4780
              t2 = 0x20;
              if(v1 == t1)
              {
                //4814
              }
              else
              {
                if(v1 != t2)
                {
                  return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                }
                
                v1 = buf[0x4 / 4] & 0xFFFF;
                v0 = 0x800;
                a0 = v1 & 0x1E00;
                t5 = a0 < 0x801;
                if(a0 == v0)
                {
                  //47fc
                  a1 = 0x7C(buf); //b
                  v1 = a1 & 0xFF;
                  v0 = 0x3;
                  if(v1 != v0)
                  {
                    return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                  }
                }
                else
                {
                  a3 = 0x1000;
                  if(t5 == 0)
                  {
                    //47dc
                    if(a0 != a3)
                    {
                      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                    }

                    a1 = 0x7C(buf); //b
                    v1 = a1 & 0xFF;
                    v0 = 0x1;
                    if(v1 != v0)
                    {
                      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                    }
                  }
                  else
                  {
                    if(a0 != 0)
                    {
                      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                    }

                    a1 = 0x7C(buf); //b
                    v1 = a1 & 0xFF;
                    v0 = 0x4;
                    if(v1 != v0)
                    {
                      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
                    }
                  }
                }
              }
            }
            else
            {
              if(v1 != 0)
              {
                return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
              }
            }
          }
        }
      }
    }
  }

  int do_4588 = 1;
  switch(0x7C(buf))
  {
    case 0:
    {
      do_4588 = 0;
      break;
    }

    case 1:
    {
      if(((buf[0x4 / 4] & 0xFFFF) & 0x1E00) != 0x1000)
      {
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      if(var_75B4 != 0)
      {
        ret = var_75B4(buf, execFileInfo->unk_10, unk_a2);
      }
      else
      {
        ret = sceUtilsGetLoadModuleABLength(buf, execFileInfo->unk_10, unk_a2);
        if(ret != 0)
        {
          execFileInfo->unk_48 = 0;

          ret = sceUtilsGetLoadModuleABLengthByPolling(buf, execFileInfo->unk_10, unk_a2);
          if(ret < 0)
          {
            return ret;
          }
        }
        else
        {
          execFileInfo->unk_48 = 1;
        }

        do_4588 = 0;
      }

      break;
    }

    case 3:
    {
      if(((buf[0x4 / 4] & 0xFFFF) & 0x1E00) != 0x800)
      {
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      ret = sceMesgLed_driver_84A04017(buf, execFileInfo->unk_10, unk_a2);
      break;
    }

    case 4:
    {
      if((buf[0x4 / 4] & 0xFFFF) & 0x1E00)
      {
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      ret = sceMesgLed_driver_A4547DF1(buf, execFileInfo->unk_10, unk_a2);
      break;
    }

    case 9:
    {
      if((buf[0x4 / 4] & 0xFFFF) & 0x1E00)
      {
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      ret = sceMesgLed_driver_198FD3BE(buf, execFileInfo->unk_10, unk_a2);
      break;
    }

    case 10:
    {
      if(((buf[0x4 / 4] & 0xFFFF) & 0x1E00) != 0x400)
      {
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      ret = sceMesgLed_driver_07E152BE(buf, execFileInfo->unk_10, unk_a2);
      break;
    }

    case 12:
    {
      if(((buf[0x4 / 4] & 0xFFFF) & 0x1E00) != 0x800)
      {
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      ret = sceMesgLed_driver_67A5ECDF(buf, execFileInfo->unk_10, unk_a2);
      break;
    }

    case 13:
    {
      if((buf[0x4 / 4] & 0xFFFF) & 0x1E00)
      {
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      ret = sceMesgLed_driver_951F4A5B(buf, execFileInfo->unk_10, unk_a2);
      break;
    }

    case 14:
    {
      if((buf[0x4 / 4] & 0xFFFF) & 0x1E00)
      {
        return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
      }

      ret = sceMesgLed_driver_9FC926A0(buf, execFileInfo->unk_10, unk_a2);
      break;
    }

    default:
    {
      return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
    }
  }

  if(do_4588)
  {
    if(ret == 0)
    {
      execFileInfo->unk_48 = 1;
    }

    if(ret < 0)
    {
      return ret;
    }
  }

  if(!(execFileInfo->unk_5A & 0x1))
  {
    return ret;
  }

  if(execFileInfo->unk_24 == 0)
  {
    return ret;
  }

  if(execFileInfo->unk_24 == -1)
  {
    return SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
  }

  if(sub_00005890(execFileInfo->unk_24, execFileInfo->unk_5C, buf, &sp_0) > 0)
  {
    execFileInfo->unk_48 = 1;
  }

  return ret;
}
