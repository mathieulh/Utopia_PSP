//module.c
//4ad8
SceModule *sceKernelCreateModule()
{
  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  ASSERT(sceKernelCreateUID(var_75B8, "module", 0, &cb) >= 0);

  SceModule *module = UID_INFO(SceModule, cb, var_75B8);
  if(module)
  {
    module->modid = cb->UID;

    module->unk_A0 = -1;
    module->unk_A4 = -1;
    module->unk_A8 = -1;
    module->unk_AC = -1;
    module->unk_B0 = -1;
    module->unk_B4 = -1;
    module->unk_B8 = -1;
    module->unk_BC = -1;
    module->unk_C0 = -1;
  }

  sceKernelCpuResumeIntr(intr);

  return module;
}

//4ba8
int sceKernelAssignModule(SceModule *module, SceLoadCoreExecFileInfo *execFileInfo)
{
  if(module == 0)
  {
    Kprintf("module.c:%s:invalid arg: 0x%p\n", __FUNCTION__, 0);

    return SCE_KERNEL_ERROR_ERROR;
  }

  module->next = 0;

  SceUID found_uid = 0;
  SceModule *tmp_mod, *found_mod = 0;
  for(tmp_mod = var_8C54; tmp_mod; tmp_mod = tmp_mod->next)
  {
    if(!sub_000055E8(execFileInfo->unk_50->modname, tmp_mod->modname))
    {
      if(tmp_mod->modid > found_uid)
      {
        found_uid = tmp_mod->modid;
        found_mod = tmp_mod;
      }
    }
  }

  if(tmp_mod != 0)
  {
    Kprintf("module.c:%s:Duplicated module [%s]\n", __FUNCTION__, execFileInfo->unk_50->modname);

    if(tmp_mod->attribute & 0x2)
    {
      Kprintf("module.c:%s:Cann't load module [%s]\n", __FUNCTION__, execFileInfo->unk_50->modname);

      return SCE_KERNEL_ERROR_EXCLUSIVE_LOAD;
    }
  }

  module->unknown1 = 0; //h
  if(execFileInfo->unk_50 == 0 || !(execFileInfo->unk_50->attribute & 0x1000))
  {
    module->unknown1 |= 0x100;
  }

  module->attribute = execFileInfo->unk_58 | execFileInfo->unk_50->attribute; //h

  module->version[0] = execFileInfo->unk_50->version[0]; //b
  module->version[1] = execFileInfo->unk_50->version[1]; //b

  sub_000056A0(module->modname, execFileInfo->unk_50->modname, 0x1B);
  module->terminal = 0; //b

  module->ent_top = execFileInfo->unk_50->ent_top;
  module->ent_size = execFileInfo->unk_50->ent_bottom - execFileInfo->unk_50->ent_top;

  module->stub_top = execFileInfo->unk_50->stub_top;
  module->stub_size = execFileInfo->unk_50->stub_bottom - execFileInfo->unk_50->stub_top;

  module->gp_value = execFileInfo->unk_28;

  module->text_addr = execFileInfo->unk_50->text_addr;
  module->text_size = execFileInfo->unk_24; //???

  module->data_size = execFileInfo->unk_34;

  module->bss_size = execFileInfo->unk_38;

  module->nsegment = execFileInfo->unk_3C;
  module->unk_7C[0] = execFileInfo->unk_84; //b???

  int i;
  for(i = 0; i < 4; i++)
  {
    module->segmentaddr[i] = execFileInfo->segmentaddr[i];
    module->segmentsize[i] = execFileInfo->segmentsize[i];
  }

  if(module->unk_34 > 0)
  {
    sceKernelRenameUID(module->unk_34, module->modname);
  }

  return 0;
}

//4dc8
SceUID sceKernelGetModuleListWithAlloc(int *count)
{
  if(count == 0)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;
  }

  int intr = sceKernelCpuSuspendIntr();

  *count = var_8C40.unk_18;

  SceUID listuid = sceKernelAllocPartitionMemory(1, "SceModmgrModuleList", 1, var_8C40.unk_18 * sizeof(SceUID), 0);
  if(listuid <= 0)
  {
    sceKernelCpuResumeIntr(intr);

    return listuid;
  }

  SceUID *list = (SceUID *)sceKernelGetBlockHeadAddr(listuid);

  int i;
  SceModule *mod;
  for(mod = var_8C40.unk_14, i = 0; mod; mod = mod->next, i++)
  {
    list[i] = mod->modid;
  }

  sceKernelCpuResumeIntr(intr);

  return listuid;
}

//4e78
int sceKernelGetModuleIdListForKernel(u32 *idlist, SceSize size, int *count, int flag)
{
  if(idlist == 0 || count == 0)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;
  }

  int intr = sceKernelCpuSuspendIntr();

  *count = 0;
  SceModule *mod;
  for(mod = var_8C40.unk_14; mod; mod = mod->next)
  {
    if((flag & 0x1) && !(mod->unk_24 & 0x100))
    {
      Kprintf("module.c:%s:skip kernel module\n", __FUNCTION__);

      continue;
    }

    if(*count < (size / sizeof(u32)))
    {
      idlist[*count++] = mod->modid;
    }
  }

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

UidFuncTable var_7584[] =
{
  { UIDFUNC_CREATE, ModuleCreateUID ),
  { UIDFUNC_DELETE, ModuleDeleteUID },
  { 0, 0 }
};

