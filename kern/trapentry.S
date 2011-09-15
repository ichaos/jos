/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* The TRAPHANDLER macro defines a globally-visible function for handling
 * a trap.  It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 */ 
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
	//whether there is a error code decided by intel manual 
	TRAPHANDLER_NOEC(jos_divide_trap, T_DIVIDE);
	TRAPHANDLER_NOEC(jos_debug_trap, T_DEBUG);
	TRAPHANDLER_NOEC(jos_nmi_trap, T_NMI);
	TRAPHANDLER_NOEC(jos_brkpt_trap, T_BRKPT);
	TRAPHANDLER_NOEC(jos_oflow_trap, T_OFLOW);
	TRAPHANDLER_NOEC(jos_bound_trap, T_BOUND);
	TRAPHANDLER_NOEC(jos_illop_trap, T_ILLOP);
	TRAPHANDLER_NOEC(jos_device_trap, T_DEVICE);
	TRAPHANDLER(jos_dblflt_trap, T_DBLFLT);
	TRAPHANDLER(jos_tss_trap, T_TSS);
	TRAPHANDLER(jos_segnp_trap, T_SEGNP);
	TRAPHANDLER(jos_stack_trap, T_STACK);
	TRAPHANDLER(jos_gpflt_trap, T_GPFLT);//13
	TRAPHANDLER(jos_pgflt_trap, T_PGFLT);//14
	TRAPHANDLER_NOEC(jos_fperr_trap, T_FPERR);//16
	TRAPHANDLER(jos_align_trap, T_ALIGN);//17
	TRAPHANDLER_NOEC(jos_mchk_trap, T_MCHK);//18
	TRAPHANDLER_NOEC(jos_simderr_trap, T_SIMDERR);//19

	TRAPHANDLER_NOEC(jos_syscall_trap, T_SYSCALL);//int 0x80

	TRAPHANDLER_NOEC(jos_irq_timer_trap, IRQ_TIMER+IRQ_OFFSET) ;
	TRAPHANDLER_NOEC(jos_irq_kbd_trap, IRQ_KBD+IRQ_OFFSET) ;
	TRAPHANDLER_NOEC(jos_irq_spurious_trap, IRQ_SPURIOUS+IRQ_OFFSET) ;
	TRAPHANDLER_NOEC(jos_irq_ide_trap, IRQ_IDE+IRQ_OFFSET) ;
	TRAPHANDLER_NOEC(jos_irq_error_trap, IRQ_ERROR+IRQ_OFFSET) ;


/*
 * Lab 3: Your code here for _alltraps
 */
.global _alltraps
_alltraps:
	//push values to make the stack look like a struct Trapframe
	cli			;//there may be some other question???
	pushl %ds;
	pushl %es;
	pushal	 ;

	//load GD_KD into %ds and %es
	movl $GD_KD, %eax;
	movw %ax, %ds	 ;
	movw %ax, %es	 ;

	//pushl %esp  to pass a pointer to the Trapframe as an argument to trap()
	pushl %esp;

	//call trap
	call trap;

	
	//pop the values pushed in steps 1-3 
	pop %esp		;
	popal			;
	pop %es			;
	pop %ds			;

	//make sure the stack holds what iret expects 
	addl $8,%esp		;
	sti			;

	iret			;

.global sysenter_handler
.type sysenter_handler, @function	
sysenter_handler:
	pushl $GD_UD|3;
	pushl %ebp;
	pushfl;
	pushl $GD_UT|3;
	pushl %esi;
	movl  4(%ebp), %esi   ;
	pushl $0; 
	pushl $0x80; //or pushl $0x80 make a int 0x80 call.
	pushl %ds;
	pushl %es;
	pushal;
	movw $GD_KD, %ax;
	movw %ax, %ds;
	movw %ax, %es;
	pushl %esp;
	call syscall_stub;	//call trap;
	popl %esp;
	popal;
	movl 16(%esp), %esi	;
	popl %es;
	popl %ds;
	movl %ebp, %ecx;
	movl %esi, %edx;
	sti	       ;
	sysexit;
	
	
