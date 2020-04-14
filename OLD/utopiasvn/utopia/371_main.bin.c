// 0x0400A300
u8 g_payloadSeed1[0x40] = 
{
    0x15, 0x1C, 0xE1, 0x2E, 0xF0, 0x89, 0x66, 0x99,
    0x6A, 0xF7, 0x03, 0x19, 0xD4, 0x90, 0xC6, 0xA8,
    0xF5, 0xC3, 0x2A, 0xDE, 0x87, 0x67, 0x58, 0xDE,
    0xE4, 0x2E, 0x70, 0xB7, 0xE6, 0x45, 0xE3, 0xE1,
    0x75, 0x74, 0x78, 0xC0, 0x95, 0x3E, 0xE8, 0x19,
    0x2D, 0xA6, 0x70, 0xFA, 0x98, 0xC7, 0xE1, 0x45,
    0xA9, 0x77, 0x0E, 0x73, 0x29, 0x28, 0x3E, 0x74,
    0x85, 0x74, 0x91, 0x8A, 0xF9, 0x89, 0x00, 0x53,
};

// 0x0400A340
u8 g_payloadSeed2[0x40] =
{
    0xAD, 0xFB, 0x99, 0xDB, 0x1D, 0xCE, 0xFB, 0x60,
    0xFB, 0xFD, 0x68, 0x04, 0x89, 0xB8, 0x72, 0xBA,
    0xE0, 0x41, 0xAA, 0x18, 0x24, 0xD1, 0x51, 0xC0,
    0xFC, 0x1F, 0xFB, 0x39, 0x76, 0x7E, 0x42, 0xA0,
    0xEA, 0x1B, 0x8A, 0x62, 0xDB, 0xDF, 0x41, 0xC0,
    0x16, 0xD7, 0x8B, 0xF5, 0xB8, 0xDA, 0xD7, 0x1D,
    0x00, 0x31, 0xA4, 0x1C, 0xCB, 0x98, 0x72, 0x2A,
    0xE8, 0x49, 0x6D, 0x93, 0xF1, 0xB0, 0x60, 0x13,
};

// 0x0400A380
u8 g_BFCSeed_1[0x40] =
{
    0x0F, 0xCA, 0x45, 0xF2, 0x8A, 0x7D, 0x13, 0x1A,
    0x65, 0x61, 0x50, 0xE5, 0xDA, 0x6A, 0x4E, 0x6B,
    0x4E, 0xF5, 0x5A, 0xCF, 0xDC, 0xFD, 0x56, 0x67,
    0x76, 0x4F, 0x95, 0x6A, 0x51, 0xB7, 0xE4, 0x28,
    0x50, 0x3D, 0x67, 0x42, 0xE3, 0x03, 0xA7, 0x8E,
    0x17, 0xB4, 0x63, 0xD0, 0xE3, 0x09, 0x1B, 0x30,
    0x03, 0x5B, 0x36, 0x59, 0x76, 0xB4, 0x88, 0xBF,
    0x14, 0xDA, 0x08, 0xEE, 0xED, 0xC7, 0xF2, 0xC1,
};

// 0x0400A3C0
u8 g_BFCSeed_2{0x40] =
{
    0xC7, 0xBB, 0xCE, 0x14, 0x86, 0xB3, 0x18, 0xC0,
    0xFF, 0x09, 0x67, 0xAB, 0xEE, 0xC7, 0xB7, 0x12,
    0x21, 0xD9, 0x60, 0xB7, 0x0B, 0x1A, 0x42, 0x1D,
    0xD6, 0x26, 0x64, 0xAF, 0x22, 0x5E, 0xA5, 0x10,
    0xB1, 0x51, 0x6E, 0xAD, 0x70, 0xC4, 0x9F, 0xE4,
    0x7F, 0x0A, 0x82, 0x30, 0xF6, 0xDF, 0x7B, 0x42,
    0xC3, 0x21, 0x57, 0xC7, 0x66, 0x2C, 0x76, 0x87,
    0xBC, 0xEC, 0xE6, 0x36, 0xBC, 0xA9, 0x23, 0x97,
};


