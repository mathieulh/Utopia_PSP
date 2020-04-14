/*
	Modulemgr 1.50
	by Davee

	Used for Utopia
*/

#include <pspkernel.h>
#include <pspsysmem_kernel.h>
#include <pspthreadman_kernel.h>

#include <stdio.h>
#include <string.h>

#include "loadcore.h"

PSP_MODULE_INFO("sceModuleManager", 0x3007, 1, 2);

typedef struct
{
	SceUID exe_thid; //0
	SceUID sema_id;//4
	SceUID event_id; //8
	SceUID user_thread;//12
	SceUID *unk2;//16 (unused)
} ModuleMgrUIDStruct; //0x6550

/*typedef struct
{
	SceModule *	next; //0, 0x00
	u16 		attribute; //4, 0x04
	u8 			version[2]; //6, 0x06
	char 		modname[27]; //8, 0x08
	char 		terminal; //35, 0x23
	u16 		status; //36, 0x24 (AND 0x100 ? (usermodule) | (kernelmodule))
	u16 		padding; //38, 0x26
	u32 		unk_28; //40, 0x28
	SceUID 		modid; //44, 0x2C
	SceUID 		usermod_thid; //48, 0x30
	SceUID 		memid; //52, 0x34
	SceUID 		mpidtext; //56, 0x38
	SceUID 		mpiddata; //60, 0x3C
	void *		ent_top; //64, 0x40
	u32 		ent_size; //68, 0x44
	void *		stub_top; //72, 0x48
	u32 		stub_size; //76, 0x4C
	int 		(* module_start)(SceSize, void *); //80, 0x50
	int 		(* module_stop)(SceSize, void *); //84, 0x54
	int 		(* module_bootstart)(SceSize, void *); //88, 0x58
	int 		(* module_reboot_before)(SceSize, void *); //92, 0x5C
	int 		(* module_reboot_phase)(SceSize, void *); //96, 0x60
	u32 		entry_addr; //100, 0x64(seems to be repeated)
	u32 		gp_value; //104, 0x68
	u32 		text_addr; //108, 0x6C
	u32 		text_size; //112, 0x70
	u32 		data_size; //116, 0x74
	u32 		bss_size; //120, 0x78
	u8 			nsegment; //124, 0x7C
	u8			padding[3]; //125, 0x7D
	u32 		segmentaddr[4]; //128, 0x80
	u32 		segmentsize[4]; //144, 0x90
	int 		module_start_thread_priority; //160, 0xA0
	SceSize 	module_start_thread_stacksize; //164, 0xA4
	SceUInt 	module_start_thread_attr; //168, 0xA8
	int 		module_stop_thread_priority; //172, 0xAC
	SceSize 	module_stop_thread_stacksize; //176, 0xB0
	SceUInt 	module_stop_thread_attr; //180, 0xB4
	int 		module_reboot_before_thread_priority; //184, 0xB8
	SceSize 	module_reboot_before_thread_stacksize; //188, 0xBC
	SceUInt 	module_reboot_before_thread_attr; //192, 0xC0
} SceModule;*/

typedef struct
{
	u8 mode_start; //0 The Operation to start on, Use one of the ModuleMgrExeModes modes
	u8 mode_finish; //1 The Operation to finish on, Use one of the ModuleMgrExeModes modes
	u8 position; //2
	u8 access; //3
	u32 apitype; //4
	SceUID *return_id; //8
	SceModule *mod; //12
	SceUID mpidtext; //16
	SceUID mpiddata; //20
	SceUID mpidstack; //24
	SceSize stacksize; //28
	int priority; //32
	u32 attribute; //36
	SceUID modid; //40
	SceUID caller_modid; //44
	SceUID fd; //48
	SceLoadCoreExecFileInfo *exec_info; //52
	void *file_buffer; //56
	SceSize argsize; //60
	void *argp;
	u32 unk_68;
	u32 unk_72;
	int *status;
	u32 event_id; //80
	u32 unk_84; //84
	u32 unk_88; //88
} SceModuleMgrParam;

typedef struct
{
	char pbp_magic[4]; //0
	u32 pbp_version; //4
	u32 ofs_param; //8
	u32 ofs_icon0;//12
	u32 ofs_icon1;//16
	u32 ofs_pic0;//20
	u32 ofs_pic1;//24
	u32 ofs_snd0;//28
	u32 ofs_data;//32
	u32 ofs_psar; //36
} pbpFileStructure;

/*typedef struct SceKernelModuleInfo
{
	SceSize			size; //0
	char 			nsegment; //4
	char 			reserved[3]; //5
	u32 			segmentaddr[4]; //8
	u32 			segmentsize[4]; //24
	unsigned int    entry_addr; //40
	unsigned int    gp_value; //44
	unsigned int    text_addr;//48
	unsigned int    text_size;//52
	unsigned int    data_size;//56
	unsigned int    bss_size;//60
	unsigned short  attribute;//64
	unsigned char   version[2];//66
	char            name[27];
	char 			terminal;
} SceKernelModuleInfo;*/

enum ModuleMgrMcbStatus
{
	MCB_STATUS_NOT_LOADED, //0
	MCB_STATUS_LOADING, //1
	MCB_STATUS_LOADED, //2
	MCB_STATUS_RELOCATED, //3
	MCB_STATUS_STARTING, //4
	MCB_STATUS_STARTED, //5
	MCB_STATUS_STOPPING, //6
	MCB_STATUS_STOPPED, //7
	MCB_STATUS_UNLOADED, //8
};

enum ModuleMgrExeModes
{
	EXE_MODE_LOAD_MODULE, //0
	EXE_MODE_RELOCATE_MODULE, //1
	EXE_MODE_START_MODULE, //2
	EXE_MODE_STOP_MODULE, //3
	EXE_MODE_UNLOAD, //Unregister also?
};

enum ModuleMgrApiType
{
	MODULEMGR_API_LOADMODULE 			= 0x10,
	MODULEMGR_API_LOADMODULE_MS 		= 0x11,
	MODULEMGR_API_LOADMODULE_VSH 		= 0x20,
	MODULEMGR_API_LOADMODULE_USBWLAN	= 0x30,
};

enum ModuleMgrIoCtl
{
	SCE_IOCTL_PERMIT_LOADMODULE 	= 0x8001,
	SCE_IOCTL_PERMIT_LOADMODULE_MS 	= 0x8002,
	SCE_IOCTL_PERMIT_LOADMODULE_VSH	= 0x8003,
};

#define MCB_STATUS_USERMODE(v) (v & 0x100)
#define SET_MCB_STATUS(v, m) (v = (v & 0xFFF0) | m)

//GP
#define GET_GP(gp) asm volatile ("move %0, $gp\n" : "=r" (gp))
#define SET_GP(gp) asm volatile ("move $gp, %0\n" :: "r" (gp))

//RA
#define GET_RA(ra) asm volatile ("move %0, $ra\n" : "=r" (ra))

//K1 Checking Params
#define SHIFT_K1(k1_res) asm volatile ("move %0, $k1\n" "srl $k1, $k1, 16\n" : "=r" (k1_res))
#define RESTORE_K1(k1_res) asm volatile ("move $k1, %0\n" :: "r" (k1_res))

#define CHECK_K1_POINTER(p) (isUserK1() && ((int)p < 0))
#define CHECK_K1_POINTER_DEPANDANT(p) (!p || CHECK_K1_POINTER(p))
#define CHECK_K1_PARAMETER(p, psize) (p && (CHECK_K1_POINTER(((u32)p | ((u32)p + psize)))) < 0)

ModuleMgrUIDStruct modMgrCB;

/*
	Internal Funcs
*/
int UnloadModule(SceModule *mod);

static __inline__ int isUserK1()
{
	int umode;
	asm volatile ("andi %0, $k1, 0x18\n" : "=r" (umode));
	return umode;
}

static __inline__ int CheckMsHeader(pbpFileStructure *header, u32 *data_size)
{
	u32 data_ofs = 0;
	if (header->pbp_magic[0] == 0 && header->pbp_magic[1] == 'P' && header->pbp_magic[2] == 'B' && header->pbp_magic[3] == 'P')
	{
		data_ofs = header->ofs_data;
		u32 psar_ofs = header->ofs_psar;

		data_size[0] = psar_ofs - data_ofs;
		Kprintf("modulemgr.c:%s:MsHeader detected: 0x%08x - size 0x%08x [%d]\n", __FUNCTION__, data_ofs, psar_ofs - data_ofs, psar_ofs - data_ofs);
	}
	else
	{
		data_size[0] = 0;
	}

	return data_ofs;
}

static __inline__ int IsKernelOnlyModulePartition(u32 mpidtext)
{
	PspSysmemPartitionInfo partition_info;

	if (mpidtext)
	{
		partition_info.size = sizeof(PspSysmemPartitionInfo);
		int res = sceKernelQueryMemoryPartitionInfo(mpidtext, &partition_info);

		if (res < 0)
		{
			Kprintf("modulemgr.c:%s:%s failed 0x%08x [pid 0x%08x]\n", __FUNCTION__, "sceKernelQueryMemoryPartitionInfo", res, mpidtext);
			return res;
		}

		if (partition_info.attr != 0xC)
		{
			Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_ILLEGAL_PERM: 0x%08x for kernel only\n", __FUNCTION__, mpidtext);
			return SCE_KERNEL_ERROR_PARTITION_MISMATCH;
		}

		return mpidtext;
	}

	return 1;
}

static __inline__ int IsUserModulePartition(u32 mpidtext)
{
	PspSysmemPartitionInfo partition_info;

	if (mpidtext)
	{
		partition_info.size = sizeof(PspSysmemPartitionInfo);
		int ret = sceKernelQueryMemoryPartitionInfo(mpidtext, &partition_info);

		if (ret < 0)
		{
			Kprintf("modulemgr.c:%s:%s failed 0x%08x [pid 0x%08x]\n", __FUNCTION__, "sceKernelQueryMemoryPartitionInfo", ret, mpidtext);
			return ret;
		}

		else if (partition_info.attr & 0xFC)
		{
			Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_ILLEGAL_PERM: 0x%08x [for user]\n", __FUNCTION__, mpidtext);
			return SCE_KERNEL_ERROR_PARTITION_MISMATCH;
		}

		return mpidtext;
	}

	return 2;
}

static __inline__ int LockSema()
{
	int ret = sceKernelWaitSema(modMgrCB.sema_id, 1, NULL);

	if (ret < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelWaitSema failed 0x%08x\n", __FUNCTION__, ret);
	}

	return ret;
}

