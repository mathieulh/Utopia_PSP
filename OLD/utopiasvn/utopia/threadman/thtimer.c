#include "../common/common.h"

#include "../sdk/include/pspthreadman.h"
#include "../sdk/include/pspthreadman_kernel.h"

#include "threadman.h"
#include "thtimer.h"

extern ThreadmanInfo gInfo;

u32 var_15e00;

//ac58
void InitSystemTimer()
{
  gInfo.unk_690 = 1;
  gInfo.unk_694 = 1;

  SceKernelSysClock clock;
  sceKernelUSec2SysClock(0x64, &clock);

  gInfo.unk_688 = clock.low;
  gInfo.unk_68C = clock.low << 1;

  gInfo.unk_424 = sceKernelGetSystemTimeLow;

  gInfo.unk_72C = gInfo.cpuID ? 0x4 : 0x13;

  *(0xBC600008) = 0x30;
  *(0xBC60000C) = 0x1;
  *(0xBC600010) = 0;
  _SYNC();

  sceKernelRegisterIntrHandler(gInfo.unk_72C, 0x1, DispatchTimers, &gInfo, 0);

  sceKernelSetIntrLevel(gInfo.unk_72C, 0x2);

  CLEAR_LIST(gInfo.unk_604);

  CLEAR_LIST(gInfo.timers);

  gInfo.unk_5F8 = 0;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  ASSERT(sceKernelCreateUID(uidTimerType, "SceThreadmanSysTimerOVF", 0, &cb) <= 0);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType); //t6
  timer->time.low = 0xF0000000;
  timer->time.hi = 0;

  ADD_TO_LIST(timer, gInfo.timers);

  gInfo.unk_430.low = 0;
  gInfo.unk_430.hi = 0;

  gInfo.unk_438.low = timer->time.low;

  for(i = 31; i >= 0; i--)
  {
    ASSERT(sceKernelCreateUID(uidDelayType, "SceThreadmanDelay", 0, &cb) <= 0);

    AddToDelayList(cb);
  }

  *(0xBC600004) = gInfo.unk_438.low;
  _SYNC();

  *(0xBC600000) = 0;
  _SYNC();

  sceKernelSetPrimarySyscallHandler(0x18, loc_0000DDB0);

  sceKernelRegisterSuspendHandler(0x18, sub_0000DDC8, 0);

  sceKernelRegisterResumeHandler(0x18, loc_0000DE3C, 0);

  sceKernelEnableIntr(gInfo.unk_72C);

  sceKernelCpuResumeIntr(intr);
}

//aef8
int DispatchTimers(void *arg1, ThreadmanInfo *info)
{
  int ret;

  int intr = sceKernelCpuSuspendIntr();

  info->unk_430.low = info->unk_438.low;
  info->unk_430.hi = info->unk_438.hi;

  v0v1 = sub_0000B490();

  TimerInfo *timer, *next_timer;
  LOOP_LIST_SAFE(timer, next_timer, info->timers)
  {
    t7 = timer->time.low;
    s2 = timer->time.hi;
    s1 = 0;
    t6 = s2 << 0;
    s6 = s1 + t7;
    t5 = 0;
    t4 = s6 < t7;
    v0 = t6 + t5;
    s7 = v0 + t4;
    t3 = s6 < t0;
    a0 = s6 - t0;
    t0 = s7 - v1;
    a1 = t0 - t3;
    s2 = s6;
    s3 = s7;
    if(a1 > 0)
    {
      break;
    }

    if(a1 == 0 && a0 != 0)
    {
      break;
    }

    REMOVE_FROM_LIST(timer);

    uidControlBlock *cb = UIDCB_FROM_INFO(timer, uidTimerType);
    if(SysMemForKernel_82D3CEE3(cb, uidDelayType))
    {
      sceKernelCpuResumeIntr(intr);

      DelayInfo *delay = UID_INFO(DelayInfo, cb, uidDelayType);
      delay->handler(delay->arg);

      intr = sceKernelCpuSuspendIntr();

      AddToDelayList(cb);
    }
    else if(SysMemForKernel_82D3CEE3(cb, uidAlarmType))
    {
      gInfo.timerId = cb->UID;

      AlarmInfo *alarm = UID_INFO(AlarmInfo, cb, uidAlarmType);

      int old_gp = _GET_GPREG(GPREG_GP);
      _SET_GPREG(GPREG_GP, alarm->gpreg);
      if(alarm->unk_0)
      {
        ret = sceKernelCallUserIntrHandler(alarm->arg, 0, 0, 0, alarm->handler, sceKernelGetUserIntrStack());
      }
      else
      {
        sceKernelCpuResumeIntr(intr);

        ret = alarm->handler(alarm->arg);

        intr = sceKernelCpuSuspendIntr();
      }

      _SET_GPREG(GPREG_GP, old_gp);

      gInfo.timerId = 0;

      if(ret == 0)
      {
        timer->unk_10 = 0x10000;

        sub_0000F0DC(cb->UID);
      }

      t0 = MAX(ret, info->unk_688);

      v0 = t0;
      v1 = 0;
      s1 = s6 + v0;

      a3 = s1 < v0;
      s6 = s7 + v1;
      v1 = s6 + a3;
      a1 = v1 >> 0;
      timer->time.low = s1;
      timer->time.hi = a1;

      AddToTimerList(timer);
    }
    else if(SysMemForKernel_82D3CEE3(cb, uidVTimerType))
    {
      VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType);

      ret = sub_0000DF4C(cb);
      if(ret != 0)
      {
        v1 = MAX(ret, info->unk_688);

        t1 = s2 + v1;
        t3 = 0;
        s4 = t1 < v1;
        t9 = s3 + t3;
        t8 = t9 + s4;
        a0 = t8 >> 0;
        timer->time.hi = a0;
        s6 = s1 + 0xC;
        v0 = 0;
        timer->time.low = t1;
        t2 = 0;
        t4 = v1;
        t6 = vtimer->schedule.low;
        t7 = vtimer->schedule.hi;
        a0 = timer;
        t0 = v0 + t6;
        t5 = t7 << 0;
        s3 = t0 < t6;
        s2 = t5 + t2;
        v1 = t0 + v1;
        a3 = s2 + s3;
        a2 = v1 < t4;
        timer = a3 + t3;
        fp = timer + a2;
        s7 = fp >> 0;
        vtimer->schedule.low = v1;
        vtimer->schedule.hi = s7;

        AddToTimerList(timer);
      }
      else
      {
        vtimer->handler = 0;
      }
    }
    else
    {
      v1 = 0;
      v0 = 0xF0000000;
      s1 = s6 + v0;

      a3 = s1 < v0;
      s6 = s7 + v1;
      v1 = s6 + a3;
      a1 = v1 >> 0;
      timer->time.low = s1;
      timer->time.hi = a1;

      AddToTimerList(timer);
    }

    v0v1 = sub_0000B490();
  }

  sub_0000B304();

  sceKernelCpuResumeIntr(intr);

  return -1;
}

