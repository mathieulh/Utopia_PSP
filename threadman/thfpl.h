#ifndef __THFPL_H
#define __THFPL_H

#define SCE_FPL_LEGAL_ATTR     (0x4000 | SCE_WAITQ_LEGAL_ATTR)

typedef struct
{
  SceKernelFplOptParam *opt; //0
  void *addr; //4
  u32 *firstFreeBlock; //8
  u32 *heapaddr; //C
  int freeBlocks; //10
  int blockSize; //14
  int numBlocks; //18
  int actualBlockSize; //1c
  int unk_20;
  int unk_24;
  int unk_28;
  int unk_2C;
} FplInfo;

#endif