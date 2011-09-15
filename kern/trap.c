#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/time.h>

#include <dev/ide.h>

static struct Taskstate ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
	return "(unknown trap)";
}


void
idt_init(void)
{
	extern struct Segdesc gdt[];
	// LAB 3: Your code here.
        //the address of entry point for T_DIVIDE trap i set in the trapentry.S
        extern physaddr_t jos_divide_trap;
        //set the trap get for the T_DIVIDE trap.
        SETGATE(idt[T_DIVIDE], 1, GD_KT, &jos_divide_trap, 0);//fault

        extern physaddr_t jos_debug_trap;
        SETGATE(idt[T_DEBUG], 1, GD_KT, &jos_debug_trap, 0);

        extern physaddr_t jos_nmi_trap;
        SETGATE(idt[T_NMI], 1, GD_KT, &jos_nmi_trap, 0);

        extern physaddr_t jos_brkpt_trap;
        SETGATE(idt[T_BRKPT], 1, GD_KT, &jos_brkpt_trap, 3);

        extern physaddr_t jos_oflow_trap;
        SETGATE(idt[T_OFLOW], 1, GD_KT, &jos_oflow_trap, 0);

        extern physaddr_t jos_bound_trap;
        SETGATE(idt[T_BOUND], 1, GD_KT, &jos_bound_trap, 0);

        extern physaddr_t jos_illop_trap;
        SETGATE(idt[T_ILLOP], 1, GD_KT, &jos_illop_trap, 0);

        extern physaddr_t jos_device_trap;
        SETGATE(idt[T_DEVICE], 1, GD_KT, &jos_device_trap, 0);

        extern physaddr_t jos_dblflt_trap;
        SETGATE(idt[T_DBLFLT], 1, GD_KT, &jos_dblflt_trap, 0);

        extern physaddr_t jos_tss_trap;
        SETGATE(idt[T_TSS], 1, GD_KT, &jos_tss_trap, 0);

        extern physaddr_t jos_segnp_trap;
        SETGATE(idt[T_SEGNP], 1, GD_KT, &jos_segnp_trap, 0);

        extern physaddr_t jos_stack_trap;
        SETGATE(idt[T_STACK], 1, GD_KT, &jos_stack_trap, 0);

        extern physaddr_t jos_gpflt_trap;
        SETGATE(idt[T_GPFLT], 1, GD_KT, &jos_gpflt_trap, 0);

        extern physaddr_t jos_pgflt_trap;
        SETGATE(idt[T_PGFLT], 1, GD_KT, &jos_pgflt_trap, 0);

        extern physaddr_t jos_fperr_trap;
        SETGATE(idt[T_FPERR], 1, GD_KT, &jos_fperr_trap, 0);

        extern physaddr_t jos_align_trap;
        SETGATE(idt[T_ALIGN], 1, GD_KT, &jos_align_trap, 0);

        extern physaddr_t jos_mchk_trap;
        SETGATE(idt[T_MCHK], 1, GD_KT, &jos_mchk_trap, 0);

        extern physaddr_t jos_simderr_trap;
        SETGATE(idt[T_SIMDERR], 1, GD_KT, &jos_simderr_trap, 0);

        extern physaddr_t jos_syscall_trap;
        SETGATE(idt[T_SYSCALL], 1, GD_KT, &jos_syscall_trap, 3);

        extern physaddr_t jos_irq_timer_trap;
        SETGATE(idt[IRQ_TIMER+IRQ_OFFSET], 0, GD_KT, &jos_irq_timer_trap, 0);

        extern physaddr_t jos_irq_kbd_trap;
        SETGATE(idt[IRQ_KBD+IRQ_OFFSET], 0, GD_KT, &jos_irq_kbd_trap, 0);

        extern physaddr_t jos_irq_spurious_trap;
        SETGATE(idt[IRQ_SPURIOUS+IRQ_OFFSET], 0, GD_KT, &jos_irq_spurious_trap, 0);

        extern physaddr_t jos_irq_ide_trap;
        SETGATE(idt[IRQ_IDE+IRQ_OFFSET], 0, GD_KT, &jos_irq_ide_trap, 0);

        extern physaddr_t jos_irq_error_trap;
        SETGATE(idt[IRQ_ERROR+IRQ_OFFSET], 0, GD_KT, &jos_irq_error_trap, 0);

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Initialize the TSS field of the gdt.
	gdt[GD_TSS >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[GD_TSS >> 3].sd_s = 0;

	// Load the TSS
	ltr(GD_TSS);

	// Load the IDT
	asm volatile("lidt idt_pd");
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	cprintf("  err  0x%08x\n", tf->tf_err);
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

//system call stub for sysenter
void
syscall_stub(struct Trapframe *tf)
{
        curenv->env_tf = *tf;
        tf->tf_regs.reg_eax =
                syscall(tf->tf_regs.reg_eax,
                        tf->tf_regs.reg_edx,
                        tf->tf_regs.reg_ecx,
                        tf->tf_regs.reg_ebx,
                        tf->tf_regs.reg_edi,
                        tf->tf_regs.reg_esi);
        if(tf->tf_regs.reg_eax==6)
                cprintf("in syscall_stub : the ret is %d\n", tf->tf_regs.reg_eax);
        return;
}

static void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
	// LAB 3: Your code here.
	switch(tf->tf_trapno){
        case T_PGFLT:
                page_fault_handler(tf);break;
        case T_BRKPT:
                 monitor(tf);break;
        case T_SYSCALL:
                syscall_stub(tf);break;
        case IRQ_TIMER+IRQ_OFFSET:
                //lab6: time tick increment
                time_tick();
                sched_yield();
                break;
        case IRQ_KBD+IRQ_OFFSET:
                break;
        case IRQ_IDE+IRQ_OFFSET:
                disk_irq_op.handler(disk_irq_op.arg);
                break;
        case IRQ_ERROR+IRQ_OFFSET:
                break;
        default:goto bad;
        }
        return;
 bad:

	// Handle clock interrupts.
	// LAB 4: Your code here.

	// Add time tick increment to clock interrupts.
	// LAB 6: Your code here.

	// Handle spurious interupts
	// The hardware sometimes raises these because of noise on the
	// IRQ line or other reasons. We don't care.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
		cprintf("Spurious interrupt on irq 7\n");
		print_trapframe(tf);
		return;
	}


	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}
}

