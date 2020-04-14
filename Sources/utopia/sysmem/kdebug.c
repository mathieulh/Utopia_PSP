#include "kdebug.h"

var_C510;
var_C514;
var_C518; //c518 - debug echo handler
DebugPutChar debug_putchar; //c82c
debug_write_handler; //c830 - debug write handler
debug_read_handler; //c834 - debug read handler
void (*assert_handler)(int); //c838 - assert handler
int var_C83C; //c83c - bitmask
int var_C840; //c840
int var_C844; //c844

//70b4
DebugPutChar sceKernelGetDebugPutchar()
{
  return debug_putchar;
}

//70c0
void sceKernelRegisterDebugPutchar(PspDebugPutChar putchar)
{
  debug_putchar = putchar;
}

//70cc
void sceKernelRegisterKprintfHandler(unk_a0, unk_a1)
{
  var_C510 = unk_a0;

  var_C514 = unk_a1;
}

//70e0
char *strchr(char *haystack, char needle, int maxlen)
{
  char *p;

  //unk_a1 = unk_a1 & 0xFF;
  if(haystack == 0 || maxlen < 0)
  {
    return 0;
  }

  for(p = haystack; maxlen > 0; p++, maxlen--)
  {
    if(*p == needle)
    {
      return p;
    }
  }

  return 0;
}

//7118
int strlen(char *str)
{
  int len;
  char *p;

  if(str == 0)
  {
    return 0;
  }

  for(p = str, len = 0; *p; p++, len++);

  return len;
}

//714c
sub_0000714C()
{
}

//7880
void Kprintf(char *format, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6, void *arg7)
{
  int ret = 0;

  int intr = sceKernelCpuSuspendIntr();

  void *args[7] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };

  if(var_C510 != 0)
  {
    //78f8
    ret = var_C510(var_C514, format, args);
  }

  //78d4
  sceKernelCpuResumeIntr(intr);

  return ret;
}

//790c
int sceKernelDebugWrite()
{
  if(debug_write_handler == 0)
  {
    return SCE_KERNEL_ERROR_ERROR;
  }

  return debug_write_handler();
}

//793c
int sceKernelRegisterDebugWrite(int unk_a0)
{
  debug_write_handler = unk_a0;

  return 0;
}

//794c
int sceKernelDebugRead()
{
  if(debug_read_handler == 0)
  {
    return SCE_KERNEL_ERROR_ERROR;
  }

  return debug_read_handler();
}

//797c
int sceKernelRegisterDebugRead(unk_a0)
{
  debug_read_handler = unk_a0;

  return 0;
}

//798c
int sceKernelDebugEcho()
{
  return var_C518;
}

//7998
int sceKernelDebugEchoSet(int unk_a0)
{
  var_C518 = unk_a0;

  return unk_a0;
}

//79a8
void sceKernelRegisterAssertHandler(void (*handler)(int))
{
  assert_handler = handler;
}

//79b4
void sceKernelAssert(int unk_a0, int level)
{
  int ret = sceKernelRemoveByDebugSection();

  if(unk_a0 != 0)
  {
    return;
  }

  if(level >= (ret & 0x300) / 0x100)
  {
    Kprintf("assertion ignore (level %d >= %d)\n", level, (ret & 0x300) / 0x100);

    return;
  }

  if(assert_handler == 0)
  {
    Kprintf("There is no assert handler, stop\n");
    while(1 == 1) { };
  }

  assert_handler(0);
}

//7a38
int InitKDebug(unk_a0, unk_a1)
{
  var_C83C = unk_a0;

  var_C840 = unk_a1;

  return 0;
}

//7a50
//bit 22 = CPU profiler mode (0 = global, 1 = thread)
//bit 23 = CPU profiler enabled
int KDebugForKernel_24C32559(int bitpos)
{
  if(unk_a0 > 31)
  {
    return SCE_KERNEL_ERROR_ERROR;
  }

  return ((var_C83C >> bitpos) & 0x1);
}

//7a7c
int sceKernelRemoveByDebugSection()
{
  return var_C83C;
}

//7a88
int KDebugForKernel_5282DD5E(int bitpos)
{
  if(bitpos > 31)
  {
    return SCE_KERNEL_ERROR_ERROR;
  }

  var_C83C |= 1 << bitpos;

  return var_C83C;
}

//7abc
int KDebugForKernel_9F8703E4()
{
  return var_C840;
}

//7ac8
int KDebugForKernel_333DCEC7(unk_a0)
{
  var_C844 = unk_a0;

  return 0;
}

//7ad8
int KDebugForKernel_E892D9A1()
{
  return var_C844;
}