//b258
void AddToTimerList(TimerInfo *timer)
{
  t4 = gInfo.timers;
  t7 = &gInfo.timers;
  t5 = timer;
  t6 = &timer.time;
  TimerInfo *tmp;
  LOOP_LIST(tmp, gInfo.timers)
  {
    a1 = tmp->time.low;

    t3 = tmp->time.hi;

    t8 = timer->time.hi;
    v1 = timer->time.low;
    a0 = 0;
    t2 = t3 << 0;
    t1 = 0;
    t3 = t8 << 0;
    t9 = 0;
    t8 = a0 + a1;
    v0 = t8 < a1;
    a3 = 0;
    a2 = t2 + t1;
    a1 = t9 + v1;
    t1 = a2 + v0;
    t2 = a1 < v1;
    v0 = t3 + a3;
    a3 = v0 + t2;
    a0 = a3 < t1;
    v1 = a1 < t8;
    if(a0 != 0)
    {
      break;
    }

    if(t1 == a3 && v1 != 0)
    {
      break;
    }
  }

  INSERT_IN_LIST_BEFORE(timer, tmp);
}

//b304
void sub_0000B304()
{
  t4 = &gInfo.timers;
  t2 = 0;
  t1 = 0;
  t3 = 0;
  t0 = gInfo.timers.next;
  a3 = gInfo.unk_68C;
  a1 = t0->time.low;
  t5 = t0->time.hi;
  s0 = a1 + t2;
  v1 = t5 << 0;
  v0 = v1 + t1;
  a2 = s0 < a1;
  s1 = v0 + a2;
  t2 = s0 + a3;
  a0 = t2 < a3;
  v1 = s1 + t3;
  t3 = v1 + a0;

  LOOP_LIST(tmp, gInfo.timers)
  {
    v1 = tmp->time.low;
    a1 = tmp->time.hi;
    v0 = 0;
    a0 = v0 + v1;
    a2 = a1 << 0;
    t1 = 0;
    a3 = a0 < v1;
    t9 = a2 + t1;
    a1 = t9 + a3;
    t8 = t2 < a0;
    t7 = t3 - a1;
    v1 = t7 - t8;
    v0 = t2 - a0;
    if(v1 <= 0 && (v1 != 0 || v0 == 0))
    {
      break;
    }

    s0 = a0;
    s1 = a1;
  }

  v0v1 = sub_0000B490();

  t2 = s0 < v0;
  t3 = s1 - v1;
  a1 = 0;
  a3 = t3 - t2;
  t4 = a3 < a1;
  t0 = v0;
  t1 = v1;
  a0 = gInfo.unk_68C;
  a2 = s0 - v0;
  t6 = a2 < a0;
  if(t4 != 0 || (a1 == a3 && t6 != 0))
  {
    s0 = t0 + a0;
    t7 = s0 < a0;
    t0 = t1 + a1;
    s1 = t0 + t7;
  }

  t9 = &gInfo.unk_438;
  a0 = s1 >> 0;

  gInfo.unk_438.low = s0;
  gInfo.unk_438.hi = a0;

  *((u32 *)0xBC600004) = s0;
  _SYNC();
}

//b44c
int sceKernelUSec2SysClock(int usec, SceKernelSysClock *clk)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(clk))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  clk->low = usec;
  clk->hi = 0;

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//b490
SceUInt64 sub_0000B490()
{
  t1 = gInfo.unk_430.hi;
  t2 = *(0xBC600000);
  t3 = gInfo.unk_430.low;
  a0 = 0;
  v1 = 0;
  v0 = t2 < t3;
  t0 = t1 + v0;
  a3 = t0 << 0;
  v0 = a0 + t2;
  a2 = v0 < t2;
  a0 = a3 + v1;
  v1 = a0 + a2;

  return v0v1;
}

//b4d0
int sceKernelCancelAlarm(SceUID alarmid)
{
  SET_K1_SRL16;

  if(alarmid == gInfo.timerId)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_ALMID;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(alarmid, uidAlarmType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_ALMID;
  }

  if(!CAN_CANCEL_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x72, 1, alarmid);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType);
  if(timer->unk_10)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_ALMID;
  }

  REMOVE_FROM_LIST(timer);

  if(!sceKernelIsIntrContext())
  {
    ASSERT(sceKernelDeleteUID(alarmid) <= 0);
  }

  timer->unk_10 = 1;

  int ret = sub_0000F0DC(alarmid);

  if(ret == 0)
  {
    sub_0000B304();
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//b694
void AddToDelayList(uidControlBlock *cb)
{
  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType);
  if(gInfo.unk_5F8 >= 32)
  {
    timer->unk_10 = 1;

    sub_0000F0DC(cb->UID);
  }
  else
  {
    ADD_TO_LIST(timer, gInfo.unk_604);

    gInfo.delay_count++;
  }
}

