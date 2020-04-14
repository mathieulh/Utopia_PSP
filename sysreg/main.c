// Utopia Project
// 1.50 sysreg.prx clone
// SilverSpring Oct 2007

// not yet fully tested

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspsysevent.h>

#include "sysreg.h"

PSP_MODULE_INFO("sceSYSREG_Driver", 0x1007, 1, 1);

#define SYNC()    asm("sync");

// NOTE: need a proper psperror header file
#define SCE_ERROR_NOT_SUPPORTED	    (0x80000004)
#define SCE_ERROR_INVALID_INDEX     (0x80000102)
#define SCE_ERROR_INVALID_MODE      (0x80000107)
#define SCE_ERROR_INVALID_VALUE     (0x800001FE)

// make sure does not default to 'double'
#define PLL_BASE_FREQ    ((float)37.0)

const float pll_table[0x10] =
{
    1/9.0,    // 0.1...
    4/9.0,    // 0.4...
    4/7.0,    // 0.571428...
    6/9.0,    // 0.6...
    4/5.0,    // 0.8
    9/9.0,    // 1.0
    0.0,
    0.0,

    1/18.0,   // 0.05...
    4/18.0,   // 0.2...
    4/14.0,   // 0.285714...
    6/18.0,   // 0.3...
    4/10.0,   // 0.4
    9/18.0,   // 0.5
    0.0,
    0.0
};

//////////////////////////////////////////////////////////////////////
//                  PROTOTYPES                                      //
//////////////////////////////////////////////////////////////////////

inline int GetCPUID(void);
int SysEventHandler(int ev_id, char* ev_name, void* param, int* result);
void unk_40(void);
int sysreg_suspend(void);
int sysreg_resume(void);
int reset_enable(u32 devmask, int enable);
int bus_clk_enable(u32 devmask, int enable);
int clk_enable_1(u32 devmask, int enable);
int clk_enable_2(u32 devmask, int enable);
int io_enable(u32 devmask, int enable);
int gpio_io_enable(int gpio, int enable);
int unk_io_enable(u32 devmask, int enable);
int intr_handler(int unk0, int unk1, int unk2);
int enable_intr(int unk, int intr);
int disable_intr(int unk, int intr);
int disable_intr_with_status(int unk, int intr, int *stat);
int enable_intr_with_offset(int unk, int intr, int offset);
int get_global_intr_stat(int unk, int intr);

// .data
PspSysEventHandler g_event_handler =
{
    sizeof(PspSysEventHandler),
    "SceSysreg",
    0x00FFFF00,
    SysEventHandler
};

int g_tachyon_ver = -1;            // 0x40
u32 g_var_44 = 0x2C;               // 0x44
u32 g_var_48 = 0;                  // 0x48
u32 g_var_4C = 0;                  // 0x4C
u32 g_var_50 = 0;                  // 0x50
u32 g_var_54 = 0;                  // 0x54

// intr funcs
u32 (* g_func_58)(int, int)        = (void*)disable_intr;                // 0x58
u32 (* g_func_5C)(int, int)        = (void*)enable_intr;                 // 0x5C
u32 (* g_func_60)(int, int)        = (void*)disable_intr;                // 0x60
u32 (* g_func_64)(int, int, int *) = (void*)disable_intr_with_status;    // 0x64
u32 (* g_func_68)(int, int, int)   = (void*)enable_intr_with_offset;     // 0x68
u32 (* g_func_6C)(int, int)        = (void*)get_global_intr_stat;        // 0x6C

// arg2 for sceKernelRegisterIntrHandler
u32 g_intr_handler_arg2[3] = {0x0C, 0x20, 0x44};

// .bss
float g_pll_freq;

struct
{
    u32 mask;
    int bit; // 0 - 31
} g_intr;




//////////////////////////////////////////////////////////////////////
//                  INIT                                            //
//////////////////////////////////////////////////////////////////////

// this should be put in a common header file
inline int GetCPUID()
{
   int id;
   asm(
   "mfc0 %0, $22\n"
   : "=r"(id)
   );
   return id;
}

int SysEventHandler(int ev_id, char* ev_name, void* param, int* result)
{
    return 0;
}

int sceSysregInit()
{
    sceKernelUnregisterSysEventHandler(&g_event_handler);
    sceSysregIntrInit();
    return 0;
}

int sceSysregEnd()
{
    sceSysregIntrEnd();
    return 0;
}

// not called by anything anyway
void unk_40()
{
    // jr $ra
    // addiu $sp, 0x10
    return;
}

int sysreg_suspend()
{
    return 0;
}

int sysreg_resume()
{
    return 0;
}

