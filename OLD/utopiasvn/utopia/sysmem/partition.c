int membase; //ca54
int memsize; //ca58
PartitionInfo *partitions; //ca5c
PartitionInfo *var_ca60; //ca60 - set to kernel partition in sysmem init
PartitionInfo *var_ca64; //ca64 - set to user partition in sysmem init
PartitionInfo *other_kernel_partition; //ca68
int var_ca6c; //ca6c
PartitionInfo *var_CA70; //ca70 - set to kernel/other kernel partition in sysmem init
PartitionInfo *var_CA74; //ca74 - set to other kernel/kernel partition in sysmem init
PartitionInfo *vsh_partition; //ca78
PartitionInfo *var_CA7C; //ca7c - set to user/unknown partition in sysmem init
PartitionInfo *var_CA80; //ca80 - set to unknown/user partition in sysmem init
PartitionInfo *kernel_partition; //ca84 - set to kernel partition in sysmem init
PartitionInfo *user_partition; //ca88
int var_ca8c; //ca8c
int var_ca90; //ca90
int var_ca94; //ca94

//fcc
PartitionInfo *GetPartition(int pid)
{
  PartitionInfo *ret;

  switch(pid)
  {
    //ffc
    case 0: ret = 0; break;
    //1018
    case SYSMEM_PARTITION_KERNEL: ret = kernel_partition; break;
    //1024
    case SYSMEM_PARTITION_USER: ret = user_partition; break;
    //1030
    case 3: ret = var_CA70; break;
    //103c
    case 4: ret = var_CA74; break;
    //1048
    case SYSMEM_PARTITION_VSHELL: ret = vsh_partition; break;
    //1054
    case 6: ret = var_CA7C; break;
    //1060
    case 7: ret = var_CA80; break;
    //106c
    default:
    {
      uidControlBlock *block;
      if(sceKernelGetUIDcontrolBlockWithType(pid, uidPartitionType, &block) != 0)
      {
        return 0;
      }

      ret = UID_INFO(PartitionInfo, block, uidPartitionType);
      break;
    }
  }

  //1004
  return ret;
}

//partition.c
//109c
PartitionInfo *AddrToCB(void *addr)
{
  ASSERT(addr > 0);

  //10d8
  PartitionInfo *part;

  //10e8
  for(part = partitions; part; part = part->next)
  {
    if((addr & 0x1fffffff) >= (part->addr & 0x1fffffff) && (addr & 0x1fffffff) < ((part->addr + part->size) & 0x1fffffff))
    {
      break;
    }
  }

  if(part == 0)
  {
    //117c
    Kprintf("partition.c:%s:%04d:illegal addr %x ! This address is not managed by System Memory Manager\n",
            __FUNC__, __LINE__, addr);

    ASSERT(part != 0);
    return;
  }

  //1128
  ASSERT(part > 0);

  return part;
}

UidLookupFunction PartitionUidFuncTable[] =
{
  { UIDFUNC_CREATE, PartitionCreateUID },
  { UIDFUNC_DELETE, PartitionDeleteUID },
  { 0xBB0E0136, sub_00001b6c },
  { 0xE6BA04A0, sub_00001b74 },
  { 0x6D7A1D29, sub_00001b7c },
  { 0, 0 }
};

//11e8
void PartitionServiceInit()
{
  int ret = sceKernelCreateUIDtype("SceSysmemMemoryPartition", sizeof(PartitionInfo), PartitionUidFuncTable, 0, &uidPartitionType);

  ASSERT(ret == 0);
}