//4f84
void ModuleServiceInit()
{
  sceKernelCreateUIDtype("SceModule", sizeof(SceModule), var_7584, 0, &var_75B8);
  ASSERT(var_75B8 > 0);
}

//5008
SceModule *sceKernelGetModuleFromUID(SceUID uid)
{
  SceModule *mod;
  s1 = uid;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  int ret;
  if((ret = sceKernelGetUIDcontrolBlockWithType(uid, var_75B8, &cb)) >= 0)
  {
    mod = UID_INFO(SceModule, cb, var_75B8);

    sceKernelCpuResumeIntr(intr);

    return mod;
  }

  //why??
  intr = sceKernelCpuSuspendIntr();

  for(mod = var_8C40.unk_14; mod; mod = mod->next)
  {
    if(mod->modid == uid || mod->unk_28 == uid)
    {
      sceKernelCpuResumeIntr(intr);

      return mod;
    }
  }

  sceKernelCpuResumeIntr(intr);

  Kprintf("module.c:%s:0x%08x not found\n", __FUNCTION__, uid);

  Kprintf("module.c:%s:failed: uid[0x%08x] ret=0x%08x\n", __FUNCTION__, uid, ret);

  return 0;
}

//5120
int sceKernelDeleteModule(SceModule *module)
{
  int intr = sceKernelCpuSuspendIntr();

  int ret = sceKernelDeleteUID(module->modid);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//5168
SceModule *sceKernelCreateAssignModule(SceLoadCoreExecFileInfo *execFileInfo)
{
  int intr = sceKernelCpuSuspendIntr();

  SceModule *module = sceKernelCreateModule();
  if(module)
  {
    sceKernelAssignModule(module, execFileInfo);
  }

  sceKernelCpuResumeIntr(intr);

  return module;
}

//51c8
int sceKernelRegisterModule(SceModule *mod)
{
  int intr = sceKernelCpuSuspendIntr();

  if(mod->unk_34 > 0)
  {
    sceKernelRenameUID(mod->unk_34, mod->modname);
  }

  mod->unk_28 = var_8C40.unk_1C++;

  SceModule *tmp, *last = 0;
  for(tmp = var_8C40.unk_14; tmp; tmp = tmp->next)
  {
    last = tmp;
  }

  if(last != 0)
  {
    last->next = mod;
  }

  mod->next = 0;

  var_8C40.unk_18++;

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

//526c
int sceKernelReleaseModule(SceModule *mod)
{
  if(mod == 0)
  {
    return SCE_KERNEL_ERROR_INVAL;
  }

  int intr = sceKernelCpuSuspendIntr();

  SceModule *tmpmod;
  for(tmpmod = var_8C40.unk_14; tmpmod && tmpmod->next; tmpmod = tmpmod->next)
  {
    if(tmpmod->next == mod)
    {
      tmpmod->next = mod->next;
      var_8C40.unk_18--;

      sceKernelCpuResumeIntr(intr);

      return SCE_KERNEL_ERROR_OK;
    }
  }

  Kprintf("module.c:%s:Cannot found modcb 0x%p\n", __FUNCTION__, unk_a0);

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_ERROR;
}

//5324
SceModule *sceKernelFindModuleByName(char *name)
{
  int intr = sceKernelCpuSuspendIntr();

  SceModule *mod, *found = 0;
  SceUID uid = 0;
  for(mod = var_8C40.unk_14; mod; mod = mod->next)
  {
    if(!sub_000055E8(name, mod->modname))
    {
      if(mod->modid > uid)
      {
        uid = mod->modid;
        found = mod;
      }
    }
  }

  sceKernelCpuResumeIntr(intr);

  return found;
}

//53c4
SceModule *sceKernelFindModuleByAddress(int addr)
{
  SceModule *mod;

  for(mod = var_8C40.unk_14; mod; mod = mod->next)
  {
    //this seems wrong???
    if(addr >= mod->unk_6C && addr < (mod->unk_6C + mod->unk_70 + mod->unk_74 + mod->unk_78))
    {
      return mod;
    }
  }

  return 0;
}

//5420
SceModule *sceKernelFindModuleByUID(SceUID uid)
{
  int intr = sceKernelCpuSuspendIntr();

  SceModule *mod;
  for(mod = var_8C40.unk_14; mod; mod = mod->next)
  {
    if(mod->modid == uid || mod->unk_28 == uid)
    {
      sceKernelCpuResumeIntr(intr);

      return mod;
    }
  }

  sceKernelCpuResumeIntr(intr);

  Kprintf("module.c:%s:0x%08x not found\n", __FUNCTION__, uid);

  return 0;
}

//54b4
int ModuleCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);
  
  a2 = var_75B8;
  a0 = cb->UID;
  a3 = -1;
  SceModule *mod = UID_INFO(SceModule, cb, uidModuleType); //a2
  mod->unk_2C = cb->UID;
  mod->unk_64 = -1;
  mod->unk_6 = 0;
  mod->unk_7 = 0;
  mod->unk_23 = 0;
  mod->unk_50 = -1;
  mod->unk_54 = -1;
  mod->unk_58 = -1;
  mod->unk_5C = -1;
  mod->unk_60 = -1;
  mod->unk_70 = 0;  
  mod->unk_74 = 0;
  mod->unk_78 = 0;
  mod->unk_24 = 0;
  mod->unk_0 = 0;
  mod->unk_4 = 0;
  mod->unk_40 = -1;
  mod->unk_48 = -1;
  
  return cb->UID;
}

//5540
int ModuleDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}
