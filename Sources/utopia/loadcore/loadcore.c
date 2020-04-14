//loadcore.c
int var_65E8[] =
{
  0xACE23476, 0x7BE1421C, 0xBF983EF2, 0x7068E6BA, 0xB4D6FECC, 0x54AB2675, 0x2952F5AC, 0xD8779AC6,
  0x99A695F0, 0x5873A31F, 0x0B464512, 0x9BAF90F6, 0x0E760DBA, 0x0DE1F600, 0xDA1B09AA, 0xC99DD47A,
  0x616FCCCD, 0x52A86C21, 0xCD0F3BAC, 0x6B2371C2, 0x8D8A8ACE, 0xAFF947D4, 0xAE7C6E76, 0x74CF001A,
  0xCF8A41B1, 0xFB8AE27D, 0xCCE4A157, 0x929B5C69, 0x05D915DB
};

reglibin var_64A4 =
{
  "LoadCoreForKernel", //unk_0
  0x0011, //unk_4
  0x0001, //unk_6
  0x04, //unk_8
  0x00, //unk_9
  29, //unk_A
  var_65E8 //unk_C
};

int module_bootstart(int arglen, void *argp) __attribute__((alias("LoadCoreInit")));

//ab8
int LoadCoreInit(SceSize argSize, void *args)
{
  SceLoadCoreBootInfo *boot = (SceLoadCoreBootInfo *)args[0];
  SysMemThreadConfig *sm_config = (SysMemThreadConfig *)args[1];

  if(argSize != 8)
  {
    Kprintf("illegal argSize %d\n", argSize);
  }

  var_8C40.unk_28 = -1;
  var_8C40.unk_8 = 0x2000;
  var_8C40.unk_1C = 0x10001;
  var_8C40.unk_2C = 0;
  var_8C40.unk_4 = 0;
  var_8C40.unk_C = 0;
  var_8C40.unk_10 = 0;
  var_8C40.unk_14 = 0;
  var_8C40.unk_18 = 0;
  var_8C40.unk_20 = 0;
  var_8C40.unk_0 = 0;

  //initialise library and stub storage
  sub_00000000();
  sub_0000010C();

  if(sm_config->unk_64 != 0)
  {
    sub_000039F4(sm_config->unk_64);
  }

  //register SysMem kernel libraries
  int i;
  for(i = 0; i < sm_config->user_lib_start; i++)
  {
    ASSERT(sceKernelRegisterLibrary(sm_config->unk_8[i]) == 0);
  }

  //register LoadCoreForKernel library
  ASSERT(sceKernelRegisterLibrary(&var_64A4) == 0);

  //link import stubs for loadcore
  int ret = sceKernelLinkLibraryEntries(sm_config->lc_stub, sm_config->lc_stubsize);

  var_8C40.unk_2C = 1;
  ModuleServiceInit();

  //create and register sysmem and loadcore modules
  sceKernelRegisterModule(sceKernelCreateAssignModule(sm_config->sysmem_execinfo));
  sceKernelRegisterModule(sceKernelCreateAssignModule(sm_config->lc_execinfo));

  //continue to boot the rest of the modules
  SysBoot(boot, sm_config);

  return ret;
}