//b708
int sceKernelReferAlarmStatus(SceUID alarmid, SceKernelAlarmInfo *info)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(IS_USER_MODE && IS_ADDR_KERNEL(info))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(alarmid, uidAlarmType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_ALMID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x73, 2, alarmid, info);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType);
  AlarmInfo *alarm = UID_INFO(AlarmInfo, cb, uidAlarmType);

  SceKernelAlarmInfo *buf = (SceKernelAlarmInfo *)gInfo.unk_6A0;

  memset(buf, 0, sizeof(SceKernelAlarmInfo));

  buf->size = sizeof(SceKernelAlarmInfo);

  buf->schedule.low = timer->unk_8.low;
  buf->schedule.high = timer->unk_8.high;
  buf->handler = alarm->handler;
  buf->common = alarm->arg;

  memcpy(info, buf, MIN(info->size, buf->size));

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

//b8c0
SceUID sceKernelCreateVTimer(const char *name, struct SceKernelVTimerOptParam *opt)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(name) || IS_ADDR_KERNEL(opt)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  uidControlBlock *cb;
  int ret = sceKernelCreateUID(uidVTimerType, name, IS_USER_MODE ? 0xFF : 0, &cb);
  if(ret != 0)
  {
    RESET_K1;

    return ret;
  }

  int intr = sceKernelCpuSuspendIntr();

  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType);
  vtimer->active = 0;
  vtimer->handler = 0;

  THREADMAN_TRACE(0x74, 1, cb->UID);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return cb->UID;
}

//ba00
int sceKernelDeleteVTimer(SceUID uid)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }

  if(!CAN_DELETE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x75, 1, uid);

  ASSERT(sceKernelDeleteUID(uid) <= 0);

  sub_0000B304();

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//bb60
int sceKernelGetVTimerBase(SceUID uid, SceKernelSysClock *base)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(base))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType);
  if(!vtimer->active)
  {
    base->low = 0;
    base->high = 0;
  }
  else
  {
    base->low = vtimer->current.low;
    base->high = vtimer->current.high;
  }

  THREADMAN_TRACE(0x76, 3, uid, base.hi, base.low);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//bcd0
SceInt64 sceKernelGetVTimerBaseWide(SceUID uid)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return -1;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return -1;
  }

  s0 = 0;
  s1 = 0;

  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType);
  if(vtimer->active)
  {
    s0 = vtimer->current.high;
    t6 = vtimer->current.low;
    t7 = 0;
    t5 = s0 << 0;
    t4 = 0;
    s0 = t7 + t6;
    t3 = s0 < t6;
    a0 = t5 + t4;
    s1 = a0 + t3;
  }

  THREADMAN_TRACE(0x7D, 2, uid, s0);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  v0 = s0;
  v1 = s1;
  return v0v1;
}

//be14
int sceKernelGetVTimerTime(SceUID uid, SceKernelSysClock *time)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(time))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType);
  if(vtimer->active)
  {
    v0v1 = sub_0000B490();

    t5 = vtimer->current.low;
    t7 = vtimer->current.high;
    t6 = 0;
    t2 = t6 + t5;
    t0 = t7 << 0;
    a3 = 0;
    t4 = t2 < t5;
    t3 = t0 + a3;
    t1 = t3 + t4;
    a1 = v0 < t2;
    a0 = v1 - t1;
    t9 = a0 - a1;
    s0 = v0 - t2;
    t8 = t9 >> 0;
    time->high = t8;
    time->low = s0;
  }
  else
  {
    time->low = vtimer->current.low;
    time->high = vtimer->current.high;
  }

  THREADMAN_TRACE(0x77, 3, uid, time->high, time->low);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//bfcc
SceInt64 sceKernelGetVTimerTimeWide(SceUID uid)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0
          || (IS_USER_MODE && !(cb->attr & 0x1)))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return -1;
  }

  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType);
  t1 = 0;
  if(vtimer->active)
  {
    v0v1 = sub_0000B490();

    t8 = vtimer->current.low;
    a0 = vtimer->current.hi;
    t9 = 0;
    t5 = t9 + t8;
    s1 = a0 << 0;
    s0 = 0;
    t6 = s1 + s0;
    t7 = t5 < t8;
    t4 = t6 + t7;
    t3 = v0 < t5;
    a1 = v1 - t4;
    s0 = v0 - t5;
    s1 = a1 - t3;
  }
  else
  {
    t2 = vtimer->current.hi;
    t0 = vtimer->current.low;
    v1 = t2 << 0;
    a2 = 0;
    s0 = t1 + t0;
    a3 = s0 < t0;
    v0 = v1 + a2;
    s1 = v0 + a3;
  }

  THREADMAN_TRACE(0x7E, 2, uid, vtimer);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return s0s1;
}

