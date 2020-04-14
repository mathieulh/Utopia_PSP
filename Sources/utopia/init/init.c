void *var_2B38[MAX_CHUNKS];
int var_2CA0; //api type
struct SceKernelLoadExecVSHParam var_2DD0; //should be 2DCC?
struct ??? *var_2E10; //array of init callbacks
struct ??? *var_2E14; //next free init callback

int module_bootstart(int arglen, void *argp) __attribute__((alias("InitInit")));

//0
void ReleaseAllChunks()
{
  for(i = MAX_CHUNKS - 1; i >= 0; i--)
  {
    var_2B38[i] = CHUNK_EMPTY; //w
  }
}

//28
void *sceKernelGetChunk(int chunkid)
{
  if(chunkid > 0xF)
  {
    Kprintf("chunk.c:%s:failed SCE_KERNEL_ERROR_ILLEGAL_CHUNK_ID\n", __FUNCTION__);

    return SCE_KERNEL_ERROR_ILLEGAL_CHUNK_ID;
  }

  return var_2B38[chunkid];
}

//7c
int sceKernelRegisterChunk(int chunkid, void *data)
{
  if(chunkid > 0xF)
  {
    Kprintf("chunk.c:%s:failed SCE_KERNEL_ERROR_ILLEGAL_CHUNK_ID\n", __FUNCTION__);

    return SCE_KERNEL_ERROR_ILLEGAL_CHUNK_ID;
  }

  var_2B38[chunkid] = data;
  
  return data;
}

//dc
int sceKernelReleaseChunk(int chunkid)
{
  if(chunkid > 0xF)
  {
    Kprintf("chunk.c:%s:failed SCE_KERNEL_ERROR_ILLEGAL_CHUNK_ID\n", __FUNCTION__);

    return SCE_KERNEL_ERROR_ILLEGAL_CHUNK_ID;
  }

  var_2B38[chunkid] = CHUNK_EMPTY;
  
  return CHUNK_EMPTY;
}

//13c
int read_file(char *name, void *buf, SceSize size)
{
  SceUID fd = sceIoOpen(name, PSP_O_RDONLY, 0xABCD);
  if(fd < 0)
  {
    Kprintf("init.c:%s:open error(%d), filename %s\n", __FUNCTION__, fd, name);

    return SCE_KERNEL_ERROR_ERROR;
  }

  sceIoLseek(fd, 0, PSP_SEEK_END);

  sceIoLseek(fd, 0, PSP_SEEK_SET);

  int total_bytes = 0, bytes;

  do
  {
    bytes = sceIoRead(fd, buf, 0x200);
    if(bytes < 0)
    {
      Kprintf("init.c:%s:size[error] = 0x%x\n", __FUNCTION__, bytes);
      break;
    }

    total_bytes += bytes;
    buf += 0x200;
  } while(total_bytes <= size && bytes == 0x200);

  sceIoClose(fd);

  return total_bytes;
}

