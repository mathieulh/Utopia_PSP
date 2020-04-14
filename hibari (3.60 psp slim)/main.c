#include <pspsdk.h>
#include <pspkernel.h>
#include <pspsysevent.h>

#include <stdio.h>
#include <string.h>

#include "table.h"

PSP_MODULE_INFO("sceHibari_Driver", 0x1007, 1, 1);

int sceSysregSpiClkSelect(int, int);
int sceSysregSpiClkEnable(int);
int sceSysregSpiClkDisable(int);
int sceSysregSpiIoEnable(int);
int sceSysregSpiIoDisable(int);
int sceSysregGetTachyonVersion(void);
int sceGpioSetPortMode(int, int);
int sceGpioPortClear(int);
int sceGpioPortSet(int);
int sceGpioPortRead(void);

int sceHibariUnk1(int x);
int sceHibariUnk2();
int sceHibariUnk3(u32 a, u32 b, u8 c);
int sceHibariUnk4(u32 a, u8 b);
int sceHibariUnk5(u16 a, u8 b);
int sceHibariUnk6(u8 *p, u16 *q, u8 *r);
int sceHibariUnk7(u32 a, u8 *p, int size);
int sceHibariUnk8(u32 a, u32 b, u8 *p, int size);
int sceHibariUnk9(u8 a);
int sceHibariUnk10(void);
int sceHibariUnk11(void);
int sceHibariUnk12(void);
int sceHibariUnk13(void);
int sceHibariUnk14(u32 a);
int sceHibariUnk15(u32 a, u32 b);
int sceHibariUnk16(void);
int sceHibariUnk17(void);
int sceHibariUnk18(int x);
int sceHibariUnk19(u8 *p);
int sceHibariUnk20(u8 *p);
int sceHibariUnk21(void);
int sceHibariUnk22(void);
int sceHibariUnk23(u8 *p);
int sceHibariUnk24(u8 a);
int sceHibariUnk25(void);
int sceHibariUnk26(void);
int sceHibariUnk27(int a);
int sceHibariResetEnable(void);
int sceHibariGetDisplayStatus(void);
int sceHibariDisplayOn(void);
int sceHibariDisplayOff(void);

typedef struct HibariStruct2
{
	int delaymode;
	u8 *ptr;
	u32 unk1;
} HibariStruct2;

static int hibari_struct[4];
static HibariStruct2 st2;

static int SysEventHandler(int ev_id, char* ev_name, void* param, int* result);

static PspSysEventHandler event_handler =
{
	sizeof(PspSysEventHandler),
	"sceHibari",
	0x00FFFF00,
	SysEventHandler
};

static void _Init()
{
	sceSysregSpiClkSelect(1, 3);
	sceSysregSpiClkEnable(1);
	sceSysregSpiIoEnable(1);

	*(volatile u32 *)0xbe5c0000 = 0xF;
	*(volatile u32 *)0xbe5c0004 = 0;
	*(volatile u32 *)0xbe5c0010 = 2;
	*(volatile u32 *)0xbe5c0014 = 0;
	*(volatile u32 *)0xbe5c0024 = 0;
	asm("sync\n");

	sceSysregSpiClkDisable(1);
	sceGpioSetPortMode(0, 0);

	hibari_struct[1] = hibari_struct[2] = hibari_struct[3] = -1;
}

int sceHibariInit()
{
	Kprintf("Hibari Display Driver Clone v 0.1\n");
	Kprintf("Copyright (C) 2007, Dark_AleX\n");
	
	int tachyon = sceSysregGetTachyonVersion();

	Kprintf("Tachyon version = 0x%08X.\n", tachyon);

	if (tachyon < 0x00500000)
		return 0x80000004;

	_Init();

	hibari_struct[0] = 0;
	st2.ptr = NULL;
	st2.unk1 = 0;	
	sceKernelRegisterSysEventHandler(&event_handler);
	
	return 0;
}

