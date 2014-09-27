#include <linux/kernel.h>
#include <linux/bug.h>
#include <linux/sort.h>
#include <asm/traps.h>

#include "insn.h"

extern struct kmemcheck_entry  __start__kmemcheck_table[];
extern struct kmemcheck_entry  __stop__kmemcheck_table[];

static int cmp_entry(const void *a, const void *b)
{
	const struct kmemcheck_entry *x = a, *y = b;

	if (x->ldrex_start > y->ldrex_start)
		return 1;
	if (x->ldrex_start < y->ldrex_start)
		return -1;
	return 0;
}

void __init sort_kmemcheck_table(void)
{
        if (__stop__kmemcheck_table > __start__kmemcheck_table) {
                pr_notice("Sorting kmemcheck_table...\n");
		sort(__start__kmemcheck_table, __stop__kmemcheck_table - __start__kmemcheck_table,
			sizeof(struct kmemcheck_entry), cmp_entry, NULL);
        }
}

const struct kmemcheck_entry *
search_kmemcheck_table(unsigned long addr)
{
	const struct kmemcheck_entry *first = __start__kmemcheck_table;
	const struct kmemcheck_entry *last  = __stop__kmemcheck_table;

	while(first <= last) {
		const struct kmemcheck_entry *mid;

		mid = ((last - first) >> 1) + first;
		if (mid->ldrex_start < addr)
			first = mid + 1;
		else if (mid->ldrex_start > addr)
			last = mid - 1;
		else
			return mid;
	}

	return NULL;
}
static unsigned long insn_field_value(unsigned long insn, int fstart, int fend)
{
	return (insn >> fstart) & ((1 << (fend - fstart + 1)) - 1);
}

void strb_check(unsigned long insn, struct pt_regs *regs, 
		unsigned long *addr, unsigned long *size)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long offset = insn_field_value(insn, 0, 11);
	unsigned long u = insn_field_value(insn, 23, 23);
	unsigned long b = insn_field_value(insn, 22, 22);

	if (u)
		*addr = regs->uregs[rn] + offset;
	else
		*addr = regs->uregs[rn] - offset;

	if (b)
		*size = 1;
	else
		*size = 4;
}

void strbr_check(unsigned long insn, struct pt_regs *regs,
		 unsigned long *addr, unsigned long *size)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long rm = insn_field_value(insn, 0, 3);
	unsigned long u = insn_field_value(insn, 23, 23);
	unsigned long b = insn_field_value(insn, 22, 22);

	*addr = u? regs->uregs[rn] + regs->uregs[rm]: 
		   regs->uregs[rn] - regs->uregs[rm];
	*size = b? 1 : 4;
}

int strb_exec(unsigned long insn, struct pt_regs *regs)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long offset = insn_field_value(insn, 0, 11);
	unsigned long rd = insn_field_value(insn, 12, 15);
	unsigned long u = insn_field_value(insn, 23, 23);
	unsigned long b = insn_field_value(insn, 22, 22);
	unsigned address = u? regs->uregs[rn] + offset : regs->uregs[rn] - offset;

	if (b)
		*(unsigned char *)address = (unsigned char)regs->uregs[rd];
	else
		*(unsigned long *)address = regs->uregs[rd];

	return 0;
}

int strbr_exec(unsigned long insn, struct pt_regs *regs)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long rm = insn_field_value(insn, 0, 3);
	unsigned long rd = insn_field_value(insn, 12, 15);
	unsigned long u = insn_field_value(insn, 23, 23);
	unsigned long b = insn_field_value(insn, 22, 22);
	unsigned address = u? regs->uregs[rn] + regs->uregs[rm] :
				regs->uregs[rn] - regs->uregs[rm];

	if (b)
		*(unsigned char *)address = (unsigned char)regs->uregs[rd];
	else
		*(unsigned long *)address = regs->uregs[rd];

	return 0;
}

