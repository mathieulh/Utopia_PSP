#ifndef __THSEMA_H
#define __THSEMA_H

#define SCE_SEMA_LEGAL_ATTR   SCE_WAITQ_LEGAL_ATTR

typedef struct
{
  int unk_0; //option
  int cur_count; //4
  int max_count; //8
  int init_count; //C
  int unk_10;
  int unk_14;
  int unk_18;
  int unk_1C;
  int unk_20;
  int unk_24;
  int unk_28;
  int unk_2C;
} SemaphoreInfo;

#endif