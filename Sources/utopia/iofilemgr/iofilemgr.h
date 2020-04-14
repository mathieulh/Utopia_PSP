#ifndef __IOFILEMGR_H
#define __IOFILEMGR_H

#define MAX_FILES    63

typedef struct _DeviceAlias
{
  struct _DeviceAlias *next; //0
  PspIoDrvArg *drvArg;
  int dev_type; //0x10 or 0x20
  int fs_num;
  char aliasname[32]; //10
  char fsdevname[32]; //30
  char physdevname[32]; //50
} DeviceAlias;

typedef struct _DrvTable
{
  struct _DrvTable *next;
  PspIoDrvArg drvArg;
} DrvTable;

#endif