static inline int reglist_bits(unsigned long reglist)
{
	int cnt = 0;

	while(reglist) {
		if (reglist & 1)
			cnt++;

		reglist >>= 1;
	}

	return cnt;
}

void stmia_check(unsigned long insn, struct pt_regs *regs,
		 unsigned long *addr, unsigned long *size)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long reglist = insn_field_value(insn, 0, 15);
	unsigned long cnt = 0;

	*addr = regs->uregs[rn];

	while(reglist) {
		if (reglist & 1)
			cnt++;

		reglist >>= 1;
			
	}

	*size = sizeof(unsigned long) * cnt;

}

int stmia_simulate(unsigned long insn, struct pt_regs *regs)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long reglist = insn_field_value(insn, 0, 15);
	unsigned long w = insn_field_value(insn, 21, 21);
	unsigned long s = insn_field_value(insn, 22, 22);
	unsigned long cnt = 0;
	unsigned long *addr;
	int i;

	addr = (unsigned long *)regs->uregs[rn];

	for (i = 0; i < 16; i++) {
		if (reglist & (1 << i)) {
			*addr = regs->uregs[i];
			++addr;	
			++cnt;
		}
	}
	
	if (w)
		regs->uregs[rn] += sizeof(unsigned long) * cnt;	

	if (s)
		BUG();

	return 0;
}

void strb_rpi_check(unsigned long insn, struct pt_regs *regs,
		    unsigned long *addr, unsigned long *size)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long b  = insn_field_value(insn, 22, 22);

	*addr = regs->uregs[rn];
	*size = b? 1 : 4;
}

int strb_rpi_exec(unsigned long insn, struct pt_regs *regs)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long rd = insn_field_value(insn, 12, 15);
	unsigned long rm = insn_field_value(insn,  0,  3);
	unsigned long b  = insn_field_value(insn, 22, 22);
	unsigned long u  = insn_field_value(insn, 23, 23);

	if (b)
		*(unsigned char *)regs->uregs[rn] = (unsigned char)regs->uregs[rd];
	else
		*(unsigned long *)regs->uregs[rn] = regs->uregs[rd];

	regs->uregs[rn] = u? regs->uregs[rn] + regs->uregs[rm] :
			     regs->uregs[rn] - regs->uregs[rm];

	return 0;
}

void ldrb_reg_check(unsigned long insn, struct pt_regs *regs,
		    unsigned long *addr, unsigned long *size)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long rm = insn_field_value(insn,  0,  3);
	unsigned long u  = insn_field_value(insn, 23, 23);
	unsigned long b  = insn_field_value(insn, 22, 22);

	*addr = u? regs->uregs[rn] + regs->uregs[rm] :
		   regs->uregs[rn] - regs->uregs[rm];

	*size = b? 1 : 4;
}

int ldrb_reg_exec(unsigned long insn, struct pt_regs *regs)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long rd = insn_field_value(insn, 12, 15);
	unsigned long rm = insn_field_value(insn,  0,  3);
	unsigned long u  = insn_field_value(insn, 23, 23);
	unsigned long b  = insn_field_value(insn, 22, 22);
	unsigned long addr = u? regs->uregs[rn] + regs->uregs[rm]:
				regs->uregs[rn] - regs->uregs[rm];

	if (b)
		regs->uregs[rd] = *(unsigned char *)addr;
	else
		regs->uregs[rd] = *(unsigned long *)addr;
	
	return 0;
}

void strb_post_imm_check(unsigned long insn, struct pt_regs *regs,
			 unsigned long *addr, unsigned long *size)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long b = insn_field_value(insn, 22, 22);

	*addr = regs->uregs[rn];
	*size = b? 1 : 4;
}

