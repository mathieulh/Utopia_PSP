#pragma once

// constants for use with _MFC0/_MFT0
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

// constants for use with _CFC0/_CTC0
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

// usage: int res = _MFC0(9);
#define _MFC0(c0dr) \
	({ \
		int res; \
		__asm__ __volatile__ \
		( \
			".set push"      "\n" \
			".set noreorder" "\n" \
			"mfc0 %0, $%1"   "\n" \
			".set pop"       "\n" \
			: "=r"(res) \
			: "i"(c0dr) \
			: "memory" \
		); \
		res; \
	})

// usage: _MTC0(count, 9);
#define _MTC0(value, c0dr) \
	do \
	{ \
		__asm__ __volatile__ \
		( \
			".set push"      "\n" \
			".set noreorder" "\n" \
			"mtc0 %0, $%1"   "\n" \
			".set pop"       "\n" \
			: \
			: "r"(value), "i"(c0dr) \
			: "memory" \
		); \
	} \
	while (0)

// usage: int res = _CFC0(9);
#define _CFC0(c0cr) \
	({ \
		int res; \
		__asm__ __volatile__ \
		( \
			".set push"      "\n" \
			".set noreorder" "\n" \
			"cfc0 %0, $%1"   "\n" \
			".set pop"       "\n" \
			: "=r"(res) \
			: "i"(c0cr) \
			: "memory" \
		); \
		res; \
	})

// usage: _CTC0(value, 9);
#define _CTC0(value, c0cr) \
	do \
	{ \
		__asm__ __volatile__ \
		( \
			".set push"      "\n" \
			".set noreorder" "\n" \
			"ctc0 %0, $%1"   "\n" \
			".set pop"       "\n" \
			: \
			: "r"(value), "i"(c0cr) \
			: "memory" \
		); \
	} \
	while (0)

// usage: int res = _MFIC();
#define _MFIC() \
	({ \
		int res; \
		__asm__ __volatile__ \
		( \
			".set push"      "\n" \
			".set noreorder" "\n" \
			"mfic %0, $0"    "\n" \
			".set pop"       "\n" \
			: "=r"(res) \
			: "memory" \
		); \
		res; \
	})

// usage: _MTIC(mask);
#define _MTIC(mask) \
	do \
	{ \
		__asm__ __volatile__ \
		( \
			".set push"      "\n" \
			".set noreorder" "\n" \
			"mtic %0, $0"    "\n" \
			".set pop"       "\n" \
			: \
			: "r"(res), "i"(mask) \
			: "memory" \
		); \
	} \
	while (0)

#define _SYNC() \
	do \
	{ \
		__asm__ __volatile__ \
		( \
			".set push"      "\n" \
			".set noreorder" "\n" \
			"sync"           "\n" \
			".set pop"       "\n" \
			: \
			: \
			: "memory" \
		); \
	} \
	while (0)

// usage: _CACHE(code,reference);
#define _CACHE(code,reference) \
	do \
	{ \
		__asm__ __volatile__ \
		( \
			".set push"      "\n" \
			".set noreorder" "\n" \
			"cache %0, %1"   "\n" \
			".set pop"       "\n" \
			: \
			: "i"(code), "m"(reference) \
			: "memory" \
		); \
	} \
	while (0)

// LL : Load Linked, read a value at reference and set LLbit to 1 (an exception execution will reset this LLBit to 0)
#define _LL(reference) \
	({ \
	  int res;
		__asm__ __volatile__ \
		( \
			".set push"           "\n" \
			".set noreorder"      "\n" \
      "ll %0, %1"           "\n" \
			".set pop"            "\n" \
			: "=r"(res) \
			: "m"(reference) \
			: "memory" \
		); \
		res;
	})

// SC : Store Conditional, write a value at reference if LLBit is 1 and return 1, otherwise just return 0
#define _SC(value,reference) \
	({ \
		int res = value;
		__asm__ __volatile__ \
		( \
			".set push"           "\n" \
			".set noreorder"      "\n" \
      "sc %0, %1"           "\n" \
			".set pop"            "\n" \
			: "+r"(res) \
			: "m"(reference) \
			: "memory" \
		); \
		res;
	})

// example of usage of _LL/_SC : atomic single FIFO list push/pop without interrups disabled
// principle : just redo if an interrupt occurs between LL and SC.   
#if 0
#define _ATOMIC_SLIST_PUSH(element_ptr,field_next,top_element_ptr) \
	do \
	{ \
		typeof(top_element_ptr) next_element_ptr = (typeof(top_element_ptr))_LL(*top_element_ptr); \
		element_ptr -> field_next = next_element_ptr; \
	} \
	while (!_SC(element_ptr, top_element_ptr));
		
#define _ATOMIC_SLIST_POP(field_next,top_element_ptr) \
	({ \
		typeof(top_element_ptr) element_ptr = 0; \
		typeof(element_ptr) next_element_ptr; \
		do \
		{ \
			element_ptr = (typeof(element_ptr))_LL(top_element_ptr); \
			next_element_ptr = element_ptr -> field_next; \
		} \
		while (!_SC(next_element_ptr, top_element_ptr));
		res; \
	})
#endif
