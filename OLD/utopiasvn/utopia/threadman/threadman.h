#ifndef __THREADMAN_H
#define __THREADMAN_H

#include "../common/common.h"

#define MAX_THREAD_PRIORITY     127
#define MIN_THREAD_STACKSIZE    0x200
#define THREAD_KSTACKSIZE       0x800

#define CPU_PROFILER_NONE       0
#define CPU_PROFILER_THREAD     1
#define CPU_PROFILER_GLOBAL     2

#define THREADMAN_TRACE(code, nargs, ...)       { if(gInfo.trace) gInfo.trace(code, _GET_GPREG(GPREG_RA), nargs, ##__VA_ARGS__); }

#define SET_THREAD_PRIORITY_FLAG(p)   gInfo.priority_mask[(p) / 32] |= 1 << ((p) % 32)
#define CLEAR_THREAD_PRIORITY_FLAG(p) gInfo.priority_mask[(p) / 32] &= ~(1 << ((p) % 32))

typedef int (*SceKernelThreadEntry)(SceSize args, void *argp);

typedef struct _SceProfilerInfo
{
  unsigned int systemck;
  unsigned int cpuck;
  unsigned int stallTotal;
  unsigned int stallInternal;
  unsigned int stallMemory;
  unsigned int stallCOPz;
  unsigned int stallVFPU;
  unsigned int sleep;
  unsigned int busAccess;
  unsigned int uncachedLoad;
  unsigned int uncachedStore;
  unsigned int cachedLoad;
  unsigned int cachedStore;
  unsigned int iCacheMiss;
  unsigned int dCacheMiss;
  unsigned int dCacheWB;
  unsigned int instrCOP0;
  unsigned int instrFPU;
  unsigned int instrVFPU;
  unsigned int localBus;
} SceProfilerInfo;

typedef struct _SceThreadProfilerInfo
{
  int unk_0;
  int unk_4;
  int unk_8;
  int unk_C;
  SceProfilerInfo info;
} SceThreadProfilerInfo;

#define THREAD_PROFILER_INFO         ((SceProfilerInfo *)0xBC400000)

typedef struct _SceThreadStack
{
  SceUID threadid;
  int unk_4;
  int unk_8;
  int unk_C;
  int *value; //array of values on the stack
} SceThreadStack;

typedef struct _SceSyscallStackInfo
{
  struct _SceSyscallStackInfo *next;
  SceThreadStack *stack;
  SceUInt stackSize;
} SceSyscallStackInfo;

typedef struct
{
  ThreadInfo *next; //0
  ThreadInfo *prev; //4
  int UID; //8 - uid of thread's uidControlBlock
  int status; //c
  int currentPriority; //10
  int wakeupCount; //14
  int exitStatus; //18
  int waitType; //1c
  uidControlBlock *waitTypeCB; //20
  int cbArg1; //24
  int cbArg2; //28
  int cbArg3; //2c
  int cbArg4; //30
  int cbArg5; //34
  int timeoutId; //38 - UID of Delay for timeout
  int *timeout; //3c
  int callbackStatus; //40 - callback status
  int eventMask; //44 - events being fired
  uidControlBlock *eventHandlerCB; //48 - event handler being fired
  LinkedList callbacks; //4c - callbacks for thread
  int isCallback; //54 - is callback
  int callbackNotify; //58 - callback notify
  uidControlBlock *callbackCB; //5c - callback
  int initPriority; //60
  SceKernelSysClock runClocks; //64
  SceKernelThreadEntry *entry; //6c
  void *stack; //70
  int stackSize; //74 (aligned by 0x100)
  int stackPointer; //78 stack pointer
  void *kStack; //7c
  int kStackSize; //80
  SceSyscallStackInfo *scStackInfo; //84
  void *gpreg; //88 ($gp or the module address)
  void *unk_8C;
  int ktls[0x10]; //90 - kernel thread local storage
  int attr; //d0
  int initAttr; //d4
  int arglen; //d8
  void *argp; //dc
  SceKernelThreadOptParam *option; //e0
  int *pCount; //e4 - pointer to intrPreemptCount or threadPreemptCount
  int intrPreemptCount; //e8
  int threadPreemptCount; //ec
  int releaseCount; //f0
  struct SceThreadContext *threadContext; //f4 - context for the current thread
  struct SceThreadContext *callbackContext; //f8 - context saved during callback
  void *vfpuContext; //fc - VFPU context (0x240 bytes)
  SceSCContext *scContext; //100
  int kStackPointer; //104
  SceThreadProfilerInfo *thread_profiler_info; //108
} ThreadInfo;