int sceSysregDoTimerEvent(int bit, int mode)
{
    if (bit > 1) return(SCE_ERROR_INVALID_INDEX);
    if (mode > 3) return(SCE_ERROR_INVALID_MODE);

    u32 intr = sceKernelCpuSuspendIntr();

    if (mode == 0)
    {
        *(volatile u32 *)(0xBC10003C) &= ~(1<<bit);
    }
    else if (mode == 1)
    {
        *(volatile u32 *)(0xBC10003C) |=  (1<<bit);
    }
    else if (mode == 2)
    {
        *(volatile u32 *)(0xBC10003C) |=  (1<<bit);
        *(volatile u32 *)(0xBC10003C) &= ~(1<<bit);
    }
    else if (mode == 3)
    {
        *(volatile u32 *)(0xBC10003C) &= ~(1<<bit);
        *(volatile u32 *)(0xBC10003C) |=  (1<<bit);
    }

    sceKernelCpuResumeIntrWithSync(intr);

    return 0;
}

int sceSysreg_driver_E88B77ED(int mode)
{
    int prev=0;
    u32 intr = sceKernelCpuSuspendIntr();

    if (mode)
    {
        prev = *(volatile u32 *)(0xBC10003C) & 1;
        *(volatile u32 *)(0xBC10003C) |= 1;
    }
    else
    {
        *(volatile u32 *)(0xBC10003C) &= ~1;
    }

    sceKernelCpuResumeIntrWithSync(intr);

    return(prev);
}

int sceSysregGetTachyonVersion()
{
    if (g_tachyon_ver != -1)
        return(g_tachyon_ver);

    u32 intr = sceKernelCpuSuspendIntr();

    // on devkit?
    if (*(volatile u32*)(0xBC100068) & 0xFFFF0000)
    {
        // date of devkit preipl?
        // retail preipl date: 0x20040420
        g_tachyon_ver = (*(volatile u32*)(0xBFC00FFC) > 0x20040224) ? 0x00010000 : 0;
    }
    else
    {
        if (*(u32*)(0xBC100040) & 0xFF000000) // single mem read, no need to volatile
            g_tachyon_ver = (*(u32*)(0xBC100040) & 0xFF000000) >> 8;
        else
            g_tachyon_ver = 0x00100000;

    }

    sceKernelCpuResumeIntr(intr);

    return(g_tachyon_ver);
}

// interrupt devices
// mask = 1 to interrupt ME
int sceSysreg_driver_844AF6BD(u32 mask, int enable)
{
    SYNC();

    if (enable)
        *(volatile u32*)(0xBC000044) |=  mask;
    else
        *(volatile u32*)(0xBC000044) &= ~mask;

    SYNC();
    return 0;
}

u64 sceSysreg_driver_4F46EEDE()
{
    //v0 = *0xBC100090;
    //v1 = *0xBC100094;
    return(*(volatile u64 *)(0xBC100090));
}

u32 sceSysreg_driver_8F4F4E96()
{
    return(*(volatile u32 *)(0xBC100098));
}


//////////////////////////////////////////////////////////////////////
//                  RESET ENABLE                                    //
//////////////////////////////////////////////////////////////////////

int reset_enable(u32 devmask, int enable)
{
    u32 intr = sceKernelCpuSuspendIntr();

    int prev = ((SYSREG_RESET_ENABLE_REG & devmask) > 0) ? SYSREG_ENABLE : SYSREG_DISABLE;

    if (enable)
        SYSREG_RESET_ENABLE_REG |=  devmask;
    else
        SYSREG_RESET_ENABLE_REG &= ~devmask;

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregTopResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_TOP), SYSREG_ENABLE));
}

int sceSysregScResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_SC), SYSREG_ENABLE));
}

int sceSysregMeResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_ME), SYSREG_ENABLE));
}

int sceSysregMeResetDisable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_ME), SYSREG_DISABLE));
}

int sceSysregAwResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_AW), SYSREG_ENABLE));
}

int sceSysregAwResetDisable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_AW), SYSREG_DISABLE));
}

int sceSysregVmeResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_VME), SYSREG_ENABLE));
}

int sceSysregVmeResetDisable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_VME), SYSREG_DISABLE));
}

int sceSysregAvcResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_AVC), SYSREG_ENABLE));
}

int sceSysregAvcResetDisable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_AVC), SYSREG_DISABLE));
}

int sceSysregUsbResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_USB), SYSREG_ENABLE));
}

int sceSysregUsbResetDisable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_USB), SYSREG_DISABLE));
}

int sceSysregAtaResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_ATA), SYSREG_ENABLE));
}

int sceSysregAtaResetDisable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_ATA), SYSREG_DISABLE));
}

int sceSysregMsifResetEnable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(reset_enable((SET_MASK(SYSREG_BUSCLK_MSIF0))<<no, SYSREG_ENABLE));
}

int sceSysregMsifResetDisable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(reset_enable((SET_MASK(SYSREG_BUSCLK_MSIF0))<<no, SYSREG_DISABLE));
}

int sceSysregKirkResetEnable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_KIRK), SYSREG_ENABLE));
}

int sceSysregKirkResetDisable()
{
    return(reset_enable(SET_MASK(SYSREG_RESET_KIRK), SYSREG_DISABLE));
}

//////////////////////////////////////////////////////////////////////
//                  BUS CLOCK ENABLE                                //
//////////////////////////////////////////////////////////////////////

