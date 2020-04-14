char var_BA58[4] = "";
var_C310; //c310 - storage for setjmp

//heap.c
UidLookupFunction HeapUidFuncTable[] =
{
  { UIDFUNC_CREATE, HeapCreateUID },
  { UIDFUNC_DELETE, HeapDeleteUID },
  { 0x0DE3B1BD, sub_00000ea0 },
  { 0xA9CE362D, sub_00000f14 },
  { 0x01DB36E1, sub_00000f64 },
  { 0, 0 }
};

//0
void HeapInit()
{
  int ret = sceKernelCreateUIDtype("SceSysmemHeap", sizeof(HeapInfo), HeapUidFuncTable, 0, &uidHeapType);

  ASSERT(ret == 0);
}

//74
int _CreateHeap(PartitionInfo *part, SceSize size, int attr, int *heapaddr)
{
  SET_MEM_STATUS(0x01010021);

  void *heapbuf = _AllocPartitionMemory(part, (attr & 0x2) ? PSP_SMEM_High : PSP_SMEM_Low, size, 0);

  SET_MEM_STATUS(0x01010022);

  if(heapbuf == 0)
  {
    return SCE_KERNEL_ERROR_HEAPBLOCK_ALLOC_FAILED;
  }

  *heapaddr = (int)heapbuf;

  SET_MEM_STATUS(0x01010023);

  ((HeapHeader)heapbuf)->unk_0 = heapbuf;
  ((HeapHeader)heapbuf)->unk_4 = heapbuf;

  SET_MEM_STATUS(0x01010024);

  sub_00004004(heapbuf + sizeof(HeapHeader), size - sizeof(HeapHeader));

  SET_MEM_STATUS(0x01010025);

  return 0;
}

//1c8
SceUID sceKernelCreateHeap(SceUID partitionid, SceSize size, int attr, const char *name)
{
  int ret;
  uidControlBlock *cb;
  PartitionInfo *pinfo;
  HeapInfo *hinfo;
  int heapaddr = 0;

  int intr = sceKernelCpuSuspendIntr();

  pinfo = GetPartition(partitionid);
  if(pinfo == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ILLEGAL_PARTITION;
  }

  ret = sceKernelCreateUID(uidHeapType, name, IS_USER_MODE ? 0 : 0xFF, &cb);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return ret;
  }

  ASSERT(cb > 0);

  hinfo = UID_INFO(HeapInfo, cb, uidHeapType);
  SET_HEAP_SIZE(hinfo, IS_SET(attr, SCE_SYSMEM_HEAP_ATTR_UNK1) ? (size - (size % 8)) : 0));
  if(IS_SET(attr, SCE_SYSMEM_HEAP_ATTR_UNK2))
  {
    SET_HEAP_ATTR_UNK2(hinfo);
  }
  hinfo->partition = partitionid;

  ret = _CreateHeap(pinfo, size, attr, &heapaddr);
  if(ret != 0)
  {
    Kprintf("heap.c:%s:Cannot allocate memory for mpid 0x%08x [%s], size 0x%x\n", __FUNC__, partitionid, name, size);

    sceKernelDeleteUID(cb->UID);

    sceKernelCpuResumeIntr(intr);

    return ret;
  }

  hinfo->addr = heapaddr;

  sceKernelCpuResumeIntr(intr);

  return cb->UID;
}

//38c
int _DeleteHeap(HeapInfo *hinfo)
{
  int ret;
  u32 *u = (u32 *)hinfo->addr;

  s2 = unk_a0.8;

  ASSERT(hinfo->addr > 0);

  v0 = u[0];

  //3cc
  if(u[0] != hinfo->addr)
  {
    //3d4
    for(v0 = 0x0(s2); v0 != hinfo->addr; v0 = next)
    {
      next = 0x0(v0);

      ret = _FreePartitionMemory(v0);

      ASSERT(ret <= 0);
    }
  }

  //404
  0x8(s2) = 0;
  ret = _FreePartitionMemory(hinfo->addr);

  ASSERT(ret <= 0);

  return ret;
}

//4a8
int sceKernelDeleteHeap(SceUID heapid)
{
  int intr = sceKernelCpuSuspendIntr();

  int ret = sceKernelDeleteUID(heapid);

  sceKernelCpuResumeIntr(intr);

  return ret;
}

