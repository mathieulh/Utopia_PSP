#pragma once

// used to expand constants
#define STR(x) #x

// declare a function with its nid
#define PSP_FUNC_NID_NOALIAS(res,module,nid,...) \
	res module##_##nid( __VAR_ARGS__ )

// declare a function with its nid and its alias with its known name
#define PSP_FUNC_NID_ALIAS(res,module,nid,name,...) \
	res module##name( __VAR_ARGS__ ) __attribute__((weak, alias(#module "_" #nid))); \
	res module##_##nid( __VAR_ARGS__ )

extern int sceKernelAssert(int, int);

// assert condition is true
#define ASSERT(cond) { if(!(cond)) { Kprintf("Assertion failed at %s:%s:%04d\n", __FILE__, __FUNCTION__, __LINE__); sceKernelAssert(0, 0); } }

// assert condition is true
#define ASSERT_MSG(cond, msg, ...) \
        { \
          if(!(cond)) \
          { \
            Kprintf(msg, ##__VA_ARGS__); \
            Kprintf("Assertion failed at %s:%s:%04d\n", __FILE__, __FUNCTION__, __LINE__); \
            sceKernelAssert(0, 0); \
          } \
        }

// various k1 reg macros
#define SET_K1_SRL16         int old_k1 = _GET_GPREG(GPREG_K1); int k1 = old_k1 >> 16; _SET_GPREG(GPREG_K1, k1);
//restore k1 reg to its previous value; use before returning from any function that has used SET_K1_SRL16
#define RESET_K1             _SET_GPREG(GPREG_K1, old_k1);
//can only be used after SET_K1_SRL16 above
#define IS_USER_MODE         ((k1 & 0x18) ? 1 : 0)

// various helpers for bitmasks
#define IS_SET(x, b)         (((x) & (b)) ? 1 : 0)
#define SET_BIT(x, b)        (x) |= (b)
#define CLEAR_BIT(x, b)      (x) &= ~(b)
#define COPY_BITS(x, y, b)   { (x) &= ~(b); (x) |= (y) & (b); }

// return the content of the specified MIPS gp register; usage: int res = _GET_GPREG(31)
// NOTE: you can pass a #define'd constant
#define _GET_GPREG(r) \
	({ \
		int res; \
		__asm__ __volatile__ \
		( \
			".set push"           "\n" \
			".set noreorder"      "\n" \
      ".frame $sp, 0, $31"  "\n" \
      "move %0,$%1"         "\n" \
			".set pop"            "\n" \
			: "=r"(res) \
			: "i"(r) \
			: "memory" \
		); \
		res; \
	})

// sets the contents of the specified MIPS gp register; usage: _GET_GPREG(31, 0x12345678)
// NOTE: you can pass #define'd constants or variables for the args
#define _SET_GPREG(reg, val) \
	({ \
		__asm__ __volatile__ \
		( \
			".set push"           "\n" \
			".set noreorder"      "\n" \
      ".frame $sp, 0, $31"  "\n" \
      "move $" STR(reg) ", %0"          "\n" \
			".set pop"            "\n" \
			: \
			: "r"(val) \
		); \
	})
