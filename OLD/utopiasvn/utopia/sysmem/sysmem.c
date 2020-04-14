#include "sysmem.h"
#include "partition.h"
#include "block.h"
#include "kdebug.h"
#include "util.h"

int var_C848; //c848
PartitionInfo my_kernel_partition; //c84c
PartitionInfo tmp_other_kernel_partition; //c868
PartitionInfo tmp_vshell_partition; //c884
int var_C888; //c888
//c88c, c890, c894, c898, c89c ?
PartitionInfo tmp_my_user_partition; //c8a0
PartitionInfo tmp_unknown_partition; //c8bc
int var_C8D8[2]; //c8d8
BlockInfo tmp_sysmemmgmt; //c8e0
BlockInfo tmp_blockmgmt; //c8ec
SuspendResumeInfo suspend_info; //c8f8

extern PspDebugPutChar debug_putchar;

extern int membase; //ca54
extern int memsize; //ca58
extern PartitionInfo *partitions; //ca5c
extern PartitionInfo *var_ca60; //ca60 - set to kernel partition in sysmem init
extern PartitionInfo *var_ca64; //ca64 - set to user partition in sysmem init
extern PartitionInfo *other_kernel_partition; //ca68
extern int var_ca6c; //ca6c
extern PartitionInfo *var_CA70; //ca70 - set to kernel/other kernel partition in sysmem init
extern PartitionInfo *var_CA74; //ca74 - set to other kernel/kernel partition in sysmem init
extern PartitionInfo *vsh_partition; //ca78
extern PartitionInfo *var_CA7C; //ca7c - set to user/unknown partition in sysmem init
extern PartitionInfo *var_CA80; //ca80 - set to unknown/user partition in sysmem init
extern PartitionInfo *kernel_partition; //ca84 - set to kernel partition in sysmem init
extern PartitionInfo *user_partition; //ca88
extern int var_ca8c; //ca8c
extern int var_ca90; //ca90
extern int var_ca94; //ca94

//9498
int _SuspendHandler()
{
  suspend_info.unk_0 = *((int *)0xBC100040);
  suspend_info.unk_4 = *((int *)0xBC000000);
  suspend_info.unk_8 = *((int *)0xBC000004);
  suspend_info.unk_C = *((int *)0xBC000008);
  suspend_info.unk_10 = *((int *)0xBC00000C);
  suspend_info.unk_14 = *((int *)0xBC000030);
  suspend_info.unk_18 = *((int *)0xBC000034);
  suspend_info.unk_1C = *((int *)0xBC000038);
  suspend_info.unk_20 = *((int *)0xBC00003C);
  suspend_info.unk_24 = *((int *)0xBC000040);
  suspend_info.unk_28 = *((int *)0xBC000044);

  return 0;
}

//9554
int _ResumeHandler()
{
  *((int *)0xBC100040) = suspend_info.unk_0;
  *((int *)0xBC000000) = suspend_info.unk_4;
  *((int *)0xBC000004) = suspend_info.unk_8;
  *((int *)0xBC000008) = suspend_info.unk_C;
  *((int *)0xBC00000C) = suspend_info.unk_10;
  *((int *)0xBC000030) = suspend_info.unk_14;
  *((int *)0xBC000034) = suspend_info.unk_18;
  *((int *)0xBC000038) = suspend_info.unk_1C;
  *((int *)0xBC00003C) = suspend_info.unk_20;
  *((int *)0xBC000040) = suspend_info.unk_24;
  *((int *)0xBC000044) = suspend_info.unk_28;

  return 0;
}

//9610
#if 0
int loc_00009610(unk_a0)
{
  0x0(unk_a0) = var_A6E4;
  0x4(unk_a0) = var_A734;

  return 0x2;
}
#endif