int strb_post_imm_exec(unsigned long insn, struct pt_regs *regs)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long rd = insn_field_value(insn, 12, 15);
	unsigned long offset = insn_field_value(insn, 0, 11);
	unsigned long u = insn_field_value(insn, 23, 23);
	unsigned long b = insn_field_value(insn, 22, 22);
	unsigned long addr = regs->uregs[rn];

	if (b)
		*(unsigned char *)addr = (unsigned char)regs->uregs[rd];
	else
		*(unsigned long *)addr = regs->uregs[rd];

	regs->uregs[rn] = u? regs->uregs[rn] + offset : regs->uregs[rn] - offset;

	return 0;
}

void ldrexb_check(unsigned long insn, struct pt_regs *regs,
		  unsigned long *addr, unsigned long *size)
{
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long b  = insn_field_value(insn, 22, 22);

	*addr = regs->uregs[rn];
	*size = b? 1 : 4;
}

int ldrexb_exec(unsigned long insn, struct pt_regs *regs)
{
	const struct kmemcheck_entry *entry;

	entry = search_kmemcheck_table(regs->ARM_pc);
	if (!entry)
		return 1;

#if 1
	{
		register unsigned int r0 asm("r0") = regs->ARM_r0;
		register unsigned int r1 asm("r1") = regs->ARM_r1;
		register unsigned int r2 asm("r2") = regs->ARM_r2;
		register unsigned int r3 asm("r3") = regs->ARM_r3;
		register unsigned int r4 asm("r4") = regs->ARM_r4;
		register unsigned int r5 asm("r5") = regs->ARM_r5;
		register unsigned int r6 asm("r6") = regs->ARM_r6;
		register unsigned int r7 asm("r7") = regs->ARM_r7;
		register unsigned int r8 asm("r8") = regs->ARM_r8;
		register unsigned int r9 asm("r9") = regs->ARM_r9;
		register unsigned int r10 asm("r10") = regs->ARM_r10;

		__asm__ __volatile__ (
			"blx	%[func]\n"
			: "=r"(r0), "=r"(r1), "=r"(r2), "=r"(r3), "=r"(r4),
			  "=r"(r5), "=r"(r6), "=r"(r7), "=r"(r8), "=r"(r9),
			  "=r"(r10)
			: "0"(r0), "1"(r1), "2"(r2), "3"(r3), "4"(r4), "5"(r5),
			  "6"(r6), "7"(r7), "8"(r8), "9"(r9), "10"(r10),
			  [func] "r" (entry->fixup_start)
			: "lr", "cc"
		);

		regs->ARM_r0 = r0;
		regs->ARM_r1 = r1;
		regs->ARM_r2 = r2;
		regs->ARM_r3 = r3;
		regs->ARM_r4 = r4;
		regs->ARM_r5 = r5;
		regs->ARM_r6 = r6;
		regs->ARM_r7 = r7;
		regs->ARM_r8 = r8;
		regs->ARM_r9 = r9;
		regs->ARM_r10 = r10;
	}
#else
		__asm__	__volatile__ (
		"	mov	r0, %[r0]\n"
		"	mov	r1, %[r1]\n"
		"	mov	r2, %[r2]\n"
		"	mov	r3, %[r3]\n"
		"	mov	r4, %[r4]\n"
		"	mov	r5, %[r5]\n"
		"	mov	r6, %[r6]\n"
		"	mov	r7, %[r7]\n"
		"	mov	r8, %[r8]\n"
		"	mov	r9, %[r9]\n"
		"	mov	r10, %[r10]\n"
		"	blx	%[func]\n"
		:
		: [r0] "r" (regs->ARM_r0),
		  [r1] "r" (regs->ARM_r1),	
		  [r2] "r" (regs->ARM_r2),	
		  [r3] "r" (regs->ARM_r3),	
		  [r4] "r" (regs->ARM_r4),	
		  [r5] "r" (regs->ARM_r5),	
		  [r6] "r" (regs->ARM_r6),	
		  [r7] "r" (regs->ARM_r7),	
		  [r8] "r" (regs->ARM_r8),	
		  [r9] "r" (regs->ARM_r9),	
		  [r10] "r" (regs->ARM_r10),
		  [func] "r" (entry->fixup_start)
		: "lr", "cc"
		);
	}
