#ifndef __SYSREG_H__

#define __SYSREG_H__

#define SYSREG_ENABLE   (1)
#define SYSREG_DISABLE  (0)

#define SET_MASK(_dev)  (1<<(_dev))

#define SYSREG_RESET_ENABLE_REG     (*(volatile u32 *)(0xBC10004C))
#define SYSREG_BUSCLK_ENABLE_REG    (*(volatile u32 *)(0xBC100050))
#define SYSREG_CLK1_ENABLE_REG      (*(volatile u32 *)(0xBC100054))
#define SYSREG_CLK2_ENABLE_REG      (*(volatile u32 *)(0xBC100058))
#define SYSREG_UNK_IO_ENABLE_REG    (*(volatile u32 *)(0xBC100074))
#define SYSREG_IO_ENABLE_REG        (*(volatile u32 *)(0xBC100078))
#define SYSREG_GPIO_ENABLE_REG      (*(volatile u32 *)(0xBC10007C))

enum SysregResetDevices
{
    SYSREG_RESET_TOP = 0,
    SYSREG_RESET_SC,
    SYSREG_RESET_ME,
    SYSREG_RESET_AW,
    SYSREG_RESET_VME,
    SYSREG_RESET_AVC,
    SYSREG_RESET_USB,
    SYSREG_RESET_ATA,
    SYSREG_RESET_MSIF0,
    SYSREG_RESET_MSIF1,
    SYSREG_RESET_KIRK,
};

enum SysregBusClkDevices
{
    SYSREG_BUSCLK_ME = 0,
    SYSREG_BUSCLK_AWA,
    SYSREG_BUSCLK_AWB,
    SYSREG_BUSCLK_EDRAM,
    SYSREG_BUSCLK_DMACPLUS,
    SYSREG_BUSCLK_DMAC0,
    SYSREG_BUSCLK_DMAC1,
    SYSREG_BUSCLK_KIRK,
    SYSREG_BUSCLK_ATA,
    SYSREG_BUSCLK_USB,
    SYSREG_BUSCLK_MSIF0,
    SYSREG_BUSCLK_MSIF1,
    SYSREG_BUSCLK_EMCDDR,
    SYSREG_BUSCLK_EMCSM,
    SYSREG_BUSCLK_APB,
    SYSREG_BUSCLK_AUDIO0,
    SYSREG_BUSCLK_AUDIO1
};

enum SysregClk1Devices
{
    SYSREG_CLK1_ATA = 0,
    SYSREG_CLK1_USB = 4,
    SYSREG_CLK1_MSIF0 = 8,
    SYSREG_CLK1_MSIF1,
};

enum SysregClk2Devices
{
    SYSREG_CLK2_SPI0 = 0,
    SYSREG_CLK2_SPI1,
    SYSREG_CLK2_SPI2,
    SYSREG_CLK2_SPI3,
    SYSREG_CLK2_SPI4,
    SYSREG_CLK2_SPI5,
    SYSREG_CLK2_UART0,
    SYSREG_CLK2_UART1,
    SYSREG_CLK2_UART2,
    SYSREG_CLK2_UART3,
    SYSREG_CLK2_UART4,
    SYSREG_CLK2_UART5,
    SYSREG_CLK2_APB0,
    SYSREG_CLK2_APB1,
    SYSREG_CLK2_APB2,
    SYSREG_CLK2_APB3,
    SYSREG_CLK2_AUDIO0,
    SYSREG_CLK2_AUDIO1,
    SYSREG_CLK2_LCDC,
    SYSREG_CLK2_PWM,
    SYSREG_CLK2_KEY,
    SYSREG_CLK2_IIC ,
    SYSREG_CLK2_SIRCS,
    SYSREG_CLK2_GPIO,
    SYSREG_CLK2_UNK        // audio related ?
};

enum SysregIoDevices
{
    SYSREG_IO_EMCSM = 1,
    SYSREG_IO_USB,
    SYSREG_IO_ATA,
    SYSREG_IO_MSIF0,
    SYSREG_IO_MSIF1,
    SYSREG_IO_LCDC,
    SYSREG_IO_AUDIO0,
    SYSREG_IO_AUDIO1,
    SYSREG_IO_IIC,
    SYSREG_IO_SIRCS,
    SYSREG_IO_UNK,
    SYSREG_IO_KEY,
    SYSREG_IO_PWM,
    SYSREG_IO_UART0 = 16,
    SYSREG_IO_UART1,
    SYSREG_IO_UART2,
    SYSREG_IO_UART3,
    SYSREG_IO_UART4,
    SYSREG_IO_UART5,
    SYSREG_IO_SPI0 = 24,
    SYSREG_IO_SPI1,
    SYSREG_IO_SPI2,
    SYSREG_IO_SPI3,
    SYSREG_IO_SPI4,
    SYSREG_IO_SPI5
};


