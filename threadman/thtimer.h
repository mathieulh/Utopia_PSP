#ifndef __THTIMER_H
#define __THTIMER_H

typedef struct _TimerInfo
{
  struct _TimerInfo *next; //0
  struct _TimerInfo *prev; //4
  SceKernelSysClock time; //8
  int unk_10; //10
} TimerInfo;

typedef struct _DelayInfo
{
  void (*handler)(void *); //0
  void *arg; //4
  SceKernelSysClock time; //8
  int unk_10;
  int unk_14;
  int unk_18;
  int unk_1C;
  int unk_20;
  int unk_24;
  int unk_28;
  int unk_2C;
} DelayInfo;

typedef struct _AlarmInfo
{
  int unk_0; //is user mode
  int handler; //4
  void *arg; //8
  int gpreg; //C
  int unk_10;
  int unk_14;
  int unk_18;
  int unk_1C;
  int unk_20;
  int unk_24;
  int unk_28;
  int unk_2C;
} AlarmInfo;

// typedef SceUInt (*SceKernelVTimerHandler)(SceUID uid, SceKernelSysClock *, SceKernelSysClock *, void *);
// typedef SceUInt (*SceKernelVTimerHandlerWide)(SceUID uid, SceInt64, SceInt64, void *);

typedef struct _VTimerInfo
{
  int active; //0
  SceKernelSysClock current; //4 - current?
  SceKernelSysClock schedule; //C - schedule?
  int unk_14; //14 - is user mode
  SceKernelVTimerHandler handler; //18
  void *arg; //1c
  int gpreg; //20
  int unk_24;
  int unk_28;
  int unk_2C;
} VTimerInfo;

#endif