#endif

	regs->ARM_pc = entry->ldrex_next - 4;
	return 0;
}

void ldrexd_check(unsigned long insn, struct pt_regs *regs,
		  unsigned long *addr, unsigned long *size)
{
	unsigned long rn = insn_field_value(insn, 16, 19);

	*addr = regs->uregs[rn];
	*size = 8;
}

int ldrexd_exec(unsigned long insn, struct pt_regs *regs)
{
	return 1;
}

void str_ldr_imm_check(unsigned long insn, struct pt_regs *regs,
		       unsigned long *addr, unsigned long *size)
{
	int rn = insn_field_value(insn, 16, 19);
	int offset = insn_field_value(insn, 0, 11);
	int u = insn_field_value(insn, 23, 23);
	int b = insn_field_value(insn, 22, 22);
	int p = insn_field_value(insn, 24, 24);

	if (!u)
		offset = -offset;

	*addr = regs->uregs[rn];
	if (p)
		*addr += offset;

	*size = b? 1 : 4;
}

void generic_single_register_access(unsigned long *reg, unsigned long addr,
				    unsigned long size, int load)
{

	if (load) {
		switch(size) {
		case 1:
			*reg = *(u8 *)addr;
			break;
		case 2:
			*reg = *(u16 *)addr;
			break;
		case 4:
			*reg = *(u32 *)addr;
			break;
		default:
			BUG();
			break;
		}
	} else {
		switch(size) {
		case 1:
			*(u8 *)addr = *reg;
			break;
		case 2:
			*(u16 *)addr = *reg;
			break;
		case 4:
			*(u32 *)addr = *reg;
			break;
		default:
			BUG();
			break;
		}
	}

}

int str_ldr_imm_exec(unsigned long insn, struct pt_regs *regs)
{
	int rn = insn_field_value(insn, 16, 19);
	int rd = insn_field_value(insn, 12, 15);
	int offset = insn_field_value(insn, 0, 11);
	int u = insn_field_value(insn, 23, 23);
	int b = insn_field_value(insn, 22, 22);
	int p = insn_field_value(insn, 24, 24);
	int w = insn_field_value(insn, 21, 21);
	int l = insn_field_value(insn, 20, 20);

	unsigned long size;
	unsigned long addr;
	unsigned long base;
	

	size = b? 1 : 4;
	if (!u)
		offset = -offset;

	base = addr = regs->uregs[rn];
	if (p)
		addr += offset;

	generic_single_register_access(&regs->uregs[rd], addr, size, l);

	if(!p || w)
		regs->uregs[rn] = base + offset;

	return 0;
}

void str_ldr_reg_check(unsigned long insn, struct pt_regs *regs,
		       unsigned long *addr, unsigned long *size)
{
	int rn = insn_field_value(insn, 16, 19);
	int rm = insn_field_value(insn,  0,  3);
	int b  = insn_field_value(insn, 22, 22);
	int u  = insn_field_value(insn, 23, 23);
	int p  = insn_field_value(insn, 24, 24);
	unsigned long offset;

	offset = regs->uregs[rm];
	if(!u)
		offset = -offset;

	*addr = regs->uregs[rn];
	if (p)
		*addr += offset;

	*size = b? 1 : 4;
}

int str_ldr_reg_exec(unsigned long insn, struct pt_regs *regs)
{
	int rn = insn_field_value(insn, 16, 19);
	int rm = insn_field_value(insn,  0,  3);
	int rd = insn_field_value(insn, 12, 15);
	int l  = insn_field_value(insn, 20, 20);
	int w  = insn_field_value(insn, 21, 21);
	int b  = insn_field_value(insn, 22, 22);
	int u  = insn_field_value(insn, 23, 23);
	int p  = insn_field_value(insn, 24, 24);
	unsigned long offset;
	unsigned long size;
	unsigned long addr;
	unsigned long base;

	offset = regs->uregs[rm];
	base = addr = regs->uregs[rn];

	if (!u)
		offset = -offset;

	if (p)
		addr += offset;

	size = b? 1 : 4;
	
	generic_single_register_access(&regs->uregs[rd], addr, size, l);

	if(!p || w)
		regs->uregs[rn] = base + offset;		

	return 0;
}

