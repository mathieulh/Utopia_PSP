//5568
void sceKernelDcacheWBinvAll()
{
  int config;

  MFC0_V(config, COP0_SR_CONFIG); //a2
  //ext        $a1, $a2, 6, 3
  a0 = 1;
  v0 = a1 + 0xC;
  a1 = a0 << v0;
  a0 = a1 >> 1;
  v1 = 0;
  if(a0 != 0)
  {
    while(1 == 1)
    {
      //cache      0x14, 0x0($v1)
      //cache      0x14, 0x0($v1)
      v1 += 0x40;
      a3 = v1 < a0;
      if(a3 == 0)
      {
        break;
      }
    }
  }
}

//55a8
void sceKernelIcacheClearAll()
{
  int config;

  MFC0_V(config, COP0_SR_CONFIG); //a2
  //ext        $a1, $a2, 9, 3
  a0 = 1;
  v0 = a1 + 0xC;
  a1 = a0 << v0;
  a0 = a1 >> 1;
  v1 = 0;
  if(a0 != 0)
  {
    //cache      0x4, 0x0($v1)
    //cache      0x4, 0x0($v1)
    v1 += 0x40;
    a3 = v1 < a0;
    if(a3 == 0)
    {
      break;
    }
  }
}

//55e8
int sub_000055E8(unk_a0, unk_a1)
{
  if(unk_a0 <= 0 || unk_a1 <= 0)
  {
    if(unk_a0 == unk_a1)
    {
      return 0;
    }

    return (unk_a0 <= 0) ? -1 : 1;
  }

  t0 = *unk_a1;
  //seb v1, a2
  a1++;
  if(v1 != t0)
  {
    t2 = -0x1(a1);
    //seb a1, a2
    v0 = a1 - t2;

    return v0;
  }
  
  a3 = v1;
  while(1 == 1)
  {
    a0++;
    v0 = 0;
    if(a3 == 0)
    {
      return 0;
    }
    
    a2 = *unk_a0;
    t1 = *unk_a1;
    a1++;
    //seb v1, a2
    a3 = v1;
    if(v1 != t1)
    {
      break;
    }
  }
  
  t2 = -0x1(a1);
  //seb a1, a2
  v0 = a1 - t2;
  
  return v0;
}

//5668
loc_00005668(...)
{
}

//56a0
sub_000056A0(...)
{
}

//57b8
sub_000057B8(...)
{
}

//5820
sub_00005820(...)
{
}

//5890
sub_00005890(...)
{
}

//5954
sub_00005954(...)
{
}

//5ad4
sub_00005AD4(...)
{
}

//5bbc
int sceKernelCpuSuspendIntr()
{
  int intr;

  MFIC_V(intr, 0);
  MTIC_R(GPREG_ZR, 0);
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  return intr;
}

//5be8
void sceKernelCpuResumeIntr(int intr)
{
  MTIC_V(intr, 0);
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
}

//5c10
loc_00005c10(...)
{
}

//5c48
int loc_00005c48(int unk_a0)
{
  return (unk_a0 <= 0);
}

//5c50
int loc_00005c50()
{
  int ret;

  MFIC_V(ret, 0);

  return ret;
}

//5c5c - ASM
loc_00005c5c(...)
{
}

//5c90
sub_00005C90(...)
{
}

//5f60
sub_00005F60(...)
{

}

//5f7c
sub_00005F7C(...)
{
}

//6008
sub_00006008(...)
{
}

//6094
sub_00006094(...)
{
}

//6098
sub_00006098(...)
{
}

//61b0
sub_000061B0(...)
{
}

//62d0
sub_000062D0(...)
{
}

//634c
sub_0000634C(...)
{
}