//c9c
int doRegisterLibrary(reglibin *lib, int is_user_mode)
{
  if(lib == 0)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_LIBRARY;
  }

  if(is_user_mode && (lib->attr & 0x4000)) //h
  {
    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  int intr = sceKernelCpuSuspendIntr();

  SceLibraryEntry *libent = sub_00000048(); //s2/v0
  s3 = 0;
  libent->name = lib->name;
  libent->version = lib->version;
  libent->attr = lib->attr;
  libent->size = lib->size;
  libent->num_vars = lib->num_vars;
  libent->num_funcs = lib->num_funcs;
  libent->nid = lib->nid;
  libent->unk_14 = lib;
  libent->is_user_mode = is_user_mode;

  SceLibraryEntry *tmplib;
  for(tmplib = var_8C40.unk_0; tmplib != 0; tmplib = tmplib->next)
  {
    if(!sub_000055E8(libent->name, tmplib->name))
    {
      if(tmplib->is_user_mode == libent->is_user_mode)
      {
        //1000
        if((tmplib->attr & 0x2) || libent->version == tmplib->version)
        {
          if(tmplib->version >= libent->version)
          {
            sceKernelCpuResumeIntr(intr);

            Kprintf("loadcore.c:%s:SCE_KERNEL_ERROR_LIBRARY_FOUND\n", __FUNCTION__);

            Kprintf("loadcore.c:%s:lib 0x%p[%s]\n", __FUNCTION__, lib, lib->name);

            return SCE_KERNEL_ERROR_LIBRARY_FOUND;
          }

          SceLibraryStub *stub = tmplib->stub, *next_stub, *last_stub = 0;
          tmplib->stub = 0;
          a0 = &tmplib->stub;
          for(; stub != 0; stub = next_stub)
          {
            next_stub = stub->next;

            if(stub->unk_24 & 0x1)
            {
              *a0 = stub;
              a0 = &stub->next;
              stub->next = 0;
            }
            else
            {
              stub->next = last_stub;
              last_stub = stub;
            }
          }
        }
      }
    }
  }

  //d84
  if((libent->attr & 0x4000))
  {
    if(var_8C40.unk_4 == 0)
    {
      ret = SyscallTableInit(var_8C40.unk_8, &var_8C40.unk_4); //s0
      if(ret < 0)
      {
        Kprintf("loadcore.c:%s:SyscallTableInit failed: 0x%08x\n", __FUNCTION__, ret);
  
        sceKernelCpuResumeIntr(intr);
  
        return ret;
      }
    }

    //dbc
    ret = AllocSysTable(libent->num_funcs); //s1/v0
    libent->unk_1C = var_8C40.unk_8 + ret;
    if(ret < 0)
    {
      Kprintf("loadcore.c:%s:AllocSysTable failed: 0x%x\n", __FUNCTION__, ret);

      sceKernelCpuResumeIntr(intr);

      return ret;
    }

    for(a1 = 0; a1 < libent->num_funcs; a1++)
    {
      var_8C40.unk_4[ret + 4 + a1] = libent->entry_table[libent->num_funcs + libent->num_vars + a1];
    }
  }

  //e2c
  v1 = var_8C40.unk_10;
  s1 = &var_8C40.unk_10;
  if(var_8C40.unk_10 != 0)
  {
    while(1 == 1)
    {
      a0 = v1->libname;
      a1 = libent->name;
      v1 = 0x0(s1);
      if(!sub_000055E8(v1->libname, libent->name))
      {
        s4 = libent->is_user_mode;
        a0 = v1->is_user_mode;
        if(libent->is_user_mode == 0 || v1->is_user_mode != 0)
        {
          a1 = v1->next;
          v1->next = s3;
          s3 = *s1;
          *s1 = a1;
        }
        else
        {
          s1 = &v1->next;
        }
      }
      else
      {
        s1 = &v1->next;
      }

      s5 = v1->unk_0;
      v1 = s5;
      if(s5 == 0)
      {
        break;
      }
    }
  }

  libent->stub = 0;
  if(s3 != 0)
  {
    for(; s3 != 0; s3 = next_s3)
    {
      next_s3 = s3->unk_4;

      aLinkClient(s3, libent);

      if(sub_000055E8(s3->unk_8, "memlmd") == 0)
      {
        var_75B0 = KDebugForKernel_24C32559(0x1E);
        if(var_75B0 != 0)
        {
          if(KDebugForKernel_24C32559(0x15) == 1)
          {
            var_75B0 = 0;
          }
        }

        if(sceKernelSetPrimarySyscallHandler(0x15, loc_00005C5C) < 0)
        {
          Kprintf("loadcore.c:%s:sceKernelSetPrimarySyscallHandler failed 0x%08x\n", "SetNotYetSyscall", v0);
        }

        sub_000039F4(0);
      }

      //ec4
      s3->unk_4 = libent->unk_18;
      libent->unk_18 = s3;
    }
  }

  libent->attr &= ~0x1;
  libent->next = var_8C40.unk_0;
  var_8C40.unk_0 = libent;

  sceKernelCpuResumeIntr(intr);

  sceKernelIcacheClearAll();

  sceKernelDcacheWBinvAll();

  return SCE_KERNEL_ERROR_OK;
}