int bus_clk_enable(u32 devmask, int enable)
{
    u32 intr = sceKernelCpuSuspendIntr();

    int prev = ((SYSREG_BUSCLK_ENABLE_REG & devmask) > 0) ? SYSREG_ENABLE : SYSREG_DISABLE;

    if (enable)
        SYSREG_BUSCLK_ENABLE_REG |=  devmask;
    else
        SYSREG_BUSCLK_ENABLE_REG &= ~devmask;

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregMeBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_ME), SYSREG_ENABLE));
}

int sceSysregMeBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_ME), SYSREG_DISABLE));
}

int sceSysregAwRegABusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_AWA), SYSREG_ENABLE));
}

int sceSysregAwRegABusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_AWA), SYSREG_DISABLE));
}

int sceSysregAwRegBBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_AWB), SYSREG_ENABLE));
}

int sceSysregAwRegBBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_AWB), SYSREG_DISABLE));
}

int sceSysregAwEdramBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_EDRAM), SYSREG_ENABLE));
}

int sceSysregAwEdramBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_EDRAM), SYSREG_DISABLE));
}

int sceSysregDmacplusBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_DMACPLUS), SYSREG_ENABLE));
}

int sceSysregDmacplusBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_DMACPLUS), SYSREG_DISABLE));
}

int sceSysregDmacBusClockEnable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(bus_clk_enable((SET_MASK(SYSREG_BUSCLK_DMAC0))<<no, SYSREG_ENABLE));
}

int sceSysregDmacBusClockDisable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(bus_clk_enable((SET_MASK(SYSREG_BUSCLK_DMAC0))<<no, SYSREG_DISABLE));
}

int sceSysregKirkBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_KIRK), SYSREG_ENABLE));
}

int sceSysregKirkBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_KIRK), SYSREG_DISABLE));
}

int sceSysregAtaBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_ATA), SYSREG_ENABLE));
}

int sceSysregAtaBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_ATA), SYSREG_DISABLE));
}

int sceSysregUsbBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_USB), SYSREG_ENABLE));
}

int sceSysregUsbBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_USB), SYSREG_DISABLE));
}

int sceSysregMsifBusClockEnable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);
        
    return(bus_clk_enable((SET_MASK(SYSREG_BUSCLK_MSIF0))<<no, SYSREG_ENABLE));
}

int sceSysregMsifBusClockDisable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(bus_clk_enable((SET_MASK(SYSREG_BUSCLK_MSIF0))<<no, SYSREG_DISABLE));
}

int sceSysregEmcddrBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_EMCDDR), SYSREG_ENABLE));
}

int sceSysregEmcddrBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_EMCDDR), SYSREG_DISABLE));
}

int sceSysregEmcsmBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_EMCSM), SYSREG_ENABLE));
}

int sceSysregEmcsmBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_EMCSM), SYSREG_DISABLE));
}

int sceSysregApbBusClockEnable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_APB), SYSREG_ENABLE));
}

int sceSysregApbBusClockDisable()
{
    return(bus_clk_enable(SET_MASK(SYSREG_BUSCLK_APB), SYSREG_DISABLE));
}

int sceSysregAudioBusClockEnable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(bus_clk_enable((SET_MASK(SYSREG_BUSCLK_AUDIO0))<<no, SYSREG_ENABLE));
}

int sceSysregAudioBusClockDisable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(bus_clk_enable((SET_MASK(SYSREG_BUSCLK_AUDIO0))<<no, SYSREG_DISABLE));
}

//////////////////////////////////////////////////////////////////////
//                  CLOCK ENABLE 1                                  //
//////////////////////////////////////////////////////////////////////

int clk_enable_1(u32 devmask, int enable)
{
    u32 intr = sceKernelCpuSuspendIntr();

    int prev = ((SYSREG_CLK1_ENABLE_REG & devmask) > 0) ? SYSREG_ENABLE : SYSREG_DISABLE;

    if (enable)
        SYSREG_CLK1_ENABLE_REG |=  devmask;
    else
        SYSREG_CLK1_ENABLE_REG &= ~devmask;

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregMsifClkEnable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);
        
    return(clk_enable_1((SET_MASK(SYSREG_CLK1_MSIF0))<<no, SYSREG_ENABLE));
}

int sceSysregMsifClkDisable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(clk_enable_2((SET_MASK(SYSREG_CLK1_MSIF0))<<no, SYSREG_DISABLE));
}

int sceSysregAtaClkEnable()
{
    return(clk_enable_1(SET_MASK(SYSREG_CLK1_ATA), SYSREG_ENABLE));
}

int sceSysregAtaClkDisable()
{
    return(clk_enable_1(SET_MASK(SYSREG_CLK1_ATA), SYSREG_DISABLE));
}

int sceSysregUsbClkEnable(int no)
{
    return(clk_enable_1((no&0xF)<<SYSREG_CLK1_USB, SYSREG_ENABLE));
}

int sceSysregUsbClkDisable(int no)
{
    return(clk_enable_1((no&0xF)<<SYSREG_CLK1_USB, SYSREG_DISABLE));
}