//248
int load_start(unk_a0, char *file, unk_a2, unk_a3, unk_t0, SceSize args, void *argp)
{
  SceUID fd, modid;
  SceKernelLMOption lmopt;
  int status, ret;

  if(unk_a2 == 2 && unk_a0->unk_24 == -1)
  {
    modid = sceKernelLoadModule(file, 0, 0);

    sub_00001330();

    if(modid < 0)
    {
      Kprintf("init.c:%s:sceKernelLoadModule failed: 0x%08x\n", __FUNCTION__, modid); //__FUNCTION__ = load_start_anchor?
    }
    else
    {
      ret = sceKernelStartModule(modid, 0, 0, &status, 0);
      if(ret < 0)
      {
        Kprintf("init.c:%s:sceKernelLoadModule failed: 0x%08x\n", __FUNCTION__, ret); //__FUNCTION__ = load_start_anchor?
      }
    }

    ret = SCE_KERNEL_ERROR_OK;
  }
  else
  {
    if(unk_a3 == 0)
    {
      fd = sceIoOpen(file, PSP_O_RDONLY, 0xABCD);
      if(fd < 0)
      {
        Kprintf("init.c:%s:Cannot open file [%s]: 0x%x\n", __FUNCTION__, file, fd);
        return fd;
      }

      lmopt.size = sizeof(SceKernelLMOption);
      lmopt.position = 0;
      lmopt.access = 1;

      if(unk_a2 > 0)
      {
        lmopt.mpidtext = 2;
        lmopt.mpiddata = 2;
      }
      else
      {
        lmopt.mpidtext = 1;
        lmopt.mpiddata = 1;
      }

      modid = sceKernelLoadModuleByID(fd, 0, &lmopt);

      sceIoClose(fd);

      if(modid < 0)
      {
        Kprintf("init.c:%s:error result=0x%08x [%s]\n", __FUNCTION__, modid);

        return modid;
      }

      ret = sceKernelStartModule(modid, unk_t1, (unk_t1 > 0) ? unk_t2 : 0, &status, 0);
      if(ret < 0)
      {
        Kprintf("init.c:%s:sceKernelStartModule, modid=0x%08x, res=0x%x\n", __FUNCTION__, modid, ret);
      }
    }
    else if(unk_a3 == 1)
    {
      modid = sceKernelLoadModule(file, 0, 0);
      if(modid < 0)
      {
        Kprintf("init.c:%s:sceKernelLoadStartModuleByFileName failed 0x%08x\n", __FUNCTION__, modid);

        return modid;
      }

      ret = sceKernelStartModule(modid, unk_t1, (unk_t1 > 0) ? unk_t2 : 0, &status, 0);
    }
    else if(unk_a3 == 2)
    {
      fd = sceIoOpen(file, PSP_O_RDONLY, 0xABCD);
      if(fd < 0)
      {
        Kprintf("init.c:%s:open failed: fd 0x%08x\n", __FUNCTION__, fd);

        ret = fd;
      }
      else
      {
        int len = sceIoLseek(fd, 0, PSP_SEEK_END);

        sceIoLseek(fd, 0, PSP_SEEK_SET);

        if(unk_a2 > 0)
        {
          lmopt.mpidtext = unk_a3;
          lmopt.mpiddata = unk_a3;
        }
        else
        {
          lmopt.mpidtext = 1;
          lmopt.mpiddata = 1;
        }

        SceUID bufid = sceKernelAllocPartitionMemory(lmopt.mpidtext, "SceInitMode2Buffer", 1, len, 0); //s1
        if(bufid >= 0)
        {
          ret = bufid;
        }
        else
        {
          void *buf = sceKernelGetBlockHeadAddr(bufid);

          ret = sceIoRead(fd, buf, len);
          if(ret != len)
          {
            Kprintf("init.c:%s:sceIoRead failed 0x%x, name[%s]\n", __FUNCTION__, ret, file);

            sceKernelFreePartitionMemory(bufid);

            sceIoClose(fd);
          }
          else
          {
            modid = sceKernelLoadModuleBuffer(buf, 0, &lmopt);
            if(modid <= 0)
            {
              sceKernelFreePartitionMemory(bufid);

              sceIoClose(fd);
            }
            else
            {
              ret = sceKernelStartModule(modid, unk_t1, (unk_t1 > 0) ? unk_t2 : 0, &status, 0);
              if(ret < 0)
              {
                Kprintf("init.c:%s:sceKernelStartModule, modid=0x%08x, res=0x%x\n", __FUNCTION__, modid, ret);
              }

              sceKernelFreePartitionMemory(bufid);

              sceIoClose(fd);
            }
          }
        }
      }
    }
    else
    {
      Kprintf("init.c:%s:Unknown mode=%d\n", __FUNCTION__, unk_a3);

      return SCE_KERNEL_ERROR_ERROR;
    }
  }

  if(unk_t0 != 0 && modid > 0)
  {
    ret = sceKernelStopModule(modid, 0, 0, &status, 0);
    if(ret < 0)
    {
      Kprintf("init.c:%s:sceKernelStopModule failed 0x%08x\n", __FUNCTION__, ret);
    }

    ret = sceKernelUnloadModule(modid);
    if(ret < 0)
    {
      Kprintf("init.c:%s:sceKernelUnloadModule failed 0x%08x\n", __FUNCTION__, ret);
    }
  }

  return ret;
}

int InitInit(SceSize argSize, void *argp)
{
  SceLoadCoreBootInfo *bootinfo = (SceLoadCoreBootInfo *)argp;

  var_2CA0.unk_24 = 0;

  var_2CA0.unk_0 = 0;

  if(bootinfo->unk_38 != 0)
  {
    strncpy(var_2CCC, bootinfo->unk_38, 0x100);

    var_2CA0.unk_28 = 1;
  }
  else
  {
    var_2CA0.unk_28 = 0;
  }

  var_2E10 = 0;
  var_2E14 = 0;

  void args[2];
  args[0] = sceKernelGetThreadId();
  if(args[0] < 0)
  {
    Kprintf("init.c:%s:sceKernelGetThreadId failed 0x%08x\n", __FUNCTION__, args[0]);

    return args[0];
  }

  args[1] = argp;

  int ret = sceKernelStartThread(sceKernelCreateThread("SceKernelInitThread", sub_000006CC, 0x20, 0x4000, 0, 0), 0x8, args);
  if(ret != 0)
  {
    Kprintf("init.c:%s:sceKernelStartThread failed 0x%08x\n", __FUNCTION__, ret);
  }

  return 0;
}