static __inline__ int UnlockSema()
{
	int ret = sceKernelSignalSema(modMgrCB.sema_id, 1);

	if (ret < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelSignalSema failed 0x%08x\n", __FUNCTION__, ret);
	}

	return ret;
}

int start_exe_thread(SceModuleMgrParam *params)
{
	SceUID return_id;

	if (sceKernelIsIntrContext())
	{
		Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_ILLEGAL_CONTEXT\n", __FUNCTION__);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	SceUID mythid = sceKernelGetThreadId();

	if (mythid < 0)
	{
		Kprintf("modulemgr.c:%s:mythid = 0x%08x\n", __FUNCTION__, mythid);
		return mythid;
	}

	if (modMgrCB.user_thread == mythid)
	{
		Kprintf("module managaer busy.\n");
		return SCE_KERNEL_ERROR_MODULE_MGR_BUSY;
	}

	params->return_id = &return_id;
	params->event_id = modMgrCB.event_id;

	int ret = LockSema();

	if (ret < 0)
	{
		Kprintf("modulemgr.c:%s:wresult=0x%08x for 0x%x\n", __FUNCTION__, ret, modMgrCB.sema_id);
		Kprintf("Assertion failed at %s:%s:%04d\n", __FILE__, __FUNCTION__, __LINE__);

		sceKernelAssert(0, 0);
		return ret;
	}

	ret = sceKernelStartThread(modMgrCB.exe_thid, sizeof(SceModuleMgrParam), params);

	if (ret >= 0)
	{
		sceKernelWaitEventFlag(params->event_id, 1, 0x11, 0, NULL);
	}
	else
	{
		Kprintf("modulemgr.c:%s:sceKernelStartThread failed: 0x%08x\n", __FUNCTION__, ret);
	}

	ret = UnlockSema();

	if (ret < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelSignalSema failed: 0x%08x\n", __FUNCTION__, ret);
	}

	return return_id;
}

int LoadModuleResolveOptions(SceModuleMgrParam *params, SceKernelLMOption *options)
{
	if (options)
	{
		params->mpidtext = options->mpidtext;
		params->mpiddata = options->mpiddata;
		params->position = options->position;
		params->access = options->access;
	}
	else
	{
		params->position = 0;
		params->access = 1;
		params->mpidtext = 0;
		params->mpiddata = 0;
	}

	params->exec_info = NULL;
	params->unk_68 = 0;
	params->unk_72 = 0;
	params->unk_88 = 0;

	return start_exe_thread(params);
}

int sceKernelStopModule(SceUID modid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
	u32 k1, ra;
	SceModuleMgrParam params;

	GET_RA(ra);
	SHIFT_K1(k1);

	if ((CHECK_K1_PARAMETER(argp, argsize)) || (CHECK_K1_PARAMETER(status, sizeof(int *))) || (CHECK_K1_PARAMETER(option, sizeof(SceKernelSMOption *)))) //Sony fail
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	SceModule *mod = sceKernelFindModuleByAddress(ra);

	if (!mod)
	{
		Kprintf("StopModule(): panic !!! call from unknown Module !!!\n");
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_CAN_NOT_STOP;
	}

	memset(&params, 0, sizeof(SceModuleMgrParam));

	params.mod = sceKernelFindModuleByUID(modid);

	if (params.mod && (params.mod->attribute & 0x1))
	{
		Kprintf("modulemgr.c:%s:module has SCE_MODULE_ATTR_CANT_STOP attribute\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_CAN_NOT_STOP;
	}

	params.mode_start = EXE_MODE_STOP_MODULE;
	params.mode_finish = EXE_MODE_STOP_MODULE;

	params.modid = modid;
	params.caller_modid = mod->modid;
	params.argp = argp;
	params.argsize = argsize;
	params.status = status;

	if (option)
	{
		params.mpidstack = option->mpidstack;
		params.stacksize = option->stacksize;
		params.priority = option->priority;
		params.attribute = option->attribute;
	}
	else
	{
		params.mpidstack = 0;
		params.stacksize = 0;
		params.priority = 0;
		params.attribute = 0;
	}

	int res = start_exe_thread(&params);

	RESTORE_K1(k1);
	return res;
}

int SelfStopUnloadModule(u32 ra, SceSize argsize, void *argp, int *status, SceKernelSMOption *option) //Silly sony doesn't use the option param >.<
{
	int status_2;
	SceModuleMgrParam params;
	SceModule *mod = sceKernelFindModuleByAddress(ra);

	if (!mod)
	{
		Kprintf("modulemgr.c:%s:call from unknown Module !!!\n", __FUNCTION__);
		Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_CAN_NOT_STOP\n", __FUNCTION__);
		return SCE_KERNEL_ERROR_CAN_NOT_STOP;
	}

	memset(&params, 0, sizeof(SceModuleMgrParam));

	params.mode_start = EXE_MODE_STOP_MODULE;
	params.mode_finish = EXE_MODE_UNLOAD;

	params.caller_modid = mod->modid;
	params.argp = argp;
	params.argsize = argsize;

	if (!status)
		params.status = &status_2;

	else
		params.status = status;

	//psst, Sony. I fixed your SelfStopUnload ;)
	/*if (option)
	{
		params.mpidstack = option->mpidstack;
		params.stacksize = option->stacksize;
		params.priority = option->priority;
		params.attribute = option->attribute;
	}
	else
	{
		params.mpidstack = 0;
		params.stacksize = 0;
		params.priority = 0;
		params.attribute = 0;
	} */

	int res = start_exe_thread(&params);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:Cannot stop/unload module 0x%08x\n", __FUNCTION__, res);
	}

	return res;
}

int sceKernelSelfStopUnloadModule(int unk, SceSize argsize, void *argp)
{
	u32 k1, ra;

	GET_RA(ra);
	SHIFT_K1(k1);

	if (CHECK_K1_PARAMETER(argp, argsize))
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1())
	{
		ra = sceKernelGetSyscallRA();
	}

	int res = SelfStopUnloadModule(ra, argsize, argp, NULL, NULL);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:SelfSTopUnloadModule failed: 0x%08x\n", __FUNCTION__, res);
	}

	RESTORE_K1(k1);
	return res;
}

int sceKernelStopUnloadSelfModule(SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
	u32 k1, ra;

	GET_RA(ra);
	SHIFT_K1(k1);

	if (CHECK_K1_PARAMETER(argp, argsize) || CHECK_K1_PARAMETER(status, sizeof(int *)) || CHECK_K1_PARAMETER(option, sizeof(SceKernelSMOption *))) //Sony fail
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1())
	{
		ra = sceKernelGetSyscallRA();
	}

	int res = SelfStopUnloadModule(ra, argsize, argp, status, option);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:SelfStopUnloadModule failed: 0x%08x\n", __FUNCTION__, res);
	}

	RESTORE_K1(k1);
	return res;
}

int sceKernelGetModuleId()
{
	u32 ra, k1;

	GET_RA(ra);
	SHIFT_K1(k1);

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1())
	{
		ra = sceKernelGetSyscallRA();
	}

	SceModule *mod = sceKernelFindModuleByAddress(ra);

	if (!mod)
	{
		Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_CAN_NOT_STOP\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ERROR;
	}

	u32 modid = mod->modid;
	RESTORE_K1(k1);
	return modid;
}

int sceKernelGetModuleIdList(SceUID *readbuf, int readbufsize, int *idcount)
{
	u32 k1, isusermode = 0;

	if (!readbuf || !idcount)
	{
		return SCE_KERNEL_ERROR_ILLEGAL_ADDRESS;
	}

	SHIFT_K1(k1);

	if (isUserK1())
	{
		isusermode = 1;

		if (CHECK_K1_PARAMETER(readbuf, readbufsize) || CHECK_K1_PARAMETER(idcount, sizeof(int *)))
		{
			RESTORE_K1(k1);
			return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
		}
	}

	int res = sceKernelGetModuleIdListForKernel(readbuf, readbufsize, idcount, isusermode);

	RESTORE_K1(k1);
	return res;
}