//10b8
int sceKernelRegisterLibrary(reglibin *libent)
{
  return doRegisterLibrary(libent, 0);
}

//10d4
int ReleaseLibEntCB(SceLibraryEntry *lib, SceLibraryEntry *prev_lib)
{
  if(lib->stub != 0)
  {
    s0 = &lib->stub;
    while(*s0 != 0)
    {
      if(lib->stub->attr & 0x8)
      {
        next_stub = lib->stub->next;

        lib->stub->next = var_8C40.unk_10;

        var_8C40.unk_10 = lib->stub;

        for(v1 = lib->stub->num_funcs; v1 > 0; v1--)
        {
          lib->stub->func[v1].unk_0 = 0x54C;
          lib->stub->func[v1].unk_4 = 0;
        }

        lib->stub->unk_24 &= ~0x2;

        sub_00000710(&lib->name, &lib->stub->libname);

        *s0 = next_stub;
      }
      else
      {
        s0 = &lib->stub->next;
      }
    }

    if(lib->stub != 0)
    {
      Kprintf("loadcore.c:%s:SCE_KERNEL_ERROR_LIBRARY_INUSE\n", __FUNCTION__);

      return SCE_KERNEL_ERROR_LIBRARY_INUSE;
    }
  }

  prev_lib->next = lib->next;
  lib->stub = 0;
  lib->next = 0;
  if(lib->attr & 0x4000)
  {
    FreeSysTable(lib->unk_1C - var_8C40.unk_8);
  }

  sub_00000098(lib);

  //how can s0 not be 0 here???
  for(; s0; s0 = next_stub)
  {
    next_stub = s0->next;

    if(aLinkLibEntries(s0) != 0)
    {
      for(v1 = s0->num_funcs; v1 > 0; v1--)
      {
        s0->func[v1].unk_0 = 0x3E00008;
        s0->func[v1].unk_4 = 0;
      }

      s0->next = var_8C40->unk_10->next;
      s0->unk_24 &= ~0x2;
      s0->unk_24 |= 0x4;
      var_8C40.unk_10->next = s0;
    }
  }

  sceKernelIcacheClearAll();

  sceKernelDcacheWBinvAll();

  return 0;
}