int sceHibariEnd()
{
	sceKernelUnregisterSysEventHandler(&event_handler);
	return 0;
}

static int SysEventHandler(int ev_id, char* ev_name, void* param, int* result)
{
	switch (ev_id)
	{
		case 0x4008: // going to sleep mode
			sceSysregSpiIoDisable(1);
			sceSysregSpiClkDisable(1);
			sceGpioSetPortMode(0, 2);
		break;

		case 0x10008: // returning from sleep mode
			_Init();
		break;

		case  0x402: 
			if (sceHibariUnk1(1) != 0)
				return 0x80000021;
		break;
	}
	
	return 0;
}

int sceHibariUnk7(u32 a, u8 *p, int size)
{
	int i;
	
	if (hibari_struct[0] == 0)
	{
		sceSysregSpiClkEnable(1);

		while (((*(volatile u32 *)0xbe5c000c) & 4))
		{
			_lw(0xbe5c0008);
		}

		*(volatile u32 *)0xbe5c0004 = 2;
		asm("sync\n");
	}

	hibari_struct[0]++;

	if (hibari_struct[1] != a)
	{
		while (!((*(volatile u32 *)0xbe5c000c) & 2));	
		*(volatile u32 *)0xbe5c0008 = ((a&0xFFFF) | 0x8000);
	}

	for (i = 0; i < size; i++)
	{
		while (!((*(volatile u32 *)0xbe5c000c) & 2));
		*(volatile u32 *)0xbe5c0008 = (p[i] | 0x8100);
		
		a++;
	}

	hibari_struct[0]--;
	hibari_struct[1] = a;
	hibari_struct[3] = -1;

	if (hibari_struct[0] == 0)
	{
		while (((*(volatile u32 *)0xbe5c000c) & 0x10));
		
		*(volatile u32 *)0xbe5c0004 = 0;
		asm("sync\n");

		sceSysregSpiClkDisable(1);
	}

	return 0;
}

int sceHibariUnk8(u32 a, u32 b, u8 *p, int size)
{
	int i;
	
	if (hibari_struct[0] == 0)
	{
		sceSysregSpiClkEnable(1);

		while (((*(volatile u32 *)0xbe5c000c) & 4))
		{
			_lw(0xbe5c0008);
		}

		*(volatile u32 *)0xbe5c0004 = 2;
		asm("sync\n");
	}

	hibari_struct[0]++;

	if (hibari_struct[2] != a && b != 0xEF)
	{
		while (!((*(volatile u32 *)0xbe5c000c) & 2));	
		*(volatile u32 *)0xbe5c0008 = 0xEF;

		while (!((*(volatile u32 *)0xbe5c000c) & 2));	
		*(volatile u32 *)0xbe5c0008 = (a | 0x0100) & 0xFFFF;

		hibari_struct[2] = a;
		hibari_struct[3] = -1;
	}

	if (hibari_struct[3] != b)
	{
		while (!((*(volatile u32 *)0xbe5c000c) & 2));	
		*(volatile u32 *)0xbe5c0008 = (b&0xFFFF);		
	}

	for (i = 0; i < size; i++)
	{
		if (b == 0xEF)
		{
			hibari_struct[2] = (u32)p[i];
		}

		while (!((*(volatile u32 *)0xbe5c000c) & 2));	
		*(volatile u32 *)0xbe5c0008 = (p[i] | 0x0100);
		
		b++;
	}

	hibari_struct[0]--;
	hibari_struct[1] = -1;
	hibari_struct[3] = b;

	if (hibari_struct[0] == 0)
	{
		while (((*(volatile u32 *)0xbe5c000c) & 0x10));
		
		*(volatile u32 *)0xbe5c0004 = 0;
		asm("sync\n");

		sceSysregSpiClkDisable(1);
	}

	return 0;
}

int sceHibariUnk10()
{
	sceSysregSpiIoDisable(1);
	sceSysregSpiClkDisable(1);
	sceGpioSetPortMode(0, 2);

	return 0;
}