void sub_4000008()
{
    decryptData(NULL, 0, g_BFCSeed_1, g_BFCSeed_2, 0xBFC00000, 0x100);
    
    memset(g_BFCSeed_1, 0, sizeof(g_BFCSeed_1));
    memset(g_BFCSeed_2, 0, sizeof(g_BFCSeed_2));

    // reset kirk
    sceSysregKirkResetEnable();
    sceSysregKirkBusClockEnable();
    sceSysregKirkBusClockDisable();
    sceSysregKirkResetDisable();
    sceSysregKirkBusClockEnable();
    
    if (kirkCmd1(0xBFC00000, 0xBFC00000) != 0)
    {
        memset(0xBFC00000, 0, 0x1000);
        sceKernelDcacheWritebackInvalidateAll
        clearSeedBuffers();
        sceSysconInit();
        sceSysconPowerStandby();
        
        while(1);
    }

    memcpy(0xBFC00200, 0xBFC00000, 0x20);
    memset(0xBFC00000, 0, 0x1000);
    
    return;
}


// 0x04000280
void decryptPayload(unk0, unk1, unk2, unk3, unk4, unk5)
{
    decryptData(NULL, 0, g_payloadSeed1, g_payloadSeed1, (void*)0x04100000, 0x8000);
    
    // kill the seeds
    memset(g_payloadSeed1, 0, sizeof(g_payloadSeed1));
    memset(g_payloadSeed2, 0, sizeof(g_payloadSeed2));

    memcpy(0x08600000, 0x04100000, 0x100000);
    memset(0x04100000, 0, 0x10);

    sceKernelDcacheWritebackInvalidateAll();

    if (kirkCmd1(0x08600000, 0x08600000) != 0)
    {
        memset(0x08600000, 0, 0x1000000);
        sceKernelDcacheWritebackInvalidateAll
        clearSeedBuffers();
        sceSysconPowerStandby();
        
        while(1);
    }
    
    // clear scratchpad ram
    memset(0x10000, 0, 0x4000);
    memcpy(0x10000, 0x0400A5D0, 0x22C);
    
    sceKernelDcacheWritebackInvalidateAll();
    sceKernelIcacheInvalidateAll();
    
    call_0x10000(unk0, unk1, unk2, unk3, unk4, unk5, 0, 0);
    
    return;
}

__inline__ void errorExit1();
{
    clearSeedBuffers();
    sceSysconInit();
    sceSysconPowerStandby();
    while(1);
}

__inline__ void errorExit2();
{
    clearSeedBuffers();
    sceSysconPowerStandby();
    while(1);
}