//12a8
int sceKernelCanReleaseLibrary(reglibin *libent)
{
  int intr = sceKernelCpuSuspendIntr();

  SceLibraryEntry *lib;
  for(lib = var_8C40.unk_0; lib; lib = lib->next)
  {
    if(lib->unk_14 == libent)
    {
      break;
    }
  }

  if(lib != 0)
  {
    SceLibraryStub *stub;
    for(stub = lib->stub; stub; stub = stub->next)
    {
      if(!(stub->attr & 0x8))
      {
        Kprintf("loadcore.c:%s:in use [%s]\n", __FUNCTION__, libent->name);

        sceKernelCpuResumeIntr(intr);

        return SCE_KERNEL_ERROR_LIBRARY_INUSE;
      }
    }
  }

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

//136c
int doLinkLibraryEntries(stubin *unk_a0, SceSize unk_a1, int is_user_mode)
{
  stubin *stub;
  SceLibraryStub *newstub, *tmpstub;
  for(stub = unk_a0; stub < unk_a0 + unk_a1 / 4 * 4; stub += stub->unk_8 * sizeof(int))
  {
    newstub = sub_00000154();
    newstub->libname = stub->libname;
    newstub->version = stub->version;
    newstub->attr = stub->attr;
    newstub->len = stub->len;
    newstub->num_vars = stub->num_vars;
    newstub->num_funcs = stub->num_funcs;
    newstub->nid = stub->nid;
    newstub->func = stub->func;
    if(stub->len < 6)
    {
      newstub->var = 0;
    }
    else
    {
      newstub->var = stub->var;
    }
    newstub->original = stub;
    newstub->unk_24 = 0;
    newstub->is_user_mode = is_user_mode;

    if(aLinkLibEntries(newstub) != 0)
    {
      if(!(newstub->attr & 0x8))
      {
        Kprintf("loadcore.c:%s:Cannot link library [%s]\n", __FUNCTION__, newstub->libname);

        sceKernelUnLinkLibraryEntries(stub, unk_a1);

        return SCE_KERNEL_ERROR_ERROR;
      }

      int ok = 1;
      for(tmpstub = var_8C40->unk_10; tmpstub; tmpstub = tmpstub->unk_4)
      {
        if(tmpstub->unk_20 == newstub->unk_20)
        {
          Kprintf("loadcore.c:%s:libstub 0x%p duplicated\n", __FUNCTION__, newstub);

          sub_000001A0(newstub);

          ok = 0;
        }
      }

      //1474
      if(ok)
      {
        newstub->unk_4 = var_8C40->unk_10;
        var_8C40->unk_10 = newstub;
        for(i = newstub->unk_12; i; i--)
        {
          newstub->unk_18[i].unk_0 = 0x54C;
          newstub->unk_18[i].unk_4 = 0;
        }

        newstub->unk_24 &= ~0x2;
      }
    }
  }

  return 0;
}

//1544
sceKernelLinkLibraryEntries(unk_a0, unk_a1)
{
  doLinkLibraryEntries(unk_a0, unk_a1, 0);
}

//1560
aUnLinkLibEntries(...)
{
}

//1708
int sceKernelUnLinkLibraryEntries(unk_a0, unk_a1)
{
  for(s0 = unk_a0; s0 < unk_a0 + (unk_a1 / 4 * 4); s0 += s0->unk_8 * sizeof(int))
  {
    aUnLinkLibEntries(s0);
  }

  return SCE_KERNEL_ERROR_OK;
}

//1770
void SysBoot(SceLoadCoreBootInfo *p_bootinfo, SysMemThreadConfig *p_sysmem)
{
  SceUID gzip_buffer = 0, prx_buffer = 0;
  int ret, i, j;
  reglibin *lib;
  int status;

  MFC0_V(status, COP0_SR_STATUS);
  var_8C3C = status;
  status &= ~0x1F;
  MTC0_V(status, COP0_SR_STATUS);

  mem = sceKernelAllocPartitionMemory(1, "SceLoadCoreBootInfo", 1, sizeof(SceLoadCoreBootInfo), 0); //s2
  if(mem < 0)
  {
    Kprintf("loadcore.c:%s:Cannot allocate memory: res=0x%08x\n", __FUNCTION__, mem);

    ASSERT(1 == 2);
  }

  SceLoadCoreBootInfo *bootinfo = (SceLoadCoreBootInfo *)sceKernelGetBlockHeadAddr(mem); //s3

  *bootinfo = *p_bootinfo;

  if(bootinfo->num_modules <= 0)
  {
    bootinfo->module = 0;
  }
  else
  {
    mem = sceKernelAllocPartitionMemory(1, "SceLoadCoreBootModuleInfo", 1, bootinfo->num_modules * sizeof(SceLoadCoreBootModuleInfo), 0); //s2
    if(mem < 0)
    {
      Kprintf("loadcore.c:%s:Cannot allocate memory: res=0x%08x\n", __FUNCTION__, mem);

      ASSERT(1 == 2);
    }

    SceLoadCoreBootModuleInfo *moduleinfo = (SceLoadCoreBootModuleInfo *)sceKernelGetBlockHeadAddr(mem); //a3
    for(i = bootinfo->num_modules - 1; i >= 0; i--)
    {
      moduleinfo[i] = bootinfo->module[i];
    }

    bootinfo->module = moduleinfo;
  }

  if(bootinfo->num_protects != 0)
  {
    mem = sceKernelAllocPartitionMemory(1, "SceLoadCoreProtectInfo", 1, bootinfo->num_protects * sizeof(SceLoadCoreProtectInfo), 0); //s2
    if(mem < 0)
    {
      Kprintf("loadcore.c:%s:Cannot allocate memory: res=0x%08x\n", __FUNCTION__, mem);

      ASSERT(1 == 2);
    }

    SceLoadCoreProtectInfo *protectinfo = (SceLoadCoreProtectInfo *)sceKernelGetBlockHeadAddr(mem); //a3
    for(i = bootinfo->num_protects - 1; i >= 0; i--)
    {
      protectinfo[i] = bootinfo->protect[i];
    }

    bootinfo->protect = protectinfo;
  }

  ??? unk[bootinfo->num_modules]; //sp/var_8C60/s6 - size of one is 8 bytes
  var_8C60[0].unk_0 = 0;

  sceKernelSetBootCallbackLevel(loc_00002258, 3, &ret);

  SceLoadCoreExecFileInfo fp;
  for(i = bootinfo->modules_loaded; i < bootinfo->num_modules; i++)
  {
    sub_00005820(&fp, 0, sizeof(SceLoadCoreExecFileInfo)));

    fp.unk_8 = 0;
    fp.unk_24 = 0;
    fp.unk_4 = 0;

    sceKernelCheckExecFile(bootinfo->module[i].unk_4, &fp);

    if(fp.unk_5A & 0x1)
    {
      gzip_buffer = sceKernelAllocPartitionMemory(1, "SceLoadcoreGzipBuffer", 1, fp.unk_5C, 0);
      fp.unk_24 = sceKernelGetBlockHeadAddr(gzip_buffer);

      sceKernelCheckExecFile(bootinfo->module[i].unk_4, &fp);

      fp.unk_24 = 0;
    }

    ret = sceKernelProbeExecutableObject(bootinfo->module[i].unk_4, &fp);
    if(ret < 0)
    {
      Kprintf("loadcore.c:%s:sceKernelProbeExecutableObject failed 0x%08x\n", __FUNCTION__, ret);
    }
    else
    {
      switch(fp.unk_20 + 1)
      {
        case 2:
        case 3:
        {
          if(fp.unk_24 != 0)
          {
            Kprintf("loadcore.c:%s:execFileInfo.topaddr 0x%08x\n", __FUNCTION__, fp.unk_24);

            Kprintf("loadcore.c:%s:something wrong\n", __FUNCTION__);

          }

          if(fp.unk_58 & 0x1000)
          {
            prx_buffer = sceKernelAllocPartitionMemory(1, "SceLoadcorePRXKernel", 0, fp.unk_30, 0);
          }
          else
          {
            prx_buffer = sceKernelAllocPartitionMemory(2, "SceLoadcorePRXUser", 0, fp.unk_30, 0);
          }

          fp.unk_24 = sceKernelGetBlockHeadAddr(prx_buffer);
          break;
        }

        default:
        {
          Kprintf("loadcore.c:%s:Unsupported type %d, i=%d\n", __FUNCTION__, fp.unk_20, i);

          ASSERT(1 == 2);
          break;
        }
      }
    }

    ret = sceKernelLoadExecutableObject(bootinfo->module[s4].unk_4, &fp);
    if(ret < 0)
    {
      Kprintf("loadcore.c:%s:sceKernelLoadExecutableObject failed 0x%08x\n", __FUNCTION__, ret);
    }

    //free the gzip buffer if we used one
    if(gzip_buffer > 0)
    {
      ret = sceKernelFreePartitionMemory(gzip_buffer);
      if(ret < 0)
      {
        Kprintf("loadcore.c:%s:sceKernelFreePartitionMemory failed 0x%08x: res\n", __FUNCTION__, ret);
      }

      gzip_buffer = 0;
    }

    for(lib = fp.unk_70; lib < fp.unk_70 + fp.unk_74; lib += lib->size * sizeof(int))
    {
      if(lib->unk_6 & 0x1)
      {
        ASSERT(sceKernelRegisterLibrary(lib) == 0);
      }
    }

    //1a80
    if(fp.unk_78 != -1)
    {
      sceKernelLinkLibraryEntries(fp.unk_78, fp.unk_7C);
    }

    sceKernelIcacheClearAll();

    sceKernelDcacheWBinvAll();

    SceModule *mod = sceKernelCreateAssignModule(&fp); //s2
    mod->unk_34 = prx_buffer;

    sceKernelRegisterModule(mod);

    for(lib = fp.export_libs; lib < fp.unk_70 + fp.unk_74; lib += lib->size * sizeof(int))
    {
      if(lib->unk_6 < 0) //h
      {
        ProcessModuleExportEnt(mod, lib);
      }
    }

    if(!strcmp(mod->modname, "sceInit"))
    {
      bootinfo->modules_loaded = i + 1;

      fp.bootstart(4, bootinfo);
      break;
    }

    fp.bootstart(0, 0);
  }

  for(i = 0; i < bootinfo->num_protects; i++)
  {
    if((bootinfo->protect[i].attr & 0xFFFF) == 0x200 && (bootinfo->protect[i].attr & 0x10000))
    {
      sceKernelFreePartitionMemory(bootinfo->protect[i].UID);

      bootinfo->protect[i].attr &= ~0x10000;
    }
  }

  //1b88
  sub_000039F4(0);

  //register SysMem user libraries
  for(i = p_sysmem->user_lib_start; i < p_sysmem->num_export_libs; i++)
  {
    ret = sceKernelRegisterLibrary(p_sysmem->export_lib[i]);
    if(ret != 0)
    {
      Kprintf("loadcore.c:%s:sceKernelRegisterLibraryForUser failed 0x%08x\n", __FUNCTION__, ret);
    }
  }

  for(i = 0; i < 4; i++)
  {
    if(i == 3)
    {
      var_8C60[0].unk_0 = 0;
    }

    for(j = 0; var_8C60[j].unk_0 != 0; j++)
    {
      a0 = s0 + 2; //s0 = &var_8C60[j]?
      if((var_8C60[j].unk_0 & 0x3) == i)
      {
        if(i != 3)
        {
          a0 = s6; //s6 = &var_8C60[0]?
        }

        (var_8C60[j].unk_0 & ~0x3)(a0, 1, 0);
      }
    }

    if(i == 2)
    {
      for(; j >= 0; j--)
      {
        if((var_8C60[j].unk_0 & 0x3) == 0x3)
        {
          break;
        }

        var_8C60[j].unk_0 = 0;
      }
    }
  }

  Kprintf("loadcore.c:%s:Does not reach here!!!\n", __FUNCTION__);
}