//125c
int _CreateMemoryPartition(PartitionInfo *part, int attr, int addr, int size)
{
  if((addr & 0x1fffffff) < (membase & 0x1fffffff) || (addr & 0x1fffffff) + size > (membase & 0x1fffffff) + memsize)
  {
    //12c0
    Kprintf("partition.c:%s:p_membase 0x%08x, p_topaddr 0x%08x\n", "_CreateMemoryPartition",
            (membase & 0x1fffffff), (addr & 0x1fffffff));

    Kprintf("partition.c:%s:p_membase +size 0x%08x, p_topaddr+size 0x%08x\n", "_CreateMemoryPartition",
            (membase & 0x1fffffff) + size, (membase & 0x1fffffff) + memsize);

    return SCE_KERNEL_ERROR_OUT_OF_RANGE;
  }

  for(tmp = partitions; tmp; tmp = tmp->next)
  {
    int tmp_addr = tmp->addr;

    if(((tmp_addr & 0x1fffffff) < (addr & 0x1fffffff) && (addr & 0x1fffffff) < (tmp_addr & 0x1fffffff) + tmp->size)
            || ((tmp_addr & 0x1fffffff) < (addr & 0x1fffffff) + size && (addr & 0x1fffffff) + size < (tmp_addr & 0x1fffffff) + tmp->size))
    {
      //1410
      Kprintf("partition.c:%s:overlap(new) 0x%08p, size 0x%08x\n", "_CreateMemoryPartition",
              (addr & 0x1fffffff), size, (tmp_addr & 0x1fffffff), tmp->size);

      Kprintf("partition.c:%s:overlap(old) 0x%08p, psize 0x%08x\n", "_CreateMemoryPartition",
              (addr & 0x1fffffff), size, (tmp_addr & 0x1fffffff), tmp->size);

      return SCE_KERNEL_ERROR_MEM_RANGE_OVERLAP;
    }
  }

  //138c
  part->next = 0;
  part->addr = addr;
  part->size = size;
  part->attr = (part->attr & 0xFFFFFFF0) | (attr & 0xF);
  part->nBlocksUsed = 0;
  part->unk_18 = 0;

  PartitionInfo *tmp;
  for(tmp = partitions; tmp && tmp->next; tmp = tmp->next);

  tmp->next = part;

  //13e0
  SceUID blockid = sceKernelAllocPartitionMemory(0x1, "Memory Managing Block", 0, 0x100, 0);

  part->head = sceKernelGetBlockHeadAddr(blockid);

  return 0;
}

//1458
int sceKernelCreateMemoryPartition(char *name, int attr, int addr, int size)
{
  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *block;
  int ret = sceKernelCreateUID(uidPartitionType, name, IS_USER_MODE ? 0xFF : 0, &block);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);
    return ret;
  }

  //14fc
  if(_CreateMemoryPartition(UID_INFO(PartitionInfo, block, uidPartitionType), attr, addr, size) != 0)
  {
    //1558
    Kprintf("partition.c:%s:_CreateMemoryPartition failed\n", __FUNC__);

    sceKernelDeleteUID(block->UID);

    sceKernelCpuResumeIntr(intr);
    return SCE_KERNEL_ERROR_MEM_RANGE_OVERLAP;
  }

  if(size != 0)
  {
    //1548
    PartitionInit(UID_INFO(PartitionInfo, block, uidPartitionType));
  }

  //1534
  sceKernelCpuResumeIntr(intr);

  return block->UID;
}

//1580
int _DeleteMemoryPartition(PartitionInfo *part)
{
  PartitionInfo *kernel = GetPartition(SYSMEM_PARTITION_KERNEL);

  if(part == kernel || (part->head->subblocks[0].pos & MEMBLOCK_USEFLAG) || (part->head->subblocks[1].pos & MEMBLOCK_USEFLAG))
  {
    return SCE_KERNEL_ERROR_PARTITION_INUSE;
  }

  PartitionInfo *tmp;
  for(tmp = partitions; tmp; tmp = tmp->next)
  {
    if(tmp->next == part)
    {
      tmp->next = part->next;
      return 0;
    }
  }

  ASSERT(1 == 2);

  return SCE_KERNEL_ERROR_ERROR;
}

//1668
int sceKernelQueryMemoryPartitionInfo(int pid, PspSysmemPartitionInfo *info)
{
  PartitionInfo *part = GetPartition(pid);
  if(part == 0)
  {
    //170c
    return SCE_KERNEL_ERROR_ILLEGAL_PARTITION;
  }

  ASSERT(info != 0);

  int intr = sceKernelCpuSuspendIntr();

  if(info->size != sizeof(PspSysmemPartitionInfo))
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT;
  }

  //wtf is this here?
  PspSysmemPartitionInfo part2;
  part2.size = sizeof(PspSysmemPartitionInfo);
  part2.startaddr = part->addr;
  part2.memsize = part->size;
  part2.attr = part->attr & 0xF;

  info->size = sizeof(PspSysmemPartitionInfo);
  info->startaddr = part->addr;
  info->memsize = part->size;
  info->attr = part->attr & 0xF;

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//1750
int PartitionInit(PartitionInfo *part)
{
  if(part->size == 0)
  {
    return 0;
  }

  ASSERT(part > 0);

  int i;
  for(i = 0x13; i >= 0; i--)
  {
    part->head->subblocks[i].next = &part->head->subblocks[i] + sizeof(MemMgmtSubBlock);
    part->head->subblocks[i].pos = 0;
    part->head->subblocks[i].nblocks = 0;
  }

  part->head->subblocks[0x13].next = 0;

  part->head->subblocks[0].nblocks &= PARTITION_DELETED | PARTITION_SIZELOCKED;
  part->head->subblocks[0].nblocks |= ((part->size / 0x100) << 2);

  part->nBlocksUsed = 1;
}

