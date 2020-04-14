#ifndef __INTERRUPTMAN_H
#define __INTERRUPTMAN_H

typedef struct _InterruptInfo
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
  int unk_28; //heap
  int unk_2C; //sub-intr
  int unk_30; //attr
  int unk_34;
} InterruptInfo;

typedef struct _InterruptManCB
{
  InterruptInfo *unk_60[0x43];
  int unk_16C;
  int unk_170;
  int unk_18C;
  int unk_190;
  SceKernelSysClock unk_194;
  int unk_19C;
  int unk_1A0;
  int unk_1A4;
  int unk_1A8;
  int unk_1BC;
  InterruptInfo unk_1D4[0x43];
} InterruptManCB;

#endif