//c144
int sceKernelSetVTimerTime(SceUID uid, SceKernelSysClock *time)
{
  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
  }

  if(IS_USER_MODE && IS_ADDR_KERNEL(time))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_VTID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }
  
  THREADMAN_TRACE(0x78, 3, uid, time->hi, time->low);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType); //s5
  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType); //s0

  if(vtimer->active)
  {
    s6 = &vtimer->current;
    v0v1 = sub_0000B490();

    a1 = vtimer->current.low;
    t5 = vtimer->current.hi;
    a0 = 0;
    t3 = a0 + a1;
    t0 = t5 << 0;
    t2 = 0;
    t4 = t3 < a1;
    s2 = t0 + t2;
    s3 = s2 + t4;
    t9 = v0 < t3;
    s2 = v0 - t3;
    v0 = v1 - s3;
    s3 = v0 - t9;
    
    v0v1 = sub_0000B490();

    t8 = time->hi;
    t6 = time->low;
    t7 = 0;
    t1 = t8 << 0;
    t4 = t7 + t6;
    a3 = 0;
    t5 = t4 < t6;
    t2 = t1 + a3;
    t3 = t2 + t5;
    t0 = v0 < t4;
    a2 = v1 - t3;
    t8 = vtimer->handler;
    a0 = a2 - t0;
    t9 = v0 - t4;
    a1 = a0 >> 0;
    vtimer->current.low = t9;
    vtimer->current.hi = a1;

    if(vtimer->handler)
    {
      a2 = vtimer->schedule.low;
      t3 = vtimer->schedule.hi;
      t2 = vtimer->current.hi;
      s6 = vtimer->current.low;
      t9 = 0;
      v1 = 0;
      t5 = t9 + a2;
      t4 = t2 << 0;
      t8 = t3 << 0;
      t1 = 0;
      t3 = v1 + s6;
      a3 = 0;
      t6 = t5 < a2;
      s0 = t8 + t1;
      a0 = t3 < s6;
      a1 = t4 + a3;
      t2 = s0 + t6;
      v0 = a1 + a0;
      s6 = t5 + t3;
      t9 = s6 < t3;
      t8 = t2 + v0;
      a2 = t8 + t9;
      v1 = s5 + 8;
      s0 = a2 >> 0;
      a0 = s5;
      timer->time.low = s6;
      timer->time.hi = s0;

      REMOVE_FROM_LIST(timer);

      AddToTimerList(timer);

      sub_0000B304();
    }
  }
  else
  {
    t4 = time->low;
    a1 = vtimer->current.low;
    s2 = vtimer->current.hi;
    vtimer->current.low = time->low;
    a0 = 0;
    s3 = 0;
    s5 = time->hi;
    t5 = s2 << 0;
    s2 = a0 + a1;
    vtimer->current.hi = time->hi;
    v0 = s2 < a1;
    t7 = t5 + s3;
    s3 = t7 + v0;
  }

  a3 = s3 >> 0;
  time->low = s2;
  time->hi = a3;

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//c438
SceInt64 sceKernelSetVTimerTimeWide(SceUID uid, SceInt64 time)
{
  //s2s3 = time;

  SET_K1_SRL16;

  if(sceKernelIsIntrContext())
  {
    RESET_K1;

    return -1;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0
          || (IS_USER_MODE && !(cb->attr & 0x2)))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return -1;
  }

  THREADMAN_TRACE(0x7F, 2, uid, s2);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType); //s1
  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType); //s0

  if(vtimer->active)
  {
    s5 = s0 + 4;
    v0v1 = sub_0000B490();

    t2 = vtimer->current.hi;
    a1 = vtimer->current.low;
    t4 = 0;
    t0 = t2 << 0;
    t1 = t4 + a1;
    a3 = 0;
    s6 = t0 + a3;
    t3 = t1 < a1;
    s7 = s6 + t3;
    a0 = v0 < t1;
    t9 = v1 - s7;
    s6 = v0 - t1;
    s7 = t9 - a0;
    
    v0v1 = sub_0000B490();

    t8 = v0 < s2;
    t5 = v0 - s2;
    t2 = vtimer->handler;
    s2 = v1 - s3;
    t7 = s2 - t8;
    t6 = t7 >> 0;
    vtimer->current.hi = t6;
    vtimer->current.low = t5;
    t6 = 0;

    if(vtimer->handler)
    {
      a1 = vtimer->schedule.low;
      t2 = vtimer->current.low;
      a2 = vtimer->schedule.hi;
      t7 = vtimer->current.hi;
      s3 = 0;
      t8 = s3 + t2;
      t9 = t6 + a1;
      t4 = t7 << 0;
      v1 = a2 << 0;
      t1 = 0;
      a3 = 0;
      s0 = v1 + t1;
      s5 = t9 < a1;
      a0 = t4 + a3;
      t3 = t8 < t2;
      s2 = s0 + s5;
      v0 = a0 + t3;
      s0 = t9 + t8;
      t7 = s0 < t8;
      t6 = s2 + v0;
      a2 = t6 + t7;
      s5 = a2 >> 0;
      s3 = s1 + 8;
      a0 = s1;
      timer->time.hi = s5;
      timer->time.lo = s0;

      REMOVE_FROM_LIST(timer);

      sub_0000B258(timer);

      sub_0000B304();
    }
  }
  else
  {
    a0 = s0 + 4;
    t9 = vtimer->current.low;
    s6 = vtimer->current.hi;
    t3 = s3 >> 0;
    v0 = 0;
    s7 = 0;
    t8 = s6 << 0;
    vtimer->current.hi = t3;
    s6 = v0 + t9;
    t5 = s6 < t9;
    vtimer->current.low = s2;
    s1 = t8 + s7;
    s7 = s1 + t5;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return s6s7;
}

