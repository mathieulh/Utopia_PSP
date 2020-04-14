#include <pspsdk.h>

#include "../common/common.h"

#include "../sdk/include/interruptman_kernel.h"
#include "../sdk/include/kdebug_kernel.h"
#include "../sdk/include/sysmem_kernel.h"
#include "../sdk/include/sysclib_kernel.h"
#include "../sdk/include/threadman_kernel.h"
#include "../sdk/include/iofilemgr.h"
#include "../sdk/include/iofilemgr_dirent.h"
#include "../sdk/include/iofilemgr_fcntl.h"
#include "../sdk/include/iofilemgr_kernel.h"
#include "../sdk/include/iofilemgr_stat.h"

#include "iofilemgr.h"

PSP_MODULE_INFO("sceIOFileManager", 0x1007, 1, 2);
PSP_MAIN_THREAD_ATTR(0);

var_4D50;
PspIoDrvFuncs var_4DAC =
{
  deleted_drv_func32, //IoInit
  deleted_drv_func32, //IoExit
  deleted_drv_func32, //IoOpen
  deleted_drv_func32, //IoClose
  deleted_drv_func32, //IoRead
  deleted_drv_func32, //IoWrite
  deleted_drv_func, //IoLseek
  deleted_drv_func32, //IoIoctl
  deleted_drv_func32, //IoRemove
  deleted_drv_func32, //IoMkdir
  deleted_drv_func32, //IoRmdir
  deleted_drv_func32, //IoDopen
  deleted_drv_func32, //IoDclose
  deleted_drv_func32, //IoDread
  deleted_drv_func32, //IoGetstat
  deleted_drv_func32, //IoChstat
  deleted_drv_func32, //IoRename
  deleted_drv_func32, //IoChdir
  deleted_drv_func32, //IoMount
  deleted_drv_func32, //IoUmount
  deleted_drv_func32, //IoDevctl
  deleted_drv_func32 //IoUnk21
};
PspIoDrv var_4DAC = { "", 0, 0, "", var_4D54 };
PspIoDrvArg var_4DC0 = { var_4DAC, 0 };

PspIoDrvFuncs var_4DC8 =
{
  dummy_drv_func32, //IoInit
  dummy_drv_func32, //IoExit
  dummy_drv_func32, //IoOpen
  dummy_drv_func32, //IoClose
  dummy_drv_func32, //IoRead
  dummy_drv_write, //IoWrite
  dummy_drv_func, //IoLseek
  dummy_drv_func32, //IoIoctl
  dummy_drv_func32, //IoRemove
  dummy_drv_func32, //IoMkdir
  dummy_drv_func32, //IoRmdir
  dummy_drv_func32, //IoDopen
  dummy_drv_func32, //IoDclose
  dummy_drv_func32, //IoDread
  dummy_drv_func32, //IoGetstat
  dummy_drv_func32, //IoChstat
  dummy_drv_func32, //IoRename
  dummy_drv_func32, //IoChdir
  dummy_drv_func32, //IoMount
  dummy_drv_func32, //IoUmount
  dummy_drv_func32, //IoDevctl
  dummy_drv_func32 //IoUnk21
};
PspIoDrv var_4E20 = { "dummy_drv_iofile", 0, 0x00000800, "DUMMY_DRV", var_4DC8 };

DrvTable *var_4E40;
DeviceAlias *var_4E44;
var_4E48;
var_4E4C;
int var_4E50; //the id of the KTLS allocated by threadman
PspIoDrvFileArg var_4E58[MAX_FILES + 1];
var_6E58[...];

int module_bootstart(SceSize argSize, void *args) __attribute__((alias("init_iofm")));
int module_before_reboot() __attribute__((alias("terminate_iofm")));

//0
int sceIoChangeAsyncPriority(SceUID fd, int priority)
{
  int ret;

  SET_K1_SRL16;

  if(priority > 127 || (IS_USER_MODE && priority >= 120))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
  }

  if(fd == -1)
  {
    var_4D50 = priority;

    RESET_K1;

    return SCE_KERNEL_ERROR_OK;
  }

  PspIoDrvFileArg *desc;
  ret = validate_fd(fd, 0, 2, 1, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  desc->unk_38 = priority;
  if(desc->unk_3C != 0)
  {
    ret = sceKernelChangeThreadPriority(desc->unk_3C, (priority < 0) ? sceKernelGetThreadCurrentPriority() : priority);
    if(ret != 0)
    {
      RESET_K1;

      return ret;
    }
  }

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//f8
int open_main(PspIoDrvFileArg *desc)
{
  char *pathbuf = alloc_pathbuf();
  if(pathbuf == 0)
  {
    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", "open_main");

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  PspIoDrvArg *drv;
  int fs_num;
  char *file = pathbuf;
  ret = parsefile(desc->unk_68[0], &drv, &fs_num, &file);
  if(ret >= 0)
  {
    desc->dev_type = ret;
    desc->fs_num = fs_num;
    desc->unk1 = desc->unk_68[1];
    desc->drv = drv;

    if(drv->drv->funcs->IoOpen != 0)
    {
      if(desc->unk_68[1] & 0x2000000)
      {
        power_lock(desc);
      }

      ret = drv->drv->funcs->IoOpen(desc, file, desc->unk_68[1], desc->unk_68[2]);
      if(ret >= 0)
      {
        desc->unk_28 = ret;
      }
    }
    else
    {
      ret = SCE_KERNEL_ERROR_UNSUP;
    }
  }

  if(pathbuf != 0)
  {
    free_pathbuf(pathbuf);
  }

  return ret;
}

int sceIoReopen(const char *file, int flags, SceMode mode, SceUID fd)
{
  int ret;

  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(file))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, file);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  ret = do_close(fd, 0, 0);
  if(ret < 0)
  {
    Kprintf("iofilemgr.c:%s:CANNOT Close old fd %d, retval:0x%x\n", __FUNCTION__, fd, ret);

    RESET_K1;

    return ret;
  }

  PspIoDrvFileArg *desc;
  ret = validate_fd(fd, 0, 2, 3, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  power_unlock(desc);

  desc->unk_68[0] = file;
  desc->unk_68[1] = flags;
  desc->unk_68[2] = mode;

  ret = open_main(desc);
  if(ret < 0)
  {
    unalloc_iob(desc);
  }

  RESET_K1;

  return ret;
}

int sceIoDopen(const char *dirname)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(dirname))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: %x\n", __FUNCTION__, dirname);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  char *pathbuf = alloc_pathbuf();
  if(pathbuf == 0)
  {
    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  PspIoDrvFileArg *desc;
  int ret = alloc_iob(&desc);
  if(ret >= 0)
  {
    PspIoDrvArg *drv;
    int fs_num;
    char *actualdir = pathbuf;
    ret = parsefile(dirname, &drv, &fs_num, &actualdir);
    if(ret >= 0)
    {
      desc->dev_type = ret;
      desc->fs_num = fs_num;
      desc->unk1 = 0x1000008;
      desc->drv = drv;

      if(drv->drv->funcs->IoDopen != 0)
      {
        ret = drv->drv->funcs->IoDopen(desc, actualdir);
        if(ret < 0)
        {
          unalloc_iob(desc);
        }
        else
        {
          ret = desc->unk_28;
        }
      }
    }
    else
    {
      unalloc_iob(desc);
    }
  }

  if(pathbuf != 0)
  {
    free_pathbuf(pathbuf);
  }

  RESET_K1;

  return ret;
}

