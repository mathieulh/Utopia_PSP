#ifndef __SYSTIMER_H
#define __SYSTIMER_H

enum SceSysTimerAttribute
{
  SCE_STIMER_ATTR_UNK400000 = 0x00400000,
  SCE_STIMER_ATTR_BUSY = 0x00800000,
  SCE_STIMER_ATTR_UNK80000000 = 0x80000000
};

typedef struct _SceSysTimerHWReg
{
  int unk_0; //0
  int unk_4;
  int unk_8;
  int unk_C;
} SceSysTimerHWReg;

typedef struct _SceSysTimer
{
  SceSysTimerHWReg *hwReg; //0
  int intr_code;
  SceSysTimerId id;
  int unk_C;
  int unk_10; //10
  int (*handler)(void);
  int unk_18;
  void *arg;
} SceSysTimer;

typedef struct _SceSysTimerSave
{
  int unk_0; //0
  int unk_4;
  int unk_8;
} SceSysTimerSave;

#endif