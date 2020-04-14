//protection.c
//4498
void PartitionProtectInit()
{
  int ret = KDebugForKernel_24C32559(0xA);

  if(ret == 1)
  {
    //452c
    *(BC100040) = (*(0xBC100040) & 0xFFFFFFFC) | 0x2;
  }
  else
  {
    *(BC100040) = (*(0xBC100040) & 0xFFFFFFFC) | 0x1;
  }

  //44d4
  *(0xBC000000) = 0xCCCCCCCC;
  *(0xBC000004) = 0xCCCCCCCC;
  *(0xBC000008) = 0xFFFFFFFF;
  *(0xBC00000C) = 0xFFFFFFFF;

  if(GetCPUID() != 0)
  {
    sub_0000437C();
  }
  else
  {
    sub_000043DC();
  }
}