int sceIoDread(SceUID fd, SceIoDirent *dir)
{
  int ret;

  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(dir))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, dir);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  PspIoDrvFileArg *desc;
  ret = validate_fd(fd, 8, 4, 0, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  if(desc->drv->drv->funcs->IoDread != 0)
  {
    ret = desc->drv->drv->funcs->IoDread(desc, dir);
  }
  else
  {
    ret = SCE_KERNEL_ERROR_UNSUP;
  }

  RESET_K1;

  return ret;
}

int sceIoDclose(SceUID fd)
{
  int ret;

  SET_K1_SRL16;

  PspIoDrvFileArg *desc;
  ret = validate_fd(fd, 8, 4, 2, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  if(desc->drv == &var_4DC0)
  {
    unalloc_iob(desc);

    RESET_K1;

    return 0;
  }
  else if(desc->drv->drv->funcs->IoDclose != 0)
  {
    ret = desc->drv->drv->funcs->IoDclose(desc);
    if(ret < 0)
    {
      RESET_K1;

      return ret;
    }
  }

  RESET_K1;

  return SCE_KERNEL_ERROR_UNSUP;
}

int sceIoRemove(const char *name)
{
  int ret;

  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(name))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, name);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  void *path = alloc_pathbuf();
  if(path == 0)
  {
    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  PspIoDrvFileArg *desc;
  ret = alloc_iob(&desc);
  if(ret >= 0)
  {
    PspIoDrvArg *drv;
    int fs_num;
    char *filename = path;
    ret = parsefile(name, &drv, &fs_num, &path);
    if(ret >= 0)
    {
      desc->dev_type = ret;
      desc->fs_num = fs_num;
      desc->unk1 = 0x1000000;
      desc->drv = drv;

      if(drv->drv->funcs->IoRemove != 0)
      {
        ret = drv->drv->funcs->IoRemove(desc, filename);
      }
      else
      {
        ret = SCE_KERNEL_ERROR_UNSUP;
      }
    }

    unalloc_iob(desc);
  }

  if(path != 0)
  {
    free_pathbuf(path);
  }

  RESET_K1;

  return ret;
}

int sceIoRename(const char *oldname, const char *newname)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(oldname))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer oldname: 0x%x\n", __FUNCTION__, oldname);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE && IS_ADDR_KERNEL(newname))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer newname: 0x%x\n", __FUNCTION__, newname);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  void *pathbuf = alloc_pathbuf();
  if(pathbuf == 0)
  {
    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  void *pathbuf2 = alloc_pathbuf();
  if(pathbuf2 == 0)
  {
    free_pathbuf(pathbuf);

    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  PspIoDrvFileArg *desc;
  ret = alloc_iob(&desc);
  if(ret >= 0)
  {
    PspIoDrvArg *old_drv;
    int old_fs_num;
    char *poldname = pathbuf;
    ret = parsefile(oldname, &drv, &fs_num, &poldname);
    if(ret >= 0)
    {
      desc->dev_type = ret;
      if(strchr(oldname, ':') != 0)
      {
        PspIoDrvArg *new_drv;
        int new_fs_num;
        char *pnewname = pathbuf2;
        ret = parsefile(newname, &new_drv, &new_fs_num, &pnewname);
        if(ret >= 0)
        {
          if(new_drv == old_drv && new_fs_num == old_fs_num)
          {
            desc->fs_num = fs_num;
            desc->unk1 = 0x1000000;
            desc->drv = old_drv;

            if(old_drv->drv->funcs->IoRename != 0)
            {
              old_drv->drv->funcs->IoRename(desc, poldname, newname);
            }
            else
            {
              ret = SCE_KERNEL_ERROR_UNSUP;
            }
          }
          else
          {
            ret = SCE_KERNEL_ERROR_XDEV;
          }
        }
      }
      else
      {
        desc->fs_num = old_fs_num;
        desc->unk1 = 0x1000000;
        desc->drv = old_drv;

        if(old_drv->drv->funcs->IoRename != 0)
        {
          ret = old_drv->drv->funcs->IoRename(desc, poldname, newname);
        }
        else
        {
          ret = SCE_KERNEL_ERROR_UNSUP;
        }
      }
    }

    unalloc_iob(desc);
  }

  //834
  if(pathbuf != 0)
  {
    free_pathbuf(pathbuf);
  }

  if(pathbuf2 != 0)
  {
    free_pathbuf(pathbuf2);
  }

  RESET_K1;

  return ret;
}

int sceIoDevctl(const char *devname, unsigned int cmd, void *arg, int argsize, void *bufp, int bufsize)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDRSIZE_KERNEL(arg, argsize))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size, arg: 0x%x %d\n", __FUNCTION__, arg, argsize);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE && IS_ADDRSIZE_KERNEL(bufp, bufsize))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size, bufp: 0x%x %d\n", __FUNCTION__, bufp, bufsize);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE && (cmd & (1 << 15)))
  {
    Kprintf("iofilemgr.c:%s:Privileged ioctl operation\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  void *pathbuf = alloc_pathbuf();
  if(pathbuf == 0)
  {
    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  PspIoDrvFileArg *desc;
  ret = alloc_iob(&desc);
  if(ret >= 0)
  {
    PspIoDrvArg *drv;
    int fs_num;
    char *name = pathbuf;
    ret = parsefile(devname, &drv, &fs_num, &name);
    if(ret >= 0)
    {
      desc->dev_type = ret;
      desc->fs_num = fs_num;
      desc->unk1 = 0x1000000;
      desc->drv = drv;

      if(drv->drv->funcs->IoDevctl != 0)
      {
        ret = drv->drv->funcs->IoDevctl(devname, name, cmd, arg, argsize, bufp, bufsize);
      }
      else
      {
        ret = SCE_KERNEL_ERROR_UNSUP;
      }
    }

    unalloc_iob(desc);
  }
  
  if(s2 != 0)
  {
    free_pathbuf(s2);
  }
  
  RESET_K1;

  return ret;
}

int sceIoAssign(const char *aliasname, const char *physdevname, const char *fsdevname, int mode, void* unk1, long unk2)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(aliasname))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer aliasname: 0x%x\n", __FUNCTION__, aliasname);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE && IS_ADDR_KERNEL(physdevname))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer physdevname: 0x%x\n", __FUNCTION__, physdevname);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE && IS_ADDR_KERNEL(fsdevname))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer fsdevname: 0x%x\n", __FUNCTION__, fsdevname);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE && IS_ADDR_KERNEL(unk1))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size: 0x%x %d\n", __FUNCTION__, unk1, unk2);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(aliasname == 0 || (physdevname == 0 && fsdevname > 0))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_NODEV;
  }

  if(physdevname == 0 && fsdevname == 0)
  {
    physdevname = "dummy_drv_iofile:";
  }

  if(find_device_alias(aliasname) != 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ALIAS_USED;
  }

  char *s5 = physdevname;
  int dev_type = 0x20;
  int mount = 0;
  if(fsdevname != 0)
  {
    dev_type = 0x10;
    mount = 1;
    s5 = fsdevname;
  }

  PspIoDrvArg *drv;
  int fs_num;
  ret = sub_00001D20(s5, &drv, &fs_num);
  if(ret == 0)
  {
    Kprintf("iofilemgr.c:%s:No such FS(device) name:%s\n", __FUNCTION__, physdevname);

    RESET_K1;

    return SCE_KERNEL_ERROR_NODEV;
  }

  DeviceAlias *alias = allocate_device_alias();
  if(alias == 0)
  {
    RESET_K1,

    return SCE_KERNEL_ERROR_MFILE;
  }

  PspIoDrvFileArg *desc;
  ret = alloc_iob(&desc);
  if(ret < 0)
  {
    free_device_alias(alias);

    RESET_K1;

    return ret;
  }

  desc->fs_num = fs_num;
  desc->unk1 = 0x1000000;
  desc->drv = drv;

  if(mount != 0)
  {
    if(drv->drv->funcs->IoMount != 0)
    {
      ret = drv->drv->funcs->IoMount(desc, fsdevname, physdevname, mode, unk1);
    }
    else
    {
      ret = SCE_KERNEL_ERROR_UNSUP;
    }

    if(drv->drv->funcs->IoMount == 0 || ret < 0)
    {
      Kprintf("iofilemgr.c:%s:df_mount():err:0x%x\n", __FUNCTION__, ret);

      unalloc_iob(desc);

      free_device_alias(alias);

      RESET_K1;

      return ret;
    }
  }

  alias->drvArg = drv;
  alias->dev_type = dev_type;
  alias->fs_num = fs_num;

  while(*aliasname == ' ')
  {
    aliasname++;
  }

  char *colon = strchr(aliasname, ':');
  if(colon == 0)
  {
    Kprintf("iofilemgr.c:%s:Not driver name: %s\n", __FUNCTION__, aliasname);

    unalloc_iob(desc);

    free_device_alias(alias);

    RESET_K1;

    return SCE_KERNEL_ERROR_NODEV;
  }

  strncpy(alias->aliasname, aliasname, colon - aliasname);
  alias->aliasname[colon - aliasname] = 0;

  strcpy(alias->fsdevname, s5);

  while(*physdevname == ' ')
  {
    physdevname++;
  }

  colon = strchr(physdevname, ':');
  if(colon == 0)
  {
    //hmm...SCE being stoopid again? this should be physdevname, no?
    Kprintf("iofilemgr.c:%s:Not driver name: %s\n", __FUNCTION__, aliasname);

    unalloc_iob(desc);

    free_device_alias(alias);

    RESET_K1;

    return SCE_KERNEL_ERROR_NODEV;
  }

  do
  {
    colon--;
  } while(!(look_ctype_table(*colon) & 0x4));

  strncpy(alias->physdevname, physdevname, colon - physdevname);
  alias->physdevname[colon - physdevname] = 0;

  add_to_alias_tbl(alias);

  unalloc_iob(desc);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

