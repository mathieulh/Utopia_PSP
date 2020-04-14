#ifndef __UIDMEMORY_H
#define __UIDMEMORY_H

#define UID_ROOTNAME          "Root"
#define UID_METAROOTNAME      "MetaRoot"
#define UID_BASICNAME         "Basic"
#define UID_METABASICNAME     "MetaBasic"

#define UID_INFO(type,uid,uidtype)  ((type *)((int)uid + (uidtype)->size * sizeof(int)))
#define UIDCB_FROM_INFO(info, type) ((uidControlBlock *)((info) - (type)->size * sizeof(int)))

#define UIDFUNC_CREATE            0xD310D2D9
#define UIDFUNC_DELETE            0x87089863
#define UIDFUNC_GETTYPEUID        0x58D965CE
#define UIDFUNC_EQUALS            0x9AFB14E2
#define UIDFUNC_GETNAME           0xF0ADE1B6
#define UIDFUNC_ISTYPE            0xE19A43D1
#define UIDFUNC_CANCEL            0x86D94883

enum SceUidAttributes
{
  SCE_UID_ATTR_CAN_READ = 0x1,
  SCE_UID_ATTR_CAN_WRITE = 0x2,
  SCE_UID_ATTR_UNK4 = 0x4, //wait/poll/etc.
  SCE_UID_ATTR_CAN_CANCEL = 0x8,
  SCE_UID_ATTR_CAN_DELETE = 0x10,
  SCE_UID_ATTR_UNK20 = 0x20,
  SCE_UID_ATTR_UNK40 = 0x40,
  SCE_UID_ATTR_UNK80 = 0x80
};

#define UID_LEGAL_ATTR       0xFF
#define UID_GRANT_ALL        0xFF
#define CAN_READ_UID(cb)     (!IS_USER_MODE || IS_SET((cb)->attr, SCE_UID_ATTR_CAN_READ))
#define CAN_WRITE_UID(cb)    (!IS_USER_MODE || IS_SET((cb)->attr, SCE_UID_ATTR_CAN_WRITE))
#define CAN_UNK4_UID(cb)     (!IS_USER_MODE || IS_SET((cb)->attr, SCE_UID_ATTR_UNK4))
#define CAN_CANCEL_UID(cb)   (!IS_USER_MODE || IS_SET((cb)->attr, SCE_UID_ATTR_CAN_CANCEL))
#define CAN_DELETE_UID(cb)   (!IS_USER_MODE || IS_SET((cb)->attr, SCE_UID_ATTR_CAN_DELETE))

typedef struct _UidLookupFunction
{
  int funcid;
  int (*func)(uidControlBlock *cb, unk_a1, int funcid, void *args);
} UidLookupFunction;

typedef struct _UidMemoryInfo
{
  uidControlBlock *uidRoot;
  uidControlBlock *uidMetaRoot;
  uidControlBlock *uidBasicType;
  int uid_count;
} UidMemoryInfo;

typedef struct _BasicInfo
{
  int unk_0;
} BasicInfo;

#endif