int sceHibariUnk11()
{
	_Init();
	return 0;
}

int sceHibariResetEnable()
{
	sceGpioPortClear(1);
	return 0;
}

int sceHibariUnk12()
{
	sceGpioPortSet(1);
	return 0;
}

int sceHibariUnk13()
{
	return !(sceGpioPortRead() & 1);
}

int sceHibariUnk14(u32 a)
{
	u32 res;
	
	if (hibari_struct[0] == 0)
	{
		sceSysregSpiClkEnable(1);

		while (((*(volatile u32 *)0xbe5c000c) & 4))
		{
			_lw(0xbe5c0008);
		}

		*(volatile u32 *)0xbe5c0004 = 2;
		asm("sync\n");
	}

	hibari_struct[0]++;

	while (((*(volatile u32 *)0xbe5c000c) & 0x10));

	while (((*(volatile u32 *)0xbe5c000c) & 4))
	{
		_lw(0xbe5c0008);
	}

	hibari_struct[1] = a;
	hibari_struct[3] = -1;

	while (!((*(volatile u32 *)0xbe5c000c) & 2));	
	*(volatile u32 *)0xbe5c0008 = ((a&0xFFFF) | 0x8000);	

	while (!((*(volatile u32 *)0xbe5c000c) & 4));
	_lw(0xbe5c0008);
	
	while (!((*(volatile u32 *)0xbe5c000c) & 2));
	*(volatile u32 *)0xbe5c0008 = 0xA000;

	while (!((*(volatile u32 *)0xbe5c000c) & 4));
	res = (*(volatile u32 *)0xbe5c0008) & 0xFF;
	
	hibari_struct[0]--;

	if (hibari_struct[0] == 0)
	{
		while (((*(volatile u32 *)0xbe5c000c) & 0x10));
		
		*(volatile u32 *)0xbe5c0004 = 0;
		asm("sync\n");

		sceSysregSpiClkDisable(1);
	}

	return res;
}

int sceHibariUnk4(u32 a, u8 b)
{
	return sceHibariUnk7(a, &b, 1);
}

int sceHibariUnk15(u32 a, u32 b)
{
	u32 res;

	if (hibari_struct[0] == 0)
	{
		sceSysregSpiClkEnable(1);

		while (((*(volatile u32 *)0xbe5c000c) & 4))
		{
			_lw(0xbe5c0008);
		}

		*(volatile u32 *)0xbe5c0004 = 2;
		asm("sync\n");
	}

	hibari_struct[0]++;

	if (hibari_struct[2] != a)
	{
		while (!((*(volatile u32 *)0xbe5c000c) & 2));	
		*(volatile u32 *)0xbe5c0008 = 0xEF;

		while (!((*(volatile u32 *)0xbe5c000c) & 2));	
		*(volatile u32 *)0xbe5c0008 = (a | 0x0100) & 0xFFFF;
		
		hibari_struct[2] = a;
	}

	while (((*(volatile u32 *)0xbe5c000c) & 0x10));

	while (((*(volatile u32 *)0xbe5c000c) & 4))
	{
		_lw(0xbe5c0008);
	}

	hibari_struct[1] = -1;
	hibari_struct[3] = b;

	while (!((*(volatile u32 *)0xbe5c000c) & 2));
	*(volatile u32 *)0xbe5c0008 = (b&0xFFFF);

	while (!((*(volatile u32 *)0xbe5c000c) & 4));
	_lw(0xbe5c0008); 

	while (!((*(volatile u32 *)0xbe5c000c) & 2));
	*(volatile u32 *)0xbe5c0008 = 0x2000;
	
	while (!((*(volatile u32 *)0xbe5c000c) & 4));
	res = (*(volatile u32 *)0xbe5c0008) & 0xFF; 

	hibari_struct[0]--;

	if (hibari_struct[0] == 0)
	{
		while (((*(volatile u32 *)0xbe5c000c) & 0x10));
		
		*(volatile u32 *)0xbe5c0004 = 0;
		asm("sync\n");

		sceSysregSpiClkDisable(1);
	}
	
	return res;
}