int sceIoUnassign(const char *dev)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(dev))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, dev);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  DeviceAlias *alias = find_device_alias(dev);
  if(alias == 0)
  {
    Kprintf("iofilemgr.c:%s:No such alias name:%s\n", __FUNCTION__, dev);

    RESET_K1;

    return SCE_KERNEL_ERROR_NODEV;
  }

  PspIoDrvFileArg *desc;
  int ret = alloc_iob(&desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  desc->unk1 = 0x1000000;
  desc->fs_num = alias->fs_num;
  desc->drv = alias->drvArg;

  if(alias->dev_type == 0x10)
  {
    if(alias->drvArg->drv->funcs->IoUmount != 0)
    {
      ret = alias->drvArg->drv->funcs->IoUmount(desc, alias->fsdevname);
    }
    else
    {
      ret = SCE_KERNEL_ERROR_UNSUP;
    }

    if(alias->drvArg->drv->funcs->IoUmount == 0 || ret < 0)
    {
      Kprintf("iofilemgr.c:%s:df_umount() Fail!!\n", __FUNCTION__);

      unalloc_iob(sp_0);

      RESET_K1;

      return ret;
    }
  }

  if(free_alias_tbl(alias) != 0)
  {
    Kprintf("iofilemgr.c:%s:free_alias_tbl() Fail!!\n", __FUNCTION__);

    unalloc_iob(sp_0);

    RESET_K1;

    return SCE_KERNEL_ERROR_NODEV;
  }

  free_device_alias(alias);

  unalloc_iob(sp_0);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

int sceIoChangeThreadCwd(SceUID uid, char *dir)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(dir))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, dir);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  char *buf = alloc_pathbuf();
  if(buf == 0)
  {
    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  PspIoDrvArg *drv;
  int fs_num;
  char *dirname = buf;
  int ret = parsefile(dir, &drv, &fs_num, &dirname);
  if(ret >= 0)
  {
    char **cwd = sceKernelGetThreadKTLS(var_4E50, uid, 1);
    if(cwd != 0)
    {
      if(*cwd != 0)
      {
        sceKernelFreeHeapMemory(var_4E4C, *cwd);
      }

      *cwd = sceKernelAllocHeapMemory(var_4E4C, strlen(dirname) + 1);

      strcpy(*cwd, dirname);
    }
    else
    {
      ret = SCE_KERNEL_ERROR_NO_MEMORY;
    }
  }

  if(buf != 0)
  {
    free_pathbuf(buf);
  }

  RESET_K1;

  return ret;
}

int sceIoCancel(SceUID fd)
{
  SET_K1_SRL16;

  PspIoDrvFileArg *desc;
  ret = validate_fd(fd, 0, 8, 1, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  if(desc->drv->drv->funcs->IoCancel == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_UNSUP;
  }

  ret = desc->drv->drv->funcs->IoCancel(desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  int intr = sceKernelCpuSuspendIntr();

  if(desc->unk_3C != 0)
  {
    sceKernelTerminateDeleteThread(desc->unk_3C);
    desc->unk_3C = 0;

    sceKernelDeleteSema(desc->unk_40);
    desc->unk_40 = 0;

    sceKernelDeleteEventFlag(desc->unk_44);
    desc->unk_44 = 0;
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

int IoFileMgrForUser_5C2BE2CC(int *list, int listsize, int *count)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDRSIZE_KERNEL(list, listsize * sizeof(int)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE && IS_ADDR_KERNEL(count))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  int intr = sceKernelCpuSuspendIntr();

  int listcount = 0, total = 0;
  for(i = 0; i < MAX_FILES + 1; i++)
  {
    if(validate_fd(i, 0, 4, 0xF, &sp_0) >= 0)
    {
      total++;
      if(count < listsize)
      {
        list[listcount++] = i;
      }
    }
  }

  sceKernelCpuResumeIntr(intr);

  RESET_K1;

  if(count != 0)
  {
    *count = total;
  }

  return listcount;
}

int sceIoAddDrv(PspIoDrv *drv)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(drv))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, drv);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(find_drv_table(drv->name) != 0)
  {
    Kprintf("iofilemgr.c:%s:device '%s' is already registered\n", __FUNCTION__, drv->name);

    RESET_K1;

    return SCE_KERNEL_ERROR_REGDEV;
  }

  DrvTable *table = alloc_drv_table();
  if(table == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  table->drvArg.drv = drv;

  if(drv->funcs->IoInit == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_UNSUP;
  }

  int ret = drv->funcs->IoInit(&table->drvArg);
  if(ret < 0)
  {
    Kprintf("iofilemgr.c:%s:AddDrv(): cannot init\n", __FUNCTION__);

    free_drv_table(table);

    RESET_K1;

    return ret;
  }

  add_to_drv_table(table);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

int sceIoDelDrv(const char *drv_name)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(drv_name))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, drv_name);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  DrvTable *table = find_drv_table(drv_name);
  if(table == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_NODEV;
  }

  if(table->drv->funcs->IoExit == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_UNSUP;
  }

  remove_from_drv_table(table);

  int ret = table->drvArg.drv->funcs->IoExit(&table->drvArg);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  int intr = sceKernelCpuSuspendIntr();

  PspIoDrvFileArg *desc;
  for(i = 0; i < MAX_FILES + 1; i++)
  {
    if(validate_fd(i, 0, 0, 0xD, &desc) == 0 && desc->drv == &table->drvArg)
    {
      desc->drv = &var_4DC0;
    }
  }

  sceKernelCpuResumeIntr(intr);

  free_drv_table(table);

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

