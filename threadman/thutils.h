#ifndef __THUTILS_H
#define __THUTILS_H

#define SCE_WAITQ_LEGAL_ATTR   (0x100 | UID_LEGAL_ATTR)

typedef struct
{
  int attr; //0
  int unk_4;
  int numWaitThreads; //8
  LinkedList waitThread; //c
} WaitQInfo;

#endif