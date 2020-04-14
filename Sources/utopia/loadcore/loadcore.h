#ifndef __LOADCORE_H
#define __LOADCORE_H

#define ELF_TYPE_PRX    0xFFA0

#define NID_MODULE_BOOTSTART            0xD3744BE0
#define NID_MODULE_REBOOT_PHASE         0xADF12745
#define NID_MODULE_START                0xD632ACDB
#define NID_MODULE_STOP                 0xCEE8593C
#define NID_MODULE_REBOOT_BEFORE        0x2F064FA6
#define NID_MODULE_INFO                 0xF01D73A7

typedef struct _SceSysCallTableEntry
{
  unk_0;
  unk_4;
  unk_8;
  unk_C;
} SceSysCallTableEntry;

typedef struct _SceSysCallTable
{
  unk_0;
  unk_4;
  unk_8;
  unk_C;
  SceSysCallTableEntry *entry;
} SceSysCallTable;

typedef struct _SceSysCallTableIndex
{
  struct _SceSysCallTableIndex *next;
  short unk_4;
  short unk_6; //free
  int unk_8;
} SceSysCallTableIndex;

typedef struct _SceHeader
{
  u32 unk_0[16];
} SceHeader;

typedef struct _ElfHeader
{
  u32 code; //.ELF
  u16 unk_4;
  u16 unk_6;
  u8 unk_8[8];
  u16 type; //10
  u16 machine; //12
  u32 version; //14
  void *entry; //18
  u32 phoff; //1C
  u32 shoff; //20
  u32 flags; //24
  u16 ehsize; //28
  u16 phentsize; //2a
  u16 phnum; //2c
  u16 shentsize; //2e
  u16 shnum; //30
  u16 shstrndx; //32
} ElfHeader;

typedef struct _ElfProgramHeader
{
  u32 type; //0
  u32 offset; //4
  u32 vaddr; //8
  u32 paddr; //C
  u32 filesz; //10
  u32 memsz; //14
  u32 flags; //18
  u32 align; //1C
} ElfProgramHeader;

typedef struct _PspHeader
{
  u32 code; //~PSP
  short unk_4;
  short unk_6;
  short unk_8;
  char modname[0x1C]; //A
  u8 unk_26;
  u8 unk_27;
  u32 unk_28; //gzip buffer size
  u32 unk_2C;
  u32 unk_30; //bootstart
  u32 unk_34; //addr? bit 31 = kernel mode
  u32 unk_38;
  u32 unk_3C;
  u32 unk_40;
  u32 unk_44;
} PspHeader;

typedef struct _reglibin
{
  char *name; //0
  short version; //4 BCD version
  short attr; //6
  char len; //8 - size in words
  char num_vars; //9 - number of variables
  short num_funcs; //A - number of functions
  void *entry_table; //C - pointer to entry table in .rodata.sceResident
} reglibin;

//size 0x24
typedef struct _SceLibraryEntry
{
  struct _SceLibraryEntry *next;
  char *name; //4
  short version; //8
  short attr; //A
  char len; //C
  char num_vars; //D
  short num_funcs; //E
  void *entry_table; //10
  int unk_14; //address of libent
  SceLibraryStub *stub; //18
  int unk_1C;
  int is_user_mode;
} SceLibraryEntry;

typedef struct _stubin
{
  char *libname; //0 - library name
  short version; //4 - version
  short attr; //6 - attribute
  char len; //8 - length of table in words
  char num_vars; //9 - number of variables imported
  short num_funcs; //A - number of functions imported
  u32 *nid; //C - array of NIDs
  ??? *func; //10 - array of functions
  ??? *var; //14 - array of variables
} stubin;

typedef struct _SceLibraryStub
{
  int unk_0; //0
  struct _SceLibraryStub *next; //4
  char *libname; //8 - library name
  short version; //C - version
  short attr; //E - attribute
  char len; //10 - length of table in words
  char num_vars; //11 - number of variables imported
  short num_funcs; //12 - number of functions imported
  u32 *nid; //14 - array of NIDs
  ??? *func; //18 - array of functions
  ??? *var; //1C - array of variables
  stubin *original; //20
  int unk_24; //24
  int is_user_mode; //28
} SceLibraryStub;

