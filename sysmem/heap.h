#ifndef __HEAP__H
#define __HEAP__H

#define GET_HEAP_ATTR_UNK2(heap)   ((heap)->size & (SCE_SYSMEM_HEAP_ATTR_UNK2 >> 1))
#define SET_HEAP_ATTR_UNK2(heap)   (heap)->size |= (SCE_SYSMEM_HEAP_ATTR_UNK2 >> 1)
#define UNSET_HEAP_ATTR_UNK2(heap) (heap)->size &= ~(SCE_SYSMEM_HEAP_ATTR_UNK2 >> 1)

#define GET_HEAP_SIZE(heap)        ((heap)->size & ~0x1)
#define SET_HEAP_SIZE(heap, size)  (heap)->size = (size) | GET_HEAP_ATTR_UNK2(heap)

typedef struct _SceKernelHeapOptParam
{
  SceSize size;
  int alignment;
} SceKernelHeapOptParam;

typedef struct _HeapMgmtBlock
{
  int unk_0;
  int unk_4; //size
  int unk_8;
  int unk_C;
  int unk_10;
  int unk_14;
} HeapMgmtBlock;

typedef struct _HeapHeader
{
  int unk_0;
  int unk_4;
  HeapMgmtBlock heap;
} HeapHeader;

enum SceSysmemHeapAttributes
{
  SCE_SYSMEM_HEAP_ATTR_UNK1 = 0x1,
  SCE_SYSMEM_HEAP_ATTR_UNK2 = 0x2
};

typedef struct _HeapInfo
{
  int size; //bit 0 = SCE_SYSMEM_HEAP_ATTR_UNK2
  int partition;
  int addr;
} HeapInfo;

#endif