DrvTable *find_drv_table(char *drvname)
{
  int intr = sceKernelCpuSuspendIntr();

  DrvTable *table;
  for(table = var_4E40; table; table = table->next)
  {
    if(!stricmp(drvname, table->drvArg.drv->name))
    {
      sceKernelCpuResumeIntr(intr);

      return table;
    }
  }

  sceKernelCpuResumeIntr(intr);

  return 0;
}

void add_to_drv_table(DrvTable *table)
{
  int intr = sceKernelCpuSuspendIntr();

  table->next = var_4E40;
  var_4E40 = table;

  sceKernelCpuResumeIntr(intr);
}

int remove_from_drv_table(DrvTable *table)
{
  int intr = sceKernelCpuSuspendIntr();

  DrvTable *tmp;
  for(tmp = var_4E40; tmp->next; tmp = tmp->next)
  {
    if(tmp->next == table)
    {
      tmp->next = table->next;

      sceKernelCpuResumeIntr(intr);

      return 0;
    }
  }

  sceKernelCpuResumeIntr(intr);

  return -1;
}

DrvTable *alloc_drv_table()
{
  DrvTable *table = (DrvTable *)sceKernelAllocHeapMemory(var_4E4C, sizeof(DrvTable));
  if(table != 0)
  {
    memset(table, 0, sizeof(DrvTable));
  }

  return table;
}

void free_drv_table(DrvTable *table)
{
  sceKernelFreeHeapMemory(var_4E4C, table);
}

void add_to_alias_tbl(DeviceAlias *alias)
{
  int intr = sceKernelCpuSuspendIntr();

  alias->next = var_4E44;
  var_4E44 = alias;

  sceKernelCpuResumeIntr(intr);
}

int free_alias_tbl(DeviceAlias *alias)
{
  int intr = sceKernelCpuSuspendIntr();

  DeviceAlias *tmp;
  for(tmp = var_4E44; tmp->next; tmp = tmp->next)
  {
    if(tmp->next == alias)
    {
      tmp->next = alias->next;

      sceKernelCpuResumeIntr(intr);

      return 0;
    }
  }

  sceKernelCpuResumeIntr(intr);

  return -1;
}

DeviceAlias *allocate_device_alias()
{
  DeviceAlias *alias = (DeviceAlias *)sceKernelAllocHeapMemory(var_4E4C, sizeof(DeviceAlias));
  if(alias != 0)
  {
    memset(alias, 0, sizeof(DeviceAlias));
  }

  return alias;
}

void free_device_alias(DeviceAlias *alias)
{
  sceKernelFreeHeapMemory(var_4E4C, alias);
}

//1990
int validate_fd(SceUID fd, unk_a1, unk_a2, unk_a3, PspIoDrvFileArg **unk_t0)
{
  if(fd > MAX_FILES || var_4E58[fd].unk_28 < 0)
  {
    if(!(unk_a3 & 0x4))
    {
      Kprintf("iofilemgr.c:%s:bad file descriptor %d\n", __FUNCTION__, fd);
    }

    return SCE_KERNEL_ERROR_BADF;
  }

  *unk_t0 = &var_4E58[fd];
  if(!(unk_a3 & 0x2) && (var_4E58[fd].unk_8 == &var_4DC0 || var_4E58[fd].unk_8 == 0))
  {
    return SCE_KERNEL_ERROR_DRIVER_DELETED;
  }

  if(!(unk_a3 & 0x1) && var_4E58[fd].unk_3C != 0)
  {
    if(sceKernelPollEventFlag(var_4E58[fd].unk_44, 1, 1, 0) == 0)
    {
      if(!(unk_a3 & 0x4))
      {
        Kprintf("iofilemgr.c:%s:fd %d: Async mode BUSY\n", __FUNCTION__, fd);
      }

      return SCE_KERNEL_ERROR_ASYNC_BUSY;
    }
  }

  if(unk_a1 != 0 && !(var_4E58[fd].unk_0 & unk_a1))
  {
    if(!(unk_a3 & 0x4))
    {
      Kprintf("iofilemgr.c:%s:bad file descriptor %d\n", __FUNCTION__, fd);
    }

    return SCE_KERNEL_ERROR_BADF;
  }

  if(!(unk_a3 & 0x8) && (var_4E58[fd].unk_0 & 0x1000000))
  {
    int thid = sceKernelGetThreadId();
    if(thid != var_4E58[fd].unk_2C && thid != var_4E58[fd].unk_3C)
    {
      return SCE_KERNEL_ERROR_BADF;
    }
  }

  return SCE_KERNEL_ERROR_OK;
}

int alloc_iob(PspIoDrvFileArg **unk_a0)
{
  int intr = sceKernelCpuSuspendIntr();

  for(int i = 0; i < MAX_FILES + 1; i++)
  {
    if(var_4E58[i].unk_28 < 0)
    {
      memset(&var_4E58[i], 0, 0x80);

      *unk_a0 = &var_4E58[i];

      var_4E58[i].unk_28 = i;
      var_4E58[i].unk_2C = sceKernelGetThreadId();
      var_4E58[i].unk_30 = IS_USER_MODE;
      var_4E58[i].unk_34 = 0;
      var_4E58[i].unk_38 = var_4D50;

      sceKernelCpuResumeIntr(intr);

      return i;
    }
  }

  sceKernelCpuResumeIntr(intr);

  Kprintf("iofilemgr.c:%s:SCE_KERNEL_ERROR_MFILE\n", __FUNCTION__);

  return SCE_KERNEL_ERROR_MFILE;
}

//1bf0
int unalloc_iob(PspIoDrvFileArg *desc)
{
  power_unlock(desc);

  int intr = sceKernelCpuSuspendIntr();

  if(desc->unk_3C != 0)
  {
    sceKernelTerminateDeleteThread(desc->unk_3C);
    desc->unk_3C = 0;

    sceKernelDeleteSema(desc->unk_40);
    desc->unk_40 = 0;

    sceKernelDeleteEventFlag(desc->unk_44);
    desc->unk_44 = 0;
  }

  desc->unk_28 = -1;
  desc->unk_8 = &var_4DC0;
  desc->unk_0 = 0;

  sceKernelCpuResumeIntr(intr);

  return SCE_KERNEL_ERROR_OK;
}

//1c80
int power_lock(PspIoDrvFileArg *desc)
{
  if(desc->unk_34 == 0)
  {
    desc->unk_34 = 1;

    if(desc->unk_30 == 0)
    {
      sceKernelPowerLock(0);
    }
    else
    {
      sceKernelPowerLockForUser(0);
    }
  }

  return SCE_KERNEL_ERROR_OK;
}

//1cd0
int power_unlock(PspIoDrvFileArg *desc)
{
  if(desc->unk_34 != 0)
  {
    desc->unk_34 = 0;

    if(desc->unk_30 == 0)
    {
      sceKernelPowerUnlock(0);
    }
    else
    {
      sceKernelPowerUnlockForUser(0);
    }
  }
}

char *sub_00001D20(char *unk_a0, PspIoDrvArg **unk_a1, int *fs_num)
{
  while(*unk_a0 == ' ')
  {
    unk_a0++;
  }

  char *colon = strchr(unk_a0, ':');
  if(colon == 0)
  {
    return 0;
  }

  *unk_a2 = 0;
  char sp_0[0x400];
  strncpy(sp_0, unk_a0, colon - unk_a0);
  sp_0[colon - unk_a0] = 0;

  char *p = sp_0 + (colon - unk_a0);
  if(look_ctype_table(*(p - 1)) & 0x4)
  {
    for(; look_ctype_table(*(p - 1)) & 0x4; p--);

    *unk_a2 = strtol(p, 0, 0xA);
  }
  *p = 0;

  DrvTable *table = find_drv_table(sp_0);
  if(table != 0)
  {
    *unk_a1 = &table->drvArg;

    return p + 1;
  }

  return 0;
}

