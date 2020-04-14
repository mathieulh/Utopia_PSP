#ifndef __THEVENT_H
#define __THEVENT_H

#define SCE_EVF_LEGAL_ATTR  (0x200 | SCE_WAITQ_LEGAL_ATTR)

typedef struct
{
  SceKernelEventFlagOptParam *opt; //0
  int currentPattern; //4
  int initialPattern; //8
  int unk_C;
  int unk_10;
  int unk_14;
  int unk_18;
  int unk_1C;
  int unk_20;
  int unk_24;
  int unk_28;
  int unk_2C;
} EventFlagInfo;

#endif