void strh_ldrh_imm_check(unsigned long insn, struct pt_regs *regs,
			 unsigned long *addr, unsigned long *size)
{
	int rn = insn_field_value(insn, 16, 19);
	unsigned long imm = insn_field_value(insn, 8, 11) << 4
			  | insn_field_value(insn, 0,  3);
	int u = insn_field_value(insn, 23, 23);
	int p = insn_field_value(insn, 24, 24);	

	if (!u)
		imm = -imm;

	*addr = regs->uregs[rn];
	if (p)
		*addr += imm;

	*size = 2;
}

int strh_ldrh_imm_exec(unsigned long insn, struct pt_regs *regs)
{
	int rn = insn_field_value(insn, 16, 19);
	int rt = insn_field_value(insn, 12, 15);
	unsigned long imm = insn_field_value(insn, 8, 11) << 4
			  | insn_field_value(insn, 0,  3);
	int u = insn_field_value(insn, 23, 23);
	int l = insn_field_value(insn, 20, 20);
	int w = insn_field_value(insn, 21, 21);
	int p = insn_field_value(insn, 24, 24);

	unsigned long addr;
	unsigned long base;

	base = addr = regs->uregs[rn];
	
	if (!u)
		imm = -imm;

	if (p)
		addr += imm;

	generic_single_register_access(&regs->uregs[rt], addr, 2, l);

	if (!p || w)
		regs->uregs[rn] = base + imm;

	return 0;
}

unsigned long calc_scale_offset(unsigned long left, int shift,
			        unsigned long right, unsigned long carray)
{
	unsigned long offset = 0;

	switch(shift) {
	case 0x0: /* Logic Shift Left */
		offset = left << right;
		break;

	case 0x1: /* Logic Shift Right */
		if (!right)
			right = 32;
		offset = left >> right;
		break;

	case 0x2: /* ASR */
		if (!right)
			right = 32;
		offset = ((signed long)left) >> right;
		break;

	case 0x3: /* ROR or RRX */
		if (!right) { /* RRX */
			carray = !!carray;
			offset = (carray << 31) | (left >> 1);
		} else {
			offset = (left >> shift) | (left << (32 - shift)); 
		}
		break;
	default:
		BUG();	
	}

	return offset;
}

void str_ldr_scale_reg_check(unsigned long insn, struct pt_regs *regs,
			     unsigned long *addr, unsigned long *size)
{
	int rn = insn_field_value(insn, 16, 19);
	int rm = insn_field_value(insn,  0,  3);
	unsigned long shift_imm = insn_field_value(insn, 7, 11);
	unsigned int  shift = insn_field_value(insn, 5, 6);
	int b = insn_field_value(insn, 22, 22);
	int u = insn_field_value(insn, 23, 23);
	int p = insn_field_value(insn,24, 24);

	unsigned long offset = calc_scale_offset(regs->uregs[rm], shift,
					shift_imm, regs->ARM_cpsr & (1 << 29));

	if (!u)
		offset = -offset;

	*addr = regs->uregs[rn];
	if (p)
		*addr += offset;

	*size = b? 1 : 4;
}