DeviceAlias *find_device_alias(char *unk_a0)
{
  while(*unk_a0 == ' ')
  {
    unk_a0++;
  }

  char *colon = strchr(unk_a0, ':');
  if(colon == 0)
  {
    return 0;
  }

  char sp_0[0x400];
  strncpy(sp_0, unk_a0, colon - unk_a0);
  sp_0[colon - unk_a0] = 0;

  int intr = sceKernelCpuSuspendIntr();

  DeviceAlias *alias;
  for(alias = var_4E44; alias; alias = alias->next)
  {
    if(alias->unk_4 != 0 && !stricmp(unk_a0, alias->unk_10))
    {
      sceKernelCpuResumeIntr(intr);

      return alias;
    }
  }

  sceKernelCpuResumeIntr(intr);

  return 0;
}

//1f10
int parsefile(char *path, PspIoDrvArg **desc, int *fs_num, char **dirname)
{
  while(*unk_a0 == ' ')
  {
    unk_a0++;
  }

  int len;
  char *colon = strchr(unk_a0, ':');
  if(colon == 0)
  {
    char **cwd = sceKernelGetKTLS(var_4E50);
    if(cwd == 0 || *cwd == 0)
    {
      Kprintf("iofilemgr.c:%s:Unset CWD, Unknown device '%s'\n", __FUNCTION__, unk_a0);

      return SCE_KERNEL_ERROR_NOCWD;
    }

    strcpy(*unk_a3, *cwd);

    len = strlen(*unk_a3);

    if(*unk_a3[len - 1] != ':' && *unk_a3[len - 1] != '/' && *unk_a0 != '/')
    {
      *unk_a3[len] = '/';
      *unk_a3[len + 1] = 0;
    }
    else if(*unk_a3[len - 1] == '/' && *unk_a0 == '/')
    {
      *unk_a3[len - 1] = 0;
    }

    if(strlen(*unk_a3) + strlen(unk_a0) >= 0x3FF)
    {
      Kprintf("iofilemgr.c:%s:File name too long '%s'\n", __FUNCTION__, unk_a0);

      return SCE_KERNEL_ERROR_NAMETOOLONG;
    }

    strcat(*unk_a3, unk_a0);

    colon = strchr(*unk_a3, ':');
    if(colon == 0)
    {
      Kprintf("iofilemgr.c:%s:CWD: Unknown device '%s'\n", __FUNCTION__, *unk_a3);

      return SCE_KERNEL_ERROR_NODEV;
    }
  }

  char sp_0[0x400];
  strncpy(sp_0, unk_a0, colon - unk_a0);

  char *p = sp_0 + (colon - unk_a0);
  *p = 0;

  *unk_a2 = 0;
  if(look_ctype_table(*(p - 1)) & 0x4)
  {
    for(; look_ctype_table(*(p - 1)) & 0x4; p--);

    *unk_a2 = strtol(p, 0, 0xA);
  }

  int intr = sceKernelCpuSuspendIntr();

  DeviceAlias *alias;
  for(alias = var_4E44; alias; alias = alias->next)
  {
    if(alias->unk_4 != 0 && !stricmp(sp_0, alias->aliasname))
    {
      break;
    }
  }

  sceKernelCpuResumeIntr(intr);

  int ret;
  if(alias != 0)
  {
    *unk_a2 = alias->fs_num;
    *unk_a1 = alias->drvArg;
    ret = alias->unk_8;
  }
  else
  {
    *p = 0;

    DrvTable *drv = find_drv_table(sp_0);
    if(drv == 0)
    {
      Kprintf("iofilemgr.c:%s:Unknown device '%s'\n", __FUNCTION__, sp_0);

      return SCE_KERNEL_ERROR_NODEV;
    }

    *unk_a1 = &drv->drvArg;
    ret = drv->drvArg.drv->dev_type;
  }

  *unk_a3 = colon + 1;

  return ret;
}

int stricmp(unk_a0, unk_a1)
{
  if(unk_a0 == 0 || unk_a1 == 0)
  {
    return (unk_a0 != unk_a1) ? ((unk_a0 == 0 ? -1 : 0) | 1) : 0;
  }

  char *p1, *p2;
  for(p1 = unk_a0, p2 = unk_a1; *p1; p1++, p2++)
  {
    if(tolower(*p1) != tolower(*p2))
    {
      return *p1 - *p2;
    }
  }
  
  return 0;
}

//22b8
char *alloc_pathbuf()
{
  int intr = sceKernelCpuSuspendIntr();

  void *buf;
  if(var_4E48 > 0)
  {
    buf = var_6E58[--var_4E48];

    var_4E48--;
  }
  else
  {
    buf = sceKernelAllocHeapMemory(var_4E4C, 0x400);
  }

  sceKernelCpuResumeIntr(intr);

  return buf;
}

//2330
void free_pathbuf(char *buf)
{
  int intr = sceKernelCpuSuspendIntr();

  if(var_4E48 < 2)
  {
    var_6E58[var_4E48++] = buf;
  }
  else
  {
    sceKernelFreeHeapMemory(var_4E4C, buf);
  }

  sceKernelCpuResumeIntr(intr);
}

int init_iofm(SceSize argSize, void *args)
{
  var_4E4C = sceKernelCreateHeap(1, 0x2000, 1, "SceIofile");
  ASSERT_MSG(var_4E4C >= 0, "iofilemgr.c:%s:sceKernelCreateHeap failed: 0x%08x\n", var_4E4C);

  memset(var_4E58, 0, sizeof(PspIoDrvFileArg) * (MAX_FILES + 1));

  for(i = MAX_FILES; i >= 0; i--)
  {
    var_4E58[i].unk_28 = -1;
  }

  var_4E50 = sceKernelAllocateKTLS(sizeof(char **), sub_00002C08, 0);
  ASSERT_MSG(var_4E50 >= 0, "iofilemgr.c:%s:sceKernelAllocateKTLS failed: 0x%08x\n", var_4E50);

  sceIoDelDrv("dummy_drv_iofile");

  sceIoAddDrv(&var_4E20);

  return 0;
}

int terminate_iofm()
{
  DrvTable *table;
  for(table = &var_4E40; table; table = table->next)
  {
    if(table->drvArg.drv->funcs->IoExit != 0)
    {
      table->drvArg.drv->funcs->IoExit(&table->drvArg);
    }
  }

  ret = sceKernelFreeKTLS(var_4E50);
  if(ret < 0)
  {
    Kprintf("iofilemgr.c:%s:sceKernelFreeKTLS failed 0x%08x\n", __FUNCTION__, ret);
  }

  ret = sceKernelDeleteHeap(var_4E4C);
  if(ret < 0)
  {
    Kprintf("iofilemgr.c:%s:sceKernelDeleteHeap failed 0x%08x\n", __FUNCTION__, ret);
  }

  return 0;
}

int sceIoPollAsync(SceUID fd, SceInt64 *res)
{
  return do_get_async_stat(fd, res, 1, 0);
}

int sceIoWaitAsync(SceUID fd, SceInt64 *res)
{
  return do_get_async_stat(fd, res, 0, 0);
}

int sceIoWaitAsyncCB(SceUID fd, SceInt64 *res)
{
  return do_get_async_stat(fd, res, 0, 1);
}

int sceIoGetAsyncStat(SceUID fd, int poll, SceInt64 *res)
{
  return do_get_async_stat(fd, res, poll, 0);
}