int sceHibariUnk9(u8 a)
{
	return sceHibariUnk8(0, 0xEF, &a, 1);
}

int sceHibariUnk3(u32 a, u32 b, u8 c)
{
	return sceHibariUnk8(a, b, &c, 1);
}

int sceHibariUnk5(u16 a, u8 b)
{
	if (hibari_struct[0] == 0)
	{
		sceSysregSpiClkEnable(1);

		while (((*(volatile u32 *)0xbe5c000c) & 4))
		{
			_lw(0xbe5c0008);
		}

		*(volatile u32 *)0xbe5c0004 = 2;
		asm("sync\n");
	}

	hibari_struct[0]++;

	while (!((*(volatile u32 *)0xbe5c000c) & 2));	
	*(volatile u32 *)0xbe5c0008 = ((a&0xFFFF) | 0xC000);

	while (!((*(volatile u32 *)0xbe5c000c) & 2));
	*(volatile u32 *)0xbe5c0008 = (b | 0xC100);

	hibari_struct[1] = hibari_struct[3] = -1;
	hibari_struct[0]--;
	
	if (hibari_struct[0] == 0)
	{
		while (((*(volatile u32 *)0xbe5c000c) & 0x10));
		
		*(volatile u32 *)0xbe5c0004 = 0;
		asm("sync\n");

		sceSysregSpiClkDisable(1);
	}
	
	return 0;
}

int sceHibariUnk6(u8 *p, u16 *q, u8 *r)
{
	u32 i;
	int res = 0;
	
	for (i = 0; i < 4; i++)
	{
		u16 x = q[i];
		u8  y;
		
		res = sceHibariUnk3(i+1, 0xD1, p[i]);
		if (res < 0)
			break;

		if (i < 2)
		{
			res = sceHibariUnk3(i+1, 0xD2, x >> 8);
			if (res < 0)
				break;

			y = 0xD3;
		}
		else
		{
			res = sceHibariUnk3(i+1, 0xD4, x >> 8);
			if (res < 0)
				break;

			y = 0xD5;
		}

		res = sceHibariUnk3(i+1, y, x&0xFF);
		if (res < 0)
			break;

		res = sceHibariUnk8(i+1, 0, r+(i*128), 0x80);
		if (res < 0)
			break;
	}

	sceHibariUnk9(0);	
	return res;
}

int sceHibariUnk16()
{
	int i, res;
	
	sceHibariUnk12();

	if (st2.delaymode == 0)
	{
		sceKernelDelayThread(10000);
	}
	else
	{
		for (i = 830000; i != 0; i--);		
	}

	res = sceHibariUnk5(0xC0, 1);
	if (res < 0)
		return res;

	if (st2.delaymode == 0)
	{
		sceKernelDelayThread(50);
	}
	else
	{
		for (i = 4150; i != 0; i--);		
	}

	res = sceHibariUnk19(tableE4);
	if (res < 0)
		return res;

	res = sceHibariUnk19(table04);
	if (res < 0)
		return res;

	res = sceHibariUnk19(table1C);
	if (res < 0)
		return res;

	res = sceHibariUnk19(table5C);
	if (res < 0)
		return res;

	res = sceHibariUnk19(table64);
	if (res < 0)
		return res;

	res = sceHibariUnk19(table6C);
	if (res < 0)
		return res;

	return 0;
}

int sceHibariGetDisplayStatus()
{
	int res = sceHibariUnk15(0, 0x30);
	if (res < 0)
		return res;

	return (res&1);
}

int sceHibariUnk17()
{
	int res = sceHibariUnk15(0, 0x3D);
	if (res < 0)
		return res;

	return !(res&2);
}

