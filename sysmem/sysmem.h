#ifndef __SYSMEM_H
#define __SYSMEM_H

#include <pspsdk.h>
#include <pspkerror.h>

#include "kdebug.h"

#define MAX_MEM_SIZE    0xFFFFFF00

enum PspSysMemBlockTypes {
	/** Allocate from the lowest available address. */
	PSP_SMEM_Low = 0,
	/** Allocate from the highest available address. */
	PSP_SMEM_High,
	/** Allocate from the specified address. */
	PSP_SMEM_Addr
};

typedef struct
{
  int unk_0;
  int unk_4;
  int unk_8;
  int unk_C;
  int unk_10;
  int unk_14;
  int unk_18;
  int unk_1C;
  int unk_20;
  int unk_24;
  int unk_28;
} SuspendResumeInfo;

typedef struct
{
  int unk_0;
  int num_export_libs; //number of sysmem's export libraries - set in SysMemInit
  void *export_lib[8]; //array of sysmem's export tables set in SysMemInit
  int loadcore_addr; //allocated in SysMemInit
  int user_lib_start; //offset in export_lib at which user libraries begin - set in SysMemInit
  int unk_30;
  int unk_34;
  int unk_38;
  int unk_3C;
  int lc_stub; //loadcore stubs - set in kactivate before booting loadcore
  int lc_stubsize; //total size of stubs - set in kactivate before booting loadcore
  int init_thread_stack; //allocated in SysMemInit
  SceLoadCoreExecFileInfo *sysmem_execinfo; //set in kactivate before booting loadcore
  SceLoadCoreExecFileInfo *lc_execinfo; //set in kactivate before booting loadcore
  int unk_54;
  int unk_58;
  int unk_5C;
  void (*kprintf_handler)(char *format, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6, void *arg7); //set by sysmem
  int unk_64; //set in kactivate before booting loadcore
} SysMemThreadConfig;

typedef struct
{
  int addr;
  int size;
  int attr;
  SceUID UID;
  int unk_10;
  int unk_14;
  int unk_18;
} SceSysMemProtectInfo;

typedef struct
{
  int unk_0;
  int membase;
  int memsize;
  int kernel_base;
  int kernel_size; //10
  int other_kernel_base;
  int other_kernel_size;
  int vshell_base;
  int vshell_size; //20
  int my_user_base;
  int my_user_size;
  int unknown_base;
  int unknown_size; //30
  int unk_mode1; //determines which way round kernel/other partitions go
  int unk_mode2; //determines which way round ca6c/user partitions go
  int kdebug_arg1; //bitmask
  int kdebug_arg2; //40
  int nMemoryBlocks;
  SceSysMemProtectInfo *memoryBlocks;
  PspDebugPutChar putchar;
  int loadcore_size; //50
  SysMemThreadConfig *th_conf;
} SysMemConfig;

typedef struct _PspSysmemMemoryInfo
{
  int unk_0; //addr or nBlocksUsed?
  int size;
  int unk_8; //part->unk_18
  struct _MemMgmtSubBlock *unk_C;
} PspSysmemMemoryInfo;

typedef struct _PspSysmemBlockInfo
{
  SceSize size;
  int useflag;
  int sizelock;
  int addr;
  int memsize;
} PspSysmemBlockInfo;

int sceKernelCpuSuspendIntr();
void sceKernelCpuResumeIntr(int intr);

#endif