//9b8c
int SysMemReInit()
{
  int intr = sceKernelCpuSuspendIntr();

  SET_MEM_STATUS(0x01010201);

  if(my_kernel_partition.head == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return 0;
  }

  partitions = &my_kernel_partition;
  my_kernel_partition.next = 0;
  PartitionInit(&my_kernel_partition);

  ASSERT(&my_kernel_partition > 0); //what's the point of this?

  //9c18
  int size = ((int)my_kernel_partition.head & 0x1FFFFFFF) - (my_kernel_partition.addr & 0x1FFFFFFF);
  int mem = _AllocPartitionMemory(&my_kernel_partition, PSP_SMEM_Low, size, my_kernel_partition.addr);
  if(mem == 0)
  {
    Kprintf("sysmem.c:%s:%04d:Pannic ! first segment can not alloc\n", __FUNC__, __LINE__);

    ASSERT(1 == 2);
  }

  tmp_sysmemmgmt.addr = mem;
  tmp_sysmemmgmt.size = size - (size % 0x100);

  mem = _AllocPartitionMemory(&my_kernel_partition, PSP_SMEM_High, sizeof(MemMgmtBlock), my_kernel_partition.head);
  if(mem == 0)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ERROR;
  }

  if((my_kernel_partition.head & 0x1FFFFFFF) != (mem & 0x1FFFFFFF))
  {
    Kprintf("sysmem.c:%s:memory management block = 0x%08x\n", __FUNC__, mem);

    Kprintf("sysmem.c:%s:MyKernelPartitionCB.SmemCB = 0x%08x\n", __FUNC__, my_kernel_partition.head);

    Kprintf("Pannic ! seconde segment can not alloc\n");

    ASSERT(1 == 2);

    my_kernel_partition.head = 0;

    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ERROR;
  }

  tmp_blockmgmt.addr = mem;
  tmp_blockmgmt.size = 0x100;

  if(my_kernel_partition.head->subblocks != 0)
  {
    for(memblk = my_kernel_partition.head->subblocks; memblk; memblk = memblk->next)
    {
      if(!GET_MEMBLOCK_USED(memblk))
      {
        sceKernelCpuResumeIntr(intr);

        return GET_MEMBLOCK_POS(memblk) * 0x100;
      }
    }
  }

  SET_MEM_STATUS(0x01010202);

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//9630
int sceKernelSysMemInit(int membase, int memsize, int my_kernel_base, int my_kernel_size, int other_kernel_base, int other_kernel_size,
                            int vshell_base, int vshell_size, int my_user_base, int my_user_size, int unknown_base,
                            int unknown_size, unk_60, unk_64)
{
  SET_MEM_STATUS(0x01010201);

  //96a0
  if(memsize > MAX_MEM_SIZE)
  {
    //9b5c
    Kprintf("sysmem.c:%s:memsize is MAX_MEM_SIZE 0x%x\n", __FUNC__, MAX_MEM_SIZE);

    memsize = MAX_MEM_SIZE;
  }

  //96a8
  ASSERT(unk_60 == 0x3 || unk_60 == 0x4);

  //96c8
  ASSERT(unk_64 == 0x6 || unk_64 == 0x7);

  //96d8
  0x0(var_C8D8) = unk_60;
  0x4(var_C8D8) = unk_64;

  ASSERT(my_kernel_size % 0x40000 == 0);
  ASSERT(other_kernel_size % 0x40000 == 0);
  ASSERT(vshell_size % 0x40000 == 0);
  ASSERT(my_user_size % 0x40000 == 0);
  ASSERT(unknown_size % 0x40000 == 0);

  //974c
  ASSERT_MSG(my_user_base % 0x40000 == 0, "sysmem.c:%s:my_user_base = 0x%08x\n", __FUNC__, my_user_base);

  //9758
  SET_MEM_STATUS(0x01010202);

  PartitionProtectInit();

  membase = membase; //ca54
  memsize = memsize; //ca58
  var_CA94 = 0; //ca94

  SET_MEM_STATUS(0x01010203);

  //97bc
  my_kernel_partition.addr = my_kernel_base;
  my_kernel_partition.size = my_kernel_size;
  my_kernel_partition.attr = (my_kernel_partition.attr & 0xFFFFFFF0) | 0xC;
  my_kernel_partition.head = (&var_CAC4 + 0xFF) & MAX_MEM_SIZE;

  kernel_partition = &my_kernel_partition; //ca84

  if(((((&var_CAC4 + 0xFF) & MAX_MEM_SIZE) + 0xF400) & 0x1FFFFFFF) > ((my_kernel_base + my_kernel_size) & 0x1FFFFFFF))
  {
    //98dc
    SET_MEM_STATUS(0x01010205);

    tmp_other_kernel_partition.addr = other_kernel_base;
    tmp_other_kernel_partition.size = other_kernel_size;
    tmp_other_kernel_partition.attr = (tmp_other_kernel_partition.attr & 0xFFFFFF0) | 0xC;

    tmp_vshell_partition.addr = vshell_base;
    tmp_vshell_partition.size = vshell_size;
    tmp_vshell_partition.attr |= 0xF;

    tmp_my_user_partition.addr = my_user_base;
    tmp_my_user_partition.size = my_user_size;
    tmp_my_user_partition.attr |= 0xF;

    tmp_unknown_partition.addr = unknown_base;
    tmp_unknown_partition.size = unknown_size;
    tmp_unknown_partition.attr |= 0xF;

    SET_MEM_STATUS(0x01010206);

    //9894
    return SysMemReInit();
  }
  else
  {
    SET_MEM_STATUS(0x01010204);

    //9828
    Kprintf("Pannic ! first segment can not alloc\n");

    ASSERT(my_kernel_size == 0);

    ASSERT(my_kernel_size == 0);

    my_kernel_partition.head = 0;

    return 0;
  }
}