void
trap(struct Trapframe *tf)
{
        tf->tf_eflags |= FL_IF;
	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		assert(curenv);
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}
	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// If we made it to this point, then no other environment was
	// scheduled, so we should return to the current environment
	// if doing so makes sense.
	if (curenv && curenv->env_status == ENV_RUNNABLE)
		env_run(curenv);
	else
		sched_yield();
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.
	// LAB 3: Your code here.
        if((tf->tf_cs^0)==0)//tf_cs is 0 : kernel mode
                panic("kernel page fault :(");
        //}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
	//
	// The page fault upcall might cause another page fault, in which case
	// we branch to the page fault upcall recursively, pushing another
	// page fault stack frame on top of the user exception stack.
	//
	// The trap handler needs one word of scratch space at the top of the
	// trap-time stack in order to return.  In the non-recursive case, we
	// don't have to worry about this because the top of the regular user
	// stack is free.  In the recursive case, this means we have to leave
	// an extra word between the current top of the exception stack and
	// the new stack frame because the exception stack _is_ the trap-time
	// stack.
	//
	// If there's no page fault upcall, the environment didn't allocate a
	// page for its exception stack, or the exception stack overflows,
	// then destroy the environment that caused the fault.
	//
	// Hints:
	//   user_mem_assert() and env_run() are useful here.
	//   To change what the user environment runs, modify 'curenv->env_tf'
	//   (the 'tf' variable points at 'curenv->env_tf').

	// LAB 4: Your code here.
        if(curenv->env_pgfault_upcall && &(curenv->env_tf)==tf) {
                struct UTrapframe *utf;

                if(tf->tf_esp <= USTACKTOP) {
                        utf = (struct UTrapframe *)((void *)UXSTACKTOP - sizeof(struct UTrapframe));
                }else if(USTACKTOP<=tf->tf_esp && tf->tf_esp<=UXSTACKTOP) {
                        utf = (struct UTrapframe *)((void *)(tf->tf_esp) - 4 - sizeof(struct UTrapframe));
                }else
                        panic("esp error in page fault handle\n");

                //cprintf("utf is %p\n", utf);
                user_mem_assert(curenv, utf, sizeof(struct UTrapframe), PTE_P|PTE_U|PTE_W);

                //tf->tf_eip = (uintptr_t)curenv->env_pgfault_upcall;
                //tf->tf_esp = (uintptr_t)utf;

                utf->utf_fault_va = fault_va;
                utf->utf_err = tf->tf_trapno;
                utf->utf_regs = tf->tf_regs;
                utf->utf_eip = tf->tf_eip;
                utf->utf_eflags = tf->tf_eflags;
                utf->utf_esp = tf->tf_esp;

                tf->tf_eip = (uintptr_t)curenv->env_pgfault_upcall;
                tf->tf_esp = (uintptr_t)utf;

                curenv->env_tf = *tf;

                env_run(curenv);
        }

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}