int str_ldr_scale_reg_exec(unsigned long insn, struct pt_regs *regs)
{
	int rn = insn_field_value(insn, 16, 19);
	int rm = insn_field_value(insn,  0,  3);
	int rd = insn_field_value(insn, 12, 15);
	unsigned long shift_imm = insn_field_value(insn, 7, 11);
	unsigned int  shift = insn_field_value(insn, 5, 6);
	int b = insn_field_value(insn, 22, 22);
	int u = insn_field_value(insn, 23, 23);
	int l = insn_field_value(insn, 20, 20);
	int p = insn_field_value(insn, 24, 24);
	int w = insn_field_value(insn, 21, 21);

	unsigned long offset = calc_scale_offset(regs->uregs[rm], shift,
					shift_imm, regs->ARM_cpsr & (1 << 29));
	unsigned long addr;
	unsigned long base;
	unsigned long size;

	base = addr = regs->uregs[rn];

	if (!u)
		offset = -offset;

	if (p)
		addr += offset;

	size = b? 1 : 4;

	generic_single_register_access(&regs->uregs[rd], addr, size, l);

	if (!p || w)
		regs->uregs[rn] = base + offset;

	return 0;
}

void strh_ldrh_reg_check(unsigned long insn, struct pt_regs *regs,
			 unsigned long *addr, unsigned long *size)
{
	int rn = insn_field_value(insn, 16, 19);
	int rm = insn_field_value(insn,  0,  3);
	int u  = insn_field_value(insn, 23, 23);
	int p  = insn_field_value(insn, 24, 24); 

	unsigned offset = regs->uregs[rm];
	if (!u)
		offset = -offset;

	*addr = regs->uregs[rn];
	if (p)
		*addr += offset;

	*size = 2;
}

int strh_ldrh_reg_exec(unsigned long insn, struct pt_regs *regs)
{
	int rn = insn_field_value(insn, 16, 19);
	int rm = insn_field_value(insn,  0,  3);
	int rd = insn_field_value(insn, 12, 15);
	int u  = insn_field_value(insn, 23, 23);
	int p  = insn_field_value(insn, 24, 24);
	int l  = insn_field_value(insn, 20, 20);
	int w  = insn_field_value(insn, 21, 21);

	unsigned long addr;
	unsigned long base;
	unsigned long offset = regs->uregs[rm];

	base = addr = regs->uregs[rn];
	if (!u)
		offset = -offset;

	if (p)
		addr += offset; 

	generic_single_register_access(&regs->uregs[rd], addr, 2, l);

	if (!p || w)
		regs->uregs[rn] = base + offset;

	return 0;
}

void stm_ldm_check(unsigned long insn, struct pt_regs *regs,
		   unsigned long *addr, unsigned long *size)
{
	int rn = insn_field_value(insn, 16, 19);
	unsigned long reglist = insn_field_value(insn, 0, 15);
	int u = insn_field_value(insn, 23, 23);
	int p = insn_field_value(insn, 24, 24);

	int bits =  reglist_bits(reglist);

	if (u) {
		if (p) {
			/* IB */
			*addr = regs->uregs[rn] + 4;
		} else {
			/* IA */
			*addr = regs->uregs[rn];
		}
	} else {
		if (p) {
			/* DB */
			*addr = regs->uregs[rn] - bits * 4;
		} else {
			/* DA */
			*addr = regs->uregs[rn] - bits * 4 + 4;
		}
	}

	*size = 4 * bits;
}

void generic_mult_registers_access(unsigned long addr, struct pt_regs *regs,
				   unsigned long reglist, int l)
{
	int i;

	if (l) {
		for (i = 0; i < 16; ++i) {
			if (reglist & (1 << i)) {
				regs->uregs[i] = *(u32 *)addr;
				addr += 4;
			}
		}
	} else {
		for (i = 0; i < 16; ++i) {
			if (reglist & (1 << i)) {
				*(u32 *)addr = regs->uregs[i];
				addr += 4;
			}
		}
	}
}