//6cc
void InitThreadEntry(SceSize argSize, void *argp)
{
  SceUID thid = *((SceUID *)argp);
  SceLoadCoreBootInfo *bootinfo = (SceLoadCoreBootInfo *)(argp + 4); //s4
  int fp_34 = 0;

  if(argSize < 8)
  {
    return;
  }

  sceKernelWaitThreadEnd(thid, 0);

  printf("devkit version 0x%08x\n", 0x1050001);

  if(bootinfo->unk_30 != 0)
  {
    printf("build version 0x%08x\n", bootinfo->unk_30);
  }

  ??? unk[bootinfo->unk_C]; //sp/var_2E10/var_2E14 - size of one is 8 bytes
  var_2E14 = unk;
  var_2E10 = unk;
  unk[0].unk_0 = 0;

  sub_00001BB4(...);

  if(bootinfo->unk_18 != 0)
  {
    if(bootinfo->unk_18 < 3)
    {
      var_2CA0 = bootinfo->unk_10[bootinfo->unk_C - 1].unk_14;
    }
    else
    {
      var_2CA0 = 0;
    }
  }
  else
  {
    var_2CA0 = 0x220;
  }

  if(argSize >= 4) //wtf??
  {
    if(bootinfo->unk_8 < bootinfo->unk_C)
    {
      PspSysmemPartitionInfo part; //fp_0
      part.size = sizeof(PspSysmemPartitionInfo);
      sceKernelQueryMemoryPartitionInfo(5, &part);

      for(s2 = bootinfo->unk_8; s2 < bootinfo->unk_C; s2++)
      {
        if(bootinfo->unk_10[s2].unk_10 & 0x1)
        {
          s5 = s2;
          break;
        }
      }

      if(s5 < bootinfo->unk_C)
      {
        s0 = part.startaddr & 0x1FFFFFFF;
        for(s2 = s5; s2 < bootinfo->unk_C; s2++)
        {
          PspSysmemPartitionInfo part6;
          part6.size = sizeof(PspSysmemPartitionInfo);
          sceKernelQueryMemoryPartitionInfo(6, &part6);

          if(((bootinfo->unk_10[s2].unk_4 + bootinfo->unk_10[s2].unk_8) & 0x1FFFFFFF) >= part6.startaddr)
          {
            memmove(s0, bootinfo->unk_10[s2].unk_4, bootinfo->unk_10[s2].unk_8);

            bootinfo->unk_10[s2].unk_4 = s0;
            s0 = (s0 + bootinfo->unk_10[s2].unk_8 + 0x3F) / 0x40 * 0x40;
          }
        }
      }

      for(s2 = bootinfo->unk_C - 1; s2 >= s5; s2--)
      {
        if(bootinfo->unk_10[s2].unk_10 & 0x2)
        {
          fp_34 = 1;
          if(bootinfo->unk_24 != -1)
          {
            bootinfo->unk_10[s2].unk_8 = bootinfo->unk_20[bootinfo->unk_24].unk_4;
            bootinfo->unk_10[s2].unk_4 = bootinfo->unk_20[bootinfo->unk_24].unk_0;
            if(bootinfo->unk_20[bootinfo->unk_24].unk_8 & 0x2)
            {
              bootinfo->unk_10[s2].unk_10 |= 0x4;
              var_2CC4 = bootinfo->unk_20[bootinfo->unk_24].unk_0;
            }
          }

          if(bootinfo->unk_28 != -1)
          {
            bootinfo->unk_10[s2].unk_1C = bootinfo->unk_20[bootinfo->unk_28].unk_C;
            bootinfo->unk_10[s2].unk_18 = bootinfo->unk_20[bootinfo->unk_28].unk_4;
          }
        }

        PspSysmemPartitionInfo part6; //fp_20
        part6.size = sizeof(PspSysmemPartitionInfo);
        sceKernelQueryMemoryPartitionInfo(6, &part6);

        if(((bootinfo->unk_10[s2].unk_4 + bootinfo->unk_10[s2].unk_8) & 0x1FFFFFFF) >= part6.startaddr)
        {
          SceUID bkpid = sceKernelAllocPartitionMemory(2, "backup area", 1, bootinfo->unk_10[s2].unk_8, 0);
          int bkpaddr = sceKernelGetBlockHeadAddr(bkpid);
          memmove(bkpaddr, bootinfo->unk_10[s2].unk_4, bootinfo->unk_10[s2].unk_8);

          bootinfo->unk_10[s2].unk_14 = bkpid;
          bootinfo->unk_10[s2].unk_4 = bkpaddr;
        }
        else
        {
          bootinfo->unk_10[s2].unk_14 = 0;
        }
      }
      }

      for(a0 = bootinfo->unk_8; a0 < bootinfo->unk_C; a0++)
      {
        if(bootinfo->unk_10[a0].unk_14 != 0)
        {
          sceKernelFreePartitionMemory(bootinfo->unk_10[a0].unk_14);
        }

        if(bootinfo->unk_10[a0].unk_10 & 0x2)
        {
          memmove(&var_2CA4, bootinfo->unk_10[a0], sizeof(SceLoadCoreBootModuleInfo));

          sub_00001474(bootinfo);

          SceUID chunkid = sceKernelGetChunk(0);
          u32 *chunkaddr;

          if(chunkid > 0)
          {
            chunkaddr = sceKernelGetBlockHeadAddr(chunkid); //s0/v0
            if(chunkaddr[0x14 / 4] != 0)
            {
              Kprintf("init.c:%s:ExitCheck failed 0x%08x\n", "ExitCheck", chunkaddr[0x14 / 4]);
              var_2DCC = -1;

              chunkid = sceKernelGetChunk(0);

              chunkaddr = &var_2DF0;
              if(chunkid > 0)
              {
                chunkaddr = sceKernelGetBlockHeadAddr(chunkid); //a1/v0
              }
              else
              {
                var_2DF0.unk_0 = 0x20;
                var_2DF0.unk_4 = 0x20;
                var_2DF0.unk_8 = 0;
                var_2DF0.unk_10 = 0;
              }

              chunkaddr[0x18 / 4] = chunkaddr[0x14 / 4];

              var_2DD0.args = chunkaddr[0x0 / 4];
              var_2DD0.vshmain_args = chunkaddr;
              var_2DD0.vshmain_args_size = chunkaddr[0x0 / 4];
              var_2DD0.argp = chunkaddr;
              var_2DD0.key = 0;
              var_2DD0.configfile = "/kd/pspbtcnf.txt";
              var_2DD0.unk4 = 0;

              sceKernelExitVSHVSH(&var_2DD0);
            }
          }

          if(var_2CB4 & 0x4)
          {
            modid = sceKernelLoadModuleBufferWithApitype(var_2CA8, 0, var_2CA0, 0);
          }
          else
          {
            modid = sceKernelLoadModuleWithApitype(var_2CA8, 0, var_2CA0, 0);
          }

          if(ret < 0)
          {
            Kprintf("init.c:%s:LoadModule anchor failed 0x%08x\n", __FUNCTION__, ret);

            var_2DCC = -1;
            chunkid = sceKernelGetChunk(0);

            chunkaddr = &var_2DCC + 0x24;
            if(chunkid > 0)
            {
              chunkaddr = sceKernelGetBlockHeadAddr(chunkid); //a1/v0
            }
            else
            {
              var_2DF0.unk_0 = 0x20;
              var_2DF0.unk_4 = 0x20;
              var_2DF0.unk_8 = 0;
              var_2DF0.unk_10 = 0;
            }
          }
        }
        else if(bootinfo->unk_10[a0].unk_10 & 0x4)
        {
          modid = sceKernelLoadModuleWithApitype(bootinfo->unk_10[a0].unk_4, 0, 0x50, 0);
        }
        else
        {
          modid = sceKernelLoadModuleBufferWithApitype(bootinfo->unk_10[a0].unk_4, 0, 0x50, 0); //s1
        }

        if(bootinfo->unk_10[a0].unk_10 & 0x2)
        {
          ret = sceKernelFillFreeBlock(1, 1);
          if(ret < 0)
          {
            Kprintf("init.c:%s:sceKernelFillFreeBlock(0x%x) failed 0x%08x\n", "ClearFreeBlock", 1, ret);
          }

          ret = sceKernelFillFreeBlock(5, 0);
          if(ret < 0)
          {
            Kprintf("init.c:%s:sceKernelFillFreeBlock(0x%x) failed 0x%08x\n", "ClearFreeBlock", 5, ret);
          }

          sceSuspendForKernel_A569E425(0);

          ret = sceKernelFillFreeBlock(2, 0);
          if(ret < 0)
          {
            Kprintf("init.c:%s:sceKernelFillFreeBlock(0x%x) failed 0x%08x\n", "ClearFreeBlock", 2, ret);
          }

          memset(0x80010000, 0, 0x4000);

          if(ret < 0)
          {
            Kprintf("init.c:%s:ClearFreeBlock failed 0x%08x\n", __FUNCTION__, ret);
          }

          sub_00001330();
        }

        if(loadret >= 0)
        {
          if(bootinfo->unk_10[a0].unk_18 != 0 && bootinfo->unk_10[a0].unk_1C != 0)
          {
            void *argp = sceKernelGetBlockHeadAddr(bootinfo->unk_10[a0].unk_1C); //a2

            startret = sceKernelStartModule(modid, bootinfo->unk_10[a0].unk_18, argp, &fp_30, 0);
          }
          else
          {
            startret = sceKernelStartModule(modid, 0, 0, &fp_30, 0); //s1
          }

          if(startret < 0)
          {
            Kprintf("init.c:%s:sceKernelStartModule failed: 0x%08x\n", __FUNCTION__, startret);

            if(bootinfo->unk_10[a0].unk_10 & 0x2)
            {
              var_2DCC = -1;
              chunkid = sceKernelGetChunk(0);

              chunkaddr = &var_2DF0;
              if(chunkid > 0)
              {
                chunkaddr = sceKernelGetBlockHeadAddr(chunkid); //a1
              }
              else
              {
                var_2DF0.unk_0 = 0x20;
                var_2DF0.unk_4 = 0x20;
                var_2DF0.unk_8 = 0;
                var_2DF0.unk_10 = 0;
              }

              chunkaddr[0x18 / 4] = startret;

              var_2DD0.args = chunkaddr[0x0 / 4];
              var_2DD0.vshmain_args = chunkaddr;
              var_2DD0.vshmain_args_size = chunkaddr[0x0 / 4];
              var_2DD0.argp = chunkaddr;
              var_2DD0.key = 0;
              var_2DD0.configfile = "/kd/pspbtcnf.txt";
              var_2DD0.unk4 = 0;

              sceKernelExitVSHVSH(&var_2DD0);
            }
          }
        }
        else
        {
          Kprintf("init.c:%s:sceKernelLoadModule failed: 0x%08x\n", __FUNCTION__, s1);
        }

        if(bootinfo->unk_10[a0].unk_1C != 0)
        {
          sceKernelFreePartitionMemory(bootinfo->unk_10[a0].unk_1C);
        }

        bootinfo->unk_8++;
        if(bootinfo->unk_10[a0].unk_10 & 0x2)
        {
          goto loc_00000808;
          //break;
        }
      }

      if(fp_34 != 0)
      {
        goto loc_00000808;
      }
    }
  }

  if(var_2CC8 != 0)
  {
    bootinfo->unk_24 = -1;

    sub_00001784(bootinfo);
  }