//9e40
int SysMemPostInit()
{
  int ret;
  int intr = sceKernelCpuSuspendIntr();

  PartitionServiceInit();

  uidControlBlock *kernel;
  ret = sceKernelCreateUID(uidPartitionType, "My Kernel Partition", (-(IS_USER_MODE < 1) & 0xFFFFFF01) + 0xFF, &kernel);
  ASSERT(ret <= 0);

  //9ec4
  partitions = var_CA60 = kernel_partition = UID_INFO(PartitionInfo, kernel, uidPartitionType);
  kernel_partition->next = 0;
  kernel_partition->addr = my_kernel_partition.addr;
  kernel_partition->size = my_kernel_partition.size;
  kernel_partition->attr = (part->attr & 0xFFFFFFF0) | (my_kernel_partition.attr & 0xF);
  kernel_partition->head = my_kernel_partition.head;
  kernel_partition->nBlocksUsed = my_kernel_partition.nBlocksUsed;
  kernel_partition->unk_18 = my_kernel_partition.unk_18;

  tmp_sysmemmgmt.partition = kernel_partition;

  tmp_blockmgmt.partition = kernel_partition;

  MemoryBlockServiceInit();

  uidControlBlock *memmgr;
  ret = sceKernelCreateUID(uidBlockType, "System Memory Manager", IS_USER_MODE ? 0 : 0xFF, &memmgr);
  ASSERT(ret <= 0);

  //9f78
  BlockInfo *blockinfo = UID_INFO(BlockInfo, memmgr, uidBlockType);
  blockinfo->addr = tmp_sysmemmgmt.addr;
  blockinfo->size = tmp_sysmemmgmt.size;
  blockinfo->partition = tmp_sysmemmgmt.partition;

  ret = sceKernelCreateUID(uidBlockType, "Block Managing Structure", IS_USER_MODE ? 0 : 0xFF, &memmgr);
  ASSERT(ret <= 0);

  //9fec
  blockinfo = UID_INFO(BlockInfo, memmgr, uidBlockType);
  blockinfo->addr = tmp_blockmgmt.addr;
  blockinfo->size = tmp_blockmgmt.size;
  blockinfo->partition = tmp_blockmgmt.partition;

  _SetBlockDeleted(var_C8EC.partition, var_C8EC.addr);

  ret = sceKernelCreateMemoryPartition("OtherKernelPartition", tmp_other_kernel_partition.attr & 0xF,
       tmp_other_kernel_partition.addr, tmp_other_kernel_partition.size);

  uidControlBlock *other;
  sceKernelGetUIDcontrolBlockWithType(ret, uidPartitionType, &other);

  other_kernel_partition = UID_INFO(PartitionInfo, other, uidPartitionType);

  ASSERT(var_C8D8.0 == 0x3 || var_C8D8.0 == 0x4);

  if(var_C8D8.0 == 0x3)
  {
    //a204
    var_CA70 = var_CA60; //ca70, ca60
    var_CA74 = other_kernel_partition; //ca74
  }
  else if(var_C8D8.0 == 0x4)
  {
    //a1f4
    var_CA70 = other_kernel_partition; //ca70
    var_CA74 = var_CA60; //ca74, ca60
  }

  //a0b4
  ret = sceKernelCreateMemoryPartition("Vshell Partition", tmp_vshell_partition.attr & 0xF,
        tmp_vshell_partition.addr, tmp_vshell_partition.size);

  uidControlBlock *vsh;
  sceKernelGetUIDcontrolBlockWithType(ret, uidPartitionType, &vsh);

  vsh_partition = UID_INFO(PartitionInfo, vsh, uidPartitionType);

  ret = sceKernelCreateMemoryPartition("My User Partition", tmp_my_user_partition.attr & 0xF,
       tmp_my_user_partition.addr, tmp_my_user_partition.size);

  uidControlBlock *user;
  sceKernelGetUIDcontrolBlockWithType(ret, uidPartitionType, &user);

  var_CA64 = user_partition = UID_INFO(PartitionInfo, user, uidPartitionType);

  ASSERT(var_C8D8.4 == 0x6 || var_C8D8.4 == 0x7);

  if(var_C8D8.4 == 6)
  {
    //a1e4
    var_CA7C = user_partition; //ca7c
    var_CA80 = var_CA6C; //ca80, ca6c
  }
  else if(var_C8D8.4 == 7)
  {
    //a1d4
    var_CA7C = var_CA6C; //ca7c, ca6c
    var_CA80 = user_partition; //ca80
  }

  //a198
  sceKernelCpuResumeIntr(intr);

  return 0;
}

