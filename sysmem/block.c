uidControlBlock *uidBlockType; //c520

UidLookupFunction BlockUidFuncTable[] =
{
  { UIDFUNC_CREATE, BlockCreateUID },
  { UIDFUNC_DELETE, BlockDeleteUID },
  { 0xBB0E0136, sub_00003de0 },
  { 0x0927ECA3, sub_00003e20 },
  { 0x6CD3F455, sub_00003e58 },
  { 0x691F945E, sub_00003e98 },
  { 0x9A8F243F, sub_00003ed4 },
  { 0, 0 }
};

//1c4c
void MemoryBlockServiceInit()
{
  ASSERT(sceKernelCreateUIDtype("SceSysMemMemoryBlock", sizeof(BlockInfo), BlockUidFuncTable, 0, &uidBlockType) == 0);
}

//1cc0
int GetBlockAddrFromUID(SceUID uid)
{
  uidControlBlock *blk;

  if(sceKernelGetUIDcontrolBlockWithType(uid, uidBlockType, &blk) != 0)
  {
    return 0;
  }

  return UID_INFO(BlockInfo, blk, uidBlockType)->addr;
}

//1d10
int GetBlockSizeFromUID(SceUID uid)
{
  uidControlBlock *blk;

  if(sceKernelGetUIDcontrolBlockWithType(uid, uidBlockType, &blk) == 0)
  {
    return -1;
  }

  return UID_INFO(BlockInfo, blk->parent, uidBlockType)->size;
}

//1d60
SceUID GetUIDFromAddr(int addr)
{
  uidControlBlock *block;

  for(block = uidBlockType->parent; block != uidBlockType; block = block->parent)
  {
    BlockInfo *info = UID_INFO(BlockInfo, block, uidBlockType);
    if(addr >= info->addr && addr < info->addr + info->size)
    {
      return block->UID;
    }
  }

  return -1;
}

//1dc8
int GetPartitionFromAddr(int addr)
{
  PartitionInfo *part = AddrToCB(addr);
  if(part->next == uidPartitionType)
  {
    return -1;
  }

  uidControlBlock *cb;
  for(cb = uidPartitionType->parent; cb != uidPartitionType; cb = cb->parent)
  {
    PartitionInfo *pinfo = UID_INFO(PartitionInfo, cb->parent, uidPartitionType);
    if(pinfo == part)
    {
      return cb->UID;
    }
  }

  return -1;
}

//1e2c
MemMgmtSubBlock *AddrToMemMgmtSubBlock(PartitionInfo *part, int addr)
{
  if(part->head->subblocks == 0)
  {
    return 0;
  }

  int offset = (addr & 0x1FFFFFFF) - (part->addr & 0x1FFFFFFF);

  MemMgmtSubBlock *blk;
  for(blk = part->head->subblocks; blk; blk = blk->next)
  {
    if(offset >= (blk->pos >> 1) * 0x100 && offset < ((blk->pos >> 1) + (blk->nblocks >> 2)) > offset)
      {
        return blk;
      }
    }
  }

  return 0;
}

//1eac
int _AllocPartitionMemory(PartitionInfo *part, int type, SceSize size, void *addr)
{
  if(type == PSP_SMEM_Addr)
  {
    //1f44
    if(addr & 0xFF)
    {
      ASSERT(1 != 0); //wtf?

      //1f98
      ASSERT(((((addr + size + 0xFF) & ~0xFF) - (addr & ~0xFF)) & 0xFF) > 0);
    }
  }

  //1eec
  int ret = _allocSysMemory(part, type, size, addr);
  if(ret != 0)
  {
    ret = _CheckPartitionBlocks(part);
  }

  return ret;
}

//1fec
int sceKernelAllocPartitionMemoryForKernel(int pid, char *name, int type, SceSize size, void *addr)
{
  int ret;
  uidControlBlock *block;
  BlockInfo *blockinfo;

  if(type > PSP_SMEM_Addr)
  {
    Kprintf("block.c:%s:%04d:type arg illegal: type=%d \n", __FUNC__, __LINE__, type);

    return SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCKTYPE;
  }

  int intr = sceKernelCpuSuspendIntr();

  PartitionInfo *part = GetPartition(pid);
  if(part == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_PARTITION;
  }

  int old_k1 = pspSdkGetK1();

  ret = sceKernelCreateUID(uidBlockType, name, IS_USER_MODE ? 0xFF : ((IS_USER_MODE ? 0x10 : 0) & 0xFF), &block);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return ret;
  }

  //20c4
  BlockInfo *blockinfo = UID_INFO(BlockInfo, block, uidBlockType);
  void *buf = _AllocPartitionMemory(part, type, size, addr);
  if(buf == 0)
  {
    Kprintf("block.c:%s:failed, mpid 0x%08x, name %s, size 0x%x\n", __FUNC__, pid, name, size);

    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED;
  }

  blockinfo->addr = (int)buf;
  blockinfo->size = size - (size % 0x100);
  blockinfo->partition = part;

  sceKernelCpuResumeIntr(intr);

  return block.UID;
}

//2190
int sceKernelAllocPartitionMemoryForUser(int pid, char *name, int type, SceSize size, void *addr)
{
  PspSysmemPartitionInfo part;
  part.size = sizeof(PspSysmemPartitionInfo);

  int ret = sceKernelQueryMemoryPartitionInfo(pid, &part);
  if(ret < 0)
  {
    Kprintf("block.c:%s:sceKernelQueryMemoryPartitionInfo failed 0x%08x\n", __FUNC__, v0);

    return ret;
  }

  if((part.attr & 0x2) == 0)
  {
    return SCE_KERNEL_ERROR_ERROR;
  }

  SET_K1_SRL16;

  ret = sceKernelAllocPartitionMemoryForKernel(pid, "BlockForUser", type, size, addr);

  RESET_K1;

  return ret;
}