//4f0
void *_AllocHeapMemory(HeapInfo *heapinfo, int size, int align)
{
  if(align != 0)
  {
    if((align & 0x3) || (align > 0x80))
    {
      return 0;
    }

    if((align & (align - 1))
    {
      return 0;
    }
  }

  //544
  v0 = (HeapHeader)heapinfo->addr;
  v1 = heapinfo->addr->unk_0;
  sp_0 = heapinfo->addr->unk_0;

  //550
  while(1 == 1)
  {
    a0 = 0x0(sp);
    a1 = size;
    a2 = align;
    a0 = a0 + 0x8;
    ret = sub_0000405C(sp_0 + 0x8, size, align); //a0
    if(ret != 0)
    {
      return ret;
    }
    v1 = 0x0(sp);
    v0 = heapinfo->addr;
    if(sp_0 == heapinfo->addr)
    {
      v1 = heapinfo->size;
      //588
      v0 = v1 < 0x4;
      if(v0 != 0)
      {
        return a0;
      }
      v0 = v1 >> 1; //sra
      s3 = v0 << 1;
      if(align == 0)
      {
        //6d0
        v0 = s3 - 0x28;
        v0 = v0 < size;
        if(v0 == 0)
        {
          v0 = size + 0x7;
        }
        else
        {
          v0 = size + 0x7;
          v0 = v0 >> 3;
          size = v0 << 3;
          s3 = size + 0x28;
        }
      }
      else
      {
        v0 = 0x20;
        //divu v0, unk_a2
        v1 = size + 0x7;
        v1 = v1 >> 3;
        size = v1 << 3;
        if(align == 0)
        {
          //break 0x7;
        }

        //5bc
        v0 = v0 % align;
        v1 = 0;
        if(v0 != 0)
        {
          v1 = align - v0;
        }

        //5cc
        v1 = size + v1;
        v0 = s3 - 0x28;
        v0 = v0 < v1;
        if(v0 != 0)
        {
          s3 = v1 + 0x28;
        }
      }

      //5e0
      a0 = 0x4(heapinfo);
      PartitionInfo *part = GetPartition(heapinfo->partition);
      a2 = 0x0(heapinfo);
      a0 = v0;
      a1 = s3;
      a2 = a2 & 0x1;
      a3 = sp;
      v0 = _CreateHeap(part, s3, GET_HEAP_ATTR_UNK2(heapinfo), sp);
      if(v0 != 0)
      {
        a3 = 0x4(heapinfo);
        //6a8
        Kprintf("heap.c:%s:%04d:_CreateHeap failed, mpid 0x%x, hsize 0x%x, 0x%x bytes\n", __FUNC__, __LINE__,
                heapinfo->partition, s3, size);

        return 0;
      }

      a0 = 0x0(sp);
      v0 = sceKernelQueryBlockSize(...);
      v0 = 0 nor v0;
      s3 = 0 < v0;
      ASSERT(s3 != 0);

      //624
      v1 = 0x0(sp);
      a1 = size;
      a3 = 0x0(v0);
      a0 = v1 + 0x8;
      a2 = align;
      v0 = 0x4(a3);
      0x0(v1) = a3;
      0x4(v1) = v0;
      0x4(a3) = v1;
      v0 = 0x4(v1);
      0x0(v0) = v1;
      v0 = sub_0000405C(...);
      a0 = v0;
      v0 = a0;

      return v0;
    }

    v0 = 0x0(v1);
    0x0(sp) = v0;
  }
}

//6f0
void *sceKernelAllocHeapMemoryWithOption(SceUID heapid, SceSize size, SceKernelHeapOptParam *option)
{
  void *buf;
  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *block;
  if(sceKernelGetUIDcontrolBlockWithType(heapid, uidHeapType, &block) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return 0;
  }

  //764
  buf = _AllocHeapMemory(UID_INFO(void, block, uidHeapType), size, option ? option.alignment : 0);

  sceKernelCpuResumeIntr(intr);

  return buf;
}

//7ac
int _FreeHeapMemory(HeapInfo *heap, void *block)
{
  s3 = block;
  s2 = heap;
  v0 = (HeapHeader)heap->addr;
  s0 = heap->addr->unk_0;

  //7d4
  while(1 == 1)
  {
    s1 = s0 + 0x8;
    a0 = s1;
    a1 = s3;
    v0 = sub_000041A8(&v0->heap, block);
    v1 = v0;
    if(v0 == 0)
    {
      //848
      v0 = 0x8(s2);
      if(heap->addr == heap->addr->unk_0)
      {
        return 0;
      }

      a0 = s1;
      v0 = sub_00004334(&v0->heap);
      if(v0 != 0)
      {
        a1 = 0(s0); //w

        //86c
        v1 = 0x4(s0); //w
        0x8(s0) = 0; //w
        a0 = s0;
        0x4(a1) = v1; //w
        v0 = 0x4(s0); //w
        0(v0) = a1;

        _FreePartitionMemory(...);
      }

      return 0;
    }

    v0 = 0x8(s2);
    if(v0 == s0)
    {
      v0 = -1;

      break; //->800
    }

    s0 = 0(s0); //w
  }

  //800
  if(v0 == v1)
  {
    //82c
    Kprintf("heap.c:%s:illegal segment (0x%08x). This is not heap memory ... Maybe buffer overrun occured\n", "_FreeHeapMemory", s3);

    v0 = SCE_KERNEL_ERROR_ILLEGAL_ADDR;

    return v0;
  }

  ...
}

//890
int sceKernelFreeHeapMemory(SceUID heapid, void *block)
{
  int ret;
  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *block2;
  ret = sceKernelGetUIDcontrolBlockWithType(heapid, uidHeapType, &block2);

  if(ret == 0)
  {
    ret = _FreeHeapMemory(UID_INFO(HeapInfo, block2, uidHeapType), block);
  }

  //8fc
  sceKernelCpuResumeIntr(intr);

  return ret;
}

//924
int sub_00000924(HeapInfo *heapinfo)
{
  int s1 = 0;
  for(s0 = heapinfo->addr->unk_0; s0 != heapinfo->addr; s0 = s0->unk_0)
  {
    s1 += sub_0000435C(s0->heap);
  }

  return s1;
}

//980
SceSize sceKernelHeapTotalFreeSize(SceUID heapid)
{
  int ret;
  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *block;
  ret = sceKernelGetUIDcontrolBlockWithType(heapid, uidHeapType, &block);

  if(ret == 0)
  {
    ret = sub_00000924(UID_INFO(HeapInfo, block, uidHeapType));
  }

  //9e0
  sceKernelCpuResumeIntr(intr);

  return s0;
}

//a04
uidControlBlock *sceKernelGetHeapTypeCB()
{
  return uidHeapType;
}

//a10
SysMemForKernel_EFF0C6DD()
{
  return uidmgmt_heap_uid;
}

//a1c
int SysMemForKernel_EFEEBAC7(...)
{
  s0 = unk_a0;

  int intr = sceKernelCpuSuspendIntr();

  v0 = uidHeapType;
  s1 = v0->parent;
  v1 = v0;
  if(uidHeapType->parent == uidHeapType)
  {
    sceKernelCpuResumeIntr(intr);

    return -1;
  }

  //a4c
  for(s1 = uidHeapType->parent; s1 != uidHeapType; s1 = s1->parent)
  {
    HeapInfo *hinfo = UID_INFO(HeapInfo, s1, uidHeapType); //v0
    v1 = v0->addr;
    a1 = v1;

    //a60
    while(1 == 1)
    {
      v0 = s0 < v1;
      if(hinfo->addr <= unk_a0)
      {
        v0 = 0xC(v1);
        v0 += v1;
        v0 += 0x8;
        v0 = s0 < v0;
        if(v0 != 0)
        {
          //ac0
          sceKernelCpuResumeIntr(intr);

          return 0xC(s1);
        }
      }

      v0 = 0x0(v1);
      //a88
      v1 = v0;
      if(v0 == a1)
      {
        break;
      }
    }

    v0 = var_C924;
    s1 = 0x0(s1);
    v1 = v0;
    if(s1 == v0)
    {
      break;
    }
  }

  sceKernelCpuResumeIntr(intr);

  return -1;
}

//ad0
sceKernelIsValidHeap(...)
{
}

//dd0
int HeapCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  HeapInfo *info = UID_INFO(HeapInfo, cb, uidHeapType);
  info->size = 0;
  info->partition = 0;
  info->addr = 0;

  return cb->UID;
}

