#ifndef __THVPL_H
#define __THVPL_H

#define SCE_VPL_LEGAL_ATTR     (0x4000 | SCE_WAITQ_LEGAL_ATTR)

typedef struct
{
  SceKernelVplOptParam *opt; //0
  int heapAddr;
  int heapSize;
  int unk_C;
  int unk_10;
  int unk_14;
  int unk_18;
  int unk_1C;
  int unk_20;
  int unk_24;
  int unk_28;
  int unk_2C;
} VplInfo;

#endif