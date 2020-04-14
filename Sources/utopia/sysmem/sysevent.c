SceSysEventHandler *sys_event_handlers; //c524

//6620
int ClearSysEventHandlers()
{
  sys_event_handlers = 0;

  return 0;
}

//6630
int sceKernelIsRegisterSysEventHandler(SceSysEventHandler *handler)
{
  SceSysEventHandler *tmp;

  int intr = sceKernelCpuSuspendIntr();

  for(tmp = sys_event_handlers; tmp; tmp = tmp->next)
  {
    if(tmp == handler)
    {
      break;
    }
  }

  //6670
  sceKernelCpuResumeIntr(intr);

  return (tmp > 0);
}

//6690
int sceKernelRegisterSysEventHandler(SceSysEventHandler *handler)
{
  int intr = sceKernelCpuSuspendIntr();

  if(sceKernelIsRegisterSysEventHandler(handler))
  {
    //66f8
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_FOUND_HANDLER;
  }

  handler->busy = 0;
  handler->r28 = gp;
  handler->next = sys_event_handlers;

  sys_event_handlers = handler;

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//670c
int sceKernelUnregisterSysEventHandler(SceSysEventHandler *handler)
{
  int intr = sceKernelCpuSuspendIntr();

  if(handler->busy)
  {
    sceKernelCpuResumeIntr(intr);

    return SCE_KERNEL_ERROR_ERROR;
  }

  SceSysEventHandler *temp, *last = 0;

  for(temp = sys_event_handlers; temp; temp = temp->next)
  {
    if(temp == handler)
    {
      if(last)
      {
        last->next = temp->next;
      }
      else
      {
        sys_event_handlers = temp->next;
      }

      break;
    }

    last = temp;
  }

  sceKernelCpuResumeIntr(intr);

  return (temp < 1 ? SCE_KERNEL_ERROR_NOTFOUND_HANDLER : 0);
}

//67b4
int sceKernelSysEventDispatch(int ev_type_mask, int ev_id, char *ev_name, void *param, int *result,
                                  int break_nonzero, PspSysEventHandler *break_handler)
{
  int ret;
  SceSysEventHandler *handler;

//   fp = gp;
  int intr = sceKernelCpuSuspendIntr();

  for(handler = sys_event_handlers; handler; handler = handler->next)
  {
    if(handler->typemask & unk_a0)
    {
      //687c
      handler->busy = 1;

      sceKernelCpuResumeIntr(intr);

//       v0 = gp;
//       gp = s0->r28;
      ret = handler->handler(unk_a1, unk_a2, unk_a3, unk_t0);

      intr = sceKernelCpuSuspendIntr();

      handler->busy = 0;
      if((ret >> 31) & (0 < unk_t1))
      {
        //68d0
        if(unk_t2 != 0)
        {
          unk_t2->unk_0 = s0;
        }

        break;
      }

      ret = 0;
    }
  }

  //6838
  sceKernelCpuResumeIntr(intr);

//   v1 = gp;
//   gp = fp;

  return ret;
}