//a290
int sceKernelSysMemMemSize()
{
  PartitionInfo *part = GetPartition(SYSMEM_PARTITION_KERNEL);
  if(part->head == 0)
  {
    Kprintf("sysmem.c:%s:%04d:no contol data\n", __FUNC__, __LINE__);

    ASSERT(1 == 2);

    return 0;
  }

  return memsize;
}

//a314
int sceKernelSysMemMaxFreeMemSize()
{
  int maxmem, mem;
  PartitionInfo *part;
  int intr = sceKernelCpuSuspendIntr();

  part = GetPartition(SYSMEM_PARTITION_KERNEL);
  if(part->head == 0)
  {
    //a3a0
    Kprintf("sysmem.c:%s:%04d:: no contol data \n", __FUNC__, __LINE__);

    sceKernelCpuResumeIntr(intr);

    return 0;
  }

  int maxmem = 0;
  for(part = partitions; part; part = part->next)
  {
    mem = GetPartitionFreeMemSize(part);
    if(mem > maxmem)
    {
      maxmem = mem;
    }
  }

  //a37c
  sceKernelCpuResumeIntr(intr);

  return maxmem;
}

//a3c4
int sceKernelSysMemTotalFreeMemSize()
{
  int mem = 0;

  int intr = sceKernelCpuSuspendIntr();

  PartitionInfo *part = GetPartition(SYSMEM_PARTITION_KERNEL);

  if(part->head == 0)
  {
    //a440
    Kprintf("sysmem.c:%s:%04d:no contol data \n", __FUNC__, __LINE__);

    sceKernelCpuResumeIntr(intr);

    return 0;
  }

  for(part = partitions; part; part = part->next)
  {
    mem += GetPartitionTotalFreeMemSize(part);
  }

  sceKernelCpuResumeIntr(intr);

  return mem;
}

//a464
int sceKernelGetSysMemoryInfo(int pid, int unk_a1, PspSysmemMemoryInfo *info)
{
  MemMgmtSubBlock *memblk, *tmpblk;
  PartitionInfo *part = GetPartition(pid);

  int intr = sceKernelCpuSuspendIntr();

  if(unk_a1 == 0)
  {
    if(info->unk_8 == part->unk_18)
    {
      //a4f8
      memblk = info->unk_C;
      if(memblk != 0)
      {
        //a518
        info->unk_0 = part->addr + GET_MEMBLOCK_POS(memblk) * 0x100;
        info->unk_4 = GET_MEMBLOCK_NBLOCKS(memblk) * 0x100;
        if(!GET_MEMBLOCK_USED(memblk))
        {
          //a58c
          info->unk_4 |= 0x1;
        }
        else if(part->head != 0)
        {
          //a554
          for(tempblk = part->head->subblocks; tempblk; tempblk = tempblk->next)
          {
            if(tempblk == info->unk_0)
            {
              info->unk_4 |= 0x2;
              break;
            }
          }
        }

        //a56c
        info->unk_C = memblk->next;
        if(GET_MEMBLOCK_NBLOCKS(memblk->next) == 0)
        {
          info->unk_C = 0;
        }
      }
      else
      {
        info->unk_0 = part->addr;
        info->unk_4 = 0x1;
      }
    }
    else
    {
      info->unk_0 = -1;
      info->unk_4 = -1;
      info->unk_C = 0;
    }
  }
  else
  {
    info->unk_0 = part->nBlocksUsed;
    info->unk_4 = part->size;
    info->unk_8 = part->unk_18;
    info->unk_C = part->head->subblocks;
  }

  sceKernelCpuResumeIntr(intr);
}

//a594
int KDebugForKernel_A126F497(int unk_a0)
{
  var_C848 = unk_a0;

  return 0;
}