//////////////////////////////////////////////////////////////////////
//                  CLOCK ENABLE 2                                  //
//////////////////////////////////////////////////////////////////////

int clk_enable_2(u32 devmask, int enable)
{
    u32 intr = sceKernelCpuSuspendIntr();

    int prev = ((SYSREG_CLK2_ENABLE_REG & devmask) > 0) ? SYSREG_ENABLE : SYSREG_DISABLE;

    if (enable)
        SYSREG_CLK2_ENABLE_REG |=  devmask;
    else
        SYSREG_CLK2_ENABLE_REG &= ~devmask;

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregSpiClkEnable(int no)
{
    if (no > 5)
        return(SCE_ERROR_INVALID_INDEX);
        
    return(clk_enable_2((SET_MASK(SYSREG_CLK2_SPI0))<<no, SYSREG_ENABLE));
}

int sceSysregSpiClkDisable(int no)
{
    if (no > 5)
        return(SCE_ERROR_INVALID_INDEX);
        
    return(clk_enable_2((SET_MASK(SYSREG_CLK2_SPI0))<<no, SYSREG_DISABLE));
}

int sceSysregUartClkEnable(int no)
{
    if (no > 5)
        return(SCE_ERROR_INVALID_INDEX);

    return(clk_enable_2((SET_MASK(SYSREG_CLK2_UART0))<<no, SYSREG_ENABLE));
}

int sceSysregUartClkDisable(int no)
{
    if (no > 5)
        return(SCE_ERROR_INVALID_INDEX);

    return(clk_enable_2((SET_MASK(SYSREG_CLK2_UART0))<<no, SYSREG_DISABLE));
}

int sceSysregApbTimerClkEnable(int no)
{
    if (no > 3)
        return(SCE_ERROR_INVALID_INDEX);

    return(clk_enable_2((SET_MASK(SYSREG_CLK2_APB0))<<no, SYSREG_ENABLE));
}

int sceSysregApbTimerClkDisable(int no)
{
    if (no > 3)
        return(SCE_ERROR_INVALID_INDEX);

    return(clk_enable_2((SET_MASK(SYSREG_CLK2_APB0))<<no, SYSREG_DISABLE));
}

int sceSysregAudioClkEnable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(clk_enable_2((SET_MASK(SYSREG_CLK2_AUDIO0))<<no, SYSREG_ENABLE));
}

int sceSysregAudioClkDisable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(clk_enable_2((SET_MASK(SYSREG_CLK2_AUDIO0))<<no, SYSREG_DISABLE));
}

int sceSysregLcdcClkEnable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_LCDC), SYSREG_ENABLE));
}

int sceSysregLcdcClkDisable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_LCDC), SYSREG_DISABLE));
}

int sceSysregPwmClkEnable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_PWM), SYSREG_ENABLE));
}

int sceSysregPwmClkDisable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_PWM), SYSREG_DISABLE));
}

int sceSysregKeyClkEnable()
{
    return(SCE_ERROR_NOT_SUPPORTED);
}

int sceSysregKeyClkDisable()
{
    return(SCE_ERROR_NOT_SUPPORTED);
}

int sceSysregIicClkEnable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_IIC), SYSREG_ENABLE));
}

int sceSysregIicClkDisable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_IIC), SYSREG_DISABLE));
}

int sceSysregSircsClkEnable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_SIRCS), SYSREG_ENABLE));
}

int sceSysregSircsClkDisable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_SIRCS), SYSREG_DISABLE));
}

int sceSysregGpioClkEnable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_GPIO), SYSREG_ENABLE));
}

int sceSysregGpioClkDisable()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_GPIO), SYSREG_DISABLE));
}

int sceSysreg_driver_C1DA05D2()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_UNK), SYSREG_ENABLE));
}

int sceSysreg_driver_DE170397()
{
    return(clk_enable_2(SET_MASK(SYSREG_CLK2_UNK), SYSREG_DISABLE));
}

//////////////////////////////////////////////////////////////////////
//                  CLOCK SELECT                                    //
//////////////////////////////////////////////////////////////////////

int sceSysregMsifClkSelect(int no, int clk)
{
    if (no > 1)
         return(SCE_ERROR_INVALID_INDEX);
    if (clk > 3)
        return(SCE_ERROR_INVALID_VALUE);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(u32*)(0xBC10005C)>>(no<<1)) & 3;
    // only one mem write, no need to volatile
    *(u32*)(0xBC10005C) &= ~(3<<(no<<1));
    *(u32*)(0xBC10005C) |= (clk<<(no<<1));

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregMsifDelaySelect(int no, int delay)
{
    if (no > 1)
         return(SCE_ERROR_INVALID_INDEX);
    if (delay > 7)
        return(SCE_ERROR_INVALID_VALUE);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(u32*)(0xBC000050)>>(no<<2)) & 7;
    // only one mem write, no need to volatile
    *(u32*)(0xBC000050) &= ~(7<<(no<<2));
    *(u32*)(0xBC000050) |= (delay<<(no<<2));

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregApbTimerClkSelect(int no, int clk)
{
    if (no > 3)
         return(SCE_ERROR_INVALID_INDEX);
    if (clk > 7)
        return(SCE_ERROR_INVALID_VALUE);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(u32*)(0xBC100060)>>(no<<2)) & 7;
    // only one mem write, no need to volatile
    *(u32*)(0xBC100060) &= ~(7<<(no<<2));
    *(u32*)(0xBC100060) |= (clk<<(no<<2));

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregAudioClkSelect(int no, int clk)
{
    if (no > 1)
         return(SCE_ERROR_INVALID_INDEX);
    if (clk > 1)
        return(SCE_ERROR_INVALID_VALUE);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(u32*)(0xBC100060)>>(no<<0x10)) & 1;
    // only one mem write, no need to volatile
    *(u32*)(0xBC100060) &= ~(1<<(no<<0x10));
    *(u32*)(0xBC100060) |= (clk<<(no<<0x10));

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