//1fd0
sceKernelSetBootCallbackLevel(...)
{
  a3 = &var_8C40;
  v1 = unk_a0;
  s1 = unk_a2;
  a2 = 0;
  s0 = var_8C40.unk_20;
  a3 = a1 & 0x3;
  v0 = a0 + a3;
  a1 = 0;
  if(s0 != 0)
  {
    var_8C40.unk_20->unk_0 = v0;
    v0 = sceKernelFindModuleByAddress(unk_a0);
    a1 = &var_8C40;
    a0 = 0;
    if(v0 != 0)
    {
      a0 = v0->unk_68;
    }
    
    a2 = var_8C40.unk_20;
    var_8C40.unk_20->unk_4 = a0;
    a0 = 1;
    t0 = a2 + 8;
    var_8C40.unk_20 = t0;
    var_8C40.unk_20->unk_8 = 0;
    
    return 1;
  }
  else
  {
    a0 = 1;
    v0 = v1(...);
    if(s1 != 0)
    {
      *s1 = v0;
    }

    return SCE_KERNEL_ERROR_OK;
  }
}

//2070
void loc_00002070(...)
{
  a2--;
  a3 = a0;
  if(a2 >= 0)
  {
    while(1 == 1)
    {
      a0 = a2 * 4;
      v0 = a0 + a3;
      t0 = 0x0(v0);
      v1 = a0 + a1;
      a2--;
      0x0(v1) = t0;
      if(a2 < 0)
      {
        break;
      }
    }
  }
}

