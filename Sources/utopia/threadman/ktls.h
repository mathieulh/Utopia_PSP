#ifndef __KTLS_H
#define __KTLS_H

typedef struct _KTLSInfo
{
  int allocated; //0
  int used; //4 - number of threads using this KTLS
  SceSize size; //8
  int (*func)(unsigned int *size, void *arg); //c
  void *arg; //10
  int gpreg; //14
} KTLSInfo;

#endif