int sceIoSetAsyncCallback(SceUID fd, SceUID cb, void *argp)
{
  SET_K1_SRL16;
  
  uidControlBlock *uid;
  if(sceKernelGetUIDcontrolBlock(cb, &uid) != 0 || (IS_USER_MODE && !(uid->attribute & 0x2)))
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }

  PspIoDrvFileArg *desc;
  ret = validate_fd(fd, 0, 2, 1, &desc);
  if(ret < 0)
  {
    RESET_K1;
    
    return ret;
  }
  
  desc->unk_48 = cb;
  desc->unk_4C = argp;
  
  RESET_K1;
  
  return SCE_KERNEL_ERROR_OK;
}

int sceIoClose(SceUID fd)
{
  return do_close(fd, 0, 1);
}

int sceIoCloseAsync(SceUID fd)
{
  return do_close(fd, 1, 0);
}

int sceIoCloseAll()
{
  SET_K1_SRL16;

  PspIoDrvFileArg *desc;
  for(i = 0; i < MAX_FILES + 1; i++)
  {
    if(validate_fd(i, 0, 4, 0xE, &desc) >= 0 && !(desc->unk_0 & 0x8))
    {
      if(desc->drv != &var_4DC0 && desc->drv->drv->funcs->IoClose != 0)
      {
        desc->drv->drv->funcs->IoClose(desc);
      }

      unalloc_iob(desc);
    }
  }

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

SceUID sceIoOpen(const char *file, int flags, SceMode mode)
{
  return do_open(file, flags, mode, 0);
}

SceUID sceIoOpenAsync(const char *file, int flags, SceMode mode)
{
  return do_open(file, flags, mode, 1);
}

int sceIoRead(SceUID fd, void *data, SceSize size)
{
  return do_read(fd, data, size, 0);
}

int sceIoReadAsync(SceUID fd, void *data, SceSize size)
{
  return do_read(fd, data, size, 1);
}

int sceIoWrite(SceUID fd, const void *data, SceSize size)
{
  return do_write(fd, data, size, 0);
}

int sceIoWriteAsync(SceUID fd, const void *data, SceSize size)
{
  return do_write(fd, data, size, 1);
}

int sceIoLseek(SceUID fd, SceOff offset, int whence)
{
  return do_lseek(fd, offset, whence, 0);
}

int sceIoLseekAsync(SceUID fd, SceOff offset, int whence)
{
  return do_lseek(fd, offset, whence, 1);
}

int sceIoLseek32(SceUID fd, int offset, int whence)
{
  return do_lseek(fd, offset, whence, 0);
}

int sceIoLseek32Async(SceUID fd, int offset, int whence)
{
  return do_lseek(fd, offset, whence, 1);
}

int sceIoIoctl(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
  return do_ioctl(fd, cmd, indata, inlen, outdata, outlen, 0);
}

int sceIoIoctlAsync(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
  return do_ioctl(fd, cmd, indata, inlen, outdata, outlen, 1);
}

int sceIoMkdir(const char *dir, SceMode mode)
{
  return xx_dir(dir, mode, 1);
}

int sceIoRmdir(const char *path)
{
  return xx_dir(path, 0, 2);
}

int sceIoChdir(const char *path)
{
  return xx_dir(path, 0, 0);
}

//TO DO: fix this! the 1st arg should possibly be a PspIoDrvFileArg; either way, the
//prototype in the SDK is wrong, and the args for IoDevctl in PspIoDrvFuncs are wrong
int sceIoSync(const char *device, unsigned int unk)
{
  SET_K1_SRL16;

  int sp_0 = unk;
  int ret = sceIoDevctl(device, 0x100, &sp_0, 4, 0, 0);

  RESET_K1;

  return ret;
}

int sceIoGetstat(const char *file, SceIoStat *stat)
{
  return xx_stat(file, stat, 0, 1);
}

int sceIoChstat(const char *file, SceIoStat *stat, int bits)
{
  return xx_stat(file, stat, bits, 0);
}

int sceIoGetDevType(SceUID fd)
{
  SET_K1_SRL16;

  PspIoDrvFileArg *desc;
  int ret = validate_fd(fd, 0, 1, 0, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  RESET_K1;

  return desc->dev_type;
}

int sceIoGetThreadCwd(SceUID fd, char *dir, int len)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(dir))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size: 0x%x %d\n", __FUNCTION__, dir, len);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  char **cwd = sceKernelGetThreadKTLS(var_4E50);
  if(cwd != 0 && *cwd != 0)
  {
    int srclen = strlen(*cwd);
    strncpy(dir, *cwd, MIN(srclen + 1, len));

    RESET_K1;

    return srclen + 1;
  }

  RESET_K1;

  return 0;
}

int dummy_drv_write(PspIoDrvFileArg *arg, const char *data, int len)
{
  if(KDebugForKernel_24C32559(0xF) == 1)
  {
    sceKernelDebugWrite(0, data, len);
  }

  return len;
}

//2c08 - KTLS callback
void sub_00002C08(unsigned int *ktls, void *arg)
{
  if(*ktls != 0)
  {
    sceKernelFreeHeapMemory(var_4E4C, *ktls);

    *ktls = 0;
  }
}

int deleted_drv_func32()
{
  return SCE_KERNEL_ERROR_DRIVER_DELETED;
}

SceOff deleted_drv_func()
{
  return SCE_KERNEL_ERROR_DRIVER_DELETED;
}

int do_get_async_stat(SceUID fd, SceInt64 *res, int unk_a2, int is_callback)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(res))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  PspIoDrvFileArg *desc;
  ret = validate_fd(fd, 0, 1, 1, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  if(desc->unk_3C == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_NOASYNC;
  }

  u32 evfstat;
  if(sceKernelPollEventFlag(desc->unk_44, 5, PSP_EVENT_WAITCLEAR, &evfstat) != 0)
  {
    if(!(evfstat & 0x1))
    {
      RESET_K1;

      return SCE_KERNEL_ERROR_NOASYNC;
    }

    if(unk_a2 == 0)
    {
      if(is_callback == 0)
      {
        ret = sceKernelWaitEventFlag(desc->unk_44, 5, PSP_EVENT_WAITCLEAR, &evfstat, 0);
      }
      else
      {
        ret = sceKernelWaitEventFlagCB(desc->unk_44, 5, PSP_EVENT_WAITCLEAR, &evfstat, 0);
      }

      if(ret < 0)
      {
        RESET_K1;

        return ret;
      }
    }
  }

  *res = desc->unk_60;
  if(desc->unk_50 == 0)
  {
    ret = sceKernelSignalSema(desc->unk_40, 1);
    if(ret < 0)
    {
      RESET_K1;

      return ret;
    }
  }
  else
  {
    unalloc_iob(desc);
  }

  RESET_K1;

  return SCE_KERNEL_ERROR_OK;
}

//2df4
int do_close(SceUID fd, int async, int unk_a2)
{
  SET_K1_SRL16;

  PspIoDrvFileArg *desc;
  int ret = validate_fd(fd, 0, 4, async ? 2 : 3, &desc);
  if(desc->unk_0 & 0x8)
  {
    Kprintf("iofilemgr.c:%s:bad file descriptor %d\n", __FUNCTION__, fd);

    RESET_K1;

    return SCE_KERNEL_ERROR_BADF;
  }

  if(desc->drv != &var_4DC0)
  {
    if(desc->drv->drv->funcs->IoClose == 0)
    {
      RESET_K1;

      return SCE_KERNEL_ERROR_UNSUP;
    }

    if(async != 0)
    {
      ret = sub_0000396C(desc, 2);
    }
    else
    {
      ret = desc->drv->drv->funcs->IoClose(desc);
      if(ret >= 0)
      {
        ret = 0;
      }
    }
  }
  else
  {
    ret = 0;
    if(async != 0)
    {
      ret = sub_0000396C(desc, 0);
    }
  }

  if(unk_a2 > 0 && ret > 0)
  {
    unalloc_iob(desc);
  }

  RESET_K1;

  return ret;
}