loc_00000808:
  sub_00001474(bootinfo);

  sub_000015C8(bootinfo);

  if(fp_34 == 0)
  {
    ret = sceKernelFillFreeBlock(1, 0);
    if(ret < 0)
    {
      Kprintf("init.c:%s:sceKernelFillFreeBlock(0x%x) failed 0x%08x\n", __FUNCTION__, 1, ret);
    }

    ret = sceKernelFillFreeBlock(5, 0);
    if(ret < 0)
    {
      Kprintf("init.c:%s:sceKernelFillFreeBlock(0x%x) failed 0x%08x\n", __FUNCTION__, 5, ret);
    }

    sceSuspendForKernel_A569E425(0);

    ret = sceKernelFillFreeBlock(2, 0); //s0
    if(ret < 0)
    {
      Kprintf("init.c:%s:sceKernelFillFreeBlock(0x%x) failed 0x%08x\n", __FUNCTION__, 2, ret);
    }

    memset(0x80010000, 0, 0x4000);

    if(ret < 0)
    {
      Kprintf("init.c:%s:ClearFreeBlock failed 0x%08x\n", __FUNCTION__, ret);
    }
  }

  printf("Loading all modules ... Ready\n");

  v0 = KDebugForKernel_B7251823();
  if(v0 != 0)
  {
    v0->unk_8(2);
  }

  sceKernelExitDeleteThread(0);
}

