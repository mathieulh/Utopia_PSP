#ifndef __THCALLBACK_H
#define __THCALLBACK_H

#include "thutils.h"

typedef struct _ThreadEventHandlerInfo
{
  struct _ThreadEventHandlerInfo *next; //0
  struct _ThreadEventHandlerInfo *prev; //4
  int UID; //8
  int unk_C;
  int unk_10;
  ThreadInfo *thread;
  int user_mode; //18
  int mask; //1c
  SceKernelThreadEventHandler handler; //20
  void *common; //24
  int gpreg; //28 - module->unk_68 or $gp
  int unk_2C;
} ThreadEventHandlerInfo;

typedef struct _CallbackInfo
{
  struct _CallbackInfo *next; //0
  struct _CallbackInfo *prev; //4
  int UID; //8
  int unk_C;
  int unk_10;
  ThreadInfo *thread; //14
  int notifyCount; //18
  int notifyArg; //1c
  int user_mode; //20
  SceKernelCallbackFunction func; //24
  void *arg; //28
  int gpreg; //2C - module->unk_68 or $gp
} CallbackInfo;

#endif