typedef struct
{
  void *next;
  void *prev;
} LinkedList;

#define CLEAR_LIST(list)      (x).next = (x).prev = &(x);
#define ADD_TO_LIST(x, list) (x)->next = &(list); (x)->prev = (list).prev; (list).prev = (x); (x)->prev->next = (x);
#define INSERT_IN_LIST_BEFORE(x, y)  (x)->next = (y); (x)->prev = (y)->prev; (y)->prev = (x); (x)->prev->next = (x);
#define REMOVE_FROM_LIST_SAFE(x, next_x)  (next_x)->prev = (x)->prev; (x)->prev->next = (next_x); (x)->next = (x); (x)->prev = (x);
#define REMOVE_FROM_LIST(x)  REMOVE_FROM_LIST_SAFE(x, (x)->next)
#define LOOP_LIST_SAFE(x, next_x, list)   for((x) = (list).next, (next_x) = (x)->next; (x) != &(list); (x) = (next_x), (next_x) = (x)->next)
#define LOOP_LIST(x, list)   for((x) = (list).next; (x) != &(list); (x) = (x)->next)

typedef struct _ThreadCleanup
{
  struct _ThreadCleanup *next;
  SceUID uid[8];
  int used;
} ThreadCleanup;

typedef struct _ThreadmanInfo
{
  ThreadInfo *currentThread; //current thread?
  ThreadInfo *nextThread; //4
  int priority_mask[(MAX_THREAD_PRIORITY + 1) / 32]; //8 - 1 bit per priority to indicate whether there are any threads running
  LinkedList readyThreads[MAX_THREAD_PRIORITY + 1]; //18 - threads grouped by priority
  int dispatch_thread_suspended; //418
  void *vfpuContext; //41c
  int unk_420; //0 = user thread, -1 = kernel thread
  unsigned int (*unk_424)(void); //sceKernelGetSystemTimeLow
  SceKernelSysClock unk_428;
  SceKernelSysClock unk_430; //system time
  SceKernelSysClock unk_438;
  int timerId; //440
  KTLSInfo ktls[0x10]; //KTLs?
  ThreadInfo *thIdle; //5c4
  ThreadInfo *thCleaner; //5c8
  int doCleanup; //5cc
  ThreadCleanup cleanup; //5d0
  int delay_count; //5f8
  LinkedList timers; //5FC
  LinkedList delays; //604
  LinkedList eventHandlers; //60C
  LinkedList sleepingThreads; //614
  LinkedList delayedThreads; //61c
  LinkedList stoppedThreads; //624
  LinkedList suspendedThreads; //62c
  LinkedList deadThreads; //634 - dormant/killed threads
  uidControlBlock *uidWaitQType; //63c
  uidControlBlock *uidThreadType; //640
  uidControlBlock *uidSemaphoreType; //644
  uidControlBlock *uidEventFlagType; //648
  uidControlBlock *uidMboxType; //64c
  uidControlBlock *uidMsgPipeType; //650
  uidControlBlock *uidVplType; //654;
  uidControlBlock *uidFplType; //658
  uidControlBlock *uidThreadEventHandlerType; //65c
  uidControlBlock *uidCallbackType; //660
  uidControlBlock *uidTimerType; //664
  uidControlBlock *uidDelayType; //668
  uidControlBlock *uidAlarmType; //66c
  uidControlBlock *uidVTimerType; //670
  SceUID heapuid; //674
  int systemStatusFlag; //678
  int threadSwitchCount; //67c
  int unk_680;
  int vfpuSwitchCount; //684
  int unk_688;
  int unk_68C;
  int unk_690;
  int unk_694;
  int unk_698; //_sceKernelExitThread syscall
  int unk_69C; //_sceKernelReturnFromCallback syscall
  u8 unk_6A0[0x10]; //buffer for refer status - will overlap with the vars below, as the structs used tend to be about 0x30 bytes!
  u8 unk_6B0[0x8]; //buffer used for storing option to pass to sceKernelAllocHeapMemoryWithOption in sceKernelCreateThread
  u8 unk_6B8[0x8]; //buffer used for storing the SceKernelThreadOptParam passed in to sceKernelCreateThread
  u8 unk_6C0[0x68]; //buffer used for holding stack names (0x20 chars) in sceKernelCreateThread
  int cpuID; //728
  int timerIntr; //72c
  int cpuProfilerMode; //CPU profiler enabled: 0 = none, 1 = thread mode, 2 = global mode
  unk_734;
  void (*trace)(int type, int ra, int nargs, ...); //trace function?
} ThreadmanInfo;

#endif