int stm_ldm_exec(unsigned long insn, struct pt_regs *regs)
{
	int rn = insn_field_value(insn, 16, 19);
	unsigned long reglist = insn_field_value(insn, 0, 15);
	int u = insn_field_value(insn, 23, 23);
	int p = insn_field_value(insn, 24, 24);
	int w = insn_field_value(insn, 21, 21);
	int l = insn_field_value(insn, 20, 20);

	int bits = reglist_bits(reglist);
	unsigned long addr;
	unsigned long write;

        if (u) {
                if (p) {
                        /* IB */
                        addr = regs->uregs[rn] + 4;
			write = regs->uregs[rn] + bits * 4;
                } else {
                        /* IA */
                        addr = regs->uregs[rn];
			write = regs->uregs[rn] + bits * 4;
                }
        } else {
                if (p) {
                        /* DB */
                        addr = regs->uregs[rn] - bits * 4;
			write = regs->uregs[rn] - bits * 4;
                } else {
                        /* DA */
                        addr = regs->uregs[rn] - bits * 4 + 4;
			write = regs->uregs[rn] - bits * 4;
                }
        }

	generic_mult_registers_access(addr, regs, reglist, l);

	if (w)
		regs->uregs[rn] = write;

	return 0;
}

void strd_ldrd_imm_check(unsigned long insn, struct pt_regs *regs,
			 unsigned long *addr, unsigned long *size)
{
	int rn = insn_field_value(insn, 16, 19);
	unsigned imm = (insn_field_value(insn, 8, 11) << 4)
			| insn_field_value(insn, 0, 3);
	int u = insn_field_value(insn, 23, 23);
	int p = insn_field_value(insn, 24, 24);

	if (!u)
		imm = -imm;

	*addr = regs->uregs[rn];
	if (p)
		*addr += imm;

	*size = 8;
}

int strd_ldrd_imm_exec(unsigned long insn, struct pt_regs *regs)
{
	int rn = insn_field_value(insn, 16, 19);
	int rd = insn_field_value(insn, 12, 15);
	unsigned imm = (insn_field_value(insn, 8, 11) << 4)
			| insn_field_value(insn, 0, 3);
	int u = insn_field_value(insn, 23, 23);
	int p = insn_field_value(insn, 24, 24);
	int w = insn_field_value(insn, 21, 21);
	int l = !insn_field_value(insn,  5,  5); /* bit 5 means str */

	unsigned long addr;
	unsigned long base;

	base = addr = regs->uregs[rn];

	if (!u)
		imm = -imm;

	if (p)
		addr += imm;

	generic_single_register_access(&regs->uregs[rd], addr, 4, l);
	generic_single_register_access(&regs->uregs[rd + 1], addr + 4, 4, l);

	if (!p || w)
		regs->uregs[rn] = base + imm;

	return 0;
}

void strd_ldrd_reg_check(unsigned long insn, struct pt_regs *regs,
			 unsigned long *addr, unsigned long *size)
{
	int rn = insn_field_value(insn, 16, 19);
	int rm = insn_field_value(insn,  0,  3);
	int u  = insn_field_value(insn, 23, 23);
	int p  = insn_field_value(insn, 24, 24);

	unsigned long offset = regs->uregs[rm];
	if (!u)
		offset = -offset;

	*addr = regs->uregs[rn];
	if (p)
		*addr += offset;

	*size = 8;
}

int strd_ldrd_reg_exec(unsigned long insn, struct pt_regs *regs)
{
	int rn = insn_field_value(insn, 16, 19);
	int rm = insn_field_value(insn,  0,  3);
	int rd = insn_field_value(insn, 12, 15);
	int u  = insn_field_value(insn, 23, 23);
	int p  = insn_field_value(insn, 24, 24);
	int w  = insn_field_value(insn, 21, 21);
	int l  = !insn_field_value(insn,  5,  5); /* bit 5 means str */

	unsigned long addr;
	unsigned long base;
	unsigned long offset = regs->uregs[rm];

	base = addr = regs->uregs[rn];

	if (!u)
		offset = -offset;

	if (p)
		addr += offset;

	generic_single_register_access(&regs->uregs[rd], addr, 4, l);
	generic_single_register_access(&regs->uregs[rd + 1], addr + 4, 4, l);

	if (!p || w)
		regs->uregs[rn] = base + offset;

	return 0;
}