int sceKernelQueryModuleInfo(SceUID modid, SceKernelModuleInfo *minfo)
{
	int i;
	u32 k1;
	Kprintf("modulemgr.c:%s:(modid 0x%08x, pInfo 0x%08x), size=0x%08x\n", __FUNCTION__, modid, (int)minfo, minfo->size);

	SHIFT_K1(k1);

	if (CHECK_K1_PARAMETER(minfo, sizeof(SceKernelModuleInfo)))
	{
		Kprintf("modulemgr.c:%s:Illegal Address\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		Kprintf("modulemgr.c:%s:IntrContext\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	SceModule *mod = sceKernelFindModuleByUID(modid);

	if (!mod)
	{
		Kprintf("modulemgr.c:%s:Unknown module [uid 0x%08x]\n", __FUNCTION__, modid);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_UNKNOWN_MODULE;
	}

	if (isUserK1())
	{
		char *msg = NULL;

		if (mod->attribute & 0x800)
		{
			msg = "modulemgr.c:%s:VSH module\n";
		}

		else if (!MCB_STATUS_USERMODE(mod->status))
		{
			msg = "modulemgr.c:%s:kernel module\n";
		}

		else if ((mod->status & 0xF) != MCB_STATUS_RELOCATED)
		{
			msg = "modulemgr.c:%s:status mismatch\n";
		}

		else
		{
			Kprintf("modulemgr.c:%s:relocated\n", __FUNCTION__);
			goto skip_error_exit;
		}

		Kprintf(msg, __FUNCTION__);
		RESTORE_K1(k1);

		return SCE_KERNEL_ERROR_CANNOT_GET_MODULE_INFORMATION;
	}

skip_error_exit:
	minfo->nsegment = mod->nsegment;
	
	for (i = 0; i < 4; i++)
	{
		minfo->segmentaddr[i] = mod->segmentaddr[i];
		minfo->segmentsize[i] = mod->segmentsize[i];
	}

	minfo->entry_addr = mod->entry_addr;
	minfo->gp_value = mod->gp_value;
	minfo->text_addr = mod->text_addr;
	minfo->text_size = mod->text_size;
	minfo->data_size = mod->data_size;
	minfo->bss_size = mod->bss_size;

	if (minfo->size == sizeof(SceKernelModuleInfo)) //well, it was different in 1.00 xD (it didn't work though :| )
	{
		Kprintf("modulemgr.c:%s:new version\n", __FUNCTION__);

		minfo->attribute = mod->attribute;
		minfo->version[0] = mod->version[0];
		minfo->version[1] = mod->version[1];

		strncpy(minfo->name, mod->modname, 27);
	}

	else
	{
		Kprintf("modulemgr.c:%s:old version\n", __FUNCTION__);
	}

	RESTORE_K1(k1);
	return 0;
}

int sceKernelRebootBeforeForUser(void *arg)
{
	u32 prev_gp;
	char buffer[16];
	int mod_count = 0, i = 0;
	SceKernelThreadOptParam thread_opts;

	GET_GP(prev_gp);

	memcpy(buffer, arg, sizeof(buffer));
	SceUID block_id = sceKernelGetModuleListWithAlloc(&mod_count);

	if (block_id < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelGetModuleListWithAlloc failed 0x%08x\n", __FUNCTION__, block_id);
		return block_id;
	}

	SceUID *list = sceKernelGetBlockHeadAddr(block_id);

	for (i = mod_count - 1; i >= 0; i--)
	{
		SceModule *mod = sceKernelFindModuleByUID(list[i]);

		if (mod && ((int)mod->module_reboot_before != -1) && MCB_STATUS_USERMODE(mod->status))
		{
			int priority = mod->module_reboot_before_thread_priority;

			if (priority == -1)
				priority = 0x20;

			SceSize stacksize = mod->module_reboot_before_thread_stacksize;

			if (stacksize == -1)
				stacksize = 0x1000;

			SceUInt attr = mod->module_reboot_before_thread_attr;

			if (attr == -1)
				attr = PSP_THREAD_ATTR_USER;

			else
				attr |= PSP_THREAD_ATTR_USER;

			thread_opts.size = sizeof(SceKernelThreadOptParam);
			thread_opts.stackMpid = mod->mpiddata;

			SET_GP(mod->gp_value);
			mod->usermod_thid = sceKernelCreateThread("SceKernelModmgrUserRebootBefore", (void *)mod->module_reboot_before, priority, stacksize, attr, &thread_opts);
			SET_GP(prev_gp);

			if (sceKernelStartThread(mod->usermod_thid, 16, buffer) == 0)
			{
				sceKernelWaitThreadEnd(mod->usermod_thid, NULL);
			}

			sceKernelDeleteThread(mod->usermod_thid);
			mod->usermod_thid = -1;
		}
	}

	int res = sceKernelFreePartitionMemory(block_id);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelFreePartitionMemory for 0x%08x failed 0x%08x\n", __FUNCTION__, block_id, res);
		return res;
	}

	return 0;
}

int sceKernelRebootPhaseForKernel(SceSize argsize, void *argp)
{
	int mod_count, i;
	SceUID block_id = sceKernelGetModuleListWithAlloc(&mod_count);

	if (block_id < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelGetModuleListWithAlloc failed 0x%08x\n", __FUNCTION__, block_id);
		return block_id;
	}

	SceUID *list = sceKernelGetBlockHeadAddr(block_id);

	for (i = mod_count - 1; i >= 0; i--)
	{
		SceModule *mod = sceKernelFindModuleByUID(list[i]);

		if (mod)
		{
			if ((int)mod->module_reboot_phase != -1 && !MCB_STATUS_USERMODE(mod->status))
			{
				mod->module_reboot_phase(argsize, argp);
			}
		}
	}

	int res = sceKernelFreePartitionMemory(block_id);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelFreePartitionMemory for 0x%08x failed 0x%08x\n", __FUNCTION__, block_id, res);
		return res;
	}

	return 0;
}

int sceKernelRebootBeforeForKernel(void *argp)
{
	int mod_count, i;
	SceUID block_id = sceKernelGetModuleListWithAlloc(&mod_count);

	if (block_id < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelGetModuleListWithAlloc failed 0x%08x\n", __FUNCTION__, block_id);
		return block_id;
	}

	SceUID *list = sceKernelGetBlockHeadAddr(block_id);

	for (i = mod_count - 1; i >= 0; i--)
	{
		SceModule *mod = sceKernelFindModuleByUID(list[i]);

		if (mod)
		{
			if ((int)mod->module_reboot_before != -1 && !MCB_STATUS_USERMODE(mod->status))
			{
				mod->module_reboot_before(argp);
			}
		}
	}

	int res = sceKernelFreePartitionMemory(block_id);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelFreePartitionMemory for 0x%08x failed 0x%08x\n", __FUNCTION__, block_id, res);
		return res;
	}

	return 0;
}