// why ONLY this function give 'warning' ?????
// and not the others
int sceSysreg_driver_0A83FC7B(int clk)
{
    if (clk > 1)
         return(SCE_ERROR_INVALID_VALUE);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(u32*)(0xBC100060) >> 18) & 1;
    // only one mem write, no need to volatile
    *(u32*)(0xBC100060) &= ~(1<<18);
    *(u32*)(0xBC100060) |= (clk<<18);

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregSpiClkSelect(int no, int clk)
{
    if (no > 5)
         return(SCE_ERROR_INVALID_INDEX);
    if (clk > 7)
        return(SCE_ERROR_INVALID_VALUE);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(u32*)(0xBC100064)>>(no<<2)) & 7;
    // only one mem write, no need to volatile
    *(u32*)(0xBC100064) &= ~(7<<(no<<2));
    *(u32*)(0xBC100064) |= (clk<<(no<<2));

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregLcdcClkSelect(int no1, int no2)
{
    if ((no1 > 1) || (no2 > 2))
         return(SCE_ERROR_INVALID_INDEX);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(u32*)(0xBC100060)>>20) & 7;
    // only one mem write, no need to volatile
    *(u32*)(0xBC100060) &= ~(1<<20  | 1<<21  | 1<<22);
    *(u32*)(0xBC100060) |= ((no1<<22) | (no2<<20));

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregAtaClkSelect(int clk)
{
    if (clk > 2)
        return(SCE_ERROR_INVALID_VALUE);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(u32*)(0xBC10005C)>>4) & 3;
    // only one mem write, no need to volatile
    *(u32*)(0xBC10005C) &= ~(1<<4 | 1<<5);
    *(u32*)(0xBC10005C) |= (clk<<4);

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

//////////////////////////////////////////////////////////////////////
//                  IO ENABLE                                       //
//////////////////////////////////////////////////////////////////////

int io_enable(u32 devmask, int enable)
{
    u32 intr = sceKernelCpuSuspendIntr();

    int prev = ((SYSREG_IO_ENABLE_REG & devmask) > 0) ? SYSREG_ENABLE : SYSREG_DISABLE;

    if (enable)
        SYSREG_IO_ENABLE_REG |=  devmask;
    else
        SYSREG_IO_ENABLE_REG &= ~devmask;

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregEmcsmIoEnable()
{
    return(io_enable(SET_MASK(SYSREG_IO_EMCSM), SYSREG_ENABLE));
}

int sceSysregEmcsmIoDisable()
{
    return(io_enable(SET_MASK(SYSREG_IO_EMCSM), SYSREG_DISABLE));
}

int sceSysregUsbIoEnable()
{
    return(io_enable(SET_MASK(SYSREG_IO_USB), SYSREG_ENABLE));
}

int sceSysregUsbIoDisable()
{
    return(io_enable(SET_MASK(SYSREG_IO_USB), SYSREG_DISABLE));
}

int sceSysregAtaIoEnable()
{
    return(io_enable(SET_MASK(SYSREG_IO_ATA), SYSREG_ENABLE));
}

int sceSysregAtaIoDisable()
{
    return(io_enable(SET_MASK(SYSREG_IO_ATA), SYSREG_DISABLE));
}

int sceSysregMsifIoEnable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(io_enable(SET_MASK(SYSREG_IO_MSIF0)<<no, SYSREG_ENABLE));
}

int sceSysregMsifIoDisable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(io_enable(SET_MASK(SYSREG_IO_MSIF0)<<no, SYSREG_DISABLE));
}

int sceSysregLcdcIoEnable()
{
    return(io_enable(SET_MASK(SYSREG_IO_LCDC), SYSREG_ENABLE));
}

int sceSysregLcdcIoDisable()
{
    return(io_enable(SET_MASK(SYSREG_IO_LCDC), SYSREG_DISABLE));
}

int sceSysregAudioIoEnable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(io_enable(SET_MASK(SYSREG_IO_AUDIO0)<<no, SYSREG_ENABLE));
}

int sceSysregAudioIoDisable(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(io_enable(SET_MASK(SYSREG_IO_AUDIO0)<<no, SYSREG_DISABLE));
}