//c6b4
int sceKernelStartVTimer(SceUID uid)
{
  SET_K1_SRL16;

  if(uid == gInfo.timerId)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_VTID;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x79, 1, uid);

  TimerInfo timer = UID_INFO(TimerInfo, cb, uidTimerType); //s3
  VTimerInfo vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType); //s0

  int ret = 1;
  if(!vtimer->active)
  {
    vtimer->active = 1;

    s1 = &vtimer->current;
    ret = 0;

    v0v1 = sub_0000B490();

    t8 = vtimer->current.high;
    t6 = vtimer->current.low;
    t7 = 0;
    t1 = t8 << 0;
    t3 = t7 + t6;
    a3 = 0;
    t5 = t3 < t6;
    t4 = t1 + a3;
    t2 = t4 + t5;
    t0 = v0 < t3;
    a1 = v1 - t2;
    t7 = 0x18(s0);
    a0 = a1 - t0;
    t8 = v0 - t3;
    t9 = a0 >> 0;
    0x4(s1) = t9;
    vtimer->current.low = t8;
    t9 = 0;
    if(vtimer->handler)
    {
      a2 = vtimer->schedule.low;
      t8 = vtimer->current.low;
      a1 = vtimer->schedule.high;
      a0 = 0x4(s1);
      s1 = 0;
      t5 = t9 + a2;
      t4 = s1 + t8;
      v0 = a1 << 0;
      t3 = a0 << 0;
      t1 = 0;
      a3 = 0;
      v1 = t5 < a2;
      s0 = v0 + t1;
      t7 = t4 < t8;
      t6 = t3 + a3;
      a1 = s0 + v1;
      t2 = t6 + t7;
      s0 = t5 + t4;
      a0 = s0 < t4;
      t9 = a1 + t2;
      v0 = t9 + a0;
      v1 = &timer->unk_8;
      a2 = v0 >> 0;
      timer->unk_8.hi = a2;
      timer->unk_8.low = s0;

      AddToTimerList(timer);

      sub_0000B304();
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//c8dc
int sceKernelStopVTimer(SceUID uid)
{
  SET_K1_SRL16;
  
  if(uid == gInfo.timerId)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_VTID;
  }
  
  int intr = sceKernelCpuSuspendIntr();
  
  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);
    
    RESET_K1;
    
    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }
  
  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }
  
  THREADMAN_TRACE(0x7A, 1, uid);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType); //s2
  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType); //s1

  int ret = 0;
  if(vtimer->active)
  {
    vtimer->active = 0;

    s0 = s1 + 0x4;
    ret = 1;
    v0v1 = sub_0000B490();

    t8 = vtimer->current.hi;
    t6 = vtimer->current.low;
    t7 = 0;
    t1 = t8 << 0;
    t3 = t7 + t6;
    a3 = 0;
    t5 = t3 < t6;
    t4 = t1 + a3;
    t2 = t4 + t5;
    t0 = v0 < t3;
    a1 = v1 - t2;
    t7 = 0x18(s1);
    a0 = a1 - t0;
    t8 = v0 - t3;
    t9 = a0 >> 0;
    vtimer->current.hi = t9;
    vtimer->current.low = t8;
    if(vtimer->handler)
    {
      REMOVE_FROM_LIST(timer);

      sub_0000B304();
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return ret;
}

//caa8
int sceKernelSetVTimerHandler(SceUID uid, SceKernelSysClock *time, SceKernelVTimerHandler handler, void *arg)
{
  SET_K1_SRL16;

  if(uid == gInfo.timerId)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_VTID;
  }

  if(IS_USER_MODE && (IS_ADDR_KERNEL(time) || IS_ADDR_KERNEL(handler) || IS_ADDR_KERNEL(arg))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x7B, 5, uid, time->hi, time->low, handler, arg);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType); //s1
  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType); //s0

  REMOVE_FROM_LIST(timer);

  vtimer->handler = handler;
  if(handler)
  {
    vtimer->schedule.low = time->low;
    vtimer->schedule.hi = time->low;
    vtimer->unk_14 = IS_USER_MODE;
    vtimer->arg = arg;
    u32 *mod = (u32 *)sceKernelFindModuleByAddress(handler);
    vtimer->gpreg = mod ? mod[0x68 / 4] : gp;

    if(vtimer->active)
    {
      t6 = vtimer->schedule.low;
      //cc74
      t9 = vtimer->schedule.hi;
      t8 = vtimer->current.hi;
      t4 = vtimer->current.low;
      t7 = 0;
      t5 = 0;
      s3 = t7 + t6;
      a1 = t5 + t4;
      s7 = t9 << 0;
      t3 = t8 << 0;
      t1 = 0;
      a3 = 0;
      s2 = s3 < t6;
      a0 = t3 + a3;
      a2 = s7 + t1;
      t2 = a1 < t4;
      v0 = a0 + t2;
      t6 = s3 + a1;
      s6 = a2 + s2;
      v1 = t6 < a1;
      t9 = s6 + v0;
      t8 = t9 + v1;
      t7 = s1 + 0x8;
      s0 = t8 >> 0;
      timer->time.hi = s0;
      timer->time.low = t6;

      AddToTimerList(timer);
    }
  }

  sub_0000B304();

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//cd64
int sceKernelSetVTimerHandlerWide(SceUID uid, SceInt64 time, SceKernelVTimerHandlerWide handler, void *arg)
{
  s1 = uid;
  s2s3 = time;
  s4 = handler;
  fp = common;
  s7 = ra;

  SET_K1_SRL16;

  if(uid == gInfo.timerId)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_VTID;
  }

  if(IS_USER_MODE && (IS_ADDR_KERNEL(handler) || IS_ADDR_KERNEL(arg))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x80, 4, uid, s2, handler, arg);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType); //s1
  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType); //s0

  REMOVE_FROM_LIST(timer);

  vtimer->handler = handler;
  if(handler)
  {
    vtimer->schedule.low = s2;
    a2 = s3 >> 0;
    vtimer->schedule.hi = a2;
    vtimer->unk_14 = IS_USER_MODE;
    vtimer->arg = arg;
    vtimer->gpreg = gp; //bug? doesn't look for handler's module first

    if(vtimer->active)
    {
      t5 = vtimer->schedule.hi;
      v1 = vtimer->current.low;
      a1 = vtimer->schedule.low;
      t3 = vtimer->current.hi;
      a0 = 0;
      s3 = 0;
      fp = t5 << 0;
      s7 = a0 + a1;
      s1 = s3 + v1;
      s2 = t3 << 0;
      t1 = 0;
      a3 = 0;
      s4 = fp + t1;
      a2 = s7 < a1;
      t9 = s1 < v1;
      t8 = s2 + a3;
      fp = s7 + s1;
      s0 = s4 + a2;
      v0 = t8 + t9;
      t7 = fp < s1;
      t6 = s0 + v0;
      t5 = t6 + t7;
      t2 = t4 + 8;
      t3 = t5 >> 0;
      timer->time.low = fp;
      timer->time.hi = t3;

      AddToTimerList(timer);
    }
  }

  sub_0000B304();

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//cfec
int sceKernelCancelVTimerHandler(SceUID uid)
{
  s1 = uid;
  s5 = ra;
  
  SET_K1_SRL16;
  
  if(uid == gInfo.timerId)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_VTID;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }

  if(!CAN_WRITE_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x7C, 1, uid);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType); //t1
  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType); //t4
  
  vtimer->handler = 0;

  REMOVE_FROM_LIST(timer);
  
  sub_0000B304();
  
  sceKernelCpuResumeIntr(intr);
  
  RESET_K1;
  
  return SCE_KERNEL_ERROR_OK;
}

