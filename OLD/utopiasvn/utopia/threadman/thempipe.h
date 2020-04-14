#ifndef __THEMPIPE_H
#define __THEMPIPE_H

#define SCE_MPP_MODE_SEND    1
#define SCE_MPP_MODE_RECEIVE 2

#define SCE_MPP_LEGAL_ATTR      (0x5000 | SCE_WAITQ_LEGAL_ATTR)

typedef struct
{
  SceKernelMsgPipeOptParam *opt; //0
  int mode; //4 - send/receive
  void *buf; //8
  void *sendptr; //C - ptr to send to (within buf)
  int *recvptr; //10 - ptr to receive from (within buf)
  int bufsize; //14
  int used; //18
  int unk_1C;
  int unk_20;
  int unk_24;
  int unk_28;
  int unk_2C;
} MsgPipeInfo;

#endif