int sceSysregIicIoEnable()
{
    return(io_enable(SET_MASK(SYSREG_IO_IIC), SYSREG_ENABLE));
}

int sceSysregIicIoDisable()
{
    return(io_enable(SET_MASK(SYSREG_IO_IIC), SYSREG_DISABLE));
}

int sceSysregSircsIoEnable()
{
    return(io_enable(SET_MASK(SYSREG_IO_SIRCS), SYSREG_ENABLE));
}

int sceSysregSircsIoDisable()
{
    return(io_enable(SET_MASK(SYSREG_IO_SIRCS), SYSREG_DISABLE));
}

int sceSysreg_driver_F844DDF3()
{
    return(io_enable(SET_MASK(SYSREG_IO_UNK), SYSREG_ENABLE));
}

int sceSysreg_driver_29A119A1()
{
    return(io_enable(SET_MASK(SYSREG_IO_UNK), SYSREG_DISABLE));
}

int sceSysregKeyIoEnable()
{
    return(SCE_ERROR_NOT_SUPPORTED);
}

int sceSysregKeyIoDisable()
{
    return(SCE_ERROR_NOT_SUPPORTED);
}

int sceSysregPwmIoEnable()
{
    return(io_enable(SET_MASK(SYSREG_IO_PWM), SYSREG_ENABLE));
}

int sceSysregPwmIoDisable()
{
    return(io_enable(SET_MASK(SYSREG_IO_PWM), SYSREG_DISABLE));
}

int sceSysregUartIoEnable(int no)
{
    if (no > 5)
        return(SCE_ERROR_INVALID_INDEX);

    return(io_enable(SET_MASK(SYSREG_IO_UART0)<<no, SYSREG_ENABLE));
}

int sceSysregUartIoDisable(int no)
{
    if (no > 5)
        return(SCE_ERROR_INVALID_INDEX);

    return(io_enable(SET_MASK(SYSREG_IO_UART0)<<no, SYSREG_DISABLE));
}

int sceSysregSpiIoEnable(int no)
{
    if (no > 5)
        return(SCE_ERROR_INVALID_INDEX);

    return(io_enable(SET_MASK(SYSREG_IO_SPI0)<<no, SYSREG_ENABLE));
}

int sceSysregSpiIoDisable(int no)
{
    if (no > 5)
        return(SCE_ERROR_INVALID_INDEX);

    return(io_enable(SET_MASK(SYSREG_IO_SPI0)<<no, SYSREG_DISABLE));
}

//////////////////////////////////////////////////////////////////////
//                  GPIO IO ENABLE                                  //
//////////////////////////////////////////////////////////////////////

int gpio_io_enable(int gpio, int enable)
{
    if (gpio > 31)
        return(SCE_ERROR_INVALID_INDEX);

    u32 intr = sceKernelCpuSuspendIntr();

    if (enable)
        SYSREG_GPIO_ENABLE_REG |=  SET_MASK(gpio);
    else
        SYSREG_GPIO_ENABLE_REG &= ~SET_MASK(gpio);

    sceKernelCpuResumeIntr(intr);

    return((SYSREG_GPIO_ENABLE_REG >> gpio) & 1);
}

int sceSysregGpioIoEnable(int gpio)
{
    return(gpio_io_enable(gpio, SYSREG_ENABLE));
}

int sceSysregGpioIoDisable(int gpio)
{
    return(gpio_io_enable(gpio, SYSREG_DISABLE));
}

//////////////////////////////////////////////////////////////////////
//                  UNK IO ENABLE                                   //
//////////////////////////////////////////////////////////////////////

int unk_io_enable(u32 devmask, int enable)
{
    u32 intr = sceKernelCpuSuspendIntr();

    int prev = ((SYSREG_UNK_IO_ENABLE_REG & devmask) > 0) ? SYSREG_ENABLE : SYSREG_DISABLE;

    if (enable)
        SYSREG_UNK_IO_ENABLE_REG |=  devmask;
    else
        SYSREG_UNK_IO_ENABLE_REG &= ~devmask;

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysreg_driver_2112E686(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(unk_io_enable(1<<(no<<2), SYSREG_ENABLE));
}

int sceSysreg_driver_55B18B84(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);
        
    return(unk_io_enable(1<<(no<<2), SYSREG_DISABLE));
}

int sceSysreg_driver_7BDF0556(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return(unk_io_enable(2<<(no<<2), SYSREG_ENABLE));
}

int sceSysreg_driver_7B9E9A53(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);
        
    return(unk_io_enable(2<<(no<<2), SYSREG_DISABLE));
}

int sceSysreg_driver_8D0FED1E()
{
    return(unk_io_enable(0x100, SYSREG_ENABLE));
}

int sceSysreg_driver_A46E9CA8()
{
    return(unk_io_enable(0x100, SYSREG_DISABLE));
}

//////////////////////////////////////////////////////////////////////
//                  MSIF/USB STATUS                                 //
//////////////////////////////////////////////////////////////////////

int sceSysregMsifGetConnectStatus(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return((*(volatile u32*)(0xBC100080) >> (8+no*8)) & 1);
}

