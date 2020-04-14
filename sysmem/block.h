#ifndef __BLOCK_H
#define __BLOCK_H

#include <pspsdk.h>

#define MEMBLOCK_USEFLAG   0x1
#define MEMBLOCK_DELFLAG   0x1
#define MEMBLOCK_SIZELOCK  0x2

#define GET_MEMBLOCK_POS(b) ((b)->pos >> 1)
#define SET_MEMBLOCK_POS(b, p) (b)->pos = ((b)->pos & MEMBLOCK_USEFLAG) | (p << 1)

#define GET_MEMBLOCK_NBLOCKS(b) ((b)->nblocks >> 2)
#define SET_MEMBLOCK_NBLOCKS(b, n) (b)->nblocks = ((b)->nblocks & (MEMBLOCK_DELFLAG | MEMBLOCK_SIZELOCK)) | (n << 2)

#define GET_MEMBLOCK_USED(b) IS_SET((b)->pos, MEMBLOCK_USEFLAG)
#define SET_MEMBLOCK_USED(b) SET_BIT((b)->pos, MEMBLOCK_USEFLAG)
#define UNSET_MEMBLOCK_USED(b) CLEAR_BIT((b)->pos, MEMBLOCK_USEFLAG)

#define GET_MEMBLOCK_DELETED(b) IS_SET((b)->nblocks, MEMBLOCK_DELFLAG)
#define SET_MEMBLOCK_DELETED(b) SET_BIT((b)->pos, MEMBLOCK_DELFLAG)
#define UNSET_MEMBLOCK_DELETED(b) CLEAR_BIT((b)->pos, MEMBLOCK_DELFLAG)

#define GET_MEMBLOCK_SIZELOCKED(b) IS_SET((b)->nblocks, MEMBLOCK_SIZELOCK)
#define SET_MEMBLOCK_SIZELOCKED(b) SET_BIT((b)->pos, MEMBLOCK_SIZELOCK)
#define UNSET_MEMBLOCK_SIZELOCKED(b) CLEAR_BIT((b)->pos, MEMBLOCK_SIZELOCK)

typedef struct _BlockInfo
{
  int addr;
  SceSize size;
  struct _PartitionInfo *partition;
} BlockInfo;

typedef struct _MemMgmtSubBlock
{
  struct _MemMgmtSubBlock *next;
  int pos; //bit 1 = useflag, >> 1 = pos
  int nblocks; //bit 1 = delflag, bit 2 = sizelock, >> 2 = num of blocks
} MemMgmtSubBlock;

typedef struct _MemMgmtBlock
{
  struct _MemMgmtBlock *next;
  MemMgmtSubBlock subblocks[0x14];
} MemMgmtBlock;

int _AllocPartitionMemory(PartitionInfo *part, int type, SceSize size, void *addr);

#endif