//as in PSP_MODULE_INFO
typedef struct _SceModuleInfo
{
  unsigned short attribute; //0
  unsigned char version[2]; //2
  char modname[0x1C]; //4
  unsigned int text_addr; //20
  unsigned int ent_top; //24
  unsigned int ent_bottom; //28
  unsigned int stub_top; //2C
  unsigned int stub_bottom; //30
} SceModuleInfo;

//size 0xC0
typedef struct _SceLoadCoreExecFileInfo
{
  int unk_0;
  int unk_4; //attr? 0x1 = , 0x2 =
  int unk_8; //API
  int unk_C;
  int unk_10; //offset of start of file (after ~SCE header if it exists)
  int unk_14;
  int unk_18;
  int unk_1C;
  int elf_type; //20 - elf type - 1,2,3 valid
  int topaddr; //24 - address of gzip buffer
  int (*bootstart)(SceSize, void *); //28
  int unk_2C;
  int unk_30; //30 - size of PRX?
  int unk_34; //
  int unk_38;
  int unk_3C;
  int unk_40; //partition id
  int unk_44;
  int unk_48;
  int unk_4C;
  SceModuleInfo *module_info; //50 - pointer to module info i.e. PSP_MODULE_INFO(...)
  int unk_54;
  short unk_58; //attr as in PSP_MODULE_INFO - 0x1000 = kernel
  short unk_5A; //attr? 0x1 = use gzip
  int unk_5C; //size of gzip buffer to allocate
  int unk_60;
  int unk_64;
  int unk_68;
  int unk_6C;
  reglibin *export_libs; //70
  int num_export_libs; //74
  int unk_78;
  int unk_7C;
  int unk_80;
  unsigned char unk_84[4];
  unsigned int segmentaddr[4]; //88
  unsigned int segmentsize[4]; //98
  unsigned int unk_A8;
  unsigned int unk_AC;
  unsigned int unk_B0;
  unsigned int unk_B4;
  unsigned int unk_B8;
  unsigned int unk_BC;
} SceLoadCoreExecFileInfo;

//size 0x20
typedef struct _SceLoadCoreBootModuleInfo
{
  int unk_0; //full path/filename of the module
  int unk_4; //buffer with entire file
  int unk_8; //size of PRX
  int unk_C;
  int unk_10; //1 x % in pspbtcnf => bit 0 set; 2 x % in pspbtcnf => bits 0 and 1 set
  int unk_14; //set to 0 in IPL
  int unk_18; //set to 0 in IPL
  int unk_1C; //set to 0 in IPL
} SceLoadCoreBootModuleInfo;

//size 0x1C
typedef struct _SceLoadCoreProtectInfo
{
  int addr;
  int size;
  int attr;
  SceUID UID;
  int unk_10;
  int unk_14;
  int unk_18;
} SceLoadCoreProtectInfo;

//size 0x40
typedef struct _SceLoadCoreBootInfo
{
  int membase; //0 - membase
  int memsize; //4 - memsize
  int modules_loaded; //8 - number of modules loaded so far during boot
  int num_modules; //C - number of modules
  SceLoadCoreBootModuleInfo *module; //10 - list of modules
  int unk_14;
  int unk_18;
  int num_protects; //1c - number of protect infos
  SceLoadCoreProtectInfo *protect; //20 - list of protect infos
  int unk_24;
  int unk_28; //28
  int unk_2C;
  int unk_30; //30 - build version
  int unk_34;
  char *configfile; //38 - config file name (i.e. /kd/pspbtcnf.txt
  int unk_3C;
} SceLoadCoreBootInfo;

typedef struct _LoadCoreInfo
{
  SceLibraryEntry *unk_0; //list of libraries registered
  SceSysCallTable *syscalltable;
  unk_8;
  SceLibraryStubEntry *unk_10;
  SceModule *unk_14; //list of modules
  int unk_18; //number of modules
  SceUID unk_28; //heap
  int unk_2C; //heap inited?
} LoadCoreInfo;

#endif