//1078
int loc_00001078(int *unk_a0)
{
  return (*unk_a0 & ~0x3);
}

//1088
int load_start_anchor(char *path)
{
  SceUID modid = sceKernelLoadModule(path, 0, 0);

  sub_00001330();

  if(modid < 0)
  {
    Kprintf("init.c:%s:sceKernelLoadModule failed: 0x%08x\n", __FUNCTION__, modid);

    return 0;
  }

  int status;
  int ret = sceKernelStartModule(modid, 0, 0, &status, 0);
  if(ret < 0)
  {
    Kprintf("init.c:%s:sceKernelLoadModule failed: 0x%08x\n", __FUNCTION__, modid);

    return 0;
  }

  return 0;
}

//122c
int module_reboot_before()
{
  return 0;
}

//1234
int sceKernelInitApitype()
{
  return var_2CA0;
}

//1240
int sceKernelBootFrom()
{
  switch(var_2CA0)
  {
    case 0x110:
    case 0x120:
    case 0x121:
    case 0x122:
    {
      return 0x20;
    }

    case 0x130:
    case 0x131:
    {
      return 0x30;
    }

    case 0x140:
    case 0x141:
    case 0x142:
    {
      return 0x40;
    }

    default:
    {
      return 0;
    }
  }
}

char *sceKernelInitFileName()
{
  return var_2CC4;
}