int sceHibariUnk18(int x)
{
	int res = sceHibariUnk20(x ? table88 : table84);
	if (res < 0)
		return res;

	if (st2.delaymode != 0)
	{
		sceHibariUnk1(0);
	}

	return 0;
}

int sceHibariUnk2()
{
	if (st2.ptr == NULL)
		return 0;

	if ((int)st2.unk1 > 0)
	{
		st2.unk1--;
		return 0;
	}

	while (st2.ptr != NULL)
	{
		if (st2.ptr[0] == 0xFF)
		{
			st2.ptr = NULL;
			break;
		}

		switch (st2.ptr[0])
		{
			case 0:
				sceHibariUnk3(0, st2.ptr[1], st2.ptr[2]);
			break;

			case 1:
				sceHibariUnk4(st2.ptr[1], st2.ptr[2]);
			break;

			case 2:
				sceHibariUnk5(st2.ptr[1], st2.ptr[2]);
			break;

			case 3:
				st2.unk1 = st2.ptr[1];
			break;

			case 4:
				sceHibariUnk6(tableE0, (u16 *)(tableE0+4), tableE0+12);
			break;

			case 5:
				sceHibariUnk7(0x20, table8C, 0x51);
			break;
			
			default:
				st2.ptr = NULL;
		}

		if (st2.ptr != NULL)
			st2.ptr += 3;	
		
		if (st2.unk1 != 0)
			break;
	}

	return 0;
}

int sceHibariUnk19(u8 *p)
{
	int res = sceHibariUnk20(p);
	if (res < 0)
		return res;

	sceHibariUnk1(0);	
	return 0;
}

int sceHibariUnk20(u8 *p)
{
	if (st2.ptr != NULL)
		return 0x80000021;

	st2.ptr = p;
	st2.unk1 = 0;
	
	return 0;
}

int sceHibariUnk1(int x)
{
	int i;
	
	if (x != 0)
	{
		return (st2.ptr != NULL);
	}

	if (st2.delaymode == 0)
	{
		while (st2.ptr != NULL)
		{
			sceKernelDelayThread(16000);
		}		
	}

	else
	{
		if (st2.ptr == NULL)
			return 0;

		while (1)
		{		
			sceHibariUnk2();

			if (st2.ptr == NULL)
				break;

			for (i = 1328000; i != 0; i--);
		}
	}	

	return 0;

}

int sceHibariUnk21()
{
	return sceHibariUnk15(0, 0);
}

int sceHibariUnk22()
{
	return sceHibariUnk14(0);
}

int sceHibariUnk23(u8 *p)
{
	return sceHibariUnk7(0x20, p, 0x51);
}

int sceHibariUnk24(u8 a)
{
	table5C[2] = a;
	return 0;
}

int sceHibariDisplayOn()
{
	int res = sceHibariUnk20(table7C);
	if (res < 0)
		return res;

	if (st2.delaymode != 0)
	{
		sceHibariUnk1(0);
	}

	return 0;
}

int sceHibariDisplayOff()
{
	int res = sceHibariUnk20(table7C+0x94);
	if (res < 0)
		return res;

	if (st2.delaymode != 0)
	{
		sceHibariUnk1(0);
	}
	
	return 0;
}

int sceHibariUnk25()
{
	int res = sceHibariUnk20(table7C+0xD8);
	if (res < 0)
		return res;

	if (st2.delaymode != 0)
	{
		sceHibariUnk1(0);
	}
	
	return 0;
}

int sceHibariUnk26()
{
	int res = sceHibariUnk20(table7C+0xF0);
	if (res < 0)
		return res;

	if (st2.delaymode != 0)
	{
		sceHibariUnk1(0);
	}
	
	return 0;
}

int sceHibariUnk27(int a)
{
	st2.delaymode = a;
	return 0;
}

int module_start(SceSize args, void *argp)
{	
	return (sceHibariInit() < 0);
}

int module_stop(SceSize args, void *argp)
{
	sceKernelUnregisterSysEventHandler(&event_handler);
	return 0;
}

