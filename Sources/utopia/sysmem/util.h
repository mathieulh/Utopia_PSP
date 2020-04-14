#ifndef __UTIL_H
#define __UTIL_H

#define ASSERT(cond) { if(!(cond)) { Kprintf("Assertion failed at %s:%s:%04d\n", __FILE__, __FUNCTION__, __LINE__); sceKernelAssert(0, 0); } }
#define ASSERT_MSG(cond, msg, ...) { Kprintf(msg, ##__VA_ARGS__); ASSERT(cond); }

#define SET_MEM_STATUS(st) { if(GetCPUID() != 0) *((int *)0xBFC00400) = st; else *((int *)0xB0000000) = st; }

#define IS_ADDR_KERNEL(addr) (addr + sizeof(*(addr)) >= 0x80000000)

#define UIDCB_FROM_UID(uid)  ((uidControlBlock *)((((uid / 0x80) * 0x4) | 0x88000000))

#define IS_USER_MODE         ((k1 & 0x18) ? 1 : 0)

#define SET_K1_SRL16         int old_k1 = pspSdkGetK1(); int k1 = old_k1 >> 16; pspSdkSetK1(k1);

#define RESET_K1             pspSdkSetK1(old_k1);

#define IS_SET(x, b)         (((x) & (b)) ? 1 : 0)
#define SET_BIT(x, b)        (x) |= (b)
#define CLEAR_BIT(x, b)      (x) &= ~(b)
#define COPY_BITS(x, y, b)   { (x) &= ~(b); (x) |= (y) & (b); }

#define GPREG_ZR              0
#define GPREG_AT              1
#define GPREG_V0              2
#define GPREG_V1              3
#define GPREG_A0              4
#define GPREG_A1              5
#define GPREG_A2              6
#define GPREG_A3              7
#define GPREG_T0              8
#define GPREG_T1              9
#define GPREG_T2              10
#define GPREG_T3              11
#define GPREG_T4              12
#define GPREG_T5              13
#define GPREG_T6              14
#define GPREG_T7              15
#define GPREG_S0              16
#define GPREG_S1              17
#define GPREG_S2              18
#define GPREG_S3              19
#define GPREG_S4              20
#define GPREG_S5              21
#define GPREG_S6              22
#define GPREG_S7              23
#define GPREG_T8              24
#define GPREG_T9              25
#define GPREG_K0              26
#define GPREG_K1              27
#define GPREG_GP              28
#define GPREG_SP              29
#define GPREG_FP              30
#define GPREG_RA              31

#define COP0_SR_UNK7          7
#define COP0_SR_BADVADDR      8
#define COP0_SR_COUNT         9
#define COP0_SR_COMPARE       11
#define COP0_SR_STATUS        12
#define COP0_SR_CAUSE         13
#define COP0_SR_EPC           14
#define COP0_SR_PRID          15
#define COP0_SR_CONFIG        16
#define COP0_SR_UNK17         17
#define COP0_SR_WATCHLO       18
#define COP0_SR_WATCHHI       19
#define COP0_SR_SCCODE        21
#define COP0_SR_CPUID         22
#define COP0_SR_UNK23         23
#define COP0_SR_UNK24         24
#define COP0_SR_EBASE         25
#define COP0_SR_CACHE_ECC     26
#define COP0_SR_CACHE_ERR     27
#define COP0_SR_TAGLO         28
#define COP0_SR_TAGHI         29
#define COP0_SR_ERROREPC      30
#define COP0_SR_UNK31         31

#define COP0_CR_EPC           0
#define COP0_CR_EPC_ERR       1
#define COP0_CR_STATUS        2
#define COP0_CR_CAUSE         3
#define COP0_CR_GPR_V0        4
#define COP0_CR_GPR_V1        5
#define COP0_CR_GPR_V0_ERR    6
#define COP0_CR_GPR_V1_ERR    7
#define COP0_CR_EXC_TABLE     8
#define COP0_CR_EXC_31_ERROR  9
#define COP0_CR_EXC_27_DEBUG  10
#define COP0_CR_EXC_8_SYSCALL 11
#define COP0_CR_SC_TABLE      12
#define COP0_CR_SC_MAX        13
#define COP0_CR_GPR_SP_KERNEL 14
#define COP0_CR_GPR_SP_USER   15
#define COP0_CR_CURRENT_TCB   16
#define COP0_CR_UNK17         17
#define COP0_CR_NMI_TABLE     18
#define COP0_CR_COP0_STATUS_ERR 19
#define COP0_CR_COP0_CAUSE_ERR  20
#define COP0_CR_UNK21         21
#define COP0_CR_UNK22         22
#define COP0_CR_UNK_GPR_V0    23
#define COP0_CR_UNK_GPR_V1    24
#define COP0_CR_PROFILER_BASE 25
#define COP0_CR_GPR_V0_DBG    26
#define COP0_CR_GPR_V1_DBG    27
#define COP0_CR_DBGENV        28
#define COP0_CR_UNK29         29
#define COP0_CR_UNK30         30
#define COP0_CR_UNK31         31

#define STR(x)       #x

#define SYNC         asm("sync")
#define NOP          asm("nop")
#define ERET         asm("eret")

#define SYSCALL(i)   asm("syscall %0" :: "r"(i))
#define MAKE_SYSCALL(i)     ((((i) * 0x40) & 0x3FFFFC0) | 0xC)

#define MFC0_V(var, i) asm("mfc0 %0, $" STR(i) "\n" : "=r"(var))
#define MTC0_V(var, i) asm("mtc0 %0, $" STR(i) "\n" :: "r"(var))
#define MTC0_R(reg, i) asm("mtc0 $" STR(reg) ", $" STR(i) "\n")

#define CFC0_V(var, i) asm("cfc0 %0, $" STR(i) "\n" : "=r"(var))
#define CFC0_R(reg, i) asm("cfc0 $" STR(reg) ", $" STR(i) "\n")
#define CTC0_V(var, i) asm("ctc0 %0, $" STR(i) "\n" :: "r"(var))
#define CTC0_R(reg, i) asm("ctc0 $" STR(reg) ", $" STR(i) "\n")

#define MFIC_V(var, i) asm("mfic %0, $" STR(i) "\n" : "=r"(var))
#define MTIC_V(var, i) asm("mtic %0, $" STR(i) "\n" :: "r"(var))
#define MTIC_R(reg, i) asm("mtic $" STR(reg) ", $" STR(i) "\n")

int GetCPUID();

#endif