//a5a4
int KDebugForKernel_B7251823()
{
  return var_C848;
}

//a5b0
int SysMemForKernel_CDA3A2F7()
{
  return var_C888;
}

//a5bc
int SysMemForKernel_960B888C()
{
  return var_C888 + 0x20;
}

//a5d0
int loc_0000A5D0(int unk_a0)
{
  return ((unk_a0 & 0x1fffffff) | (kernel_partition->addr & 0xE0000000));
}

//a5fc
int loc_0000A5FC(int unk_a0)
{
  return ((unk_a0 & 0x1fffffff) | (kernel_partition->addr & 0xE0000000));
}

//a628
int sceKernelCpuSuspendIntr()
{
  int intr;

  asm("mfic %0, $0" : "=r"(intr));
  asm("mtic $zr, $0");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");

  return intr;
}

//a654
void sceKernelCpuResumeIntr(int intr)
{
  asm("mtic %0, $0" : "=r"(intr));
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
}

//90ec
int module_bootstart(int size, SysMemConfig *config) __attribute__((alias("SysMemInit")));

int SysMemInit(int size, SysMemConfig *config)
{
  int part;

  SET_MEM_STATUS(0x01010001);

  //913c
  debug_putchar = config->putchar;

  SET_MEM_STATUS(0x01010002);

  //9160
  if(size != sizeof(SysMemConfig *))
  {
    Kprintf("sysmem.c:%s:argument size is invalid (%d)\n", "SysMemInit", size);
  }

  //9178
  InitKDebug(config->kdebug_arg1, config->kdebug_arg2);

  SET_MEM_STATUS(0x01010003);

  sceKernelSysMemInit(config->membase, config->memsize, config->unk_C, config->unk_10, config->other_kernel_base, config->other_kernel_size,
         config->vshell_base, config->vshell_size, config->my_user_base, config->my_user_size, config->unknown_base, config->unknown_size,
         config->unk_34, config->unk_38);

  SET_MEM_STATUS(0x01010004);

  //9208
  SET_MEM_STATUS(0x01010005);

  //9220
  InitUid();

  SET_MEM_STATUS(0x01010006);

  //9240
  SysMemPostInit();

  ClearSuspendResumeHandlers();

  ClearSysEventHandlers();

  for(i = 0; i < config->nMemoryBlocks; i++)
  {
    if((config->memoryBlock[i].attr & 0xFFFF) == 0x200)
    {
      int uid = sceKernelAllocPartitionMemory(SYSMEM_PARTITION_KERNEL, "SceSysmemProtectSystem", PSP_SMEM_Addr,
              config->memoryBlock[i].size, config->memoryBlock[i].addr);
      if(uid < 0)
      {
        Kprintf("sysmem.c:%s:Cannot allocate protect memory: 0x&08x\n", "SysMemInit", part);
      }
      else
      {
        config->memoryBlock[i].UID = part;
        config->memoryBlock[i].attr |= 0x10000;
      }
    }
  }

  //92a4
  config->th_conf->num_export_libs = 0x6;
  config->th_conf->export_lib[0] = var_A6E4; //SysMemForKernel
  config->th_conf->export_lib[1] = var_A734; //KDebugForKernel
  config->th_conf->export_lib[2] = var_A714; //sceSuspendForKernel
  config->th_conf->export_lib[3] = var_A704; //sceSysEventForKernel
  config->th_conf->export_lib[4] = var_A6F4; //SysMemUserForUser
  config->th_conf->export_lib[5] = var_A724; //sceSuspendForUser
  config->th_conf->unk_2C = 0x4;

  SET_MEM_STATUS(0x01010006);

  //931c
  int mem = sceKernelAllocPartitionMemory(SYSMEM_PARTITION_KERNEL, "SceSysmemLoadCoreBlock", PSP_SMEM_Low, config->loadcore_size, 0);

  config->th_conf->loadcore_addr = sceKernelGetBlockHeadAddr(mem);

  mem = sceKernelAllocPartitionMemory(SYSMEM_PARTITION_KERNEL, "SceSysmemInitialThread.stack", PSP_SMEM_High, 0x4000, 0);

  config->th_conf->init_thread_stack = sceKernelGetBlockHeadAddr(mem) + 0x4000;

  sceKernelRegisterSuspendHandler(0x1F, _SuspendHandler, 0);

  sceKernelRegisterResumeHandler(0x1F, _ResumeHandler, 0);

  config->th_conf->kprintf_handler = Kprintf;

  return 0;
}