int do_open(const char *file, int flags, SceMode mode, int async)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(file))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, file);

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  PspIoDrvFileArg *desc;
  int ret = alloc_iob(&desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  desc->drv = -1;
  desc->unk_68[0] = file;
  desc->unk_68[1] = flags;
  desc->unk_68[2] = mode;

  if(async != 0)
  {
    ret = sub_0000396C(desc, 1);
    if(ret >= 0)
    {
      ret = desc->unk_28;
    }
  }
  else
  {
    ret = open_main(desc);
  }

  if(ret < 0)
  {
    unalloc_iob(desc);
  }

  RESET_K1;

  return ret;
}

int do_read(SceUID fd, void *data, SceSize size, int async)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDRSIZE_KERNEL(data, size))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size: 0x%x %d\n", __FUNCTION__, data, size);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  PspIoDrvFileArg *desc;
  int ret = validate_fd(fd, 1, 4, 0, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }
  
  if(desc->drv->drv->funcs->IoRead == 0)
  {
    RESET_K1;
    
    return SCE_KERNEL_ERROR_UNSUP;
  }
  
  if(async != 0)
  {
    desc->unk_68[0] = data;
    desc->unk_68[1] = size;
    ret = sub_0000396C(desc, 3);
  }
  else
  {
    ret = desc->drv->drv->funcs->IoRead(desc, data, size);
  }

  RESET_K1;

  return ret;
}

int do_write(SceUID fd, const void *data, SceSize size, int async)
{
  SET_K1_SRL16;
  
  if(IS_USER_MODE && IS_ADDRSIZE_KERNEL(data, size))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size: 0x%x %d\n", __FUNCTION__, data, size);
    
    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }
  
  PspIoDrvFileArg *desc;
  int ret = validate_fd(fd, 2, 2, 0, &desc);
  if(ret < 0)
  {
    RESET_K1;
    
    return ret;
  }
  
  if(desc->drv->drv->funcs->IoWrite == 0)
  {
    RESET_K1;
    
    return SCE_KERNEL_ERROR_UNSUP;
  }
  
  if(async != 0)
  {
    desc->unk_68[0] = data;
    desc->unk_68[1] = size;
    ret = sub_0000396C(desc, 4);
  }
  else
  {
    ret = desc->drv->drv->funcs->IoWrite(desc, data, size);
  }
  
  RESET_K1;
  
  return ret;
}

SceInt64 do_lseek(SceUID fd, SceOff offset, int whence, int async)
{
  SET_K1_SRL16;

  PspIoDrvFileArg *desc;
  SceInt64 ret = validate_fd(fd, 0, 4, 0, &desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  if(whence > PSP_SEEK_END)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_INVAL;
  }

  if(desc->drv->drv->funcs->IoLseek == 0)
  {
    RESET_K1;

    return SCE_KERNEL_ERROR_UNSUP;
  }

  if(async != 0)
  {
    (SceOff)(desc->unk_68[0]) = offset;
    desc->unk_68[2] = whence;
    ret = sub_0000396C(desc, 5);
  }
  else
  {
    ret = desc->drv->drv->funcs->IoLseek(desc, offset, whence);
  }

  RESET_K1;

  return ret;
}

int do_ioctl(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen, int async)
{
  SET_K1_SRL16;
  
  if(IS_USER_MODE && IS_ADDRSIZE_KERNEL(indata, inlen))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size, argp: 0x%x %d\n", __FUNCTION__, indata, inlen);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  if(IS_USER_MODE && IS_ADDRSIZE_KERNEL(outdata, outlen))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer and size, argp: 0x%x %d\n", __FUNCTION__, outdata, outlen);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }
  
  if(IS_USER_MODE && (cmd & (1 << 15)))
  {
    Kprintf("iofilemgr.c:%s:Privileged ioctl operation\n", __FUNCTION__);
    
    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_PERM;
  }
  
  PspIoDrvFileArg *desc;
  int ret = validate_fd(fd, 0, 2, 0, &desc);
  if(ret < 0)
  {
    RESET_K1;
    
    return ret;
  }
  
  if(desc->drv->drv->funcs->IoIoctl == 0)
  {
    RESET_K1;
    
    return SCE_KERNEL_ERROR_UNSUP;
  }
  
  if(async != 0)
  {
    desc->unk_68[0] = cmd;
    desc->unk_68[1] = indata;
    desc->unk_68[2] = inlen;
    desc->unk_68[3] = outdata;
    desc->unk_68[4] = outlen;
    ret = sub_0000396C(desc, 6);
  }
  else
  {
    ret = desc->drv->drv->funcs->IoIoctl(desc, cmd, indata, inlen, outdata, outlen);
  }
  
  RESET_K1;

  return ret;
}

int xx_dir(const char *dir, SceMode mode, int func)
{
  SET_K1_SRL16;

  if(IS_USER_MODE && IS_ADDR_KERNEL(dir))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer: 0x%x\n", __FUNCTION__, dir);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  char *pathbuf = alloc_pathbuf();
  if(pathbuf == 0)
  {
    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  PspIoDrvFileArg *desc;
  int ret = alloc_iob(&desc);
  if(ret < 0)
  {
    RESET_K1;

    return ret;
  }

  char buf[32];
  PspIoDrvArg *drv;
  int fs_num;
  char *dirname = pathbuf;
  ret = parsefile(dir, &drv, &fs_num, &dirname);
  if(ret >= 0)
  {
    desc->dev_type = ret;
    desc->unk1 = 0x1000000;
    desc->fs_num = fs_num;
    desc->drv = drv;

    if(func == 0)
    {
      sprintf(buf, "%s%d:", desc->drv->drv->name, fs_num);

      char **cwd = sceKernelGetKTLS(var_4E50);
      if(cwd != 0)
      {
        if(*cwd != 0)
        {
          sceKernelFreeHeapMemory(var_4E4C, *cwd);
        }
        
        *cwd = sceKernelAllocHeapMemory(var_4E4C, strlen(buf) + strlen(dirname));
        if(*cwd != 0)
        {
          strcpy(*cwd, buf);
          strcat(*cwd, dirname);
          
          ret = SCE_KERNEL_ERROR_OK;
        }
        else
        {
          ret = SCE_KERNEL_ERROR_NO_MEMORY;
        }
      }
      else
      {
        ret = SCE_KERNEL_ERROR_NO_MEMORY;
      }
    }
    else if(func == 1)
    {
      if(drv->drv->funcs->IoMkdir != 0)
      {
        ret = drv->drv->funcs->IoMkdir(desc, dirname, mode);
      }
      else
      {
        ret = SCE_KERNEL_ERROR_UNSUP;
      }
    }
    else if(func == 2)
    {
      if(drv->drv->funcs->IoRmdir != 0)
      {
        ret = drv->drv->funcs->IoRmdir(desc, dirname);
      }
      else
      {
        ret = SCE_KERNEL_ERROR_UNSUP;
      }
    }
    else
    {
      ret = SCE_KERNEL_ERROR_UNSUP;
    }
  }

  unalloc_iob(desc);

  if(pathbuf != 0)
  {
    free_pathbuf(pathbuf);
  }

  RESET_K1;

  return ret;
}

int xx_stat(const char *file, SceIoStat *stat, int bits, int func)
{
  s6 = bits;
  s5 = stat;
  s4 = func;
  s1 = file;
  
  SET_K1_SRL16;
  
  if(IS_USER_MODE && IS_ADDR_KERNEL(file))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer path: 0x%x\n", __FUNCTION__, file);
    
    RESET_K1;
    
    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }
  
  if(IS_USER_MODE && IS_ADDR_KERNEL(stat))
  {
    Kprintf("iofilemgr.c:%s:Overrange pointer buf: 0x%x\n", __FUNCTION__, stat);

    RESET_K1;

    return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
  }

  char *pathbuf = alloc_pathbuf();
  if(pathbuf == 0)
  {
    Kprintf("iofilemgr.c:%s:alloc_pathbuf failed\n", __FUNCTION__);

    RESET_K1;

    return SCE_KERNEL_ERROR_NO_MEMORY;
  }

  PspIoDrvFileArg *desc;
  int ret = alloc_iob(&desc);
  if(ret >= 0)
  {
    PspIoDrvArg *drv;
    int fs_num;
    char *filename = pathbuf;
    ret = parsefile(file, &drv, &fs_num, &filename);
    if(ret >= 0)
    {
      desc->dev_type = ret;
      desc->unk1 = 0x1000000;
      desc->fs_num = fs_num;
      desc->drv = drv;

      if(func == 0)
      {
        if(drv->drv->funcs->IoChstat != 0)
        {
          ret = drv->drv->funcs->IoChstat(desc, filename, stat, bits);
        }
        else
        {
          ret = SCE_KERNEL_ERROR_UNSUP;
        }
      }
      else if(func == 1)
      {
        if(drv->drv->funcs->IoGetstat != 0)
        {
          ret = drv->drv->funcs->IoGetstat(desc, filename, stat);
        }
        else
        {
          ret = SCE_KERNEL_ERROR_UNSUP;
        }
      }
      else
      {
        ret = SCE_KERNEL_ERROR_UNSUP;
      }
    }
  }

  if(pathbuf != 0)
  {
    free_pathbuf(pathbuf);
  }

  RESET_K1;

  return ret;
}