int sceSysregInit(void);
int sceSysregEnd(void);
int sceSysreg_driver_E88B77ED(int mode);
int sceSysregDoTimerEvent(int bit, int mode);
int sceSysreg_driver_844AF6BD(u32 mask, int enable);
int sceSysregGetTachyonVersion(void);
u64 sceSysreg_driver_4F46EEDE(void);
u32 sceSysreg_driver_8F4F4E96(void);

int sceSysregTopResetEnable(void);
int sceSysregScResetEnable(void);
int sceSysregMeResetEnable(void);
int sceSysregMeResetDisable(void);
int sceSysregAwResetEnable(void);
int sceSysregAwResetDisable(void);
int sceSysregVmeResetEnable(void);
int sceSysregVmeResetDisable(void);
int sceSysregAvcResetEnable(void);
int sceSysregAvcResetDisable(void);
int sceSysregUsbResetEnable(void);
int sceSysregUsbResetDisable(void);
int sceSysregAtaResetEnable(void);
int sceSysregAtaResetDisable(void);
int sceSysregMsifResetEnable(int no);
int sceSysregMsifResetDisable(int no);
int sceSysregKirkResetEnable(void);
int sceSysregKirkResetDisable(void);

float sceSysregPllGetFrequency(void);
float sceSysregPllUpdateFrequency(void);
float sceSysregPllGetBaseFrequency(void);
int sceSysreg_driver_B4560C45(void);
int sceSysreg_driver_DCA57573(int index);

int sceSysregMeBusClockEnable(void);
int sceSysregMeBusClockDisable(void);
int sceSysregAwRegABusClockEnable(void);
int sceSysregAwRegABusClockDisable(void);
int sceSysregAwRegBBusClockEnable(void);
int sceSysregAwRegBBusClockDisable(void);
int sceSysregAwEdramBusClockEnable(void);
int sceSysregAwEdramBusClockDisable(void);
int sceSysregDmacplusBusClockEnable(void);
int sceSysregDmacplusBusClockDisable(void);
int sceSysregDmacBusClockEnable(int no);
int sceSysregDmacBusClockDisable(int no);
int sceSysregKirkBusClockEnable(void);
int sceSysregKirkBusClockDisable(void);
int sceSysregAtaBusClockEnable(void);
int sceSysregAtaBusClockDisable(void);
int sceSysregUsbBusClockEnable(void);
int sceSysregUsbBusClockDisable(void);
int sceSysregMsifBusClockEnable(int no);
int sceSysregMsifBusClockDisable(int no);
int sceSysregEmcddrBusClockEnable(void);
int sceSysregEmcddrBusClockDisable(void);
int sceSysregEmcsmBusClockEnable(void);
int sceSysregEmcsmBusClockDisable(void);
int sceSysregApbBusClockEnable(void);
int sceSysregApbBusClockDisable(void);
int sceSysregAudioBusClockEnable(int no);
int sceSysregAudioBusClockDisable(int no);

int sceSysregAtaClkEnable(void);
int sceSysregAtaClkDisable(void);
int sceSysregUsbClkEnable(int no);
int sceSysregUsbClkDisable(int no);
int sceSysregMsifClkEnable(int no);
int sceSysregMsifClkDisable(int no);
int sceSysregSpiClkEnable(int no);
int sceSysregSpiClkDisable(int no);
int sceSysregUartClkEnable(int no);
int sceSysregUartClkDisable(int no);
int sceSysregApbTimerClkEnable(int no);
int sceSysregApbTimerClkDisable(int no);
int sceSysregAudioClkEnable(int no);
int sceSysregAudioClkDisable(int no);
int sceSysregLcdcClkEnable(void);
int sceSysregLcdcClkDisable(void);
int sceSysregPwmClkEnable(void);
int sceSysregPwmClkDisable(void);
int sceSysregKeyClkEnable(void);
int sceSysregKeyClkDisable(void);
int sceSysregIicClkEnable(void);
int sceSysregIicClkDisable(void);
int sceSysregSircsClkEnable(void);
int sceSysregSircsClkDisable(void);
int sceSysregGpioClkEnable(void);
int sceSysregGpioClkDisable(void);
int sceSysreg_driver_C1DA05D2(void);
int sceSysreg_driver_DE170397(void);

