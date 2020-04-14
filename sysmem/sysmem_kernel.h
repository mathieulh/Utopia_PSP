#ifndef __SYSMEM_KERNEL_H
#define __SYSMEM_KERNEL_H

typedef struct _PspSysmemPartitionInfo
{
	SceSize size;
	unsigned int startaddr;
	unsigned int memsize;
	unsigned int attr;
} PspSysmemPartitionInfo;

struct _uidControlBlock {
    struct _uidControlBlock *parent;
    struct _uidControlBlock *nextChild;
    struct _uidControlBlock *type;   //(0x8)
    u32 UID;					//(0xC)
    char *name;					//(0x10)
	unsigned char unk;
	unsigned char size;			// Size in words
    short attribute;
    struct _uidControlBlock *nextEntry;
    struct _uidControlBlock *inherited;
    UidLookupFunction *func_table;
} __attribute__((packed));
typedef struct _uidControlBlock uidControlBlock;


#endif