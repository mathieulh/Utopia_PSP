#ifndef __THMBOX_H
#define __THMBOX_H

#define SCE_MBX_LEGAL_ATTR   (0x400 | SCE_WAITQ_LEGAL_ATTR)

typedef struct
{
  SceKernelMbxOptParam *option; //0
  int unk_4; //num of messages
  int unk_8; //first message in queue
  int unk_C;
  int unk_10;
  int unk_14;
  int unk_18;
  int unk_1C;
  int unk_20;
  int unk_24;
  int unk_28;
  int unk_2C;
} MboxInfo;

#endif