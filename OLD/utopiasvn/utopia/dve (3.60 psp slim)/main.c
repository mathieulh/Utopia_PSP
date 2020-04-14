#include <pspsdk.h>
#include <pspkernel.h>

#include <stdio.h>
#include <string.h>

#include "table.h"

PSP_MODULE_INFO("sceDve_Driver", 0x1007, 1, 1);

int sceSysregGetTachyonVersion(void);
int scePwmStart(int, int, int, int);
int scePwmStop(int);
int scePwm_driver_F624C1A0(int, int, int);
int sceI2cMasterTransmitReceive(u32, u8 *, int, u32, u8 *, int);
int sceI2cMasterTransmit(u32, u8 *, int);
int sceI2cSetClock(int, int);
int sceSysconCtrlDvePower(int);

int sceDveUnk1(void);
u32 sceDveUnk2(u8 x);
int sceDveUnk3(u8 x, u8 *p, int size);
int sceDveUnk4(u32 index);
int sceDveUnk5(int a, int b, int c);
int sceDveUnk6(u32 index);
int sceDveUnk7(void);
int sceDveUnk8(void);
int sceDveUnk9(void);
int sceDveUnk10(u8 x, u8 y);
int sceDveUnk11(void);
int sceDveUnk12(u32 x);
int sceDveGetDisplayMode(void);
int sceDveSetDisplayMode(int mode);
int sceDvePowerOn(void);
int sceDvePowerOff(void);
int sceDveResetEnable(void);
int sceDveResetDisable(void);
int sceDveDisplayOn(void);
int sceDveDisplayOff(void);

#define N_DISPLAY	3

typedef struct DisplayStruct
{
	int mode1;
	int mode2;
	u8  *table;
} DisplayStruct;

DisplayStruct displays[N_DISPLAY] =
{
	{ 0,  1, table78 },
	{ 8, -1, table84 },
	{ 2,  3, table90 }
};

u32 dve_struct[9] =
{
	0xFFFFFFFF,
	0,
	0xFFFFFFFF,
	0,
	0,
	0,
	0xF,
	0,
	0
};

int sceDveInit()
{
	Kprintf("Dve Display Driver Clone v 0.1\n");
	Kprintf("Copyright (C) 2007, Dark_AleX\n");
	
	int tachyon = sceSysregGetTachyonVersion();

	Kprintf("Tachyon version = 0x%08X.\n", tachyon);

	if (tachyon < 0x00500000)
		return 0x80000004; 

	if (sceDveUnk1() == 0)
	{
		int res, res2;
		
		dve_struct[1] = 0x15;

		res = sceDveUnk2(0x30);
		if (res < 0)
			return res;

		res = sceDveUnk2(0x38);
		if (res < 0)
			return res;

		res2 = sceDveUnk2(0x91);
		if (res2 < 0)
			return res2;

		res = sceDveUnk2(0x6C);
		if (res < 0)
			return res;

		dve_struct[3] = (res == 0) ? 0 : 2;

		if (res2 == 0)
		{
			dve_struct[2] = 0;
		}
		else if (res2 == 2)
		{
			dve_struct[2] = 2;
		}
	}
	else
	{
		dve_struct[1] = 0x15;
		dve_struct[3] = 0;
	}

	return 0;
}

int sceDveEnd()
{
	return 0;
}

int sceDveUnk1()
{
	return (scePwm_driver_F624C1A0(2, 0, 0) == 0);
}

u32 sceDveUnk2(u8 x)
{
	u8 param[0x20];

	param[0x10] = x;

	int res = sceI2cMasterTransmitReceive(0x42, param+0x10, 1, 0x42, param, 1);
	if (res < 0)
	{
		return res;
	}

	return param[0];
}

int sceDveUnk3(u8 x, u8 *p, int size)
{
	u8 param[0x10];
	int i;
	
	if (size >= 0xF)
		return 0x80000104;

	param[0] = x;

	for (i = 0; i < size; i++)
	{
		param[i+1] = p[i];
	}

	int res = sceI2cMasterTransmit(0x42, param, size+1);
	if (res < 0)
	{
		return res;
	}

	return 0;
}

static int process_table(u8 *table)
{
	u8 param[0x10];
	
	while (table[0] != 0 || table[1] != 0)
	{
		if (table[0] == 0)
		{
			sceKernelDelayThread(1000*table[1]);
		}

		else
		{		
			param[0] = table[0];
			param[1] = table[1];

			int res = sceI2cMasterTransmit(0x42, param, 2);
			if (res < 0)
				return res;
		}

		table += 2;
	}

	return 0;
}