//1298
int sceKernelSetInitCallback(unk_a0, unk_a1, int *unk_a2)
{
  int ret;

  if(var_2E10 == 0)
  {
    ret = unk_a0(1, 0, 0);

    if(unk_a2 != 0)
    {
      *unk_a2 = ret;
    }

    return 0;
  }

  var_2E14->unk_0 = unk_a0 + (unk_a1 & 0x3);

  SceModule *mod = sceKernelFindModuleByAddress(unk_a0);
  var_2E14->unk_4 = mod ? mod->text_addr : 0;

  var_2E14 += 8;
  var_2E14->unk_0 = 0;

  return 1;
}

//1330
int PowerUnlock()
{
  for(i = 0; i < 4; i++)
  {
    for(j = 0; var_2E10[j].unk_0 != 0; j++)
    {
      if(!(var_2E10[j].unk_0 & 0x3))
      {
        (var_2E10[j].unk_0 & ~0x3)((i == 3) ? (&var_2E10[j] + 2) : var_2E10, (i == 3) ? 0 : 1, 0);
      }
    }

    if(i == 2)
    {
      for(; j >= 0; j--)
      {
        if((var_2E10[j].unk_0 & 0x3) == 0x3)
        {
          break;
        }

        var_2E10[j].unk_0 = 0;
      }
    }
  }

  SysMemForKernel_95F5E8DA(2);

  int ret = sceKernelPowerUnlock(0);
  if(ret < 0)
  {
    Kprintf("init.c:%s:sceKernelPowerUnlock failed 0x%08x\n", __FUNCTION__, ret);
  }

  return 0;
}

//1474
int CleanupPhase1(unk_a0)
{
  int ret;

  if(var_2B30 > 0)
  {
    return 0;
  }

  var_2B30++;

  for(i = 0; i < unk_a0->unk_1C; i++)
  {
    if((unk_a0->unk_20[i].unk_8 & 0xFFFF) == 0x200 && (unk_a0->unk_20[i].unk_8 & 0x10000))
    {
      sceKernelFreePartitionMemory(unk_a0->unk_20[i].unk_C);

      unk_a0->unk_20[i].unk_8 &= ~0x10000;
    }
  }

  if(unk_a0->unk_10 == 0)
  {
    return 0;
  }

  SceUID partid, blockid;
  ret = sceKernelQueryMemoryInfo(unk_a0->unk_10, &partid, &blockid);
  if(ret < 0)
  {
    Kprintf("init.c:%s:sceKernelQueryMemoryInfo failed 0x%08x\n", __FUNCTION__, ret);
    return ret;
  }

  ret = sceKernelFreePartitionMemory(blockid);
  if(ret < 0)
  {
    Kprintf("init.c:%s:sceKernelFreePartitionMemory failed: 0x%x\n", __FUNCTION__, ret);
    return 0;
  }

  return 0;
}

//15c8
int CleanupPhase2(SceLoadCoreBootInfo *pBoot)
{
  for(i = 0; i < pBoot->unk_1C; i++)
  {
    int attr = pBoot->unk_20[i].unk_8 & 0xFFFF;
    if(attr != 0x4 && (attr == 0x2 || (attr >= 0x4 && (attr == 0x100 || attr == 0x200))) && attr != 0)
    {
      sceKernelFreePartitionMemory(pBoot->unk_20[i].unk_C);

      pBoot->unk_20[i].unk_8 &= ~0x10000;
    }
  }
  
  SceUID partid, blockid;
  if(pBoot->unk_20 != 0)
  {
    ret = sceKernelQueryMemoryInfo(pBoot->unk_20, &partid, &blockid);
    if(ret < 0)
    {
      Kprintf("init.c:%s:sceKernelQueryMemoryInfo failed 0x%08x\n", __FUNCTION__, ret);
      return ret;
    }
    
    ret = sceKernelFreePartitionMemory(blockid);
    if(ret < 0)
    {
      Kprintf("init.c:%s:sceKernelFreePartitionMemory failed: 0x%x\n", __FUNCTION__, ret);
    }
  }
  
  sceKernelQueryMemoryInfo(pBoot, &partid, &blockid);
  ret = sceKernelFreePartitionMemory(blockid);
  if(ret < 0)
  {
    Kprintf("init.c:%s:clear pBoot failed: 0x%x\n", __FUNCTION__, ret);
  }
  
  return 0;
}

