//7ae4
int sub_00007AE4(unk_a0, unk_a1, unk_a2, unk_a3)
{
  if((unk_a0 & 0x1) && (0x0(a3) & unk_a1) || ((unk_a0 & 0x2) && (0x0(a3) & unk_a2))
  {
    return -0x2;
  }

  return 0;
}

//7b28
sub_00007B28(unk_a0)
{
  t0 = 0xE4000000;
  v1 = 0x1FF0000;
  a3 = a0 + t0;
  v1 = 0x1FFFFFE;
  v0 = v1 < a3;
  if(v0 != 0)
  {
    //8058
  }
}

//8600
sceKernelSysmemIsValidAccess(...)
{
}

//8c18
sceKernelIsValidUserAccess(...)
{
  a3 = a2;
  a2 = 0x2;
  v0 = sceKernelSysmemIsValidAccess(...);

  return (v0 < 1);
}

//memtest.c
//8c3c
sceKernelSysMemCheckCtlBlk(...)
{
}

//8e70
sceKernelSysMemDump(...)
{
}

//8f64
sceKernelSysMemDumpBlock(...)
{
  s2 = var_CA5C;
  if(s2 == 0)
  {
    return;
  }

  s0 = 0x10(s2);

  //8f8c
  while(1 == 1)
  {
    if(s0 == 0)
    {
      s2 = 0x0(s2);
      //8fb4
      if(s2 != 0)
      {
        s0 = 0x10(s2);
        continue;
      }

      return;
    }

    //8f9c
    do
    {
      Kprintf("0x%06p == \n", s0);
      s0 = 0x0(s0);
    } while(s0 != 0)

    //8fb4
    if(s2 == 0)
    {
      break;
    }

    s0 = 0x10(s2);
  }
}

//8fd4
sceKernelSysMemDumpTail(...)
{
  s5 = var_CA5C;
  if(s5 == 0)
  {
    return;
  }
  
  s3 = 0x10(s5);

  //900c
  while(1 == 1)
  {
    v1 = 0x0(s3);
    a1 = s3;
    if(v1 != 0 && 0x0(v1) != 0)
    {
      //9024
      while(1 == 1)
      {
        v1 = v0;
        if(v0 == 0)
        {
          break;
        }
  
        v0 = 0x0(v0);
        if(v0 == 0)
        {
          break;
        }
  
        v0 = 0x0(v1);
      }
    }
  
    //9038
    s3 = a1;
    if(a1 != 0)
    {
      //904c
      while(1 == 1)
      {
        Kprintf("0x%06x == \n", s3);
        s2 = 0;
        s0 = s3;
        s1 = s3 + 0x4;
  
        //9060
        while(1 == 1)
        {
          t1 = 0xC(s0);
          a3 = 0x8(s0);
          a2 = 0x4(s0);
          t3 = t1 >> 2;
          t0 = a3 >> 1;
          t2 = t1 >> 1;
          a1 = s1;
          t0 = t0 << 8;
          t2 = t2 & 0x1;
          t3 = t3 << 8;
          a3 = a3 & 0x1;
          t1 = t1 & 0x1;
          s2++;
          Kprintf("  %06x: %06x  u=%d p=%06x d=%d n=%6x\n", ...);

          v1 = s2 < 0x14;
          s1++;
          s0 += 0xC;
          if(v1 == 0)
          {
            break;
          }
        }

        s3 = 0x0(s3);
        if(s3 == 0)
        {
          break;
        }
      }

      //90b8
      s5 = 0x0(s5);
      if(s5 == 0)
      {
        break;
      }

      s3 = 0x10(s5);
    }
  }
}