int sceSysreg_driver_BF91FBDA(int no)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    return((*(volatile u32*)(0xBC100080) >> (9+no*8)) & 3);
}

int sceSysreg_driver_36A75390(int no, int mask)
{
    if (no > 1)
        return(SCE_ERROR_INVALID_INDEX);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(volatile u32*)(0xBC100080) >> (9+no*8)) & 3;
    *(volatile u32*)(0xBC100080) = (mask & prev) << (9+no*8);

    sceKernelCpuResumeIntrWithSync(intr);

    return(prev);
}

int sceSysregUsbGetConnectStatus()
{
    return(*(volatile u32*)(0xBC100080) & 1);
}

int sceSysregUsbQueryIntr()
{
    return((*(volatile u32*)(0xBC100080)>>1) & 0xF);
}

int sceSysregUsbAcquireIntr(int mask)
{
    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (*(volatile u32*)(0xBC100080)>>1) & 0xF;
    *(volatile u32*)(0xBC100080) = (mask & prev) << 1;

    sceKernelCpuResumeIntrWithSync(intr);

    return(prev);
}

//////////////////////////////////////////////////////////////////////
//                  INTERRUPT                                       //
//////////////////////////////////////////////////////////////////////

int sceSysregIntrInit()
{
    sceKernelRegisterIntrHandler(0x1F, 1, (void*)intr_handler, NULL, (void*)g_intr_handler_arg2);
    sceKernelEnableIntr(0x1F);
    return 0;
}

int sceSysregIntrEnd()
{
    sceKernelReleaseIntrHandler(0x1F);
    return 0;
}