//1784
int InitConfig(SceLoadCoreBootInfo *pBoot)
{
  SceUID userconfig = sceKernelAllocPartitionMemory(1, "SceInitUserConfigArea", 1, 0x2000, 0);
  if(userconfig < 0)
  {
    Kprintf("init.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, userconfig);

    return userconfig;
  }

  int ucaddr = sceKernelGetBlockHeadAddr(userconfig); //s1/s0
  int ret = read_file(&var_2CCC, ucaddr, 0x1F00);

  s2 = ucaddr + sceKernelCheckPspConfig(ucaddr, ret);
  if(s2 > ucaddr)
  {
    v1 = 1;
    //1860
    if(v1 == 0)
    {
      //->189c
      s0 = s1;
      if(a0 == 0)
      {
        //->1818
      }
      
      a0 = 0x0(s0);
      //18a8
      t0 = 0x40;
      //seb v1, a0
      t1 = 0x21;
      t2 = 0x23;
      a1 = 0x25;
      if(v1 == t0 || v1 == t1)
      {
        //->198c
      }
      else if(v1 == t2)
      {
        //->1970
      }
      else
      {
        sp_4 = 0;
        fp = 0;
        t0 = 0;
        sp_8 = 0;
        a3 = 0x24;
        a2 = 0x2B;
        t1 = 0x2D;
        
        //18e4
        //seb v1, a0
        t3 = sp_4;
        if(v1 == a1)
        {
          //->1b78
        }
        else
        {
          a0 = v1 < 0x26;
          if(a0 == 0)
          {
            //->1b48
          }
          else if(v1 == a3)
          {
            s0++;
            //->1b3c
          }
          else
          {
            switch(fp)
            {
              case 3:
              {
                //1924
                s5 = 1;
                //fall through to 1928 below
              }

              case 0:
              case 1:
              case 2:
              {
                //1928
                s1 = s5 < 1;
                s4 = fp < 3;
                t7 = s1 & s4;
                v1 = s0 < s2;
                if(t7 != 0)
                {
                  v1 = 0x18(s6);
                  a0 = 1;
                  a1 = 0;
                  t8 = 2;
                  if((v1 == a0 || v1 == 0))
                  {
                    if(t0 != t8)
                    {
                      a1 = 1;
                      t9 = 1;
                    }
                    else
                    {
                      t9 = 1;
                    }
                  }
                  else
                  {
                    if(v1 == s7)
                    {
                      //1b10
                    }
                    else
                    {
                      a1 = 1;
                    }

                    //1964
                    t9 = 1;
                  }
                }

                //1974
                break;
              }

              case 4:
              {
                //1b34
                break;
              }

              default:
              {
                //->1818
                break;
              }
            }
          }
        }
      }
    }
    else
    {
      a2 = 0x0(s0); //b
      //186c
      a1 = a2 < 0x20;
      if(a1 == 0)
      {
        s0++;
        //->1b8c
        v1 = s0 < s2;
        //->1860
      }
      else
      {
        //1878
        if(v1 == 0)
        {
          //->189c
        }
        else
        {
          a3 = 0x0(s0); //b
          a1 = a3 < 0x20;
          if(a1 == 0)
          {
            a2 = 0x0(s0); //b
            //->186c
          }

          s0++;
          v1 = s0 < s2;
          //->1878
        }
      }
    }
  }
}

//1bb4
void ProtectHandling(SceLoadCoreBootInfo *pBoot)
{
  for(i = 0; i < pBoot->unk_1C; i++)
  {
    v1 = pBoot->unk_20[i].unk_8; //h??
    if(v1 == 0x2)
    {
      if(pBoot->unk_20[i].unk_4 != 0)
      {
        SceUID uidInitFilename = sceKernelAllocPartitionMemory(1, "SceInitFileName", 1, pBoot->unk_20[i].unk_4, 0);
        if(uidInitFilename <= 0)
        {
          Kprintf("init.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, uidInitFilename);
        }
        else
        {
          int addrInitFilename = sceKernelGetBlockHeadAddr(uidInitFilename);
          memmove(addrInitFilename, pBoot->unk_20[i].unk_0, pBoot->unk_20[i].unk_4);

          pBoot->unk_20[i].unk_0 = addrInitFilename;
          pBoot->unk_20[i].unk_C = uidInitFilename;
          pBoot->unk_20[i].unk_8 |= 0x10000;
        }
      }

      if(i == pBoot->unk_24)
      {
        var_2CC4 = pBoot->unk_20[i].unk_0;
      }
    }
    else if(v1 == 0x4)
    {
      if(pBoot->unk_20[i].unk_4 != 0)
      {
        SceUID uidInitVSHParam = sceKernelAllocPartitionMemory(1, "SceInitVSHParam", 1, pBoot->unk_20[i].unk_4, 0);
        if(uidInitVSHParam <= 0)
        {
          Kprintf("init.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, uidInitVSHParam);
        }
        else
        {
          struct SceKernelLoadExecVSHParam *initVSHParam = (struct SceKernelLoadExecVSHParam *)sceKernelGetBlockHeadAddr(uidInitVSHParam);
          memmove(initVSHParam, pBoot->unk_20[i].unk_0, pBoot->unk_20[i].unk_4);

          pBoot->unk_20[i].unk_0 = initVSHParam;
          pBoot->unk_20[i].unk_8 |= 0x10000;
          pBoot->unk_20[i].unk_C = uidInitVSHParam;

          initVSHParam->size = sceKernelQueryBlockSize(initVSHParam);
          initVSHParam->args = 0x20;
          initVSHParam->vshmain_args = 0;
          initVSHParam->configfile = 0;

          sceKernelRegisterChunk(0, uidInitVSHParam);
        }
      }
    }
    else if(v1 = 0x100)
    {
      if(pBoot->unk_20[i].unk_4 != 0)
      {
        SceUID uidInitUserParam = sceKernelAllocPartitionMemory(1, "SceInitUserParam", 1, pBoot->unk_20[i].unk_4, 0);
        if(uidInitUserParam <= 0)
        {
          Kprintf("init.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, uidInitUserParam);
        }
        else
        {
          int addrInitUserParam = sceKernelGetBlockHeadAddr(uidInitUserParam);
          memmove(addrInitUserParam, pBoot->unk_20[i].unk_0, pBoot->unk_20[i].unk_4);

          pBoot->unk_20[i].unk_0 = addrInitUserParam;
          pBoot->unk_20[i].unk_8 |= 0x10000;
          pBoot->unk_20[i].unk_C = uidInitUserParam;
        }
      }
    }
  }

  SceUID chunkid = sceKernelGetChunk(0);
  if(chunkid < 0)
  {
    SceUID uidInitVSHParam = sceKernelAllocPartitionMemory(1, "SceInitVSHParam", 1, 0x20, 0);
    if(uidInitVSHParam > 0)
    {
      struct SceKernelLoadExecVSHParam *initVSHParam = (struct SceKernelLoadExecVSHParam *)sceKernelGetBlockHeadAddr(uidInitVSHParam);

      initVSHParam->size = sceKernelQueryBlockSize(initVSHParam);
      initVSHParam->args = 0x20;
      initVSHParam->argp = 0;
      initVSHParam->key = 0;
      initVSHParam->vshmain_args_size = 0;
      initVSHParam->vshmain_args = 0;
      initVSHParam->configfile = 0;

      sceKernelRegisterChunk(0, uidInitVSHParam);
    }
  }
}

//1e80
sub_00001E80(unk_a0)
{
  t0 = unk_a0;
  t4 = 0;
  t1 = 0;
  t3 = 0xA;
  t2 = 0;
  while(1 == 1)
  {
    a0 = 0x0(t0);
    //seb a2, a0
    a3 = a2 ^ 0x9;
    v0 = a2 ^ 0x20;
    t5 = v0 < 1;
    t6 = a3 < 1;
    a3 = t5 | t6;
    v1 = 0x30;
    if(a3 == 0)
    {
      break;
    }
    
    t0++;
    t1++;
  }
  
  if(a2 == v1)
  {
    t6 = 0x1(t0); //b
    t9 = t6 ^ 0x58;
    t5 = t6 ^ 0x78;
    t7 = t5 < 1;
    t8 = t9 < 1;
    a2 = t7 | t8;
    if(a2 != 0)
    {
      t0 += 2;
      a0 = 0x0(t0);
      t3 = 0x10;
      t1 += 2;
    }
  }
  
  while(1 == 1)
  {
    //seb a2, a0
    a0 = a2 - 0x30;
    a3 = a0 & 0xFF;
    t8 = a2 - 0x41;
    t7 = a3 < 0xA;
    a3 = a0;
    a0 = t8 < 6;
    if(t7 == 0 && (t3 != 0x10 || (a0 == 0 && a2 != 0x61)))
    {
      break;
    }
    
    //mult t2, t3
    t0++;
    a0 = 0x0(t0); //b
    t4 = 1;
    t1++
    //mflo a2
    t2 = a2 + a3;
  }

  0x0(a1) = t2;

  return (t4 == 1 ? t1 : -1);
}