//d14c
int sceKernelReferVTimerStatus(SceUID uid, SceKernelVTimerInfo *info)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(IS_USER_MODE && IS_ADDR_KERNEL(info))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  uidControlBlock *cb;
  if(sceKernelGetUIDcontrolBlockWithType(uid, uidVTimerType, &cb) != 0)
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_UNKNOWN_VTID;
  }

  if(!CAN_READ_UID(cb))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  THREADMAN_TRACE(0x81, 2, uid, info);

  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType);

  SceKernelVTimerInfo *buf = (SceKernelVTimerInfo *)gInfo.unk_6A0;

  memset(buf, 0, sizeof(SceKernelVTimerInfo));

  buf->size = sizeof(SceKernelVTimerInfo);

  if(cb->name)
  {
    strncpy(buf->name, cb->name, 0x1F);
  }

  buf->active = vtimer->active;
  if(vtimer->active)
  {
    buf->base.low = vtimer->current.low;
    buf->base.high = vtimer->current.high;

    v0v1 = sub_0000B490();

    t5 = vtimer->unk_4.low;
    t7 = vtimer->unk_4.high;
    t6 = 0;
    t2 = t6 + t5;
    t0 = t7 << 0;
    a3 = 0;
    t4 = t2 < t5;
    t3 = t0 + a3;
    t1 = t3 + t4;
    a1 = v0 < t2;
    a0 = v1 - t1;
    t9 = a0 - a1;
    s6 = v0 - t2;
    t8 = t9 >> 0;
    buf->current.low = s6;
    buf->current.high = t8;
  }
  else
  {
    buf->base.low = 0;
    buf->base.high = 0;

    buf->current.low = vtimer->current.low;
    buf->current.high = vtimer->current.high;
  }

  buf->schedule.low = vtimer->schedule.low;
  buf->schedule.high = vtimer->schedule.high;
  buf->handler = vtimer->handler;
  buf->common = vtimer->common;

  memcpy(info, buf, MIN(info->size, buf->size));

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//d3b0
_sceKernelReturnFromTimerHandler(...)
{
  return 0;
}

//d3b8
SceInt64 sceKernelUSec2SysClockWide(unsigned int usec)
{
  v0 = usec;
  v1 = 0;

  return v0v1;
}

//d3c4
int sceKernelSysClock2USec(SceKernelSysClock *clock, u32 *low, u32 *high)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(clock) || IS_ADDR_KERNEL(low) || IS_ADDR_KERNEL(high)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  //d45c
  if(low == 0)
  {
    *high = clock->low;
  }
  else
  {
    SceKernelSysClock tmp;
    sub_0000DEC0(&tmp, clock, 1000000, high);

    *low = tmp.low;
  }

  RESET_K1;

  return 0;
}

