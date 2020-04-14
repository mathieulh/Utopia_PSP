#ifndef __SUSPEND_H
#define __SUSPEND_H

typedef int (*ScePowerHandler)(int);

typedef struct
{
  SceSize size;
  ScePowerHandler power_tick);
  ScePowerHandler power_lock;
  ScePowerHandler power_unlock;
  ScePowerHandler power_lock_for_user;
  ScePowerHandler power_unlock_for_user;
} ScePowerHandlers;

typedef int (*SceSuspendResumeHandlerFunc)(void);

typedef struct
{
  SceSuspendResumeHandlerFunc func;
  int gpreg;
  void *arg;
} SceSuspendResumeHandler;

#endif