SceUID sceKernelLoadModuleBufferVSH(u32 buffer_size, char *buffer, SceKernelLMOption *option)
{
	u32 k1;
	SceModuleMgrParam mod_params;
	memset(&mod_params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (CHECK_K1_PARAMETER(option, sizeof(SceKernelLMOption)) || CHECK_K1_PARAMETER(buffer, buffer_size))
	{
		Kprintf("modulemgr.c:%s:Overrange pointer and size\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1() && sceKernelGetUserLevel() != 4)
	{
		Kprintf("modulemgr.c:%s:This API for VSH only\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL;
	}

	mod_params.mode_start = EXE_MODE_LOAD_MODULE;
	mod_params.mode_finish = EXE_MODE_RELOCATE_MODULE;
	mod_params.apitype = 0x21;
	mod_params.fd = (SceUID)buffer;
	mod_params.file_buffer = (void *)buffer;

	int res = LoadModuleResolveOptions(&mod_params, option);

	RESTORE_K1(k1);
	return res;
}

SceUID sceKernelLoadModuleBufferUsbWlan(SceSize buffer_size, void *buffer, int flags, SceKernelLMOption *option)
{
	u32 k1;
	SceModuleMgrParam mod_params;
	memset(&mod_params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (CHECK_K1_PARAMETER(option, sizeof(SceKernelLMOption)) || CHECK_K1_PARAMETER(buffer, buffer_size))
	{
		Kprintf("modulemgr.c:%s:Overrange pointer and size\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1() && sceKernelGetUserLevel() != 2)
	{
		Kprintf("modulemgr.c:%s:This API for USBWLAN only\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL;
	}

	mod_params.mode_start = EXE_MODE_LOAD_MODULE;
	mod_params.mode_finish = EXE_MODE_RELOCATE_MODULE;
	mod_params.apitype = MODULEMGR_API_LOADMODULE_USBWLAN;
	mod_params.fd = (SceUID)buffer;
	mod_params.file_buffer = (void *)buffer;

	int res = LoadModuleResolveOptions(&mod_params, option);

	RESTORE_K1(k1);
	return res;
}

SceUID sceKernelLoadModuleVSHPlain(const char *file, int flags, SceKernelLMOption *option)
{
	u32 k1;
	SceModuleMgrParam mod_params;
	memset(&mod_params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (CHECK_K1_POINTER_DEPANDANT(file))
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (CHECK_K1_PARAMETER(option, sizeof(SceKernelLMOption)))
	{
		Kprintf("modulemgr.c:%s:Overrange pointer and size\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1() && sceKernelGetUserLevel() != 4)
	{
		Kprintf("modulemgr.c:%s:This API for VSH only\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL;
	}
	
	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 0511);
	
	if (fd < 0)
	{
		Kprintf("modulemgr.c:%s:open failed [%s]: 0x%08x\n", __FUNCTION__, file, fd);
		RESTORE_K1(k1);
		return fd;
	}
	
	int res = sceIoIoctl(fd, SCE_IOCTL_PERMIT_LOADMODULE_VSH, NULL, 0, NULL, 0);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:SCE_IOCTL_PERMIT_LOADMODULE_VSH - 0x%08x\n", __FUNCTION__, res);
		sceIoClose(fd);

		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
	}

	mod_params.mode_start = EXE_MODE_LOAD_MODULE;
	mod_params.mode_finish = EXE_MODE_RELOCATE_MODULE;
	mod_params.apitype = 0; //Plain module
	mod_params.fd = fd;
	mod_params.file_buffer = NULL;

	res = LoadModuleResolveOptions(&mod_params, option);
	sceIoClose(fd);
	RESTORE_K1(k1);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:LoadModuleByBufferID failed 0x%08x\n", __FUNCTION__, res);
	}

	return res;
}

SceUID sceKernelLoadModuleVSH(const char *file, int flags, SceKernelLMOption *option)
{
	u32 k1;
	SceModuleMgrParam mod_params;
	memset(&mod_params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (CHECK_K1_POINTER_DEPANDANT(file))
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (CHECK_K1_PARAMETER(option, sizeof(SceKernelLMOption)))
	{
		Kprintf("modulemgr.c:%s:Overrange pointer and size\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1() && sceKernelGetUserLevel() != 4)
	{
		Kprintf("modulemgr.c:%s:This API for VSH only\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL;
	}

	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 511);

	if (fd < 0)
	{
		Kprintf("modulemgr.c:%s:open failed [%s]: 0x%08x\n", __FUNCTION__, file, fd);
		RESTORE_K1(k1);
		return fd;
	}

	int res = sceIoIoctl(fd, SCE_IOCTL_PERMIT_LOADMODULE_VSH, NULL, 0, NULL, 0);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:SCE_IOCTL_PERMIT_LOADMODULE_VSH - 0x%08x\n", __FUNCTION__, res);
		sceIoClose(fd);

		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
	}

	mod_params.mode_start = EXE_MODE_LOAD_MODULE;
	mod_params.mode_finish = EXE_MODE_RELOCATE_MODULE;
	mod_params.apitype = MODULEMGR_API_LOADMODULE_VSH; //vsh apitype
	mod_params.fd = fd;
	mod_params.file_buffer = NULL;

	res = LoadModuleResolveOptions(&mod_params, option);
	sceIoClose(fd);
	RESTORE_K1(k1);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:LoadModuleByBufferID failed 0x%08x\n", __FUNCTION__, res);
	}

	return res;
}

SceUID sceKernelLoadModuleVSHByID(SceUID fd, int flags, SceKernelLMOption *option)
{
	u32 k1;
	SceModuleMgrParam mod_params;
	memset(&mod_params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (CHECK_K1_PARAMETER(option, sizeof(SceKernelLMOption)))
	{
		Kprintf("modulemgr.c:%s:Overrange pointer and size\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1())
	{
		if (sceKernelGetUserLevel() != 4)
		{
			Kprintf("modulemgr.c:%s:This API for VSH only (%d)\n", __FUNCTION__, sceKernelGetUserLevel());
			RESTORE_K1(k1);
			return SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL;
		}
	}

	int res = sceIoIoctl(fd, SCE_IOCTL_PERMIT_LOADMODULE_VSH, NULL, 0, NULL, 0);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:SCE_IOCTL_PERMIT_LOADMODULE_VSH - 0x%08x\n", __FUNCTION__, res);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
	}

	mod_params.mode_start = EXE_MODE_LOAD_MODULE;
	mod_params.mode_finish = EXE_MODE_RELOCATE_MODULE;
	mod_params.apitype = MODULEMGR_API_LOADMODULE_VSH;
	mod_params.fd = fd;
	mod_params.file_buffer = NULL;

	res = LoadModuleResolveOptions(&mod_params, option);
	RESTORE_K1(k1);
	return res;
}

int sceKernelStartModule(SceUID modid, SceSize argsize,  void *argp, int *status, SceKernelSMOption *options)
{
	u32 k1;
	SceModuleMgrParam params;
	memset(&params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (CHECK_K1_PARAMETER(argp, argsize) || CHECK_K1_PARAMETER(status, sizeof(int *)) || CHECK_K1_PARAMETER(options, sizeof(SceKernelSMOption)))
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	params.mode_start = EXE_MODE_START_MODULE;
	params.mode_finish = EXE_MODE_START_MODULE;
	params.modid = modid;
	params.argsize = argsize;
	params.argp = argp;
	params.status = status;

	if (options)
	{
		params.mpidstack = options->mpidstack;
		params.stacksize = options->stacksize;
		params.priority = options->priority;
		params.attribute = options->attribute;
	}
	else
	{
		params.mpidstack = 0;
		params.stacksize = 0;
		params.priority = 0;
		params.attribute = 0;
	}

	int res = start_exe_thread(&params);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:result = 0x%08x\n", __FUNCTION__, res);
	}

	RESTORE_K1(k1);
	return res;
}

SceUID sceKernelLoadModuleByID(SceUID fd, int flags, SceKernelLMOption *options) //user + kernel
{
	u32 k1;
	SceModuleMgrParam params;
	memset(&params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (CHECK_K1_PARAMETER(options, sizeof(SceKernelLMOption)))
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (isUserK1()) //check for user k1
	{
		params.apitype = MODULEMGR_API_LOADMODULE;
		int ret = sceIoIoctl(fd, SCE_IOCTL_PERMIT_LOADMODULE, NULL, 0, NULL, 0);

		if (ret < 0)
		{
			Kprintf("modulemgr.c:%s:SCE_IOCTL_PERMIT_LOADMODULE - 0x%08x\n", __FUNCTION__, ret);
			RESTORE_K1(k1);

			return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
		}
	}
	else
	{
		params.apitype = 0; // kernel api?
	}

	params.mode_start = EXE_MODE_LOAD_MODULE;
	params.mode_finish = EXE_MODE_RELOCATE_MODULE;
	params.fd = fd;
	params.file_buffer = NULL;

	int res = LoadModuleResolveOptions(&params, options);

	RESTORE_K1(k1);
	return res;
}

SceUID sceKernelLoadModuleBufferWithApitype(void *buffer, int flags, int apitype, SceKernelLMOption *options) //kernel
{
	u32 k1;
	SceModuleMgrParam params;
	memset(&params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (isUserK1())
	{
		Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL\n", __FUNCTION__);

		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL;
	}

	if (CHECK_K1_PARAMETER(options, sizeof(SceKernelLMOption)))
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	params.mode_start = EXE_MODE_RELOCATE_MODULE;
	params.mode_finish = EXE_MODE_RELOCATE_MODULE;
	params.apitype = apitype;
	params.fd = (SceUID)buffer;
	params.file_buffer = buffer;

	int res = LoadModuleResolveOptions(&params, options);

	RESTORE_K1(k1);
	return res;
}

int LoadDeviceCheck(SceUID fd, u32 apitype)
{
	int res;
	
	if (apitype == MODULEMGR_API_LOADMODULE && isUserK1())
	{
		res = sceIoIoctl(fd, SCE_IOCTL_PERMIT_LOADMODULE, NULL, 0, NULL, 0);

		if (res < 0)
		{
			Kprintf("modulemgr.c:%s:WARNING: %s(0x%08x) failed 0x%08x, 0x%08x\n", __FUNCTION__, "SCE_IOCTL_PERMIT_LOADMODULE", SCE_IOCTL_PERMIT_LOADMODULE, res, MODULEMGR_API_LOADMODULE);
			return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
		}
	}

	else if (apitype == MODULEMGR_API_LOADMODULE_MS)
	{
		res = sceIoIoctl(fd, SCE_IOCTL_PERMIT_LOADMODULE_MS, NULL, 0, NULL, 0);

		if (res < 0)
		{
			Kprintf("modulemgr.c:%s:WARNING: %s(0x%08x) failed 0x%08x, 0x%08x\n", __FUNCTION__, "SCE_IOCTL_PERMIT_LOADMODULE_MS", SCE_IOCTL_PERMIT_LOADMODULE_MS, res, MODULEMGR_API_LOADMODULE_MS);
			return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
		}
	}

	else if (apitype == 0x110)
	{
		res = sceIoIoctl(fd, 0x8010, NULL, 0, NULL, 0);

		if (res < 0)
		{
			Kprintf("modulemgr.c:%s:IOCTL_PERMIT(0x%08x) failed 0x%08x, 0x%08x\n", __FUNCTION__, 0x8010, res, 0x110);
			return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
		}
	}

	else if (apitype == 0x120 || apitype == 0x121 || apitype == 0x122)
	{
		res = sceIoIoctl(fd, 0x8012, NULL, 0, NULL, 0);

		if (res < 0)
		{
			Kprintf("modulemgr.c:%s:SCE_IOCTL_PERMIT_XXX(0x%08x) failed 0x%08x, 0x%08x\n", __FUNCTION__, 0x8012, res, apitype);
			return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
		}
	}

	else if (apitype == 0x140 || apitype == 0x141 || apitype == 0x142)
	{
		res = sceIoIoctl(fd, 0x8013, NULL, 0, NULL, 0);

		if (res < 0)
		{
			Kprintf("modulemgr.c:%s:SCE_IOCTL_PERMIT_XXX(0x%08x) failed 0x%08x, 0x%08x\n", __FUNCTION__, 0x8013, res, apitype);
			return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
		}
	}

	else if (apitype == MODULEMGR_API_LOADMODULE_VSH)
	{
		res = sceIoIoctl(fd, SCE_IOCTL_PERMIT_LOADMODULE_VSH, NULL, 0, NULL, 0);

		if (res < 0)
		{
			Kprintf("modulemgr.c:%s:SCE_IOCTL_PERMIT_XXX(0x%08x) failed 0x%08x, 0x%08x\n", __FUNCTION__, SCE_IOCTL_PERMIT_LOADMODULE_VSH, res, MODULEMGR_API_LOADMODULE_VSH);
			return SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE;
		}
	}

	else if (apitype < 0x130 && apitype != 0x30 && apitype != 0x50 && apitype != 0)
	{
		Kprintf("modulemgr.c:%s:Unknown apitype 0x%08x\n", __FUNCTION__, apitype);
		return SCE_KERNEL_ERROR_ERROR;
	}

	return 0;
}

SceUID sceKernelLoadModuleWithApitype(const char *file, int flags, u32 apitype, SceKernelLMOption *option) //kernel
{
	u32 k1;
	SceModuleMgrParam mod_params;
	memset(&mod_params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (CHECK_K1_POINTER_DEPANDANT(file) || CHECK_K1_PARAMETER(option, sizeof(SceKernelLMOption)))
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 511);

	if (fd < 0)
	{
		Kprintf("modulemgr.c:%s:open failed [%s] apitype 0x%08x\n", __FUNCTION__, file, apitype);
		RESTORE_K1(k1);
		return fd;
	}

	mod_params.apitype = apitype;

	if (apitype != 0 && apitype != MODULEMGR_API_LOADMODULE)
	{
		if (isUserK1())
		{
			mod_params.apitype = MODULEMGR_API_LOADMODULE;
		}
		else
		{
			mod_params.apitype = 0; //kmode??
		}
	}

	int res = LoadDeviceCheck(fd, apitype);

	if (res < 0)
	{
		sceIoClose(fd);
		RESTORE_K1(k1);
		return res;
	}

	mod_params.mode_start = EXE_MODE_LOAD_MODULE;
	mod_params.mode_finish = EXE_MODE_RELOCATE_MODULE;
	mod_params.fd = fd;
	mod_params.file_buffer = NULL;

	res = LoadModuleResolveOptions(&mod_params, option);
	sceIoClose(fd);
	RESTORE_K1(k1);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelLoadModule failed 0x%08x\n", __FUNCTION__, res);
	}

	return res;
}

SceUID sceKernelLoadModuleBuffer(void *buffer, SceSize bufsize, int flags, SceKernelLMOption *option) //kernel
{
	return sceKernelLoadModuleBufferWithApitype(buffer, bufsize, 0, option);
}

SceUID sceKernelLoadModule(const char *file, int flags, SceKernelLMOption *option) //user + kernel
{
	return sceKernelLoadModuleWithApitype(file, flags, MODULEMGR_API_LOADMODULE, option);
}

SceUID sceKernelLoadModuleMs(const char *file, int flags, SceKernelLMOption *option) //user + kernel
{
	return sceKernelLoadModuleWithApitype(file, flags, MODULEMGR_API_LOADMODULE_MS, option);
}

int sceKernelUnloadModule(SceUID modid) //user + kernel
{
	u32 k1;
	SceModuleMgrParam params;
	memset(&params, 0, sizeof(SceModuleMgrParam));

	SHIFT_K1(k1);

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	params.mode_start = EXE_MODE_UNLOAD;
	params.mode_finish = EXE_MODE_UNLOAD;
	params.modid = modid;
	params.argsize = 0;
	params.argp = 0;
	params.status = 0;

	int res = start_exe_thread(&params);

	RESTORE_K1(k1);
	return res;
}

SceUID sceKernelSearchModuleByName(const char *modname) //kernel
{
	u32 k1;
	SHIFT_K1(k1);

	if (CHECK_K1_POINTER(modname))
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	SceModule *mod = sceKernelFindModuleByName(modname);

	if (!mod)
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_UNKNOWN_MODULE;
	}

	RESTORE_K1(k1);
	return mod->modid;
}

SceUID sceKernelSearchModuleByAddress(u32 address) //kernel
{
	SceModule *mod = sceKernelFindModuleByAddress(address);

	if (!mod)
		return SCE_KERNEL_ERROR_UNKNOWN_MODULE;

	return mod->modid;
}

SceUID sceKernelGetModuleIdByAddress(int address) //user + kernel
{
	u32 k1;
	SHIFT_K1(k1);

	if (sceKernelIsIntrContext())
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_CONTEXT;
	}

	if (CHECK_K1_POINTER(address)) //lame check
	{
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
	}

	SceModule *mod = sceKernelFindModuleByAddress(address);

	if (!mod)
	{
		Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_CAN_NOT_STOP\n", __FUNCTION__);
		RESTORE_K1(k1);
		return SCE_KERNEL_ERROR_ERROR;
	}

	RESTORE_K1(k1);
	return mod->modid;
}

int ReadFile(SceUID fd, char *buffer, SceSize len)
{
	u32 copy = 0xF000;
	int read = sceIoRead(fd, buffer, 512);

	if (read < 0)
	{
		Kprintf("modulemgr.c:%s:sceIoRead(512) failed: 0x%08x\n", __FUNCTION__, read);
		return read;
	}

	u32 data_size;
	u32 data_ofs = CheckMsHeader((pbpFileStructure *)buffer, &data_size);

	sceIoLseek(fd, data_ofs, PSP_SEEK_SET);

	if (data_ofs)
	{
		if (len < data_size)
		{
			Kprintf("modulemgr.c:%s:filesize mismatch size 0x%08x <- max 0x%08x\n", __FUNCTION__, len, data_size);
		}
	}

	read = 0;
	while (len > 0)
	{
		if (len < copy)
		{
			copy = len;
		}

		int tmp_read = sceIoRead(fd, buffer + read, copy);

		if (tmp_read < 0)
		{
			Kprintf("modulemgr.c:%s:sceIoRead(0x%x) failed: 0x%08x\n", __FUNCTION__, copy, tmp_read);
			return tmp_read;
		}

		len -= tmp_read;
		read += tmp_read;

		if (tmp_read < copy && len)
		{
			return SCE_KERNEL_ERROR_ERROR;
		}
	}

	return read;
}

int LoadModule(SceModuleMgrParam *mod_params)
{
	int res = 0;
	SceUID enc_buffer_id = 0;
	SceModule *mod = mod_params->mod;
	SceLoadCoreExecFileInfo *exec_info;

	if (!mod || (mod->status & 0xF) != MCB_STATUS_NOT_LOADED)
	{
		return SCE_KERNEL_ERROR_ERROR;
	}

	exec_info = mod_params->exec_info;
	SET_MCB_STATUS(mod->status, MCB_STATUS_LOADING); //Update status

	sceKernelMemset(exec_info, 0, sizeof(SceLoadCoreExecFileInfo));

	SceUID fd = mod_params->fd;
	exec_info->unk_8 = mod_params->apitype;

	SceUID mod_temp_id = sceKernelAllocPartitionMemory(1, "SceModmgrLoadModuleTmp", PSP_SMEM_High, 512, NULL);

	if (mod_temp_id < 0)
	{
		Kprintf("modulemgr.c:%s:sceKernelAllocPartitionMemory failed: 0x%08x\n", __FUNCTION__, mod_temp_id);
		return mod_temp_id;
	}

	char *temp_buffer = sceKernelGetBlockHeadAddr(mod_temp_id);

	int read = sceIoRead(fd, temp_buffer, 512);
	if (read < 0)
	{
		Kprintf("modulemgr.c:%s:sceIoRead failed: 0x%08x\n", __FUNCTION__, read);
		sceKernelFreePartitionMemory(mod_temp_id);
		return SCE_KERNEL_ERROR_FILEERR;
	}

	u32 data_size;
	u32 data_ofs = CheckMsHeader((pbpFileStructure *)temp_buffer, &data_size);

	if (data_ofs)
	{
		Kprintf("modulemgr.c:%s:MS skip 0x%08x\n", __FUNCTION__, data_ofs);

		sceIoLseek(fd, data_ofs, PSP_SEEK_SET);
		read = sceIoRead(fd, temp_buffer, 512);

		if (read < 0)
		{
			Kprintf("modulemgr.c:%s:sceIoRead failed: 0x%08x\n", __FUNCTION__, read);
			sceKernelFreePartitionMemory(mod_temp_id);
			sceIoClose(fd);
			return SCE_KERNEL_ERROR_FILEERR;
		}
	}

	sceIoLseek(fd, 0, PSP_SEEK_SET);
	exec_info->unk_4 = 1;

	sceKernelCheckExecFile(temp_buffer, exec_info);

	if (exec_info->unk_10 == 0)
	{
		exec_info->unk_10 = sceIoLseek(fd, 0, PSP_SEEK_END); /* size of executeable? */
		sceIoLseek(fd, 0, PSP_SEEK_SET);
	}

	if (exec_info->elf_type == 0)
	{
		Kprintf("modulemgr.c:%s:warning backup code\n", __FUNCTION__);
		Kprintf("modulemgr.c:%s:OTYPE_UNKNOWN\n", __FUNCTION__);

		enc_buffer_id = sceKernelAllocPartitionMemory(1, "SceModmgrLoadModuleFileBufferEnc", PSP_SMEM_High, exec_info->unk_10, NULL);

		if (enc_buffer_id < 0)
		{
			Kprintf("modulemgr.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, enc_buffer_id);
			res = enc_buffer_id;
			goto clear_mem_exit;
		}

		char *exec_buffer = sceKernelGetBlockHeadAddr(enc_buffer_id);
		res = ReadFile(fd, exec_buffer, exec_info->unk_10);

		if (res < 0)
		{
			Kprintf("modulemgr.c:%s:ReadFile failed 0x%08x\n", __FUNCTION__, res);
			goto clear_mem_exit;
		}

		exec_info->unk_4 = 0;
		sceKernelCheckExecFile(exec_buffer, exec_info);

		exec_info->unk_54 = 0;
	}
	else
	{
		if (exec_info->unk_5A & 1) //gzip
			exec_info->unk_54 = 1;

		else
			exec_info->unk_54 = 0;
	}

	if (exec_info->unk_44 == 1)
	{
		u32 mpid;
		if ((mpid = IsUserModulePartition(mod_params->mpidtext)) < 0)
		{
			Kprintf("modulemgr.c:%s:pid [0x%08x] is not for user\n", __FUNCTION__, mod_params->mpidtext);
			res = SCE_KERNEL_ERROR_PARTITION_MISMATCH;

			goto clear_mem_exit;
		}

		exec_info->unk_40 = mpid;
	}

	else
	{
		u32 mpid;
		if ((mpid = IsKernelOnlyModulePartition(mod_params->mpidtext)) < 0)
		{
			Kprintf("modulemgr.c:%s:pid [0x%08x] is not for kernel\n", __FUNCTION__, mod_params->mpidtext);
			res = SCE_KERNEL_ERROR_PARTITION_MISMATCH;

			goto clear_mem_exit;
		}

		exec_info->unk_40 = mpid;
	}

	if (exec_info->unk_5A & 0x1) //GZIP?
	{
		if (exec_info->unk_5C < exec_info->unk_30)
		{
			exec_info->unk_14 = exec_info->unk_30;
		}

		else
		{
			exec_info->unk_14 = exec_info->unk_5C;
		}
	}

	else
	{
		if (exec_info->unk_10 < exec_info->unk_30)
		{
			exec_info->unk_14 = exec_info->unk_10;
		}

		else
		{
			exec_info->unk_14 = exec_info->unk_30;
		}
	}

	int mode;
	void *addr;
	char *mem_name;

	switch (exec_info->elf_type + 1)
	{
		case 0:
		{
			Kprintf("modulemgr.c:%s:OTYPE_NOOBJ\n", __FUNCTION__); //<3 Sony
			res = SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
			goto clear_mem_exit;
		}

		case 2:
		{
			mode = PSP_SMEM_Low;
			addr = NULL;
			mem_name = "SceModmgrLoadModuleFileBufferPRX";
			break;
		}

		case 4:
		{
			mode = PSP_SMEM_Addr;
			addr = (void *)exec_info->topaddr;
			mem_name = "LoadModuleFileBufferELF";
			break;
		}

		default:
		{
			res = SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE;
			goto clear_mem_exit;
		}
	}

	exec_info->unk_18 = sceKernelAllocPartitionMemory(exec_info->unk_40, mem_name, mode, exec_info->unk_14, addr);

	if (exec_info->unk_18 < 0)
	{
		res = exec_info->unk_18;
		goto clear_mem_exit;
	}

	exec_info->unk_1C = (u32)sceKernelGetBlockHeadAddr(exec_info->unk_18);
	exec_info->unk_14 += 0xFF;

	if (mod_temp_id > 0)
	{
		sceKernelFreePartitionMemory(mod_temp_id);
	}
	
	char *gzip_buffer = NULL;
	if (exec_info->unk_5A & 0x1) //AHH! Gzip!
	{
		exec_info->topaddr = exec_info->unk_1C;
		SceUID gzip_id = sceKernelAllocPartitionMemory(exec_info->unk_40, "SceModmgrGzipBuffer", PSP_SMEM_High, exec_info->unk_10, NULL);

		if (gzip_id < 0)
		{
			Kprintf("modulemgr.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, gzip_id);
			res = gzip_id;
			goto clear_mem_exit;
		}

		gzip_buffer = sceKernelGetBlockHeadAddr(gzip_id);

		ReadFile(fd, gzip_buffer, exec_info->unk_10);
		res = ReadFile(fd, gzip_buffer, exec_info->unk_10); //lol Sony...

		if (res < 0)
		{
			goto clear_mem_exit;
		}

		exec_info->unk_4 = 0;
		sceKernelCheckExecFile(gzip_buffer, exec_info);
		exec_info->unk_54 = 1;

		if (gzip_id) //SONY?! (should be gzip_id >= 0, hell the check isn't even needed. If gzip_id is invalid it wouldn't be here...)
		{
			sceKernelFreePartitionMemory(gzip_id);
		}
	}

	else if (exec_info->unk_54 == 1)
	{
		res = ReadFile(fd, (void *)exec_info->unk_1C, exec_info->unk_10);

		if (res < 0)
		{
			goto clear_mem_exit;
		}

		exec_info->unk_4 = 0;
	}

	else
	{
		Kprintf("modulemgr.c:%s:WARNING try to copy file\n", __FUNCTION__);

		sceKernelMemmove((void *)exec_info->unk_1C, gzip_buffer, exec_info->unk_10); //Why not just give up... srsly
		sceKernelMemset(gzip_buffer, 0, exec_info->unk_10);
	}

	if (enc_buffer_id)
	{
		sceKernelFreePartitionMemory(enc_buffer_id);
	}

	SET_MCB_STATUS(mod->status, MCB_STATUS_LOADED);
	sceKernelRegisterModule(mod);
	return 0;

clear_mem_exit:

	if (mod_temp_id > 0)
		sceKernelFreePartitionMemory(mod_temp_id);

	if (enc_buffer_id > 0)
		sceKernelFreePartitionMemory(enc_buffer_id);

	if (exec_info && exec_info->unk_18 > 0)
	{
		sceKernelFreePartitionMemory(exec_info->unk_18);
	}

	return res;
}

int ModuleReleaseLibraries(SceModule *mod)
{
	u32 ent_top = (u32)mod->ent_top;
	u32 ent_size = mod->ent_size;

	while (ent_size != 0 && (ent_top < ent_size))
	{
		SceLibraryEntryTable *ent = (SceLibraryEntryTable *)ent_top;

		if (ent->attribute >= 0)
		{
			int res = sceKernelReleaseLibrary(ent);

			if (res < 0)
			{
				Kprintf("modulemgr.c:%s:sceKernelReleaseLibrary failed 0x%x\n", __FUNCTION__, res);
			}
		}

		ent_top += ent->len << 2; //ent->len * 4
	}

	return 0;
}

int EpilogueModule(SceModule *mod)
{
	u32 ent_size = mod->ent_size;
	u32 ent_top = (u32)mod->ent_top;
	u32 res = 0;

	while (ent_size > 0)
	{
		SceLibraryEntryTable *ent = (SceLibraryEntryTable *)ent_top;

		if (ent->attribute >= 0)
		{
			res = sceKernelCanReleaseLibrary(ent);

			if (res)
			{
				Kprintf("modulemgr.c:%s:sceKernelCanReleaseLibrary failed 0x%x\n", __FUNCTION__, res);
				goto exit;
			}
		}

		ent_top += sizeof(SceLibraryEntryTable);
		ent_size -= sizeof(SceLibraryEntryTable);
	}

	if ((int)mod->stub_top != -1)
	{
		sceKernelUnLinkLibraryEntries(mod->stub_top, mod->stub_size);
	}

	ModuleReleaseLibraries(mod);

exit:
	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:finish with error: 0x%08x\n", __FUNCTION__, res);
	}

	return res;
}

int ProcessModuleExportEnt(SceModule *mod, SceLibraryEntryTable *entry)
{
	int i;
	u32 *entry_table = entry->entrytable;

	for (i = 0; i < entry->stubcount; i++)
	{
		u32 nid = entry_table[i];
		
		if (nid == 0xADF12745) //reboot_phase
		{
			mod->module_reboot_phase = (void *)entry_table[entry->stubcount + entry->vstubcount + i];
		}

		else if (nid == 0xD3744BE0)
		{
			mod->module_bootstart = (void *)entry_table[entry->stubcount + entry->vstubcount + i];
		}

		else if (nid == 0xD632ACDB)
		{
			mod->module_start = (void *)entry_table[entry->stubcount + entry->vstubcount + i];
		}

		else if (nid == 0xCEE8593C)
		{
			mod->module_stop = (void *)entry_table[entry->stubcount + entry->vstubcount + i];
		}

		else if (nid == 0x2F064FA6)
		{
			mod->module_reboot_before = (void *)entry_table[entry->stubcount + entry->vstubcount + i];
		}

		else if (nid != 0x592743D8 && nid == 0x900DADE1)
		{
			Kprintf("modulemgr.c:%s:Unknown NID 0x%08x\n", __FUNCTION__, entry_table[i]);
		}
	}

	entry_table += entry->stubcount;

	for (i = 0; i < entry->vstubcount; i++)
	{
		u32 nid = entry_table[i];
		if (nid == 0xCF0CC697)
		{
			//bug? it does entry->stubcount twice ($v1 = entry->stubcount)
			/*
				0x0000446C: 0x006C5821 '!Xl.' - addu       $t3, $v1, $t4
				0x00004470: 0x01632021 '! c.' - addu       $a0, $t3, $v1
			*/

			u32 *var_ptr = (u32 *)entry_table[entry->stubcount + entry->vstubcount + i];

			mod->module_stop_thread_priority = var_ptr[1];
			mod->module_stop_thread_stacksize = var_ptr[2];
			mod->module_stop_thread_attr = var_ptr[3];
		}

		else if (nid == 0x0F7C276C)
		{
			u32 *var_ptr = (u32 *)entry_table[entry->stubcount + entry->vstubcount + i];

			mod->module_start_thread_priority = var_ptr[1];
			mod->module_start_thread_stacksize = var_ptr[2];
			mod->module_start_thread_attr = var_ptr[3];
		}

		else if (nid == 0xF4F4299D)
		{
			u32 *var_ptr = (u32 *)entry_table[entry->stubcount + entry->vstubcount + i];

			mod->module_reboot_before_thread_priority = var_ptr[1];
			mod->module_reboot_before_thread_stacksize = var_ptr[2];
			mod->module_reboot_before_thread_attr = var_ptr[3];
		}

		else if (nid != 0xF01D73A7)
		{
			Kprintf("modulemgr.c:%s:Unknown NID 0x%08x\n", __FUNCTION__, entry_table[i]);
		}
	}

	return 0;
}

int ModuleRegisterLibraries(SceModule *mod)
{
	int res;
	u32 ent_top = (u32)mod->ent_top;
	u32 ent_size = mod->ent_size;

	while (ent_size != 0 && (ent_top != ent_size))
	{	
		SceLibraryEntryTable *entry = (SceLibraryEntryTable *)ent_top;
		int attr = entry->attribute;
		
		if (attr & 1)
		{
			if (MCB_STATUS_USERMODE(mod->status))
			{
				res = sceKernelRegisterLibraryForUser(entry);
			}
			else
			{
				res = sceKernelRegisterLibrary(entry);
			}

			if (res < 0)
			{
				Kprintf("modulemgr.c:%s:sceKernelRegisterLibrary failed 0x%x\n", __FUNCTION__, res);

				if ((u32)mod->ent_top >= ent_top)
					goto error_exit;

				goto unregister_libraries;
			}
		}
		else
		{
			if (attr < 0)
			{
				ProcessModuleExportEnt(mod, entry);
			}
			else
			{
				Kprintf("modulemgr.c:%s:don't auto export library [%s]: 0x%x, size 0x%x\n", __FUNCTION__, entry->libname, entry->attribute, entry->len);
			}
		}

		ent_top += (entry->len << 2); //2^2 = 4
	}

	return 0;

unregister_libraries:
	ent_top = (u32)mod->ent_top;

	while ((ent_top != ent_size))
	{
		SceLibraryEntryTable *entry = (SceLibraryEntryTable *)ent_top;

		if (entry->attribute & 1)
		{
			sceKernelReleaseLibrary(entry);
		}

		ent_top += (entry->len << 2); //2^2 = 4
	}

error_exit:
	Kprintf("modulemgr.c:%s:failed with error 0x%08x\n", __FUNCTION__, res);
	return res;
}

int PrologueModule(SceModuleMgrParam *mod_params, SceModule *mod)
{
	u32 gp_reg;
	GET_GP(gp_reg);

	int res = ModuleRegisterLibraries(mod);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:ModuleRegisterLibraries failed: 0x%08x\n", __FUNCTION__, res);
		//loc_00003D64
	}

	else if ((int)mod->stub_top != -1)
	{
		if (MCB_STATUS_USERMODE(mod->status))
		{
			res = sceKernelLinkLibraryEntriesForUser(mod->stub_top, mod->stub_size);
		}
		else
		{
			res = sceKernelLinkLibraryEntries(mod->stub_top, mod->stub_size);
		}

		if (res >= 0)
		{
			int priority;
			u32 module_entry = (u32)mod->module_start;

			if (module_entry == -1)
			{
				if ((u32)mod->module_bootstart == -1)
					module_entry = mod->entry_addr; //Default entry?

				else
					module_entry = (u32)mod->module_bootstart;
			}

			if (!~module_entry || !module_entry)
			{
				Kprintf("modulemgr.c:%s:There is no start entry. Cannot create start thread.\n", __FUNCTION__);
				mod->usermod_thid = -1;
				return 0;
			}

			priority = mod_params->priority;
			if (!priority)
			{
				if (mod->module_start_thread_priority != -1)
					priority = mod->module_start_thread_priority;

				else
					priority = 32;
			}

			u32 stacksize = mod_params->stacksize;
			if (!stacksize)
			{
				if (mod->module_start_thread_stacksize != -1)
				{
					stacksize = mod->module_start_thread_stacksize;
				}
			}

			u32 attribute = mod_params->attribute;
			if (!attribute)
			{
				if (mod->module_start_thread_attr != -1)
				{
					attribute = mod->module_start_thread_attr;
				}
			}
			
			char *thread_name;
			if (MCB_STATUS_USERMODE(mod->status))
			{
				if (!stacksize)
				{
					stacksize = 0x40000;
				}

				u32 mod_attr = mod->attribute;

				if (mod_attr & 0x800) //0x800
				{
					thread_name = "SceModmgrStartVSH";
					attribute |= PSP_THREAD_ATTR_VSH;
				}

				else if (mod_attr & 0x400) //0x400
				{
					thread_name = "SceModmgrStartUsbWlan";
					attribute |= PSP_THREAD_ATTR_USBWLAN;
				}

				else
				{
					thread_name = "SceModmgrStartUser";
					attribute |= PSP_THREAD_ATTR_USER;
				}
			}
			else
			{
				thread_name = "SceModmgrStartKernel";

				if (!stacksize)
				{
					stacksize = 0x1000;
				}
			}

			SceKernelThreadOptParam option;
			option.size = sizeof(SceKernelThreadOptParam);

			if (mod_params->mpidstack)
			{
				option.stackMpid = mod_params->mpidstack;
			}
			else
			{
				option.stackMpid = mod->mpiddata; //data?
			}


			SET_GP(mod->gp_value);
			mod->usermod_thid = sceKernelCreateThread(thread_name, (void *)module_entry, priority, stacksize, attribute, &option);
			SET_GP(gp_reg);

			if (mod->usermod_thid < 0)
			{
				Kprintf("modulemgr.c:%s:sceKernelCreateThread failed 0x%08x\n", __FUNCTION__, mod->usermod_thid);
				return mod->usermod_thid;
			}

			return 0;
		}
		else
		{
			Kprintf("modulemgr.c:%s:sceKernelLinkLibraryEntries, res=0x%08x\n", __FUNCTION__, res);
			ModuleReleaseLibraries(mod);
		}
	}

	Kprintf("modulemgr.c:%s:res=0x%08x\n", __FUNCTION__, res);
	return res;
}

static __inline__ void shrink_file_block(SceLoadCoreExecFileInfo *exec_info, SceModule *mod)
{
	if (!exec_info->unk_54)
	{
		sceKernelFreePartitionMemory(exec_info->unk_18);

		if (exec_info->unk_A8)
			mod->memid = exec_info->unk_A8;

		exec_info->unk_18 = mod->memid;
	}
	else
	{
		int ret = sceKernelResizeMemoryBlock(exec_info->unk_18, 0, exec_info->unk_30 - exec_info->unk_14);

		if (ret < 0)
		{
			Kprintf("modulemgr.c:%s:sceKernelResizeMemoryBlock failed 0x%x, block 0x%08x\n", __FUNCTION__, ret, exec_info->unk_18);
		}
		else
		{
			exec_info->unk_14 = exec_info->unk_30;
		}
	}
}

int allocate_module_block(SceModuleMgrParam *mod_params)
{
	SceModule *mod = mod_params->mod;
	SceLoadCoreExecFileInfo *exec_info = mod_params->exec_info;

	int res = sceKernelProbeExecutableObject((void *)exec_info->unk_1C, exec_info);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:ProbeExecutableObject failed: 0x%08x\n", __FUNCTION__, res);
		return res;
	}

	switch (exec_info->elf_type + 1)
	{
		case 2:
		{
			if (exec_info->topaddr >= 1)
			{
				Kprintf("Assertion failed at %s:%s:%04d\n", __FILE__, __FUNCTION__, __LINE__);
				sceKernelAssert(0, 0);

				break;
			}

			if (exec_info->unk_58 & 0x1000)
			{
				u32 mpid;
				if ((mpid = IsKernelOnlyModulePartition(exec_info->unk_40)) < 0)
				{
					res = SCE_KERNEL_ERROR_ILLEGAL_PERM;
					Kprintf("modulemgr.c:%s:IsKernelOnlyModulePartition failed 0x%08x\n", __FUNCTION__, res);
					break;
				}
			}
			else
			{
				u32 mpid;
				if ((mpid = IsUserModulePartition(exec_info->unk_40)) < 0)
				{
					res = SCE_KERNEL_ERROR_PARTITION_MISMATCH;
					Kprintf("modulemgr.c:%s:IsUserModulePartition failed 0x%08x\n", __FUNCTION__, res);
					break;
				}
			}

			res = 0;

			if (exec_info->unk_54 != 1 || mod_params->position != 2)
			{
				exec_info->topaddr = exec_info->unk_1C;
				mod->memid = exec_info->unk_18;
			}
			else
			{
				SceUID block_id = sceKernelAllocPartitionMemory(exec_info->unk_40, "SceModmgrModuleBlockAuto", mod_params->position, exec_info->unk_30, NULL);

				if (block_id < 0)
				{
					Kprintf("modulemgr.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, block_id);
					res = block_id;
					break;
				}

				exec_info->topaddr = (u32)sceKernelGetBlockHeadAddr(block_id);
				mod->memid = block_id;
			}

			mod->mpidtext = exec_info->unk_40;
			mod->mpiddata = exec_info->unk_40;

			if (!exec_info->topaddr)
			{
				Kprintf("modulemgr.c:%s:topaddr is NULL, SCE_KERNEL_ERROR_NO_MEMORY\n", __FUNCTION__);
				res = SCE_KERNEL_ERROR_NO_MEMORY;
			}

			break;
		}

		case 4:
		{
			if (exec_info->unk_58 & 0x1000)
			{
				Kprintf("modulemgr.c:%s:Cannot use kernel mode ELF\n", __FUNCTION__);
				res = SCE_KERNEL_ERROR_ERROR;
			}

			else
			{
				if (exec_info->unk_54 == 1)
				{
					exec_info->topaddr = exec_info->unk_1C;
					mod->memid = exec_info->unk_18;
				}

				mod->mpidtext = exec_info->unk_40;
				mod->mpiddata = exec_info->unk_40;
			}

			break;
		}

		default:
		{
			res = SCE_KERNEL_ERROR_ILLEGAL_OBJECT;
			Kprintf("modulemgr.c:%s:type=%d (res 0x%08x)\n", __FUNCTION__, exec_info->elf_type, res);
		}
	}

	return res;
}

int RelocateModule(SceModuleMgrParam *mod_params)
{
	int res = 0;
	//SceUID res = 0; //modbody_id
	SceModule *mod = mod_params->mod;
	SceLoadCoreExecFileInfo *exec_info = mod_params->exec_info;

	exec_info->unk_8 = mod_params->apitype;

	if ((exec_info->elf_type + 1) >= 2)
	{
		exec_info->unk_4 = 1;
		exec_info->unk_1C = (u32)mod_params->file_buffer;

		sceKernelCheckExecFile(mod_params->file_buffer, exec_info);

		if (exec_info->unk_44 == 1)
		{
			u32 mpid;
			if ((mpid = IsUserModulePartition(mod_params->mpidtext)) < 0)
			{
				res = SCE_KERNEL_ERROR_PARTITION_MISMATCH;
				goto exit_clear_memory;
			}
		}
		else
		{
			u32 mpid;
			if ((mpid = IsKernelOnlyModulePartition(mod_params->mpidtext)) < 0)
			{
				res = SCE_KERNEL_ERROR_PARTITION_MISMATCH;
				goto exit_clear_memory;
			}
		}

		if (exec_info->unk_5A & 0x1) //gzip
		{
			u32 size = exec_info->unk_5C;

			if (size < exec_info->unk_30)
				size = exec_info->unk_30;

			exec_info->unk_14 = size + 0xFF;

			switch (exec_info->elf_type + 1)
			{
				case 2:
				{
					res = sceKernelAllocPartitionMemory(exec_info->unk_40, "SceModmgrModuleBody", mod_params->position, exec_info->unk_14, NULL);

					if (res < 0)
					{
						Kprintf("modulemgr.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, res);
						goto exit_clear_memory;
					}

					goto relocate_body;
				}

				case 4:
				{
					res = sceKernelAllocPartitionMemory(exec_info->unk_40, "SceModmgrElfBody", PSP_SMEM_Addr, exec_info->unk_14, (void *)exec_info->topaddr);

					if (res < 0)
					{
						Kprintf("modulemgr.c:%s:sceKernelAllocPartitionMemory failed 0x%08x\n", __FUNCTION__, res);
						goto exit_clear_memory;
					}

					relocate_body:
					exec_info->topaddr = (u32)sceKernelGetBlockHeadAddr(res);

					mod->memid = res;
					mod->mpidtext = exec_info->unk_40;
					mod->mpiddata = exec_info->unk_40;

					exec_info->unk_54 = 1;
					exec_info->unk_18 = res;
				}
			}
		}

		exec_info->unk_4 = 0;
		sceKernelCheckExecFile((void *)exec_info->unk_1C, exec_info);
	}

	res = allocate_module_block(mod_params);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:allocate_module_block failed: 0x%08x\n", __FUNCTION__, res);
		goto exit_clear_memory;
	}

	sceKernelLoadExecutableObject((void *)exec_info->unk_1C, exec_info);
	shrink_file_block(exec_info, mod);

	res = sceKernelAssignModule(mod, exec_info);

	if (res < 0)
	{
		goto exit_clear_memory;
	}

	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();

	SET_MCB_STATUS(mod->status, MCB_STATUS_RELOCATED);
	return res;

exit_clear_memory:
	if (exec_info)
	{
		if (exec_info->unk_18 > 0)
		{
			sceKernelFreePartitionMemory(exec_info->unk_18);
		}

		if (exec_info->unk_A8 > 0)
		{
			sceKernelFreePartitionMemory(res); //err...
		}
	}

	return res;
}

int StartModule(SceModuleMgrParam *mod_params, SceModule *mod, SceSize argsize, void *argp, int *status)
{
	u16 t_status = mod->status;

	if ((t_status & 0xF) != MCB_STATUS_RELOCATED)
	{
		Kprintf("modulemgr.c:%s:Illegal parameter: %d\n", __FUNCTION__, (t_status & 0xF));
		return SCE_KERNEL_ERROR_ERROR;
	}

	SET_MCB_STATUS(mod->status, MCB_STATUS_STARTING);
	int res = PrologueModule(mod_params, mod);

	if (res < 0)
	{
		Kprintf("modulemgr.c:%s:prologue_module failed: 0x%08x\n", __FUNCTION__, res);
		SET_MCB_STATUS(mod->status, MCB_STATUS_RELOCATED);
		return res;
	}

	sceKernelDcacheWritebackAll();

	if (mod->usermod_thid != -1)
	{
		modMgrCB.user_thread = mod->usermod_thid;

		if (sceKernelStartThread(mod->usermod_thid, argsize, argp) == 0)
		{
			res = sceKernelWaitThreadEnd(mod->usermod_thid, NULL);

			if (res < 0)
			{
				Kprintf("modulemgr.c:%s:Start failed: 0x%08x\n", __FUNCTION__, res); //ermm...
			}
		}

		modMgrCB.user_thread = -1;
		sceKernelDeleteThread(mod->usermod_thid);
		mod->usermod_thid = -1;
	}
	else
	{
		Kprintf("modulemgr.c:%s:Warning:There is no start thread\n", __FUNCTION__);
	}

	if (res)
	{
		EpilogueModule(mod);
		SET_MCB_STATUS(mod->status, MCB_STATUS_STOPPED);

		UnloadModule(mod);
		SET_MCB_STATUS(mod->status, MCB_STATUS_UNLOADED);
	}
	else
	{
		SET_MCB_STATUS(mod->status, MCB_STATUS_STARTED);
	}

	return res;
}

int StopModule(SceModuleMgrParam *mod_params, SceModule *mod, u8 mode_start, SceUID caller_modid, SceSize argsize, void *argp, int *status)
{
	u32 gp_reg;
	GET_GP(gp_reg);

	int return_status = 0;
	u32 stop_stat = (mod->status & 0xF);

	switch (stop_stat)
	{
		case (MCB_STATUS_LOADED || MCB_STATUS_RELOCATED):
		{
			Kprintf("modulemgr.c:%s:Module not started\n", __FUNCTION__);
			return SCE_KERNEL_ERROR_NOT_STARTED;
		}

		case MCB_STATUS_STARTED:
		{
			if ((int)mod->module_stop != -1)
			{
				int priority = mod_params->priority;
				u32 attribute = mod_params->attribute;
				SceSize stacksize = mod_params->stacksize;

				if (priority == 0)
				{
					if (mod->module_stop_thread_priority == -1)
						priority = 32;

					else
						priority = mod->module_stop_thread_priority;
				}

				if (stacksize == 0)
				{
					if (mod->module_stop_thread_stacksize != -1) //Sony resets it to zero again.
						stacksize = mod->module_stop_thread_stacksize;
				}

				if (attribute == 0)
				{
					if (mod->module_stop_thread_attr != -1)
						attribute = mod->module_stop_thread_attr;
				}

				char *thread_name;

				if (MCB_STATUS_USERMODE(mod->status)) //if usermode
				{
					thread_name = "SceKernelModmgrUserStart";

					if (stacksize == 0)
						stacksize = 0x40000;

					attribute |= PSP_THREAD_ATTR_USER;
				}
				else
				{
					thread_name = "SceKernelModmgrKernelStart";

					if (stacksize == 0)
						stacksize = 0x1000;
				}

				SceKernelThreadOptParam option;
				option.size = sizeof(SceKernelThreadOptParam);

				if (mod_params->mpidstack)
				{
					option.stackMpid = mod_params->mpidstack;
				}
				else
				{
					option.stackMpid = 0;
				}

				SET_GP(mod->gp_value);
				mod->usermod_thid = sceKernelCreateThread(thread_name, mod->module_stop, priority, stacksize, attribute, &option);
				SET_GP(gp_reg);

				if (mod->usermod_thid < 0)
				{
					Kprintf("modulemgr.c:%s:sceKernelCreateThread failed 0x%08x\n", __FUNCTION__, mod->usermod_thid);
					return mod->usermod_thid;
				}

				modMgrCB.user_thread = mod->usermod_thid;
				SET_MCB_STATUS(mod->status, MCB_STATUS_STOPPING);

				if (sceKernelStartThread(mod->usermod_thid, argsize, argp) == 0)
				{
					return_status = sceKernelWaitThreadEnd(mod->usermod_thid, NULL);
				}

				sceKernelDeleteThread(mod->usermod_thid);
				sceKernelChangeThreadPriority(0, 32);

				if (status)
				{
					*status = return_status;
				}

				if (return_status)
				{
					if (return_status == 1)
						return_status = SCE_KERNEL_ERROR_CAN_NOT_STOP;

					SET_MCB_STATUS(mod->status, MCB_STATUS_STARTED);
					break;
				}
			}

			return_status = EpilogueModule(mod);

			if (return_status < 0)
			{
				Kprintf("modulemgr.c:%s:EpilogueModule failed 0x%08x\n", __FUNCTION__, return_status);
				SET_MCB_STATUS(mod->status, MCB_STATUS_STARTED);
				break;
			}

			SET_MCB_STATUS(mod->status, MCB_STATUS_STOPPING);
			break;
		}

		case MCB_STATUS_STOPPING:
		{
			Kprintf("modulemgr.c:%s:MCB_STATUS_STOPPING --- failed\n", __FUNCTION__);
			return_status = SCE_KERNEL_ERROR_ALREADY_STOPPING;
			break;
		}

		case MCB_STATUS_STOPPED:
		{
			Kprintf("modulemgr.c:%s:MCB_STATUS_STOPPED --- failed\n", __FUNCTION__);
			return_status = SCE_KERNEL_ERROR_ALREADY_STOPPED;
			break;
		}

		default:
		{
			Kprintf("modulemgr.c:%s:Unknown status 0x%x\n", __FUNCTION__, stop_stat);
			return_status = SCE_KERNEL_ERROR_ERROR;
			break;
		}
	}

	return return_status;
}

int UnloadModule(SceModule *mod)
{
	u32 unload_stat = (mod->status & 0xF);
	if (unload_stat < MCB_STATUS_LOADED || (unload_stat >= MCB_STATUS_STARTING && unload_stat != MCB_STATUS_STOPPED))
	{
		Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_NOT_REMOVABLE: status=0x%08x\n", __FUNCTION__, unload_stat);
		return SCE_KERNEL_ERROR_NOT_REMOVABLE;
	}

	sceKernelMemset32((void *)mod->text_addr, 0x4D, mod->text_size);
	sceKernelMemset((void *)(mod->text_addr + mod->text_size), 0xFF, mod->data_size + mod->bss_size);

	sceKernelReleaseModule(mod);
	sceKernelFreePartitionMemory(mod->memid);
	sceKernelDeleteModule(mod);

	return 0;
}

int exe_thread(SceSize argsize, SceModuleMgrParam *params)
{
	int ret = 0;
	SceLoadCoreExecFileInfo exec_info;

	if (argsize != sizeof(SceModuleMgrParam))
	{
		Kprintf("Assertion failed at %s:%s:%04d\n", __FILE__, __FUNCTION__, __LINE__);
		sceKernelAssert(0, 0);
	}

	SceModule *mod = params->mod;
	sceKernelMemset(&exec_info, 0, sizeof(SceLoadCoreExecFileInfo));

	switch (params->mode_start)
	{
		case EXE_MODE_LOAD_MODULE:
		{
			if (!mod)
			{
				mod = sceKernelCreateModule();
				params->mod = mod;

				if (!mod)
				{
					Kprintf("modulemgr.c:%s:sceKernelCreateModule failed\n", __FUNCTION__);
					break;
				}
			}

			params->exec_info = &exec_info;
			ret = LoadModule(params);

			sceKernelChangeThreadPriority(0, 32); //Change current thread priority

			if (ret < 0)
			{
				Kprintf("modulemgr.c:%s:LoadModule failed: 0x%08x\n", __FUNCTION__, ret);
				params->return_id[0] = ret;
				break;
			}

			params->return_id[0] = mod->modid;

			if (params->mode_finish == EXE_MODE_LOAD_MODULE)
				break;
		}

		case EXE_MODE_RELOCATE_MODULE:
		{
			if (!mod)
			{
				mod = sceKernelCreateModule();
				params->mod = mod;

				if (!mod)
				{
					Kprintf("modulemgr.c:%s:sceKernelCreateModule failed\n", __FUNCTION__);
					break;
				}

				SET_MCB_STATUS(mod->status, MCB_STATUS_LOADED);
				sceKernelRegisterModule(mod);
			}

			if (!params->exec_info)
			{
				sceKernelMemset(&exec_info, 0, sizeof(SceLoadCoreExecFileInfo));
				params->exec_info = &exec_info;
			}

			ret = RelocateModule(params);

			if (ret < 0)
			{
				Kprintf("modulemgr.c:%s:RelocateModule failed: 0x%08x\n", __FUNCTION__, ret);

				if (mod)
				{
					sceKernelReleaseModule(mod);
					sceKernelDeleteModule(mod);
				}

				params->return_id[0] = ret;
				break;
			}

			params->return_id[0] = mod->modid;

			if (params->mode_finish == EXE_MODE_RELOCATE_MODULE)
				break;
		}

		case EXE_MODE_START_MODULE:
		{
			mod = sceKernelFindModuleByUID(params->modid);

			if (!mod && !(mod = sceKernelFindModuleByUID(params->modid)))
			{

				Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_UNKNOWN_MODULE\n", __FUNCTION__);
				params->return_id[0] = SCE_KERNEL_ERROR_UNKNOWN_MODULE;
			}
			else
			{
				ret = StartModule(params, mod, params->argsize, params->argp, params->status);

				if (ret == 0)
					params->return_id[0] = mod->modid;

				else if (ret == 1)
					params->return_id[0] = 0;

				else
				{
					Kprintf("modulemgr.c:%s:StartModule failed: 0x%08x\n", __FUNCTION__, ret);
					params->return_id[0] = ret;
				}
			}

			if (ret < 0 || params->mode_finish == EXE_MODE_START_MODULE)
				break;
		}

		case EXE_MODE_STOP_MODULE:
		{
			if (!mod && !(mod = sceKernelFindModuleByUID(params->modid)))
			{
				Kprintf("modulemgr.c:%s:SCE_KERNEL_ERROR_UNKNOWN_MODULE: 0x%08x\n", __FUNCTION__, params->modid);
				params->return_id[0] = SCE_KERNEL_ERROR_UNKNOWN_MODULE;
			}
			else
			{
				ret = StopModule(params, mod, params->mode_start, params->caller_modid, params->argsize, params->argp, params->status);

				if (ret == 0)
					params->return_id[0] = 0;

				else if (ret == 1)
					params->return_id[0] = mod->modid;

				else
				{
					Kprintf("modulemgr.c:%s:StopModule failed: 0x%08x\n", __FUNCTION__, ret);
					params->return_id[0] = ret;
				}
			}

			if (params->mode_finish == EXE_MODE_STOP_MODULE)
				break;
		}

		case EXE_MODE_UNLOAD:
		{
			mod = sceKernelFindModuleByUID(params->modid);

			if (!mod)
			{
				params->return_id[0] = SCE_KERNEL_ERROR_UNKNOWN_MODULE;
			}

			else if (UnloadModule(mod) < 0)
				params->return_id[0] = 0;

			else
				params->return_id[0] = mod->modid;
		}
	}

	if (params->event_id)
	{
		sceKernelChangeThreadPriority(0, 1);
		sceKernelSetEventFlag(params->event_id, 1);
	}

	return 0;
}

int reboot_phase(SceSize args, void *argp)
{
	return 0;
}

int module_reboot_before(int arglen, void *argp) __attribute__((alias("_ModuleMgrRebootBefore")));
int _ModuleMgrRebootBefore(SceSize args, void *argp) //module_reboot_before
{
	int ret = sceKernelSuspendThread(modMgrCB.exe_thid);

	if (ret < 0 && ret != SCE_KERNEL_ERROR_DORMANT)
	{
		Kprintf("modulemgr.c:%s:sceKernelSuspendThread failed 0x%08x\n", __FUNCTION__, ret);
	}

	return ret;
}

int module_start(int arglen, void *argp) __attribute__((alias("ModuleMgrInit")));
int ModuleMgrInit(SceSize args, void *argp) //module_bootstart
{
	modMgrCB.exe_thid = sceKernelCreateThread("SceKernelModmgrWorker", (void *)exe_thread, 0x20, 0x4000, 0, NULL);
	modMgrCB.sema_id = sceKernelCreateSema("SceKernelModmgr", 0, 1, 1, NULL);

	if (modMgrCB.sema_id < 0)
	{
		Kprintf("modulemgr.c:%s:modMgrCB.semid 0x%x\n", __FUNCTION__, modMgrCB.sema_id);
		Kprintf("Assertion failed at %s:%s:%04d\n", __FILE__, __FUNCTION__, __LINE__);

		sceKernelAssert(0, 0);
	}

	modMgrCB.event_id = sceKernelCreateEventFlag("SceKernelModmgr", 0, 0, 0);
	modMgrCB.user_thread = -1;
	modMgrCB.unk2 = (SceUID *)&modMgrCB.unk2;

	return 0;
}
