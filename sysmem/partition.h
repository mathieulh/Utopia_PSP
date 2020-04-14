#ifndef __PARTITION_H
#define __PARTITION_H

#define SYSMEM_PARTITION_KERNEL 0x1
#define SYSMEM_PARTITION_USER   0x2
#define SYSMEM_PARTITION_VSHELL 0x5

typedef struct _PartitionInfo
{
  struct _PartitionInfo *next; //0
  int addr; //4
  int size; //8
  int attr; //c
  struct _MemMgmtBlock *head;
  int nBlocksUsed; //14
  int unk_18;
} PartitionInfo;

int PartitionInit(PartitionInfo *part);

#endif
