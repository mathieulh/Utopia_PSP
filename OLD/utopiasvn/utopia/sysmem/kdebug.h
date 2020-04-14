#ifndef __KDEBUG_H
#define __KDEBUG_H

typedef void (*PspDebugPutChar)(unsigned short *args, unsigned int ch);

int InitKDebug(int unk_a0, int unk_a1);
void Kprintf(char *format, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6, void *arg7);
void sceKernelAssert(int unk_a0, int level);

#endif
