#ifndef __HANDLER_H
#define __HANDLER_H

typedef int (*SysEventHandlerFunc)(int ev_id, char* ev_name, void* param, int* result);

typedef struct _SysEventHandler {
  int size;
  char* name;
  int type_mask;
  int (*handler)(int ev_id, char* ev_name, void* param, int* result);
  int r28;
  int busy;
  struct _SysEventHandler *next;
  int reserved[9];
} SysEventHandler;

int sceKernelIsRegisterSysEventHandler(SceSysEventHandler *handler);

int sceKernelRegisterSysEventHandler(SceSysEventHandler *handler);

int sceKernelUnregisterSysEventHandler(SceSysEventHandler *handler);

int sceKernelSysEventDispatch(int ev_type_mask, int ev_id, char *ev_name, void *param, int *result,
                                  int break_nonzero, PspSysEventHandler *break_handler);

                                  
#endif