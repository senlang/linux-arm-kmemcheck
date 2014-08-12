#include <linux/kernel.h>
#include <linux/bug.h>

#include "insn.h"

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
#if 0
	unsigned long rn = insn_field_value(insn, 16, 19);
	unsigned long rt = insn_field_value(insn, 12, 15);
	unsigned long b  = insn_field_value(insn, 22, 22);

	if (b) {
		__asm__ __volatile__("ldrexb %0, [%1]\n"
		: "=&r" (regs->uregs[rt])
		: "r" (regs->uregs[rn])
		: "cc", "memory");
	} else {
		__asm__ __volatile__("ldrex %0, [%1]\n"
		: "=&r" (regs->uregs[rt])
		: "r" (regs->uregs[rn])
		: "cc", "memory");
	}
#endif

	/* fail to emulate, so open this page for no tracing */
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
	unsigned long copy;
	unsigned long *target;

	if (load) {
		copy = *(unsigned long *)addr;
		target = reg;
	} else {
		copy = *reg;
		target = (unsigned long *)addr;
	}

	switch(size) {
	case 1:
		*(unsigned char *)target = (unsigned char) copy;
		break;
	case 4:
		*target = copy;
		break;
	default:
		BUG();
		break;
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


struct kmemcheck_action arm_action_table[] = {
	/* str/ldr imm offset/index */
	{ .mask = 0x0e000000, .value = 0x04000000, 
		.check = str_ldr_imm_check, .exec = str_ldr_imm_exec },

	/* str/ldr register offset/index */
	{ .mask = 0x0e000ff0, .value = 0x06000000,
		.check = str_ldr_reg_check, .exec = str_ldr_reg_exec },
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
	/* stmia rn, {list} */
	{ .mask = 0x0f900000, .value = 0x08800000,
		.check = stmia_check, .exec = stmia_simulate },


	/* ldrex{b} rt, [rn] */
	{ .mask = 0x0fb00fff, .value = 0x01900f9f, 
		.check = ldrexb_check, .exec = ldrexb_exec },

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