int sceDveSetDisplayMode(int mode)
{
	int i, res;

	for (i = 0; i < N_DISPLAY; i++)
	{
		if (displays[i].mode1 == mode || displays[i].mode2 == mode)
			break;
	}

	if (i == N_DISPLAY)
		return 0x80000107;

	if (dve_struct[3] == 2)
	{
		if (mode >= 2 && mode != 8)
		{
			return 0x80000107;
		}	
	}

	if (dve_struct[2] == 0xFFFFFFFF)
	{
		if (dve_struct[1] == 0x15)
		{
			process_table(table60);
		}

		process_table(tableAC);
	}
	
	if (dve_struct[2] != 0xFFFFFFFF)
	{
		if (((dve_struct[2] ^ mode) & ~1))
		{
			res = process_table(table74);
			if (res < 0)
				return res;

			res = process_table(tableAC);
			if (res < 0)
				return res;
		}

		else if (!(mode & 1))
		{
			res = process_table(tableAC);
			if (res < 0)
				return res;
		}
	}

	res = process_table(displays[i].table);
	if (res < 0)
		return res;

	int previous = dve_struct[2];
	dve_struct[2] = mode;

	if (dve_struct[2] == 0xFFFFFFFF && dve_struct[4] != 0)
	{
		if (dve_struct[4] == 1)
		{
			res = sceDveUnk5(dve_struct[5], dve_struct[6], dve_struct[7]);
			if (res < 0)
			{
				return res;
			}			
		}

		res = sceDveUnk4(dve_struct[4]);
		if (res < 0)
		{
			return res;
		}
	}

	if (dve_struct[8] != 0)
	{
		if (mode == 1 || mode == 3)
		{
			res = sceDveUnk6(dve_struct[8]);
		}
	}
	
	return previous;
}

int sceDveUnk4(u32 index)
{
	if (index >= 2)
		return 0x80000107;

	if (dve_struct[2] != 0xFFFFFFFF)
	{
		int res = process_table(tableB4[index]);
		if (res < 0)
			return res;
	}

	int previous = dve_struct[4];
	dve_struct[4] = index;
	
	return previous;
}

int sceDveUnk5(int a, int b, int c)
{
	u8 param[3];
	
	if (a >= 4 || b >= 0x10 || c >= 0x100)
	{
		return 0x800001fe;
	}

	if (a < 0)
	{
		a = dve_struct[5];
	}

	if (b < 0)
	{
		if (c >= 0)
		{
			return 0x800001fe;
		}

		b = dve_struct[6];
		c = dve_struct[7];
	}

	else if (c < 0)
	{
		return 0x800001fe;
	}

	param[0] = ((((c << 6) | (b << 2)) | a) & 0xFFFF);
	param[1] = (param[0] >> 8) & 0x3F;
	param[2] = 0;

	if (dve_struct[2] != 0xFFFFFFFF)
	{
		int res = sceDveUnk3(0x80, param, 3);
		if (res < 0)
		{
			return 0;
		}
	}

	dve_struct[5] = a;
	dve_struct[6] = b;
	dve_struct[7] = c;

	return 0;
}

int sceDveUnk6(u32 index)
{
	u8 *p;
	
	if (index != 0)
	{
		if (dve_struct[2] != 1 && dve_struct[2] != 3)
		{
			return 0x80000107;
		}
		
		if (index >= 4)
			return 0x80000107;
	}

	p = tableBC[index];

	if (index != 0 && dve_struct[2] == 3)
	{
		p = table4C;
	}

	if (dve_struct[2] != 0xFFFFFFFF)
	{
		int res = process_table(p);
		if (res < 0)
			return res;
	}

	int previous = dve_struct[8];
	dve_struct[8] = index;

	return previous;
}

int sceDveUnk7()
{
	return 0;
}

int sceDveUnk8()
{
	return 0;
}

int sceDvePowerOn()
{
	return sceSysconCtrlDvePower(1);
}

int sceDvePowerOff()
{
	return sceSysconCtrlDvePower(0);
}

int sceDveUnk9()
{
	sceI2cSetClock(4, 4);
	scePwmStart(2, 0x18, 0x19, 0x19);

	return 0;
}

int sceDveResetEnable()
{
	scePwmStop(2);
	dve_struct[2] = 0xFFFFFFFF;

	return 0;
}

int sceDveResetDisable()
{
	scePwmStart(2, 0x18, 0x19, 0x19);
	return 0;
}

int sceDveUnk10(u8 x, u8 y)
{
	u8 param[0x10];

	param[0] = x;
	param[1] = y;

	int res = sceI2cMasterTransmit(0x42, param, 2);
	if (res < 0)
	{
		return res;
	}

	return 0;
}

int sceDveUnk11()
{
	return dve_struct[1];
}

int sceDveUnk12(u32 x)
{
	if (x >= 3 || x == 1)
		return 0x80000107;

	int previous = dve_struct[3];
	dve_struct[3] = x;

	return previous;
}

int sceDveGetDisplayMode()
{
	return dve_struct[2];
}

int sceDveDisplayOn()
{
	u8 *p;

	if (dve_struct[3] == 0)
	{
		p = table7C;
	}
	else if (dve_struct[3] == 2)
	{
		p = table88;
	}
	else
	{
		return 0x80000107;
	}

	return process_table(p);
}

int sceDveDisplayOff()
{
	return process_table(table74);
}

int module_start(SceSize args, void *argp)
{	
	return (sceDveInit < 0);
}

int module_stop(SceSize args, void *argp)
{
	if (sceDveUnk1() == 0 && dve_struct[8] != 0)
	{
		sceDveUnk6(0);
	}
	
	return 0;
}