int dummy_drv_func32()
{
  return 0;
}

SceOff dummy_drv_func()
{
  return 0;
}

int sub_0000396C(PspIoDrvFileArg *desc, int unk_a1)
{
  int ret;

  if(desc->unk_3C == 0)
  {
    ret = start_async_io(desc);
    if(ret < 0)
    {
      return ret;
    }
  }

  if(sceKernelPollSema(desc->unk_40, 1) < 0)
  {
    return SCE_KERNEL_ERROR_ASYNC_BUSY;
  }

  desc->unk_54 = 0;
  if(unk_a1 == 1)
  {
    char **ktls = (char *)sceKernelGetKTLS(var_4E50);
    if(ktls != 0 && *ktls != 0)
    {
      desc->unk_54 = sceKernelAllocHeapMemory(var_4E4C, strlen(*ktls) + 1);
      if(desc->unk_54 == 0)
      {
        sceKernelSignalSema(desc->unk_40, 1);

        return SCE_KERNEL_ERROR_NO_MEMORY;
      }

      strcpy(desc->unk_54, *ktls);
    }
  }

  desc->unk_58 = _GET_GPREG(GPREG_K1);
  desc->unk_5C = unk_a1;

  return sceKernelSetEventFlag(desc->unk_44, 3);
}

int start_async_io(PspIoDrvFileArg *desc)
{
  desc->unk_50 = 0;

  int priority = (desc->unk_38 < 0) ? sceKernelGetThreadCurrentPriority() : desc->unk_38;

  SceUID thid = sceKernelCreateThread("SceIofileAsync", async_loop, priority, 0x2000, sceKernelIsUserModeThread() ? (1 << 27) : 0, 0);
  if(thid < 0)
  {
    return thid;
  }
  desc->unk_3C = thid;

  SceUID semaid = sceKernelCreateSema("SceIofileAsync", 0, 1, 1, 0);
  if(semaid < 0)
  {
    sceKernelDeleteThread(desc->unk_3C);
    desc->unk_3C = 0;

    return semaid;
  }
  desc->unk_40 = semaid;

  SceUID evfid = sceKernelCreateEventFlag("SceIofileAsync", PSP_EVENT_WAITMULTIPLE, 0, 0);
  if(evfid < 0)
  {
    sceKernelDeleteThread(desc->unk_3C);
    desc->unk_3C = 0;

    sceKernelDeleteSema(desc->unk_40);
    desc->unk_40 = 0;

    return evfid;
  }
  desc->unk_44 = evfid;

  int ret = sceKernelStartThread(desc->unk_3C, 4, &desc);
  if(ret < 0)
  {
    sceKernelDeleteThread(desc->unk_3C);
    desc->unk_3C = 0;

    sceKernelDeleteSema(desc->unk_40);
    desc->unk_40 = 0;

    sceKernelDeleteEventFlag(desc->unk_44);
    desc->unk_44 = 0;

    return ret;
  }

  return ret;
}

int async_loop(SceSize args, void *argp)
{
  PspIoDrvFileArg **pdesc = (PspIoDrvFileArg **)argp;
  SceInt64 res;

  while(1 == 1)
  {
    u32 sp_0;
    ASSERT(sceKernelWaitEventFlag(desc->unk_44, 2, PSP_EVENT_WAITCLEAR | PSP_EVENT_WAITOR, &sp_0, 0) == 0);

    _SET_GPREG(GPREG_K1, (*pdesc)->unk_58);

    ASSERT((*pdesc)->unk_5C < 7);

    switch((*pdesc)->unk_5C)
    {
      case 0:
      {
        res = 0;
        (*pdesc)->unk_50 = 1;
        break;
      }

      case 1:
      {
        int *cwd = sceKernelGetKTLS(var_4E50);
        if(cwd != 0)
        {
          if(*cwd != 0)
          {
            sceKernelFreeHeapMemory(var_4E4C, *s1);
          }

          *cwd = (*pdesc)->unk_54;
          (*pdesc)->unk_54 = 0;
        }

        res = open_main((*pdesc));
        if(res < 0)
        {
          (*pdesc)->unk_50 = 1;
        }
        break;
      }

      case 2:
      {
        res = (*pdesc)->drv->drv->funcs->IoClose((*pdesc));
        if(res >= 0)
        {
          (*pdesc)->unk_54 = 1;
        }
        break;
      }

      case 3:
      {
        res = (*pdesc)->drv->drv->funcs->IoRead((*pdesc), (*pdesc)->unk_68[0], (*pdesc)->unk_68[1]);
        break;
      }

      case 4:
      {
        res = (*pdesc)->drv->drv->funcs->IoWrite((*pdesc), (*pdesc)->unk_68[0], (*pdesc)->unk_68[1]);
        break;
      }

      case 5:
      {
        res = (*pdesc)->drv->drv->funcs->IoLseek((*pdesc), (SceOff)(*pdesc)->unk_68[0], (*pdesc)->unk_68[2]);
        break;
      }

      case 6:
      {
        res = (*pdesc)->drv->drv->funcs->IoIoctl((*pdesc), (*pdesc)->unk_68[0], (*pdesc)->unk_68[1], (*pdesc)->unk_68[2],
                                                 (*pdesc)->unk_68[3], (*pdesc)->unk_68[4]);
        break;
      }
    }

    (*pdesc)->unk_60 = res;

    if((*pdesc)->unk_48 > 0)
    {
      sceKernelNotifyCallback((*pdesc)->unk_44, (*pdesc)->unk_4C);
    }

    ASSERT(sceKernelSetEventFlag((*pdesc)->unk_44, 0x4) == 0);
  }

  return 0;
}