//d494
int sceKernelSysClock2USecWide(SceInt64 clock, unsigned *low, unsigned int *high)
{
  //t2t3 = clock;

  SET_K1_SRL16;

  if(IS_USER_MODE && (IS_ADDR_KERNEL(low) || IS_ADDR_KERNEL(high)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  t0 = t3 >> 0;
  SceKernelSysClock clk;
  if(low == 0)
  {
    *high = t2;
  }
  else
  {
    clk.hi = t0;
    clk.low = t2;
    
    sub_0000DEC0(&clk, &clk);
    
    *low = clk.low;
  }

  RESET_K1;
  
  return SCE_KERNEL_ERROR_OK;
}

//d55c
int sceKernelGetSystemTime(SceKernelSysClock *time)
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  if(IS_USER_MODE && IS_ADDR_KERNEL(time))
  {
    sceKernelCpuResumeIntr(intr);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  a0 = s3;

  v0v1 = sub_0000B490();

  a3 = v1 >> 0;
  time->high = a3;
  time->low = v0;
  THREADMAN_TRACE(0x69, 2, time->hi, time->low);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return 0;
}

//d62c
SceInt64 sceKernelGetSystemTimeWide()
{
  SET_K1_SRL16;

  int intr = sceKernelCpuSuspendIntr();

  SceInt64 s2s3 = sub_0000B490();

  THREADMAN_TRACE(0x6A, 1, s2);

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return s2s3;
}

//d6cc
int sceKernelGetSystemTimeLow()
{
  return *((u32 *)0xBC600000);
}

//d6d8
void sub_0000D6D8(SceKernelSysClock *clock, int millisecs)
{
  v0v1 = sub_0000B490();

  a1 = v0 + millisecs;
  t1 = 0;
  t0 = a1 < millisecs;
  a3 = v1 + t1;
  a0 = a3 + t0;
  a2 = a0 >> 0;
  clock->low = a1;
  clock->hi = a2;
}

//d730
void sub_0000D730(SceKernelSysClock *clock, int usec)
{
  v0v1 = sub_0000B490();

  a1 = clock->low;
  t8 = clock->hi;
  t7 = 0;
  t3 = t7 + usec;
  t6 = t8 << 0;
  a3 = 0;
  t5 = t3 < usec;
  t4 = t6 + a3;
  a1 = v0 + t3;
  t1 = t4 + t5;
  t2 = a1 < t3;
  t0 = v1 + t1;
  a0 = t0 + t2;
  a2 = a0 >> 0;
  clock->hi = a2;
  clock->low = a1;
}

//d7a0
SceUID sceKernelSetAlarm(SceUInt clock, SceKernelAlarmHandler handler, void *arg)
{
  SET_K1_SRL16;

  //bug: should check whether arg is a valid address
  if(handler == 0 || (IS_USER_MODE && IS_ADDR_KERNEL(handler)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int ret = sub_0000E094(clock, 0, handler, arg);

  THREADMAN_TRACE(0x70, 4, clock, handler, arg, ret);

  RESET_K1;

  return ret;
}

//d87c
SceUID sceKernelSetSysClockAlarm(SceKernelSysClock *clock, SceKernelAlarmHandler handler, void *arg)
{
  s4 = common;
  s1 = clock;
  s0 = handler;
  s5 = ra;

  SET_K1_SRL16;

  if(handler == 0 || (IS_USER_MODE && (IS_KERNEL_ADDR(clock) || IS_KERNEL_ADDR(handler))))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  a2 = s0;
  a3 = s4;
  t4 = 0x0(s1);
  t6 = 0x4(s1);
  t5 = 0;
  t3 = 0;
  t2 = t6 << 0;
  a0 = t5 + t4;
  t1 = a0 < t4;
  v0 = t2 + t3;
  a1 = v0 + t1;
  int ret = sub_0000E094(...);

  THREADMAN_TRACE(0x71, 5, clock->hi, clock->low, handler, arg, ret);

  RESET_K1;

  return ret;
}

//d994
int sub_0000D994(SceKernelSysClock *clock, void (*func)(ThreadInfo *), ThreadInfo *thread)
{
  t3 = 0;
  t2 = 0;
  v1 = clock->low;
  t4 = clock->hi;
  s0 = t3 + v1;
  t1 = t4 << 0;
  t0 = s0 < v1;
  a3 = t1 + t2;
  s1 = a3 + t0;
  v0v1 = sub_0000B490();
  a1 = s0 < v0;
  v0 = s1 - v1;
  v1 = v0 - a1;
  v0 = SCE_KERNEL_ERROR_WAIT_TIMEOUT;
  t0 = &gInfo;
  if(v1 < 0)
  {
    return SCE_KERNEL_ERROR_WAIT_TIMEOUT;
  }

  uidControlBlock *cb;
  if(gInfo.unk_5F8 <= 0)
  {
    if(sceKernelCreateUID(uidDelayType, "SceThreadmanDelay", 0, &cb) != 0)
    {
      return SCE_KERNEL_ERROR_NO_MEMORY;
    }
  }
  else
  {
    gInfo.unk_5f8--;

    cb = gInfo.unk_604.next - uidTimerType->size * sizeof(int);

    REMOVE_FROM_LIST(gInfo.unk_604.next);
  }

  if(cb == 0)
  {
    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  TimerInfo *tinfo = UID_INFO(TimerInfo, cb, uidTimerType);
  tinfo->time.low = clock->low;
  tinfo->time.hi = clock->hi;

  DelayInfo *dinfo = UID_INFO(DelayInfo, cb, uidDelayType);
  dinfo->handler = func;
  dinfo->arg = thread;

  AddToTimerList(tinfo);

  sub_0000B304();

  return cb->UID;
}

//daf4
int releaseWaitState(SceUID uid, int *unk_a1)
{
  uidControlBlock *cb; //sp_10
  if(sceKernelGetUIDcontrolBlockWithType(uid, gInfo.uidDelayType, &cb) != 0)
  {
    return -1;
  }

  DelayInfo *delay = UID_INFO(DelayInfo, cb, gInfo.uidDelayType);
  if(unk_a1 != 0)
  {
    v0v1 = sub_0000B490();

    t7 = delay->time.low;
    t8 = delay->time.hi;
    a2 = s1;
    s1 = 0;
    t5 = s1 + t7;
    t6 = t8 << 0;
    t3 = 0;
    t4 = t5 < t7;
    t0 = t6 + t3;
    t1 = t0 + t4;
    a3 = t5 < v0;
    t9 = t1 - v1;
    t6 = t5 - v0;
    t5 = t9 - a3;
    t8 = t5 >> 31; //sra
    s1 = 0 nor t8;
    t4 = s1 >> 31; //sra
    t7 = s1 >> 31; //sra
    t0 = t5 & t4;
    a0 = &sp;
    a3 = t6 & t7;
    t1 = t0 >> 0;
    a1 = 0;

    SceKernelSysClock clk;
    clk.low = a3;
    clk.hi = t1;
    sceKernelSysClock2USec(&clk, unk_a1);
  }

  REMOVE_FROM_LIST(delay);

  AddToDelayList(cb);

  sub_0000B304();

  return 0;
}

UidFunction TimerUidFuncTable[] = //15DD0
{
  { UIDFUNC_CREATE, TimerCreateUID },
  { UIDFUNC_DELETE, TimerDeleteUID },
  { 0, 0 }
};

UidFunction DelayUidFuncTable[] = //15D88
{
  { UIDFUNC_CREATE, DelayCreateUID },
  { UIDFUNC_DELETE, DelayDeleteUID },
  { 0, 0 }
};

UidFunction AlarmUidFuncTable[] = //15DA0
{
  { UIDFUNC_CREATE, AlarmCreateUID },
  { UIDFUNC_DELETE, AlarmDeleteUID },
  { 0, 0 }
};

UidFunction VTimerUidFuncTable[] = //15DB8
{
  { UIDFUNC_CREATE, VTimerCreateUID },
  { UIDFUNC_DELETE, VTimerDeleteUID },
  { 0, 0 }
};

//dbf8
int initAlarm()
{
  ASSERT(sceKernelCreateUIDtype("Timer", sizeof(TimerInfo), TimerUidFuncTable, 0, &uidTimerType) <= 0);

  ASSERT(sceKernelCreateUIDtypeInherit("Timer", "Delay", sizeof(DelayInfo), DelayUidFuncTable, 0, &uidDelayType) <= 0);

  ASSERT(sceKernelCreateUIDtypeInherit("Timer", "Alarm", sizeof(AlarmInfo), AlarmUidFuncTable, 0, &uidAlarmType) <= 0);

  ASSERT(sceKernelCreateUIDtypeInherit("Timer", "VTimer", sizeof(VTimerInfo), VTimerUidFuncTable, 0, &uidVTimerType) <= 0);

  return 0;
}

//ddb0
int loc_0000DDB0()
{
  int ret = *(0xBC600000);

  //eret

  return ret;
}

//ddc8 - suspend handler
int sub_0000DDC8()
{
  var_15e00 = *((u32 *)0xBC600000);

  sceKernelDisableIntr(gInfo.unk_72C);

  if(gInfo.unk_438 - var_15e00 <= 0 || gInfo.unk_438 - var_15e00 > 0xF0000000)
  {
    var_15e00 = gInfo.unk_438 - 1;
  }

  return 0;
}

//de3c - resume handler
int loc_0000DE3C()
{
  *((u32 *)0xBC60000C) = 1;
  *((u32 *)0xBC600008) = 0x30;
  *((u32 *)0xBC600010) = 0;
  _SYNC();

  *((u32 *)0xBC600000) = var_15e00;
  _SYNC();

  *((u32 *)0xBC600004) = gInfo.unk_438;
  _SYNC();

  _SYNC();

  sceKernelEnableIntr(gInfo.unk_72C);

  return 0;
}

//dec0
void sub_0000DEC0(SceKernelSysClock *unk_a0, SceKernelSysClock *unk_a1, u32 unk_a2, u32 *unk_a3)
{
  t0 = unk_a1->unk_0;
  v0 = unk_a1->unk_4;
  t2 = 0;
  //divu v0, a2
  v1 = unk_a1->unk_4 % unk_a2;
  a1 = v0 = unk_a1->unk_4 / unk_a2;
  t1 = 0x3;

  //dee8
  while(1 == 1)
  {
    t8 = t0 >> 24;
    t7 = v1 << 8;
    t6 = t7 | t8;
    t0 = t0 << 8;
    //divu t6, a2
    v1 = t6 % a2;
    t4 = t6 = t6 / a2;
    v0 = t2 << 8;
    t5 = a1 >> 24;
    t2 = v0 | t5;
    t3 = a1 << 8;
    t1--;
    a1 = t3 + t4;
    if(t1 < 0)
    {
      break;
    }
  }

  if(unk_a0 != 0)
  {
    unk_a0->unk_0 = a1;
    unk_a0->unk_4 = t2;
  }

  if(a3 != 0)
  {
    *a3 = v1;
  }
}

//df4c
int sub_0000DF4C(uidControlBlock *cb, unk_a1)
{
  int ret;

  gInfo.timerId = cb->UID;

  VTimerInfo *vtimer = UID_INFO(VTimerInfo, cb, uidVTimerType);

  v0v1 = sub_0000B490();

  t7 = vtimer->current.low;
  t8 = vtimer->current.hi;
  a3 = 0;
  t3 = s3 + t7;
  t6 = t8 << 0;
  t5 = t3 < t7;
  t4 = t6 + a3;
  t1 = t4 + t5;
  
  t0 = v1 - t1;
  t2 = v0 < t3;
  a0 = t0 - t2;
  a1 = v0 - t3;
  a2 = a0 >> 0;
  SceKernelSysClock clk;
  clk.low = a1;
  clk.hi = a2;

  s3 = gp;
  gp = vtimer->gpreg;
  if(vtimer->unk_14)
  {
    u32 *stack = sceKernelGetUserIntrStack() - 0x10;

    stack[0] = vtimer->schedule.low;
    stack[1] = vtimer->schedule.hi;
    stack[2] = clk.low;
    stack[3] = clk.hi;

    ret = sceKernelCallUserIntrHandler(cb->UID, stack, (SceKernelSysClock *)&stack[2], vtimer->arg, vtimer->handler, stack);
  }
  else
  {
    int intr = sceKernelCpuResumeIntr();

    ret = vtimer->handler(cb->UID, &vtimer->schedule, &clk, vtimer->arg);

    sceKernelCpuSuspendIntr(intr);
  }

  gp = s3;

  return ret;
}

//e094
int sub_0000E094(unk_a0, unk_a1, handler, void *arg)
{
  if(!IS_USER_MODE && !IS_ADDR_KERNEL(arg))
  {
    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr();

  uidControlBlock *cb;
  int ret = sceKernelCreateUID(gInfo.uidAlarmType, "SceThreadmanAlarm", IS_USER_MODE ? 0xFF : 0, &cb);
  if(ret != 0)
  {
    sceKernelCpuResumeIntr(intr);

    return ret;
  }

  TimerInfo *timer = UID_INFO(TimerInfo, cb, uidTimerType);
  AlarmInfo *alarm = UID_INFO(AlarmInfo, cb, uidAlarmType);

  v0v1 = sub_0000B490();

  t7 = v0 + unk_a0;
  t8 = t7 < v0;
  s4 = v1 + unk_a1;
  t6 = unk_a0 + t8;
  t4 = t6 >> 0;
  timer->time.low = t7;
  timer->time.hi = t4;

  alarm->unk_0 = IS_USER_MODE;
  alarm->handler = handler;
  alarm->arg = arg;

  u32 *mod = (u32 *)sceKernelFindModuleByAddress(s2);
  alarm->gpreg = mod ? mod[0x68 / 4] : gp;

  AddToTimerList(timer);

  sub_0000B304();

  sceKernelCpuResumeIntr(intr);

  return cb->UID;
}

//e200
int DelayCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);
  
  return cb->UID;
}

//e228
int DelayDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return 0;
}

//e248
int AlarmCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//e270
int AlarmDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return 0;
}

//e290
int VTimerCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return cb->UID;
}

//e2b8
int VTimerDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return 0;
}

//e2d8
int TimerCreateUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  TimerInfo *timer = UID_INFO(TimerInfo, cb, type);
  timer->unk_10 = 0;

  return cb->UID;
}

//e324
int TimerDeleteUID(uidControlBlock *cb, uidControlBlock *type, int funcid, void *args)
{
  TimerInfo *timer = UID_INFO(TimerInfo, cb, unk_a1);

  REMOVE_FROM_LIST(timer);

  SysMemForKernel_CE05CCB7(cb, type, funcid, args);

  return 0;
}