struct kmemcheck_action arm_action_table[] = {
	/* str/ldr imm offset/index */
	{ .mask = 0x0e000000, .value = 0x04000000, 
		.check = str_ldr_imm_check, .exec = str_ldr_imm_exec },

	/* str/ldr register offset/index */
	{ .mask = 0x0e000ff0, .value = 0x06000000,
		.check = str_ldr_reg_check, .exec = str_ldr_reg_exec },

	/* strh/ldrh imm offset/indx */
	{ .mask = 0x0e4000f0, .value = 0x004000b0,
		.check = strh_ldrh_imm_check, .exec = strh_ldrh_imm_exec },

	/* strg/ldrh register offset/index */
	{ .mask = 0x0e400ff0, .value = 0x000000b0,
		.check = strh_ldrh_reg_check, .exec = strh_ldrh_reg_exec },

	/* str/ldr scaled register offset/index */
	{ .mask = 0x0e000010, .value = 0x06000000,
		.check = str_ldr_scale_reg_check, .exec = str_ldr_scale_reg_exec },

	/* stm/ldm */
	{ .mask = 0x0e000000, .value = 0x08000000,
		.check = stm_ldm_check, .exec = stm_ldm_exec },

	/* strd/ldrd imm offset/index */
	{ .mask = 0x0e5000d0, .value = 0x004000d0,
		.check = strd_ldrd_imm_check, .exec = strd_ldrd_imm_exec },

	/* strd/ldrd regiset offset/index */
	{ .mask = 0x0e500fd0, .value = 0x000000d0,
		.check = strd_ldrd_reg_check, .exec = strd_ldrd_reg_exec },
#if 0
	/* str{b} rd, [rn, #offset] */
	{ .mask = 0x0f300000, .value = 0x05000000, KMEMCHECK_WRITE,
		.check = strb_check, .exec = strb_exec },	
	/* str{b}, rd, [rn, rm] */
	{ .mask = 0x0f300ff0, .value = 0x07000000, KMEMCHECK_WRITE,
		.check = strbr_check, .exec = strbr_exec },
	/* str{b}, rd, [rn], rm */
	{ .mask = 0x0f300000, .value = 0x06000000, KMEMCHECK_WRITE,
		.check = strb_rpi_check, .exec = strb_rpi_exec },
	/* str{b}, rd, [rn], offset */
	{ .mask = 0x0f300000, .value = 0x04000000, KMEMCHECK_WRITE,
		.check = strb_post_imm_check, .exec = strb_post_imm_exec },
	/* ldr{b}, rd, [rn, rm] */
	{ .mask = 0x0f300000, .value = 0x07100000, KMEMCHECK_READ,
		.check = ldrb_reg_check, .exec = ldrb_reg_exec },
#endif
#if 0
	/* stmia rn, {list} */
	{ .mask = 0x0f900000, .value = 0x08800000,
		.check = stmia_check, .exec = stmia_simulate },
#endif


	/* ldrex{b} rt, [rn] */
	{ .mask = 0x0fb00fff, .value = 0x01900f9f, 
		.check = ldrexb_check, .exec = ldrexb_exec },

	/* ldrexd rt, r<t+1>, [rn] */
	{ .mask = 0x0ff00fff, .value = 0x01b00f9f,
		.check = ldrexd_check, .exec = ldrexd_exec },

};

#define ACTION_SIZE	sizeof(arm_action_table)/sizeof(arm_action_table[0])

const struct kmemcheck_action *search_action_entry(unsigned  long insn)
{
	int i;

	for (i = 0; i < ACTION_SIZE; ++i)
		if ((insn & arm_action_table[i].mask) == arm_action_table[i].value)
			return &arm_action_table[i];

	return NULL;
}