int sceSysregMsifClkSelect(int no, int clk);
int sceSysregMsifDelaySelect(int no, int delay);
int sceSysregAtaClkSelect(int clk);
int sceSysregApbTimerClkSelect(int no, int clk);
int sceSysregAudioClkSelect(int no, int clk);
int sceSysreg_driver_0A83FC7B(int clk);
int sceSysregSpiClkSelect(int no, int clk);
int sceSysregLcdcClkSelect(int no1, int no2);

int sceSysregEmcsmIoEnable(void);
int sceSysregEmcsmIoDisable(void);
int sceSysregUsbIoEnable(void);
int sceSysregUsbIoDisable(void);
int sceSysregAtaIoEnable(void);
int sceSysregAtaIoDisable(void);
int sceSysregMsifIoEnable(int no);
int sceSysregMsifIoDisable(int no);
int sceSysregLcdcIoEnable(void);
int sceSysregLcdcIoDisable(void);
int sceSysregAudioIoEnable(int no);
int sceSysregAudioIoDisable(int no);
int sceSysregIicIoEnable(void);
int sceSysregIicIoDisable(void);
int sceSysregSircsIoEnable(void);
int sceSysregSircsIoDisable(void);
int sceSysreg_driver_F844DDF3(void);
int sceSysreg_driver_29A119A1(void);
int sceSysregKeyIoEnable(void);
int sceSysregKeyIoDisable(void);
int sceSysregPwmIoEnable(void);
int sceSysregPwmIoDisable(void);
int sceSysregUartIoEnable(int no);
int sceSysregUartIoDisable(int no);
int sceSysregSpiIoEnable(int no);
int sceSysregSpiIoDisable(int no);
int sceSysregGpioIoEnable(int gpio);
int sceSysregGpioIoDisable(int gpio);
int sceSysreg_driver_55B18B84(int no);
int sceSysreg_driver_2112E686(int no);
int sceSysreg_driver_7B9E9A53(int no);
int sceSysreg_driver_7BDF0556(int no);
int sceSysreg_driver_8D0FED1E(void);
int sceSysreg_driver_A46E9CA8(void);

int sceSysregUsbGetConnectStatus(void);
int sceSysregUsbQueryIntr(void);
int sceSysregUsbAcquireIntr(int mask);
int sceSysregMsifGetConnectStatus(int no);
int sceSysreg_driver_BF91FBDA(int no);
int sceSysreg_driver_36A75390(int no, int mask);

int sceSysregIntrInit(void);
int sceSysregIntrEnd(void);
int sceSysregInterruptToOther(void);
int sceSysregSemaTryLock(void);
void sceSysregSemaUnlock(void);
int sceSysregEnableIntr(int intr);
int sceSysregDisableIntr(int intr);
int sceSysregRequestIntr(int cpu, int intr);

int sceSysreg_driver_5664F8B5(int num, int den);
int sceSysreg_driver_44704E1D(int *num, int *den);
int sceSysreg_driver_584AD989(int num, int den);
int sceSysreg_driver_377F035F(int *num, int *den);
int sceSysreg_driver_AB3185FD(float freq);
float sceSysreg_driver_0EA487FA(void);
int sceSysreg_driver_136E8F5A(float freq);
float sceSysreg_driver_F4811E00(void);


#define sceSysregPllSetIndex     sceSysreg_driver_DCA57573
#define sceSysregPllGetIndex     sceSysreg_driver_B4560C45
#define sceSysregSetCpuFreqRatio sceSysreg_driver_5664F8B5
#define sceSysregGetCpuFreqRatio sceSysreg_driver_44704E1D
#define sceSysregSetBusFreqRatio sceSysreg_driver_584AD989
#define sceSysregGetBusFreqRatio sceSysreg_driver_584AD989
#define sceSysregSetCpuFreqency  sceSysreg_driver_AB3185FD
#define sceSysregGetCpuFreqency  sceSysreg_driver_0EA487FA
#define sceSysregSetBusFreqency  sceSysreg_driver_136E8F5A
#define sceSysregGetBusFreqency  sceSysreg_driver_F4811E00

#endif