//e18
int HeapDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  HeapInfo *info = UID_INFO(HeapInfo, cb, uidHeapType);
  if(info->unk_8 != 0)
  {
    _DeleteHeap(info);
  }

  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//ea0
int sub_00000EA0(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  u32 l = 0;
  if(*(args + 4) != 0)
  {
    l = 0x4(*(args + 4));
  }

  //ef0
  *(0x4(args + 4)) = _AllocHeapMemory(UID_INFO(HeapInfo, cb, uidHeapType), *args, l, args + 4);

  return cb->UID;
}

//f14
int sub_00000F14(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  int ret = _FreeHeapMemory(UID_INFO(HeapInfo, cb, uidHeapType), *args);
  if(ret == 0)
  {
    ret = cb->UID;
  }

  return ret;
}

//f64
int sub_00000F64(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  *args.unk_0 = sub_00000924(UID_INFO(HeapInfo, cb, uidHeapType));

  return cb->UID;
}

//fb0
void *sceKernelAllocHeapMemory(SceUID heapid, SceSize size)
{
  return sceKernelAllocHeapMemoryWithOption(heapid, size, 0);
}

//65f8
void strcpy2(char *dest, char *src)
{
  char *s, *d;

  for(s = src, d = dest; *s; s++, d++)
  {
    *d = *s;
  }

  *d = '\0';
}