//20a0
SceUID LoadCoreHeap()
{
  if(var_8C40.unk_2C != 1)
  {
    Kprintf("loadcore.c:%s:Cannot allocate heap\n", __FUNCTION__);

    ASSERT(1 == 2);
  }

  if(var_8C40.unk_28 <= 0)
  {
    var_8C40.unk_28 = sceKernelCreateHeap(1, 0x1000, 1, "loadcoreheap");
  }

  return var_8C40.unk_28;
}

//2158
int sceKernelRegisterLibraryForUser(unk_a0)
{
  int doRegisterLibrary(unk_a0, 1);
}

//2174
int sceKernelReleaseLibrary(reglibin *lib)
{
  int intr = sceKernelCpuSuspendIntr();

  SceLibraryEntry *tmp_lib, *last_lib = &var_8C40.unk_0;
  for(tmp_lib = var_8C40.unk_0; tmp_lib; tmp_lib = tmp_lib->next)
  {
    if(tmp_lib->unk_14 == lib)
    {
      break;
    }
    
    last_lib = tmp_lib;
  }

  if(last_lib == 0) //???
  {
    sceKernelCpuResumeIntr(intr);

    Kprintf("There is no library [%s]\n", s0->unk_0);

    Kprintf("loadcore.c:%s:SCE_KERNEL_ERROR_LIBRARY_NOTFOUND\n", __FUNCTION__);

    return SCE_KERNEL_ERROR_LIBRARY_NOTFOUND;
  }

  int ret = ReleaseLibEntCB(tmp_lib, last_lib);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//2230
int sceKernelLinkLibraryEntriesForUser(unk_a0, unk_a1)
{
  return doLinkLibraryEntries(unk_a0, unk_a1, 1);
}

//224c
LoadCoreInfo *sceKernelQueryLoadCoreCB()
{
  return &var_8C40;
}

//2258
int loc_00002258()
{
  return 0;
}

//2260
int aLinkClient(SceLibraryStub *stub, SceLibraryEntry *lib)
{
  int i, j;

  for(i = 0; i < stub->num_funcs; i++)
  {
    if(lib->num_funcs != 0)
    {
      for(j = 0; j < lib->num_funcs; j++)
      {
        if(lib->entry_table[j] == stub->nid[i])
        {
          break;
        }
      }

      if(j == lib->num_funcs)
      {
        Kprintf("Cannot link function 0x%08x in [%s]\n", stub->nid[i], lib->name);

        Kprintf("continue\n");
      }
      else
      {
        if(lib->attr & 0x4000)
        {
          stub->func[i].unk_0 = 0x3E00008;
          stub->func[i].unk_4 = (((lib->unk_1C + j) * 64) & 0x3FFFFC0) | 0xC;
        }
        else
        {
          //2384
          stub->func[i].unk_0 = ((lib->entry_table[lib->num_funcs + lib->num_vars + j] & ~0xF0000003) >> 2) | 0x8000000;

          ASSERT(stub->func[i].unk_4 < 1);
        }
      }
    }
  }

  if(!(lib->attr & 0x4000))
  {
    sub_000006F4(lib->name, stub->name);
  }

  stub->unk_24 &= ~0x4;
  stub->unk_24 |= 0x2;

  return 0;
}

//2410
int aLinkLibEntries(SceLibraryStub *stub)
{
  SceLibraryEntry *lib;
  SceLibraryStub *tmpstub;

  for(lib = var_8C40.unk_0; lib; lib = lib->next)
  {
    if(!(lib->attr & 0x1))
    {
      if(sub_000055E8(stub->libname, lib->name))
      {
        continue;
      }

      //stubs can't be a newer version than the lib
      if(lib->version < stub->version)
      {
        continue;
      }

      //can't have user stubs in kernel libraries (unless flag set)
      if(lib->is_user_mode == 0 && stub->is_user_mode != 0 && !(lib->attr & 0x4000))
      {
        continue;
      }

      //can't have kernel stubs in user libraries
      if(lib->is_user_mode != 0 && stub->is_user_mode == 0)
      {
        continue;
      }

      for(tmpstub = lib->stub; tmpstub; tmpstub = tmpstub->next)
      {
        if(tmpstub->original == stub->original)
        {
          Kprintf("loadcore.c:%s:Warning: add libstub 0x%p twice\n", __FUNCTION__, stub);

          return 0;
        }
      }

      aLinkClient(stub, lib);

      stub->next = lib->stub;
      lib->stub = stub;

      sceKernelIcacheClearAll();

      sceKernelDcacheWBinvAll();

      return 0;
    }
  }

  return SCE_KERNEL_ERROR_ERROR;
}

//2528
int ProcessModuleExportEnt(SceModule *mod, reglibin *lib)
{
  int i;

  for(i = 0; i < lib->num_funcs; i++)
  {
    if(lib->entry_table[i] == NID_MODULE_REBOOT_PHASE)
    {
      mod->unk_60 = lib->entry_table[lib->num_funcs + lib->num_vars + i];
    }
    else if(lib->entry_table[i] == NID_MODULE_BOOTSTART)
    {
      mod->module_bootstart = lib->entry_table[lib->num_funcs + lib->num_vars + i];
    }
    else if(lib->entry_table[i] == NID_MODULE_START)
    {
      mod->module_start = lib->entry_table[lib->num_funcs + lib->num_vars + i];
    }
    else if(lib->entry_table[i] == NID_MODULE_STOP)
    {
      mod->module_stop = lib->entry_table[lib->num_funcs + lib->num_vars + i];
    }
    else if(lib->entry_table[i] == 0x592743D8)
    {
      //do nothing??
    }
    else if(lib->entry_table[i] == 0x900DADE1)
    {
      //do nothing??
    }
    else if(lib->entry_table[i] == NID_MODULE_REBOOT_BEFORE)
    {
      mod->module_reboot_before = lib->entry_table[lib->num_funcs + lib->num_vars + i];
    }
    else
    {
      Kprintf("loadcore.c:%s:Unknown NID 0x%08x\n", __FUNCTION__, lib->entry_table[i]);
    }
  }

  for(i = 0; i < lib->num_vars; i++)
  {
    if(lib->entry_table[lib->num_funcs + i] == 0xCF0CC697) //no modules in 1.50 export this
    {
      mod->unk_AC = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_4;
      mod->unk_B0 = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_8;
      mod->unk_B4 = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_C;
    }
    else if(lib->entry_table[lib->num_funcs + i] == NID_MODULE_INFO) //module_info
    {
      //do nothing??
    }
    else if(lib->entry_table[lib->num_funcs + i] == 0xF4F4299D) //no modules in 1.50 export this
    {
      mod->unk_B8 = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_4;
      mod->unk_BC = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_8;
      mod->unk_C0 = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_C;
    }
    else if(lib->entry_table[lib->num_funcs + i] == 0x0F7C276C)
    {
      //lfatfs.prx, common_gui.prx, common_util.prx, heaparea1.prx,
      //heaparea2.prx, paf.prx, pafmini.prx and  vshmain.prx export this
      mod->unk_A0 = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_4;
      mod->unk_A4 = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_8;
      mod->unk_A8 = lib->entry_table[num_funcs * 2 + num_vars + i]->unk_C;
    }
    else
    {
      Kprintf("loadcore.c:%s:Unknown NID 0x%08x\n", __FUNCTION__, lib->entry_table[lib->num_funcs + i]);
    }
  }

  return 0;
}

//2830
int sceKernelCheckPspConfig(unk_a0, unk_a1)
{
  int ret = 0;

  if(*unk_a0 != 0x5053507E) //~PSP
  {
    int cpuid;
    MFC0_V(cpuid, COP0_SR_CPUID);
    if(cpuid == 0)
    {
      Kprintf("Cannot handle plain psp configuration file.\n");
    }

    return -1;
  }

  if(var_75B4 != 0)
  {
    var_75B4(unk_a0, unk_a1, &ret);
  }
  else
  {
    sceUtilsGetLoadModuleABLength(unk_a0, unk_a1, &ret);
  }

  return ret;
}
