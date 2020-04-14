#ifndef __MODULE_H
#define __MODULE_H

typedef struct SceModule {
	struct SceModule	*next; //0
	unsigned short		attribute; //4
	unsigned char		version[2]; //6
	char			modname[27]; //8
	char			terminal; //23
	unsigned int		unknown1; //24
	unsigned int		unknown2; //28 - another uid?
	SceUID			modid; //2C
	unsigned int            unk_30;
	SceUID                  unk_34; //34 - UID of memory allocated
	unsigned int            unk_38;
	unsigned int            unk_3C;
	void *			ent_top; //40
	unsigned int		ent_size; //44
	void *			stub_top; //48
	unsigned int		stub_size; //4C
	unsigned int            module_start; //50
	unsigned int            module_stop; //54
	unsigned int            module_bootstart; //58
	unsigned int            module_reboot_before; //5C
	unsigned int		entry_addr; //60 - module_reboot_phase??
	unsigned int		gp_value; //64
	unsigned int		text_addr; //68
	unsigned int		text_size; //6C
	unsigned int		data_size; //70
	unsigned int		bss_size; //74
	unsigned int		nsegment; //78
	unsigned char           unk_7C[4];
	unsigned int		segmentaddr[4]; //80
	unsigned int		segmentsize[4]; //90
	unsigned int            unk_A0; //export NID 0x0F7C276C
	unsigned int            unk_A4; //export NID 0x0F7C276C
	unsigned int            unk_A8; //export NID 0x0F7C276C
	unsigned int            unk_AC; //export NID 0xCF0CC697
	unsigned int            unk_B0; //export NID 0xCF0CC697
	unsigned int            unk_B4; //export NID 0xCF0CC697
	unsigned int            unk_B8; //export NID 0xF4F4299D
	unsigned int            unk_BC; //export NID 0xF4F4299D
	unsigned int            unk_C0; //export NID 0xF4F4299D
} SceModule;

#endif