//225c
int sceKernelSizeLockMemoryBlock(SceUID uid)
{
  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *block;
  int ret = sceKernelGetUIDcontrolBlockWithType(uid, uidBlockType, &block);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return ret;
  }

  //22c0
  BlockInfo *binfo = UID_INFO(BlockInfo, uidBlockType, uidBlockType);

  MemMgmtSubBlock *blk = AddrToMemMgmtSubBlock(binfo->partition, binfo->addr);

  blk->nblocks |= MEMBLOCK_SIZELOCK;

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//22fc
int sceKernelResizeMemoryBlock(SceUID uid, int head_bytes, int tail_bytes)
{
  MemMgmtSubBlock oldblk1, oldblk2, *tmpblk, *tmpblk2, *memblk, *prevblk;
  int head_blocks = 0, tail_blocks = 0;

  if(head_bytes != 0)
  {
    head_blocks = head_bytes / 0x100; //s3
  }

  if(tail_bytes != 0)
  {
    tail_blocks = tail_bytes / 0x100; //s5
  }

  if(head_blocks == 0 && tail_blocks == 0)
  {
    return 0;
  }

  int intr = sceKernelCpuSuspendIntr();

  int ret;
  uidControlBlock *cb;
  if((ret = sceKernelGetUIDcontrolBlockWithType(unk_a0, uidBlockType, &cb)) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return ret;
  }

  //23f8
  BlockInfo *binfo = UID_INFO(BlockInfo, cb, uidBlockType);

  memblk = AddrToMemMgmtSubBlock(binfo->partition, binfo->addr);

  ASSERT(GET_MEMBLOCK_USED(memblk));

  if(GET_MEMBLOCK_SIZELOCKED(memblk))
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_LOCKED;
  }

  int head_addr = (binfo->partition->addr + GET_MEMBLOCK_POS(memblk) * 0x100 - head_bytes); //v1
  int tail_addr = (binfo->partition->addr + (GET_MEMBLOCK_POS(memblk) + GET_MEMBLOCK_NBLOCKS(memblk)) * 0x100 + tail_bytes + 0xFF); //v0
  if(head_addr / 0x100 >= tail_addr / 0x100)
  {
    //2950
    Kprintf("block.c:%s:head and tail position is illegal\n", __FUNC__);

    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_FAILED;
  }

  if(head_addr < binfo->partition->addr || head_addr >= (binfo->partition->addr + binfo->partition->size)))
  {
    //24a4
    Kprintf("block.c:%s:resized block addr is out of partition address range\n", __FUNC__);

    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_FAILED;
  }

  if(GET_MEMBLOCK_POS(memblk) != 0)
  {
    prevblk = AddrToMemMgmtSubBlock(binfo->partition, binfo->partition->addr + (GET_MEMBLOCK_POS(memblk) - 1) * 0x100);
  }

  if(head_bytes > 0)
  {
    if(GET_MEMBLOCK_USED(prevblk) || GET_MEMBLOCK_NBLOCKS(prevblk) < s3)
    {
      Kprintf("block.c:%s:Previous block is used, so cannot extend this block.\n", __FUNC__);

      sceKernelCpuResumeIntr(intr);

      return SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_FAILED;
    }
  }

  if(tail_bytes > 0)
  {
    if(GET_MEMBLOCK_USED(memblk->next) || GET_MEMBLOCK_NBLOCKS(memblk->next) < s5)
    {
      //2540
      Kprintf("block.c:%s:Next block is used, so cannot extend this block.\n", __FUNC__);

      sceKernelCpuResumeIntr(intr);

      return SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_FAILED;
    }
  }

  if(head_blocks != 0)
  {
    binfo->addr = head_addr;
    if(head_bytes > 0)
    {
      SET_MEMBLOCK_POS(memblk, GET_MEMBLOCK_POS(memblk) - head_blocks);
      SET_MEMBLOCK_NBLOCKS(memblk, GET_MEMBLOCK_NBLOCKS(memblk) + head_blocks);

      binfo->partition->size += head_blocks * 0x100;

      if(GET_MEMBLOCK_NBLOCKS(prevblk) == head_blocks)
      {
        binfo->partition->nBlocksUsed--;

        for(tmpblk = memblk, tmpblk2 = prevblk; tmpblk; tmpblk = tmpblk->next, tmpblk2 = tmpblk2->next)
        {
          *tmpblk2 = *tmpblk;
        }

        memblk = prevblk;

        sub_00003BC4(binfo->partition);
      }
      else
      {
        SET_MEMBLOCK_NBLOCKS(prevblk, GET_MEMBLOCK_NBLOCKS(prevblk) - head_blocks);
      }
    }
    else
    {
      SET_MEMBLOCK_POS(memblk, GET_MEMBLOCK_POS(memblk) + head_blocks);
      SET_MEMBLOCK_NBLOCKS(memblk, GET_MEMBLOCK_NBLOCKS(memblk) - head_blocks);

      binfo->addr -= head_blocks * 0x100;

      if(GET_MEMBLOCK_USED(prevblk))
      {
        SET_MEMBLOCK_NBLOCKS(prevblk, GET_MEMBLOCK_NBLOCKS(prevblk) + head_blocks);
      }
      else
      {
        SET_MEMBLOCK_POS(&oldblk1, v1);
        SET_MEMBLOCK_NBLOCKS(&oldblk1, head_blocks);

        for(tmpblk = memblk; tmpblk; tmpblk = tmpblk->next)
        {
          oldblk2 = *tmpblk;

          *tmpblk = oldblk1;

          tmpblk->next = oldblk2.next;

          oldblk1 = oldblk2;

          tmpblk = tmpblk->next;
        }

        sub_00003BC4(binfo->partition);
      }
    }
  }

  if(tail_blocks != 0)
  {
    if(tail_bytes > 0)
    {
      SET_MEMBLOCK_NBLOCKS(memblk, GET_MEMBLOCK_NBLOCKS(memblk) + tail_blocks);

      binfo->partition->addr += tail_blocks * 0x100;

      if(GET_MEMBLOCK_NBLOCKS(memblk->next) == tail_blocks)
      {
        binfo->partition->nBlocksUsed--;

        if(memblk->next->next != 0)
        {
          for(tmpblk = memblk->next; tmpblk; tmpblk = tmpblk->next)
          {
            *tmpblk = *tmpblk->next;
          }
        }

        sub_00003BC4(binfo->partition);
      }
      else
      {
        SET_MEMBLOCK_POS(memblk->next, GET_MEMBLOCK_POS(memblk->next) + tail_blocks;
        SET_MEMBLOCK_NBLOCKS(memblk->next, GET_MEMBLOCK_NBLOCKS(memblk->next) - tail_blocks);
      }
    }
    else
    {
      SET_MEMBLOCK_NBLOCKS(memblk, GET_MEMBLOCK_NBLOCKS(memblk) - tail_blocks);

      binfo->partition->addr -= tail_blocks * 0x100;

      if(GET_MEMBLOCK_USED(memblk->next))
      {
        binfo->partition->nBlocksUsed++;

        SET_MEMBLOCK_POS(&oldblk2, GET_MEMBLOCK_POS(memblk) + GET_MEMBLOCK_NBLOCKS(memblk));
        UNSET_MEMBLOCK_USED(&oldblk2);
        SET_MEMBLOCK_NBLOCKS(&oldblk2, tail_blocks);

        for(tmpblk = memblk->next; tmpblk; tmpblk = tmpblk->next)
        {
          oldblk1 = *tmpblk;

          *tmpblk = oldblk2;

          *tmpblk->next = oldblk1.next;

          oldblk2 = oldblk1;
        }

        sub_00003BC4(binfo->partition);
      }
      else
      {
        SET_MEMBLOCK_POS(memblk->next, GET_MEMBLOCK_POS(memblk->next) - tail_blocks);
        SET_MEMBLOCK_NBLOCKS(memblk->next, GET_MEMBLOCK_NBLOCKS(memblk->next) + tail_blocks);
      }
    }
  }

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//29a8
int sceKernelJointMemoryBlock()
{
  return SCE_KERNEL_ERROR_NOTIMP;
}

//29b4
int SysMemForKernel_915EF4AC()
{
  return SCE_KERNEL_ERROR_NOTIMP;
}

//29c0
int _FreePartitionMemory(int addr)
{
  PartitionInfo *part1 = GetPartition(SYSMEM_PARTITION_KERNEL);

  PartitionInfo *part = AddrToCB(addr);

  if(part == part1)
  {
    //2a34
    MemMgmtBlock *mgmtblk;
    for(mgmtblk == part->head; mgmtblk; mgmtblk = mgmtblk->next)
    {
      if(addr == mgmtblk)
      {
        //2a5c
        Kprintf("block.c:%s:%04d:illegal addr %x ! It is system control block\n", __FUNC__, __LINE__, addr);

        return SCE_KERNEL_ERROR_ERROR;
      }
    }
  }

  //29f8
  int ret = _freeSysMemory(part, addr);
  if(ret == 0)
  {
    _CheckPartitionBlocks(part);
  }

  //2a0c
  return ret;
}

//2a80
int sceKernelFreePartitionMemory(SceUID uid)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *block;
  int ret = sceKernelGetUIDcontrolBlockWithType(uid, uidBlockType, &block);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    pspSdkSetK1(old_k1);

    return ret;
  }

  if(IS_USER_MODE && !(block->attr & 0x10))
  {
    sceKernelCpuResumeIntr(intr);

    pspSdkSetK1(old_k1);

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ret = sceKernelDeleteUID(uid);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//2b40
void _QueryMemoryInfo(int addr, SceUID *partitionid, SceUID *blockid)
{
  *partitionid = GetPartitionFromAddr(addr);

  *blockid = GetUIDFromAddr(addr);
}

//2b8c
int sceKernelQueryMemoryInfo(int addr, SceUID *partitionid, SceUID *blockid)
{
  if(partitionid <= 0 || blockid <= 0)
  {
    return SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT;
  }

  int intr = sceKernelCpuSuspendIntr();

  _QueryMemoryInfo(addr, partitionid, blockid);

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//2c0c
int sceKernelQueryBlockSize(int addr)
{
  int ret = -1;

  int intr = sceKernelCpuSuspendIntr();

  PartitionInfo *part = AddrToCB(addr);

  ASSERT(part > 0);

  if(part == 0)
  {
    //2cd0
    sceKernelCpuResumeIntr(intr);

    return 0;
  }

  MemMgmtSubBlock *memblk = AddrToMemMgmtSubBlock(part, addr);
  if(memblk != 0)
  {
    ret = ((!(memblk->pos & MEMBLOCK_USEFLAG)) << 31) | ((memblk->nblocks >> 2) * 0x100);
  }

  //2ca4
  sceKernelCpuResumeIntr(intr);

  return ret;
}

//2cfc
int sceKernelQueryMemoryBlockInfo(SceUID uid, PspSysmemBlockInfo *info)
{
  int intr, ret;
  uidControlBlock *cb;
  BlockInfo *binfo;

  ASSERT(info != 0);

  if(info->size != sizeof(PspSysmemBlockInfo))
  {
    return SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT;
  }

  intr = sceKernelCpuSuspendIntr();

  ret = sceKernelGetUIDcontrolBlockWithType(uid, uidBlockType, &cb);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return ret;
  }

  binfo = UID_INFO(BlockInfo, cb, uidBlockType);
  MemMgmtSubBlock *memblk = AddrToMemMgmtSubBlock(binfo->partition, binfo->addr);

  info->useflag = memblk->pos & MEMBLOCK_USEFLAG;
  info->sizelock = ((memblk->nblocks & MEMBLOCK_SIZELOCK) != 0);
  info->addr = GetBlockAddrFromUID(uid);
  info->size = GetBlockSizeFromUID(uid);

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//2e64
int sceKernelGetBlockHeadAddr(SceUID uid)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  int ret = GetBlockAddrFromUID(uid);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//2ec4
void _SetBlockDeleted(PartitionInfo *part, int addr)
{
  MemMgmtSubBlock *memblk = AddrToMemMgmtSubBlock(part, addr);

  memblk->nblocks |= MEMBLOCK_DELFLAG;
}

//2eec
SysMemForKernel_2F3B7611(unk_a0)
{
  s4 = unk_a0;

  int intr = sceKernelCpuSuspendIntr();

  Kprintf("\n ++++ BLOCK INFO ... ++++ \n\n");

  a1 = uidBlockType;
  v0 = unk_a0 >> 7;
  a0 = 0x88000000;
  v1 = uidBlockType->size;
  v0 = v0 << 2;
  v0 |= a0;
  v1 = v1 << 2;
  s3 = (((unk_a0 >> 7) << 2) | 0x88000000) + uidBlockType->size * sizeof(int);
  a0 = 0x8(s3);
  a1 = 0x0(s3);
  s2 = AddrToMemMgmtSubBlock(s3.8, s3.0);
  v1 = s3.8;
  v0 = v1.10 + 4;
  a0 = s4;
  if(s2 == v1.10 + 4)
  {
    s1 = v0;
    //2f7c
    while(1 == 1)
    {
      v0 = 0x0(v0);
      if(v0 == s2)
      {
        break;
      }

      s1 = v0;
    }

    a0 = s1.4;
    a1 = v1.4;
    v0 = s1.4 >> 1;
    s0 = v1.4 + v0;
    a1 = "USED";
    if(s1.4 == 0)
    {
      a1 = "FREE";
    }

    //2fb4
    t0 = (s1.8 >> 2) << 8;
    Kprintf("Prev Block Info      : status <%s>, range 0x%08x - 0x%08x (size 0x%08x)\n", s1.4 ? "USED" : "FREE", s0, s0 + t0,

    if((s1.4 & 0x1) == 0)
    {
      a0 = 0; //???
      //31d4
      Kprintf(a0 - 0x4B20);
    }
    else if((s0 = GetUIDFromAddr(s0)) < 0)
    {
      //31d0
      a0 = 0x10000;
      //31d4
      Kprintf(a0 - 0x4B20);
    }
    else
    {
      if(sceKernelGetUIDname(s0, 0x1E, sp) != 0)
      {
        //31b0
        Kprintf("Can't Get Block name ... [UID 0x%08x]\n\n", s0);

        sceKernelCpuResumeIntr(intr);

        return SCE_KERNEL_ERROR_ERROR;
      }

      Kprintf("                     : UID [0x%08x], Block name [%s]\n\n", a1, sp);
    }
  }

  //3020
  if(sceKernelGetUIDname(unk_a0, 0x1E, sp) != 0)
  {
    //3164
    sceKernelCpuResumeIntr(intr);

    return 0;
  }

  v0 = 0x4(s2);
  v0 = v0 & 0x1;
  a1 = "USED";
  if(v0 == 0)
  {
    a1 = "FREE";
  }

  //304c
  t0 = 0x8(s2);
  t0 = t0 >> 2;
  t0 = t0 << 8;
  Kprintf("Aforesaid Block Info : status <%s>, range 0x%08x - 0x%08x (size 0x%08x)\n", (s2.4 & 0x1) ? "USED" : "FREE", s3.0, s3.0 + t0, t0);

  Kprintf("                     : UID [0x%08x], Block name [%s]\n\n", unk_a0, sp);

  s1 = s2.0;
  s2 = 0x1;
  if(s1 != 0)
  {
    a2 = s3.8;

    //30a4
    while(1 == 1)
    {
      v1 = s1.4;
      a1 = s2;
      a3 = a2.4;
      v0 = v1 >> 1;
      v0 = v0 << 8;
      s0 = a3 + v0;
      v1 = v1 & 0x1;
      a0 = s4;
      a3 = s0;
      a2 = "USED";
      if(v1 == 0)
      {
        a2 = "FREE";
      }

      //30d4
      t0 = s1.8;
      t0 = t0 >> 2;
      t0 = t0 << 8;
      Kprintf("Next Block Info (%d)  : status <%s>, range 0x%08x - 0x%08x (size 0x%08x)\n", a1, (s1.4 & 0x1) ? "USED" : "FREE", a3 + v0, s0 + t0, t0);

      a0 = s0;
      if((s1.4 & 0x1) == 0)
      {
        //31a0
        Kprintf(s6 - 0x4B20);

        s1 = 0x0(s1);
        //->3140
      }
      else
      {
        s0 = GetUIDFromAddr(s0);
        if(s0 >= 0)
        {
          if(sceKernelGetUIDname(s0, 0x1E, sp) != 0)
          {
            //3164
            sceKernelCpuResumeIntr(intr);

            return 0;
          }

          Kprintf("                     : UID [0x%08x], Block name [%s]\n\n", s0, sp);
        }

        //313c
        s1 = 0x0(s1);
      }

      //3140
      s2++;
      if(s2 >= 0x4 || s1 <= 0)
      {
        break;
      }

      a2 = s3.8;
    }
  }

  //3158
  Kprintf("\n ++++ BLOCK INFO END ++++ \n\n");

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//31e4
int _allocSysMemory(PartitionInfo *part, int type, SceSize size, void *addr)
{
  MemMgmtSubBlock *memblk = 0, *tempblk;
  MemMgmtSubBlock oldblk1, oldblk2;

  ASSERT(part > 0);

  int max_free_blocks = 0;
  int blocks_needed = (size + 0xFF) / 0x100;
  if(blocks_needed == 0)
  {
    Kprintf("block.c:%s:Illegal allocate size (=0)\n", "_allocSysMemory");

    return 0;
  }

  if(type == PSP_SMEM_High)
  {
    if(part->head->subblocks != 0)
    {
      for(tempblk = part->head->subblocks; tempblk; tempblk = tempblk->next)
      {
        if(!(tempblk->pos & MEMBLOCK_USEFLAG))
        {
          if((tempblk->numblocks >> 2) >= blocks_needed)
          {
            memblk = tempblk;
          }
        }
      }

      if(memblk != 0)
      {
        if(memblk->nblocks >> 2 == blocks_needed)
        {
          memblk->nblocks &= ~(MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK);
          memblk->pos |= MEMBLOCK_USEFLAG;

          return part->addr + (memblk->pos >> 1) * 0x100;
        }

        part->nBlocksUsed++;

        memblk->nblocks = (((memblk->nblocks >> 2) - blocks_needed) << 2) | (memblk->nblocks & (MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK));

        oldblk1.nblocks = ((blocks_needed << 2) | (oldblk1.nblocks & (MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK)) & ~MEMBLOCK_SIZELOCK) & ~(MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK);
        oldblk1.pos = (oldblk1.pos & MEMBLOCK_USEFLAG) | (((memblk->pos >> 1) + (memblk->nblocks >> 2)) << 1) | MEMBLOCK_USEFLAG;

        tempblk = memblk->next;
        if(oldblk1.nblocks >> 2 > 0)
        {
          do
          {
            oldblk2 = *tempblk;

            *tempblk = oldblk1;

            tempblk->next = oldblk2.next;

            oldblk1 = oldblk2;

            tempblk = tempblk->next;
          } while(oldblk2.nblocks >> 2 > 0);
        }

        return part->addr + startpos;
      }

      max_free_blocks = 0;
      for(tempblk = part->head->subblocks; tempblk; tempblk = tempblk->next)
      {
        if(!(tempblk->pos & MEMBLOCK_USEFLAG))
        {
          max_free_blocks = MAX(max_free_blocks, tempblk->nblocks >> 2);
        }
      }
    }

    Kprintf("block.c:%s:High: no more space, can not alloc \n", __FUNC__);

    Kprintf("block.c:%s:High: request nblocks %u (max %u)\n", __FUNC__, blocks_needed, max_free_blocks);

    return 0;
  }
  else if(type == PSP_SMEM_Low)
  {
    if(part->head->subblocks != 0)
    {
      for(tempblk = part->head->subblocks; tempblk; tempblk = tempblk->next)
      {
        if(!(tempblk->pos & MEMBLOCK_USEFLAG))
        {
          if((tempblk->numblocks >> 2) >= blocks_needed)
          {
            if((tempblk->numblocks >> 2) == blocks_needed)
            {
              memblk->nblocks &= ~(MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK);
              memblk->pos |= MEMBLOCK_USEFLAG;

              return part->addr + (memblk->pos >> 1) * 0x100;
            }
            else
            {
              goto label_33f0;
            }
          }
        }
      }

      max_free_blocks = 0;
      for(tempblk = part->head->subblocks; tempblk; tempblk = tempblk->next)
      {
        if(!(tempblk->pos & MEMBLOCK_USEFLAG))
        {
          max_free_blocks = MAX(max_free_blocks, tempblk->nblocks >> 2);
        }
      }
    }

    Kprintf("block.c:%s:Low: no more space, can not alloc \n", __FUNC__);

    Kprintf("block.c:%s:Low: request nblocks %u (max %u)\n", __FUNC__, blocks_needed, max_free_blocks);

    return 0;
  }
  else if(unk_a1 == PSP_SMEM_Addr)
  {
    int offset = ((addr & 0x1FFFFFFF) - (part->addr & 0x1FFFFFFF)) & 0xFF);
    if(offset != 0)
    {
      Kprintf("block.c:%s:%s: Illigal alloc address or size, offset=0x%x\n", __FUNC__, "SCE_KERNEL_SMEM_Addr", s0);

      return 0;
    }

    int startpos = offset / 0x100; //a3

    for(tempblk = part->head->subblocks; tempblk && (tempblk->pos >> 1) > startpos); tempblk = tempblk->next)
    {
      if(!(tempblk->pos & MEMBLOCK_USEFLAG) && (tempblk->pos >> 1) + (tempblk->nblocks >> 2) >= startpos + blocks_needed)
      {
        break;
      }
    }

    if(tempblk == 0 || (tempblk->pos >> 1) > startpos)
    {
      Kprintf("block.c:%s:SCE_KERNEL_SMEM_Addr: can not alloc 0x%08x\n", "_allocSysMemory", startpos);

      Kprintf("block.c:%s:offset 0x%08x, size 0x%08x\n", "_allocSysMemory", offset, size);

      return 0;
    }

    if(tempblk->pos >> 1 < startpos)
    {
      part->nBlocksUsed++;

      tempblk->nblocks = ((tempblk->nblocks) & (MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK)) |
           (((tempblk->nblocks >> 2) - ((tempblk->pos >> 1) + (tempblk->nblocks >> 2) - startpos)) << 2);

      oldblk1.pos = (((tempblk->pos >> 1) + (tempblk->nblocks >> 2)) << 1) | (oldblk1.pos & MEMBLOCK_USEFLAG) & ~MEMBLOCK_USEFLAG;
      oldblk1.nblocks = ((((tempblk->pos >> 1) + (tempblk->nblocks >> 2) - startpos) << 2) | (oldblk1.nblocks & (MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK))) & ~(MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK);

      memblk = tempblk;

      if(oldblk1.nblocks != 0)
      {
        tempblk = tempblk->next;
        do
        {
          oldblk2 = *tempblk;

          *tempblk = oldblk1;

          tempblk->next = oldblk2.next;

          oldblk1 = oldblk2;

          tempblk = tempblk->next;
        } while(oldblk2.nblocks >> 2 > 0);
      }
    }

    if(memblk->nblocks >> 2 == blocks_needed)
    {
      memblk->nblocks &= ~(MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK);
      memblk->pos |= MEMBLOCK_USEFLAG;

      return part->addr + (memblk->pos >> 1) * 0x100;
    }

    goto label_33f0;
  }

  return 0;

label_33f0:
  int startpos = (tempblk->pos >> 1) * 0x100;

  part->nBlocksUsed++;

  oldblk1 = *tempblk;

  tempblk->nblocks = blocks_needed << 2;
  tempblk->pos |= MEMBLOCK_USEFLAG;

  oldblk1.pos = (((oldblk1.pos >> 1) + blocks_needed) << 1) | (oldblk1.pos & MEMBLOCK_USEFLAG);
  oldblk1.nblocks = (((oldblk1.nblocks >> 2) - blocks_needed) << 2) | (oldblk1.nblocks & (MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK));

  tempblk = tempblk->next;
  if(oldblk1.nblocks >> 2 > 0)
  {
    do
    {
      oldblk2 = *tempblk;

      *tempblk = oldblk1;

      tempblk->next = oldblk2.next;

      oldblk1 = oldblk2;

      tempblk = tempblk->next;
    } while(oldblk2.nblocks >> 2 > 0);
  }

  return part->addr + startpos;
}

//37ac
int _freeSysMemory(PartitionInfo *part, int addr)
{
  ASSERT(part > 0);

  ASSERT(addr > 0);

  if((addr & 0xFF) != 0)
  {
    Kprintf("block.c:%s:illegal addr 0x%08x\n", "_freeSysMemory", addr);

    return -1;
  }

  MemMgmtSubBlock *memblk = part->head->subblocks, *lastblk = 0, *tempblk, *nextblk;
  int offset = (addr & 0x1FFFFFFF) - (part->addr & 0x1FFFFFFF);
  int startpos = (offset + (offset < 0) ? 0xFF : 0) / 0x100;

  if(part->head->subblocks != 0)
  {
    for(memblk = part->head->subblocks; memblk; memblk = memblk->next)
    {
      //3840
      if(memblk->nblocks >> 2 != 0)
      {
        if(memblk->pos >> 1 == startpos)
        {
          if(memblk->nblocks & MEMBLOCK_DELFLAG)
          {
            SceUID uid = GetUIDFromAddr(addr);

            char name[0x20];
            sceKernelGetUIDname(uid, 0x20, name);

            Kprintf("block.c:%s:Deleting Protected Block: name[%s] addr 0x%08x\n", "_freeSysMemory", name, addr);

            PartitionInfo *part = GetPartition(SYSMEM_PARTITION_KERNEL);
            MemMgmtSubBlock *tempblk = v0 = AddrToMemMgmtSubBlock(part, addr);

            Kprintf("block.c:%s:                        : pos      0x%x\n", __FUNC__, tempblk->pos >> 1);
            Kprintf("block.c:%s:                        : nblocks  %d\n", __FUNC__, tempblk->nblocks >> 2);
            Kprintf("block.c:%s:                        : useflag  %d\n", __FUNC__, tempblk->pos & MEMBLOCK_USEFLAG);
            Kprintf("block.c:%s:                        : delflag  %d\n", __FUNC__, tempblk->nblocks & MEMBLOCK_DELFLAG);
            Kprintf("block.c:%s:                        : sizelock %d\n", __FUNC__, (tempblk->nblocks & MEMBLOCK_SIZELOCK) ? 1 : 0);
            Kprintf("block.c:%s:                        : next     0x%08x\n", __FUNC__, tempblk->next);

            ASSERT(1 == 2);

            return SCE_KERNEL_ERROR_ERROR;
          }

          if(!(memblk->pos & MEMBLOCK_USEFLAG))
          {
            Kprintf("block.c:%s:memory segment %x,%x already free \n", __FUNC__, (memblk->pos >> 1) * 0x100, (memblk->pos >> 2) * 0x100);

            ASSERT(1 == 2);

            return SCE_KERNEL_ERROR_ERROR;
          }

          memblk->pos &= ~MEMBLOCK_USEFLAG;
          memblk->nblocks &= ~(MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK);

          int count = 0;
          tempblk = 0;
          if(memblk->next != 0 && (memblk->next->nblocks >> 2) != 0 && !(memblk->next->pos & MEMBLOCK_USEFLAG))
          {
            part->nBlocksUsed--;

            tempblk = memblk->next;
            count++;

            memblk->nblocks = (((memblk->nblocks >> 2) + (memblk->next->nblocks >> 2)) << 2) | (memblk->nblocks & (MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK));
          }

          if(lastblk != 0 && !(lastblk->pos & MEMBLOCK_USEFLAG))
          {
            part->nBlocksUsed--;

            tempblk = memblk;
            count++;

            lastblk->nblocks = (((lastblk->nblocks >> 2) + (memblk->nblocks >> 2)) << 2) | (lastblk->nblocks & (MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK));
          }

          if(count == 0)
          {
            return 0;
          }

          for(memblk = tempblk; count > 0; memblk = memblk->next, count--);

          if(memblk == 0)
          {
            return 0;
          }

          do
          {
            nextblk = tempblk->next;

            *tempblk = *memblk;

            tempblk->next = nextblk;

            memblk = memblk->next;
            tempblk = nextblk;
          } while(memblk);

          return 0;
        }
      }

      lastblk = memblk;
    }
  }

  Kprintf("block.c:%s:can not find memory segment 0x%p\n", __FUNC__, addr);

  ASSERT(1 == 2);

  return SCE_KERNEL_ERROR_ERROR;
}

//3bc4
void _CheckPartitionBlocks(PartitionInfo *part)
{
  MemMgmtBlock *mgmtblk = (MemMgmtBlock *)part->head;
  part->nBlocksUsed++;
  while(mgmtblk->next)
  {
    mgmtblk = mgmtblk->next;
  }

  //3c08
  if(mgmtblk->subblocks[0x10].nblocks >> 2 > 0)
  {
    PartitionInfo *part1 = GetPartition(SYSMEM_PARTITION_KERNEL);

    MemMgmtBlock *newblk = (MemMgmtBlock *)_allocSysMemory(part1, 0, 0x100, 0);

    mgmtblk->next = newblk;
    mgmtblk->subblocks[0x13].next = &newblk->subblocks[0];
    newblk->next = 0;

    int i;
    for(i = 0; i < 0x14; i++)
    {
      newblk->subblocks[i].next = &newblk->subblocks[i] + sizeof(MemMgmtSubBlock);
    }

    newblk->subblocks[0x13].next = 0;
  }

  if(mgmtblk->next == 0)
  {
    return;
  }

  MemMgmtBlock *tmpblk, *lastblk;
  for(tmpblk = mgmtblk->next; tmpblk->next && tmpblk->next->next; tmpblk = tmpblk->next)
  {
    lastblk = tmpblk;
  }

  if(tmpblk == 0)
  {
    return;
  }

  if(lastblk->subblocks[0x10].nblocks >> 2 == 0)
  {
    lastblk->next = 0;

    lastblk->subblocks[0x13].next = 0;

    part1 = GetPartition(SYSMEM_PARTITION_KERNEL);

    _freeSysMemory(part1, tmpblk);
  }
}

//3d08
int BlockCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  BlockInfo *info = UID_INFO(BlockInfo, cb, uidBlockType);

  info->buf = 0;
  info->size = 0;
  info->partition = 0;

  return cb->UID;
}

//3d50
int BlockDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int ret;

  BlockInfo *info = UID_INFO(BlockInfo, cb, uidBlockType);
  if(info->buf != 0)
  {
    ret = _FreePartitionMemory(info->buf);
    if(ret == 0)
    {
      return ret;
    }
  }

  //3da8
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//3de0
int sub_00003DE0(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int *args2 = (int *)args2;

  int ret = sceKernelResizeMemoryBlock(cb->UID, args[0], args[4]);
  if(ret == 0)
  {
    ret = cb->UID;
  }

  return ret;
}

//3e20
int sub_00003E20(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int ret = sceKernelSizeLockMemoryBlock(cb->UID);
  if(ret == 0)
  {
    ret = cb->UID;
  }

  return ret;
}

//3e58
sub_00003E58(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  _QueryMemoryInfo(*((int *)args), (args + 4).unk_0, (args + 4).unk_4);

  return cb->UID;
}

//3e98
int sub_00003E98(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int ret = sceKernelQueryMemoryBlockInfo(cb->UID, *args);
  if(ret == 0)
  {
    ret = cb->UID;
  }

  return ret;
}

//3ed4
int sub_00003ED4(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  (*args).unk_0 = GetBlockAddrFromUID(cb->UID);

  return cb->UID;
}

//3f14
int sceKernelFreePartitionMemory(SceUID uid)
{
  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *block;
  int ret = sceKernelGetUIDcontrolBlockWithType(uid, uidBlockType, &block);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return ret;
  }

  if(IS_USER_MODE && !(block->attr & 0x10))
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  ret = sceKernelDeleteUID(uid);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//3fbc
int sceKernelGetBlockHeadAddr(SceUID uid)
{
  int intr = sceKernelCpuSuspendIntr();

  int ret = GetBlockAddrFromUID(uid);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//4004
int sub_00004004(HeapMgmtBlock *heap, SceSize size)
{
  if(buf <= 0 || size <= sizeof(HeapMgmtBlock) + 0x10)
  {
    return v0;
  }

  heap.unk_0 = heap - 1;
  heap.unk_4 = size;
  heap.unk_8 = 0;
  heap.unk_C = heap + 0x10;
//0x30 -> heap + 0x10 + 0x20 - 0x8 -> heap + 0x28
  heap.unk_10 = (heap + 0x10 + ((size - 0x10) - ((size - 0x10) % 0x8))) - 0x8;
  heap.unk_14 = (size - 0x10) / 0x8 - 1;

//0x30 -> heap + 0x10 + 0x20 - 0x8
  heap[(0x10 + ((size - 0x10) & ~0x7)) / 4 - 2] = heap + 0x10;
  heap[(0x10 + ((size - 0x10) & ~0x7)) / 4 - 1] = 0;

  return 0;
}

//405c
int sub_0000405C(unk_a0, int size, int align)
{
  if(unk_a0 == 0)
  {
    return 0;
  }

  v1 = 0x0(unk_a0);
  v0 = unk_a0 - 1;
  if(v1 != v0)
  {
    return 0;
  }

  size = MAX(size, 0x8);

  t1 = 0xC(unk_a0);
  v0 = a1 + 0x7;
  v0 = v0 >> 3;
  a3 = 0x0(t1);
  a1 = v0 + 1;
  t2 = t1;
  t5 = a1 << 3;
  t5 = ((size + 0x7) / 0x8 + 1) * 0x8;

  //40a0
  while(1 == 1)
  {
    v1 = 0x4(a3);
    v0 = v1 < a1;
    if(v0 != 0)
    {
      //40fc
      if(a3 == t1)
      {
        return 0;
      }

      t2 = a3;
      a3 = 0x0(a3);
      continue;
    }

    t4 = 0;
    if(v1 == a1)
    {
      //417c
      t0 = a3 + 0x8;
      if(a2 == 0)
      {
        //419c
        v0 = 0x0(a3);
        0x0(t2) = v0;
        //4148
      }

      //divu t0, a2
      v0 = t0 % a2;
      if(v0 != 0)
      {
        //40fc
        if(a3 == t1)
        {
          return 0;
        }

        t2 = a3;
        a3 = 0x0(a3);
        continue;
      }

      //419c
      v0 = 0x0(a3);
      0x0(t2) = v0;
      //4148
    }

    t3 = 0;
    if(a2 == 0)
    {
      //4174
      t0 = a1;
      //4118
    }

    v0 = v1 - a1;
    v0 = v0 << 3;
    v0 = a3 + v0;
    v0 += 0x8;
    //divu v0, a2

    //40dc
    v0 = v0 % a2;
    t0 = a1;
    if(v0 == 0)
    {
      //4118
      v0 = v1 - t0;
      v1 = v0 << 3;
      0x4(a3) = v0;
      a3 = a3 + v1;
      t0 = a3 + 0x8;
      0x4(a3) = a1;
      if(t3 != 0)
      {
        v1 = 0x0(t4);
        v0 = a3 + t5;
        0x4(v0) = t3;
        0x0(v0) = v1;
        0x0(t4) = v0;
      }

      //4148
      0xC(a0) = t2;
      0x0(a3) = a0;
      v1 = 0x4(a3);
      v0 = 0x8(a0);
      v0 += v1;
      0x8(a0) = v0;
      return t0;
    }

    t3 = v0 >> 3;
    t0 = a1 + t3;
    v0 = v1 < t0;
    if(v0 == 0)
    {
      //4110
      t4 = a3;
      if(v1 == t0)
      {
        //4164
        v0 = 0x0(a3);
        t4 = t2;
        0x0(t2) = v0;
        //4118
      }
    }

    //40fc
    if(a3 == t1)
    {
      return 0;
    }

    t2 = a3;
    a3 = 0x0(a3);
  }
}

//41a8
sub_000041A8(unk_a0, unk_a1)
{
  a2 = a1 - 0x8;
  if(a0 == 0)
  {
    //41c0
    return -4;
  }

  v1 = 0x0(a0);
  v0 = a0 - 1;
  if(v1 != v0)
  {
    return -4;
  }

  v0 = a0 + 0x10;
  //41cc
  v0 = a2 < v0;
  if(v0 != 0)
  {
    return -2;
  }
  
  v0 = 0x4(a0);
  v0 = a0 + v0;
  v0 = a2 < v0;
  if(v0 == 0)
  {
    return -2;
  }

  if(a1 == 0)
  {
    return -1;
  }

  v0 = -0x8(a1);
  if(v0 != a0)
  {
    return -1;
  }

  t0 = 0x4(a2);
  //4208
  if(t0 <= 0)
  {
    return -1;
  }

  a3 = 0xC(a0);
  v0 = a3 < a2;
  ...
}

//4334
int sub_00004334(HeapMgmtBlock *heap)
{
  if(heap == 0)
  {
    return 0;
  }

  if(heap->unk_0 == (int)heap - 1)
  {
    return (heap->unk_8 <= 0);
  }

  return 0;
}

//435c
int sub_0000435C(HeapMgmtBlock *heap)
{
  return (heap->unk_4 - 0x10 - (heap->unk_8 ? 0x8 : 0));
}

//437c
void sub_0000437C()
{
  *(0xBC000030) = 0;
  *(0xBC000034) = 0;
  *(0xBC000038) = 0x400;
  *(0xBC00003C) = 0x4000000;
  *(0xBC000040) = 0;
  *(0xBC000044) = *(0xBC000044) & 0xFFFFFF9F;
}

//43dc
void sub_000043DC()
{
  *(0xBC000010) = 0xFFFFFFF3;
  *(0xBC000014) = 0xFFFFFFFF;
  *(0xBC000018) = 0xFFFFFFFF;
  *(0xBC00001C) = 0xFFFFFFFF;
  *(0xBC000020) = 0xFFFFFFFF;
  *(0xBC000024) = 0xFFFFFFFF;
  *(0xBC000028) = 0xFFFFFFFF;
  *(0xBC00002C) = 0xFFFFFFFF;
  *(0xBC000030) = 0x300;
  *(0xBC000034) = 0xF00;
  *(0xBC000038) = 0;
  *(0xBC000040) = 0;
  *(0xBC000044) &= 0xFFFFFFE0;
}