void initDevices(int *unk0, int *unk1, int *unk2, int *unk3, int *unk4, int *unk5)
{
    *(sp+0xE8) = unk4;
    *(sp+0xEC) = unk5;

    u8 macAddr[8]; // sp+0x020
    ScePowerResumeInfo resumeInfo; // sp+0x030
    u32 sha1[5];   // sp+0x090
    int clock;     // sp+0x0D0
    int wakeUpFactor;   // sp+0x0E0
    void (*resumeFunc)(ScePowerResumeInfo *resumeInfo); // sp+0x0E4
    void (*func_EC)(void); // sp+0x0EC
    int moboVer;   // sp+0x104
    
    // init main bus
    sceSysregApbBusClockEnable();

    sceGpioInit();
    sceGpioSetPortMode(0, 1);
    sceGpioPortClear(1);

    sceNandInit();

    if (sceNandCheckId() < 0)
        errorExit1();

    sceSysconInit();

    baryonVer = sceSyscon_driver_E00BFC9E();
    moboVer = baryonVer >> 16;

    // if slimMobo
    if ((moboVer & 0xF0 > 0) && (moboVer & 0xF0 > 0x10))
        errorExit2();

    // another if slim conditional
    if (sceSysregGetTachyonVersion() >= 0x500000)
        errorExit2();
        
    sysconInit();

    // clear scratchpad ram
    memset(0x10000, 0, 0x4000);

    *(sp+0x00) = 0x1C;
    *(sp+0x04) = 0;
    *(sp+0x08) = 0;
    *(sp+0x0C) = 0;
    *(sp+0x10) = 0;
    *(sp+0x14) = 0;
    *(sp+0x18) = 0;

    // init deci2p
    model = sub_4001C70(cop0.ctrl.reg17, baryonVer, sp);

    *(sp+0x0F0) = *(sp+0x0C);
    *(sp+0x0F4) = *(sp+0x08);
    *(sp+0x0F8) = *(sp+0x14); // dipsw
    *(sp+0x0FC) = *(sp+0x18);
    *(sp+0x100) = *(sp+0x10);


    if (((moboVer & 0xF0 == 0) || (moboVer & 0xF0 == 0x10)) || (moboVer & 0xFF - 0x20 < 2))
    {
        sceGpioSetPortMode(2, 0);
        sceGpioPortSet(4);
    }
    
    if (model == 2)
    {

    }
    
    sceIdStorageInit();
    
    memset(g_leaf4Buf, 0, sizeof(g_leaf4Buf));
    memset(g_leaf5Buf, 0, sizeof(g_leaf5Buf));
    memset(g_leaf6Buf, 0, sizeof(g_leaf6Buf));
    memset(g_leaf7Buf, 0, sizeof(g_leaf7Buf));
    
    sceIdStorageLookup(4, 0, g_leaf4Buf, sizeof(g_leaf4Buf));
    sceIdStorageLookup(5, 0, g_leaf5Buf, sizeof(g_leaf5Buf));
    sceIdStorageLookup(6, 0, g_leaf6Buf, sizeof(g_leaf6Buf));
    sceIdStorageLookup(7, 0, g_leaf7Buf, sizeof(g_leaf7Buf));
    
    

    sceSysregEmcddrBusClockEnable();
    sceDdrInit();
    sceDdr_driver_5B5831E5(*(sp+0xF0));
    sceDdr_driver_7251F5AB(3, 3);
    sceDdr_driver_6955346C(8, 10);
    sceDdrSetPowerDownCounter(4);
    
    if (moboVer&0xFF - 0x20 < 2)
        setDve();


    sceI2cInit();
    sceClockgenInit();
    sceClockgen_driver_C6D4C843(1);
    sceClockgenSetup();
    
    if (*(u32*)&g_leaf5Buf[0] == 0x436C6B67) // Clkg
    {
        res = sceClockgen_driver_C9AF3102(buf[0x10]);

        if ((res < 0) && (res > 0x80000004))
            while(1);
    }
    
    if (*(u32*)&g_leaf4Buf[0] == 0x4272796E) // Bryn
    {
        while(sceSysconCtrlTachyonWDT(g_leaf4Buf[0x18] & 0x7F) - 0x80250001 < 4);
    }

    btnMask = sysconCmd7();

    while(sceSysconReadClock(&clock) - 0x80250001 < 4);

    /*
    if clock ==
    0xxxxxxx00
    0xxxxxxx24
    0xxxxxxx49
    0xxxxxxx6D
    0xxxxxxx92
    0xxxxxxxB6
    0xxxxxxxDB
    0xxxxxxxFF
    */
    // 8/256 == 3.125% chance
    if ((clock>>3 ^ clock) & 0x1F == 0)
    {
        if (sub_400430C)
        {
            sub_400430C();

            if (sub_40042B8(sp+0xD8) < 0)
            {
                // secure-RTC reset
                while(sceSyscon_driver_EB277C88(0x18, macAddr, sizeof(macAddr)) - 0x80250001 < 4);
                
                *(u64*)&macAddr[0] = 0;
                
                while(sceSyscon_driver_65EB6096(0x18, macAddr, sizeof(macAddr)) - 0x80250001 < 4);
            }
        }
    }

    /*
    00
    08
    11
    19
    22
    2A
    33
    3B
    44
    4C
    55
    5D
    66
    6E
    77
    7F
    80
    88
    91
    99
    A2
    AA
    B3
    BB
    C4
    CC
    D5
    DD
    E6
    EE
    F7
    FF
    */
    // 32/256 == 12.5% chance to do the ids header check
    if ((clock>>4 ^ clock) & 7 == 0)
    {
        if ((*(u32*)&g_leaf4Buf[0] != 0) && (*(u32*)&g_leaf4Buf[0] != 0x4272796E)) // Bryn
            errorExit2();

        if ((*(u32*)&g_leaf5Buf[0] != 0) && (*(u32*)&g_leaf5Buf[0] != 0x436C6B67)) // Clkg
            errorExit2();
            
        if ((*(u32*)&g_leaf6Buf[0] != 0) && (*(u32*)&g_leaf6Buf[0] != 0x4D446472)) // MDdr
            errorExit2();

        if ((*(u32*)&g_leaf7Buf[0] != 0) && (*(u32*)&g_leaf7Buf[0] != 0x41506144)) // APaD
            errorExit2();
    }

    if (model == 2)
    {
        memset(macAddr, 0xFF, 8);

        if (sceIdStorageLookup(0x44, 0, macAddr, 8) < 0) // mac addr
            memset(macAddr, 0xFF, 8);

        for(i=0; i<8; i++)
            *(0x4010008C + i) = macAddr[i];
    }
    
    while(sceSysconGetWakeUpFactor(&wakeUpFactor) - 0x80250001 < 4);

    if (wakeUpFactor & 0x80) // resuming from sleepmode
    {
        if (func_EC)
            func_EC();

        memset(resumeInfo, 0, sizeof(resumeInfo));

        resumeInfo.size = sizeof(resumeInfo);                         // sp+0x30
        resumeInfo.uiWakeUpFactor = wakeUpFactor;                     // sp+0x34
        resumeInfo.pSyscomCmd = sceSysconCmdExec;                     // sp+0x3C
        resumeInfo.uiDipSw = *(sp+0xF8);                              // sp+0x40
        resumeInfo.pChangeClock = changeClock;                        // sp+0x44
        resumeInfo.uiBaryonClock = clock;                             // sp+0x4C
        resumeInfo.uiBaryonKey = btnMask;                             // sp+0x50
        resumeInfo.pSysconSuspend = sceSysconPowerSuspend;            // sp+0x64
        resumeInfo.pSysconStandby = sceSysconPowerStandby;            // sp+0x68
        resumeInfo.pChangeClockVoltage = changeClockVoltage;          // sp+0x6C
        resumeInfo.pI2cTransmit = sceI2cMasterTransmit;               // sp+0x78
        resumeInfo.pI2cReceive = sceI2cMasterReceive;                 // sp+0x7C
        resumeInfo.pI2cTransmitReceive = sceI2cMasterTransmitReceive; // sp+0x80

        while(sceSyscon_driver_EB277C88(0xC, resumeFunc, 4) - 0x80250001 < 4);
        while(sceSysconGetWakeUpReq(&resumeInfo.uiWakeUpReq) - 0x80250001 < 4);
        while(sceSysconGetPowerSupplyStatus(&resumeInfo.uiPowerSupplyStatus) - 0x80250001 < 4);

        if (resumeInfo.uiPowerSupplyStatus & 2)
        {
            if (sceSysconBatteryGetStatusCap(&resumeInfo.uiBatteryStatus, &resumeInfo.uiBatteryCaps) < 0)
            {
                resumeInfo.uiBatteryCaps = 0;
                resumeInfo.iBatteryFullCaps = 0;
                resumeInfo.uiPowerSupplyStatus &= ~2;
            }
            else
            {
                delay(1660000);

                if (sceSysconBatteryGetFullCap(&resumeInfo.iBatteryFullCaps) < 0)
                {
                    resumeInfo.uiBatteryCaps = 0;
                    resumeInfo.iBatteryFullCaps = 0;
                    resumeInfo.uiPowerSupplyStatus &= ~2;
                }
            }
        }

        resumeInfo.uiBaryonStatus = sceSysconGetBaryonStatus();

        sceSysregKirkBusClockDisable();
        
        while(sceSyscon_driver_EB277C88(0x10, macAddr, sizeof(macAddr)) - 0x80250001 < 4);
        
        _sceSha1Digest(macAddr, sizeof(macAddr), sha1);

        sha1[0] ^= *(vu32*)0xBC100090;
        sha1[1] ^= *(vu32*)0xBC100094;
        
        _sceSha1Digest(sha1, sizeof(sha1), sha1);

        resumeFunc ^= (sha1[0] ^ sha1[1] ^ sha1[2] ^ sha1[3] ^ sha1[4]);
        
        if (resumeFunc - 0x88000000 >= 0x02000000)
            errorExit2();

        if (~*(resumeFunc+0x8) !=  *(resumeFunc+0xC))
            errorExit2();

        if (~*(resumeFunc+0x8) - 0x03070000 >= 0x20000)
            errorExit2();
            
        resumeFunc(resumeInfo);
        
        while(1);
    }
    
    while(sceSyscon_driver_EB277C88(0x10, macAddr, sizeof(macAddr)) - 0x80250001 < 4);
    
    _sceSha1Digest(macAddr, 8, sha);

    sha[0] ^= (*(vu32*)0xBC100090 + (clock+0x12345678));
    sha[1] ^= (*(vu32*)0xBC100094 - (clock+0x12345678));
    sha[2] ^= (        0x00000000 + (clock+0x12345678));

    _sceSha1Digest(sha, 20, sha);

    s4 = (sha[0] + sha[1]) ^ (sha[2] - sha[3]) ^ sha[4];
    

    if (*(u32*)&g_leaf4Buf[0] == 0x4272796E) // Bryn
    {
        while(sceSysconSendSetParam(0, &g_leaf4Buf[0x10]) - 0x80250001 < 4); 

        if (g_leaf4Buf[0x35] & 1)
        {
            while(sceSysconSendSetParam(2, &g_leaf4Buf[0x36]) - 0x80250001 < 4);
            while(sceSysconSendSetParam(3, &g_leaf4Buf[0x3E]) - 0x80250001 < 4);
        }
        
        if (g_leaf4Buf[0x35] & 2)
        {
            while(sceSysconSendSetParam(4, &g_leaf4Buf[0x46]) - 0x80250001 < 4);
        }
    }

    if (*(u32*)&g_leaf6Buf[0] == 0x4D446472) // MDdr
    {
        if (moboVer & 0xF0 == 0)
            a1 = g_leaf6Buf[0x10];
        else if (moboVer & 0xF0 == 0x10)
            a1 = g_leaf6Buf[0x15];
        else if (moboVer & 0xFF - 0x20 < 2)
            a1 = g_leaf6Buf[0x19];
        else
            a1 = 1;

        if (a1 == 0x80)
            a1 = 0;
        else if (a1 == 0x81)
            a1 = 1;
        else if (a1 == 0x82)
            a1 = 2;
        else if (a1 == 0x83)
            a1 = 3;
        else
            a1 = (s3-2 > 1);

/*
if (buttons & PSP_CTRL_LTRIGGER)
{
    if (buttons & PSP_CTRL_TRIANGLE)
        sceDdrResetDevice(3, 0);
    if (buttons & PSP_CTRL_CIRCLE)
        sceDdrResetDevice(3, 1);
    if (buttons & PSP_CTRL_CROSS)
        sceDdrResetDevice(3, 2);
    if (buttons & PSP_CTRL_SQUARE)
        sceDdrResetDevice(3, 3);
}
*/
        if (s3-2 < 2)
        {
            if (*(sp+0xF8) & 0x200)
                a1 = 1;

            if (btnMask & 0x200 == 0)
            {
                if (btnMask & 0x10 == 0)
                    a1 = 0;
                else if (btnMask & 0x20 == 0)
                    a1 = 1;
                else if (btnMask & 0x40 == 0)
                    a1 = 2;
                else if (btnMask & 0x80 == 0)
                    a1 = 3;
            }
        }

        sceDdrResetDevice(3, a1);
        
        if (g_leaf6Buf[0x11] == 0x80)
            *0xBC000050 = (*0xBC000050 & ~0x300) | 0x000;
        else if (g_leaf6Buf[0x11] == 0x81)
            *0xBC000050 = (*0xBC000050 & ~0x300) | 0x100;
        else if (g_leaf6Buf[0x11] == 0x82)
            *0xBC000050 = (*0xBC000050 & ~0x300) | 0x200;
        else if (g_leaf6Buf[0x11] == 0x83)
            *0xBC000050 = (*0xBC000050 & ~0x300) | 0x300;
        else
            *0xBC000050 |= 0x300;
            
        Mddr = setMDdr1();
        if (Mddr >= 0)
            while(sceSysconCtrlVoltage(3, Mddr) - 0x80250001 < 4);
    }
    else
    {
        if (s3-2 < 2)
        {
            if (*(sp+0xF8) & 0x200)
                a1 = 1;

            if (btnMask & 0x200 == 0)
            {
                if (btnMask & 0x10 == 0)
                    a1 = 0;
                else if (btnMask & 0x20 == 0)
                    a1 = 1;
                else if (btnMask & 0x40 == 0)
                    a1 = 2;
                else if (btnMask & 0x80 == 0)
                    a1 = 3;
            }
        }

        sceDdrResetDevice(3, a1);
    }
    
    Bryn = setBryn1();
    Mddr = setMDdr1();

    if (Mddr  >= 0)
        while(sceSysconCtrlVoltage(3, Mddr) - 0x80250001 < 4);

    if (Bryn  >= 0)
        while(sceSysconCtrlVoltage(1, Bryn) - 0x80250001 < 4);
        
    
    if (*(u32*)&g_leaf7Buf[0] == 0x41506144) // APaD
    {
        while(sceSysconSendSetParam(1, &g_leaf7Buf[0x10]) - 0x80250001 < 4);

        // slim only
        if ((moboVer & 0xF0 != 0) && (moboVer & 0xF0 != 0x10))
        {
            if (*(u32*)&g_leaf7Buf[8] >= 0x14)
            {
                while(sceSysconGetScvCode(&g_leaf7Buf[0x18]) - 0x80250001 < 4);
            }
        }
    }
    
    if (sceSysconGetLeptonPowerCtrl() != 0)
    {
        if (s3 == 0)
            while(1);
            
        while(sceSysconResetDevice(2, 1) - 0x80250001 < 4);

        if (sceClockgenLeptonClkDisable(2) < 0)
            while(1);

        while(sceSysconCtrlLeptonPower(0) - 0x80250001 < 4);
    }
    
    if (sceSysconGetWlanPowerCtrl() != 0)
    {
        while(sceSysconResetDevice(4, 1) - 0x80250001 < 4);

        while(sceSysconCtrlWlanPower(0) - 0x80250001 < 4);
    }
    
    if (sceSyscon_driver_64FA0B22() != 0)
    {
        if (sceSysconGetUsbPowerCtrl() != 0)
        {
            if (s3 == 0)
                while(1);
                
            while(sceSysconCtrlUsbPower(0) - 0x80250001 < 4);
        }
    }


    if (unk0)
        *unk0 = *(sp+0xF4);
        
    if (unk1)
        *unk1 = *(sp+0xF8);
        
    if (unk2)
        *unk2 = *(sp+0x100);

    if (unk3)
        *unk3 = *(sp+0xFC);
        
    if (*(sp+0xE8))
        **(sp+0xE8) = s4;

    return;
}