//1810
int GetPartitionFreeMemSize(PartitionInfo *part)
{
  if(part->head->subblocks == 0)
  {
    return 0;
  }

  MemMgmtSubBlock *memblk;
  int free = 0;
  for(memblk = part->head->subblocks; memblk; memblk = memblk->next)
  {
    if(!memblk->pos & MEMBLOCK_USEFLAG && (memblk->nblocks >> 2) > free)
    {
      free = memblk->nblocks >> 2;
    }
  }

  return free * 0x100;
}

//1858
int sceKernelPartitionMaxFreeMemSize(int pid)
{
  int intr = sceKernelCpuSuspendIntr();

  PartitionInfo *part = GetPartition(pid);

  int ret = GetPartitionFreeMemSize(part);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//18a8
int SysMemUserForUser_E6581468()
{
  SET_K1_SRL16;

  int ret = sceKernelPartitionMaxFreeMemSize(SYSMEM_PARTITION_USER);

  RESET_K1;

  return ret;
}

//18d8
SceSize GetPartitionTotalFreeMemSize(PartitionInfo *part)
{
  if(part->head->subblocks == 0)
  {
    return 0;
  }

  int free = 0;
  MemMgmtSubBlock *memblk;
  for(memblk = part->head->subblocks; memblk; memblk = memblk->next)
  {
    if(!(memblk->pos & MEMBLOCK_USEFLAG))
    {
      free += memblk->nblocks >> 2;
    }
  }

  return free * 0x100;
}

//1918
SceSize sceKernelPartitionTotalFreeMemSize(int pid)
{
  int intr = sceKernelCpuSuspendIntr();

  SceSize size = GetPartitionTotalFreeMemSize(GetPartition(pid));

  sceKernelCpuResumeIntr(intr);

  return size;
}

//1968
SceSize SysMemUserForUser_9697CD32()
{
  SET_K1_SRL16;

  SceSize size = sceKernelPartitionTotalFreeMemSize(SYSMEM_PARTITION_USER);

  RESET_K1;

  return size;
}

//1998
void wmemset(int *addr, int bytes, int val)
{
  bytes /= 4;
  if(bytes <= 0)
  {
    return;
  }

  int i;
  for(i = 0; i < bytes; i++)
  {
    addr[i] = val;
  }
}

//19bc
int sceKernelFillFreeBlock(int pid, int val)
{
  PartitionInfo *part = GetPartition(pid);
  if(part == 0)
  {
    return SCE_KERNEL_ERROR_ERROR;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(part->head->subblocks[0].nblocks >> 2 == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return 0;
  }

  MemMgmtSubBlock *memblk;
  for(memblk = part->head->subblocks; memblk; memblk = memblk->next)
  {
    if(!(memblk->pos & MEMBLOCK_USEFLAG))
    {
      wmemset(part->addr + (memblk->pos >> 1) * 0x100, (memblk->nblocks >> 2) * 0x100, val);
    }
  }

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//1a80
int PartitionCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  PartitionInfo *info = UID_INFO(PartitionInfo, cb, uidPartitionType);
  info->next = 0;
  info->addr = 0;
  info->size = 0;
  info->attr &= 0xFFFFFFF0;
  info->head = 0;
  info->nBlocksUsed = 0;
  info->unk_18 = 0;

  return cb->UID;
}

//1ae4
int PartitionDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int ret;

  ret = _DeleteMemoryPartition(UID_INFO(PartitionInfo, cb, uidPartitionType));

  if(ret == 0)
  {
    SysMemForKernel_CE05CCB7(cb, type, funcid, args);

    ret = cb->UID;
  }

  return ret;
}

//1b6c
int loc_00001B6C(uidControlBlock *uid)
{
  return uid->UID;
}

//1b74
int loc_00001B74(uidControlBlock *uid)
{
  return uid->UID;
}

//1b7c
int sceKernelDeleteMemoryPartition(uidControlBlock *uid, int unk_a1, int unk_a2, SceSize *free)
{
  PartitionInfo *part = UID_INFO(PartitionInfo, uid, uidPartitionType);

  SceSize size = GetPartitionFreeMemSize(part);

  *free = size;

  return uid->UID;
}

//1bc8
int loc_00001bc8(SceUID uid)
{
  int intr = sceKernelCpuSuspendIntr();

  ASSERT(sceKernelDeleteUID(uid) == 0);

  sceKernelCpuResumeIntr(intr);

  return 0;
}