int sceSysregEnableIntr(int intrr)
{
    if (intrr > 31)
        return(SCE_ERROR_INVALID_INDEX);
        
    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (g_intr.mask >> intrr) & 1;

    g_intr.mask |= (1<<intrr);

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregDisableIntr(int intrr)
{
    if (intrr > 31)
        return(SCE_ERROR_INVALID_INDEX);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = (g_intr.mask >> intrr) & 1;

    g_intr.mask &= ~(1<<intrr);

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

int sceSysregRequestIntr(int cpu, int intr)
{
    if ((intr > 31) || (cpu > 1))
        return(SCE_ERROR_INVALID_INDEX);
        
    while(sceSysregSemaTryLock() < 0);

    *(volatile u32*)(0xBFC00400 + cpu*4) |= (1<<intr);

    sceSysregSemaUnlock();

    if (cpu != GetCPUID())
        sceSysregInterruptToOther();

    return 0;
}

int sceSysregInterruptToOther()
{
    SYNC();
    *(volatile u32*)0xBC100044 = 1;
    SYNC();
    return 0;
}

int sceSysregSemaTryLock()
{
    SYNC();
    int cpuid = GetCPUID();
    *(volatile u32*)0xBC100048 = cpuid+1;
    return(((*(volatile u32*)0xBC100048 & 3) > (cpuid+1)) ? -1 : 0);
}

void sceSysregSemaUnlock()
{
    SYNC();
    *(volatile u32*)0xBC100048 = 0;
    SYNC();
}

inline int get_next_intr(u32 mask)
{
    int i;

    for(i = g_intr.bit - 1; !((mask & g_intr.mask) & (1 << i)); i--)
    {
        if(i < 0)
            i = 31;
    }

    return(i);
}

//#define ROT(x,s)    ((((x ) >> s) | ((x ) << (32 - s))) & 0xffffffff)
//#define CLZ(x)      

int intr_handler(int unk0, int unk1, int unk2)
{
    int cpu = GetCPUID();
    u32 *mask = (u32*)0xBFC00400;

    if (mask[cpu] & g_intr.mask)
    {
        // returns the position of the next lowest set bit below g_intr.bit
        //int subIntr = (g_intr.bit + 31-CLZ(ROT(mask[cpu], g_intr.bit))) & 0x1F;
        int subIntr = get_next_intr(mask[cpu]);

        while(sceSysregSemaTryLock() < 0);

        mask[cpu] &= ~(1<<subIntr);

        sceSysregSemaUnlock();

        g_intr.bit++;
        g_intr.bit &= 0x1F;
    
        sceKernelCallSubIntrHandler(unk0, subIntr, subIntr, unk2);
    }

    return -1;
}

int enable_intr(int unk, int intr)
{
    g_intr.mask |= 1<<intr;
    return 0;
}

int disable_intr(int unk, int intr)
{
    g_intr.mask &= ~(1<<intr);
    return 0;
}

int disable_intr_with_status(int unk, int intr, int *stat)
{
    if (stat)
        *stat = (g_intr.mask>>intr) & 1;

    g_intr.mask &= ~(1<<intr);
    return 0;
}

int enable_intr_with_offset(int unk, int intr, int offset)
{
    g_intr.mask |= (offset<<intr);
    return 0;
}

int get_global_intr_stat(int unk, int intr)
{
    int stat = *(volatile u32*)(0xBFC0400+(GetCPUID()*4)) >> intr;
    return(stat & 1);
}

//////////////////////////////////////////////////////////////////////
//                  CLOCK FREQUENCY                                 //
//////////////////////////////////////////////////////////////////////

float sceSysregPllUpdateFrequency()
{
    int mult = (*(volatile u32 *)0xBC1000FC>>8) & 8;
    int index = *(volatile u32 *)0xBC100068 & 0xF;

    g_pll_freq = (mult*PLL_BASE_FREQ) * pll_table[index];

    return(g_pll_freq);
}

// get pll index
int sceSysreg_driver_B4560C45()
{
    return(*(volatile u32*)0xBC100068 & 0xF);
}

// set pll index
int sceSysreg_driver_DCA57573(int index)
{
    if ((index < 0) || ((index & 7) > 5))
        return(SCE_ERROR_INVALID_VALUE);

    u32 intr = sceKernelCpuSuspendIntr();

    int prev = *(u32*)0xBC100068 & 0xF;
    // only one mem write, no need to volatile
    *(u32*)0xBC100068 &= 0xFFFFFFF0;
    *(u32*)0xBC100068 |= (index | 0x80);
    
    sceSysregPllUpdateFrequency();

    sceKernelCpuResumeIntr(intr);

    return(prev);
}

float sceSysregPllGetBaseFrequency()
{
    return(PLL_BASE_FREQ);
}

float sceSysregPllGetFrequency()
{
    return(g_pll_freq);
}

// set cpu freq ratio (ratio of pll freq)
int sceSysreg_driver_5664F8B5(int num, int den)
{
    if ((num > 0x1FF) || (den > 0x1FF))
        return(SCE_ERROR_INVALID_VALUE);

    if (num > den)
        return(SCE_ERROR_INVALID_VALUE);
        
    *(volatile u32*)0xBC200000 = ((num&0x1FF)<<16) | (den&0x1FF);

    SYNC();
    return 0;
}

// get cpu freq ratio (ratio of pll freq)
int sceSysreg_driver_44704E1D(int *num, int *den)
{
    if (num)
        *num = (*(volatile u32*)0xBC200000>>16) & 0x1FF;
        
    if (den)
        *den = *(volatile u32*)0xBC200000 & 0x1FF;

    return 0;
}

// set bus freq ratio (ratio of pll freq)
int sceSysreg_driver_584AD989(int num, int den)
{
    if ((num > 0x1FF) || (den > 0x1FF))
        return(SCE_ERROR_INVALID_VALUE);

    if (num > den)
        return(SCE_ERROR_INVALID_VALUE);
        
    *(volatile u32*)0xBC200004 = ((num&0x1FF)<<16) | (den&0x1FF);

    SYNC();
    return 0;
}

// get bus freq ratio (ratio of pll freq)
int sceSysreg_driver_377F035F(int *num, int *den)
{
    if (num)
        *num = (*(volatile u32*)0xBC200004>>16) & 0x1FF;
        
    if (den)
        *den = *(volatile u32*)0xBC200004 & 0x1FF;

    return 0;
}

// set cpu freq
int sceSysreg_driver_AB3185FD(float freq)
{
    int numer = (int)((float)(511.0) * freq/sceSysregPllGetFrequency());

    if (numer < 0)
        numer = 1;
    else if (numer > 511)
        numer = 511;

    *(volatile u32*)(0xBC200000) = ((numer&0x1FF)<<16) | 0x1FF;

    SYNC();
    return 0;
}

// get cpu freq
float sceSysreg_driver_0EA487FA()
{
    // single mem read, no need for volatile
    float ratio = ((float)((*(u32*)(0xBC200000)>>16)&0x1FF)) / ((float)(*(u32*)(0xBC200000)&0x1FF));

    return(sceSysregPllGetFrequency() * ratio);
}

// set bus freq
int sceSysreg_driver_136E8F5A(float freq)
{
    int numer = (int)((float)(1022.0) * freq/sceSysregPllGetFrequency());

    if (numer < 0)
        numer = 1;
    else if (numer > 511)
        numer = 511;

    *(volatile u32*)(0xBC200004) = ((numer&0x1FF)<<16) | 0x1FF;

    SYNC();
    return 0;
}

// get bus freq
float sceSysreg_driver_F4811E00()
{
    // single mem read, no need for volatile
    float ratio = ((float)((*(u32*)(0xBC20004)>>16)&0x1FF)) / ((float)(*(u32*)(0xBC200004)&0x1FF));

    return(sceSysregPllGetFrequency() * ratio);
}


//////////////////////////////////////////////////////////////////////
//                  MODULE START                                    //
//////////////////////////////////////////////////////////////////////

int module_start(SceSize args, void *argp)
{
    sceKernelUnregisterSysEventHandler(&g_event_handler);
    sceSysregIntrInit();
    sceSysregPllUpdateFrequency();
    sceKernelRegisterSysEventHandler(&g_event_handler);
    return 0;
}

int module_reboot_before(SceSize args, void *argp)
{
    sceSysregIntrEnd();
    return 0;
}

