/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/time.h>

#include <dev/ide.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.
	// LAB 3: Your code here.
        user_mem_assert(curenv, (void *)s, len, PTE_U|PTE_P);
	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console.
// Returns the character.
static int
sys_cgetc(void)
{
	int c;

	// The cons_getc() primitive doesn't wait for a character,
	// but the sys_cgetc() system call does.
	while ((c = cons_getc()) == 0)
		/* do nothing */;

	return c;
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
        //cprintf("in sys_getenvid: curenv->env_id is %d\n",curenv->env_id);
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
        //cprintf("in kern/sys_exofork\n");
        envid_t parent_id = sys_getenvid();
        struct Env* envn;
        int ret = env_alloc(&envn, parent_id);
        if(ret!=0 || !envn){
                cprintf("sys_exofork : env_alloc failed.\n");
                return -E_NO_FREE_ENV;
        }
        struct Env* penv;
        if(envid2env(parent_id, &penv, 1)!=0){
                cprintf("sys_exofork : can't find penv.\n");
                return -E_INVAL;
        }
        ret = envn->env_id;
        envn->env_status = ENV_NOT_RUNNABLE;
        envn->env_tf = penv->env_tf;
        envn->env_tf.tf_regs.reg_eax = 0;
        //cprintf("out kern/sys_exofork\n");
        return ret;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
  	// Hint: Use the 'envid2env' function from kern/env.c to translate an
  	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
        if(status!=ENV_FREE && status!=ENV_RUNNABLE &&
           status!=ENV_NOT_RUNNABLE) {
                cprintf("sys_env_set_status : status illegal.\n");
                return -E_INVAL;
        }
        struct Env *env_store;
        if(envid2env(envid, &env_store, 1)==0) {
                env_store->env_status = status;
                return 0;
        }else {
                cprintf("sys_env_set_status : BAD_ENV.\n");
                return -E_BAD_ENV;
        }
        //cj done**********************OK
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 4: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
        if(!tf)
                return -E_INVAL;
        if((tf->tf_cs&3)!=3){
                cprintf("sys_env_set_trapframe : Illegal privi access\n");
                return -E_INVAL;
        }
        struct Env *env_store;
        if(envid2env(envid, &env_store, 1)!=0) {
                cprintf("sys_env_set_trapframe : BAD_ENV.\n");
                return -E_BAD_ENV;
        }
        env_store->env_tf = *tf;
        env_store->env_tf.tf_eflags |= FL_IF;
        return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
        struct Env *env_store;
        if(envid2env(envid, &env_store, 1)!=0) {
                cprintf("sys_env_set_pgfault_upcall : BAD_ENV.\n");
                return -E_BAD_ENV;
        }
        if((curenv->env_tf.tf_cs^0)==0) {
                cprintf("sys_env_set_pgfault_upcall : no permission.\n");
                return -E_BAD_ENV;
        }

        env_store->env_pgfault_upcall = func;
        return 0;
	//panic("sys_env_set_pgfault_upcall not implemented");
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
        //check para
        struct Env *env_store;
        if(envid2env(envid, &env_store, 1)!=0){
                cprintf("sys_page_alloc:envid is wrong\n");
                return -E_BAD_ENV;
        }
        if((physaddr_t)va>=UTOP||(physaddr_t)va&0xFFF){
                cprintf("sys_page_alloc:wrong va\n");
                return -E_INVAL;
        }
        if(!(perm & (PTE_U|PTE_P))){
                cprintf("sys_page_alloc : perm wrong--PTE_U|PTE_P not be set\n");
                return -E_INVAL;
        }else if(perm & (~PTE_USER)){//
                cprintf("sys_page_alloc : perm wrong--unknown perm bit.\n");
                return -E_INVAL;
        }
        struct Page *p;
        if(page_alloc(&p)!=0){
                cprintf("sys_page_alloc:page_alloc fails, no mem\n");
                return -E_NO_MEM;
        }
        //memset(page2kva, 0, sizeof(*p));
        int ret = page_insert(env_store->env_pgdir, p, va, perm);
        //cprintf("here and va is %p ret is %d perm is %08x\n", va, ret, perm);
        //memset(va, 0, sizeof(*p));
        if(ret)
                page_free(p);
        return ret;
        //cj-code-end***************************************
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
        //cprintf("perm is %08x\n",perm);
        struct Env *se, *de;
        if(envid2env(srcenvid, &se, 0)!=0 || envid2env(dstenvid, &de, 0)!=0){
                cprintf("sys_page_map:envid[%d] is wrong[%d]\n",
                        srcenvid, dstenvid);
                return -E_BAD_ENV;
        }

        if((se->env_tf.tf_cs&3)!=3 || (de->env_tf.tf_cs&3)!=3){
                cprintf("sys_page_map: no perm to access\n");
                return -E_BAD_ENV;
        }

        if((physaddr_t)srcva>=UTOP || PGOFF(srcva) ||
           (physaddr_t)dstva>=UTOP || PGOFF(dstva)) {
                cprintf("sys_page_map:wrong va\n");
                return -E_INVAL;
        }

        if( !(perm&(PTE_U|PTE_P)) ){
                cprintf("sys_page_map : perm wrong--PTE_U|PTE_P not be set\n");
                return -E_INVAL;
        }else if(perm & (~PTE_USER)){
                cprintf("sys_page_map : perm(%08x) wrong--unknown perm bit.\n",perm);
                //panic("");
                return -E_INVAL;
        }

        struct Page *sp;
        pte_t *pte_store;
        sp = page_lookup(se->env_pgdir, srcva, &pte_store);
        if(!sp){
                cprintf("sys_page_map : page_lookup return null\n");
                return -E_INVAL;
        }

        if(!(*pte_store&PTE_W) && perm&PTE_W){
                cprintf("sys_page_map : s&d invalid prim\n");
                return -E_INVAL;
        }
        // struct Page *dp;
        //if( page_alloc(&dp)!=0 ){
        //cprintf("sys_page_map : page_alloc fail\n");
        //return -E_NO_MEM;
        //}
	//memmove(page2kva(dp), page2kva(sp), PGSIZE);
        int ret = page_insert(de->env_pgdir, sp, dstva, perm);
        //cprintf("ret is %d\n",ret);
        return ret;
        //cj-code-end***************************************
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
        struct Env *env_store;
        if(envid2env(envid, &env_store, 1)!=0) {
                cprintf("sys_page_unmap:envid is wrong\n");
                return -E_BAD_ENV;
        }

        if((env_store->env_tf.tf_cs&3)!=3) {
                cprintf("sys_page_unmap: no perm to access\n");
                return -E_BAD_ENV;
        }

        if((physaddr_t)va>=UTOP||(physaddr_t)va&0xFFF){
                cprintf("sys_page_unmap:wrong va\n");
                return -E_INVAL;
        }
        page_remove(env_store->env_pgdir, va);
        return 0;
        //cj-code-end***************************************
}

// Try to send 'value' to the target env 'envid'.
// If va != 0, then also send page currently mapped at 'va',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target has not requested IPC with sys_ipc_recv.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused ipc_recv system call.
//
// If the sender sends a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc doesn't happen unless no errors occur.
//
// Returns 0 on success where no page mapping occurs,
// 1 on success where a page mapping occurs, and < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
        ///*
        struct Env *recv;
        if(envid2env(envid, &recv, 0)<0) {
                cprintf("sys_ipc_try_send : environment envid doesn't currently exist.\n");
                return -E_BAD_ENV;
        }

        if(recv->env_ipc_recving == 0) {
                //cprintf("sys_ipc_try_send : recv env[%08x] is not prepared.\n", envid);
                return -E_IPC_NOT_RECV;
        }
        //cprintf("------------------------------------------------\n");

        if((uint32_t)srcva < UTOP) {
                if(PGOFF(srcva)) {
                        cprintf("sys_ipc_try_send : srcva is not page-aligned.\n");
                        return -E_INVAL;
                }

                if(!(perm & (PTE_U|PTE_P))){
                        cprintf("sys_ipc_try_send : perm[%08x] wrong--PTE_U|PTE_P not be set\n", perm);
                        return -E_INVAL;
                }else if(perm & (0x1F8)){//
                        cprintf("sys_ipc_try_send : perm wrong--unknown perm bit.\n");
                        return -E_INVAL;
                }

                //if(!page_lookup(curenv->env_pgdir, srcva, 0)) {
                        //cprintf("sys_ipc_try_send : srcva is not mapped.\n");
                        //return -E_INVAL;
                //}

                int ret;
                if((int)recv->env_ipc_dstva!=-1)
                        ret = sys_page_map(curenv->env_id, srcva, recv->env_id,
                                           recv->env_ipc_dstva, perm);
                else
                        ;//ret = sys_page_map(curenv->env_id, srcva, recv->env_id, srcva, perm);

                if(ret < 0)
                        return ret;
                else
                        recv->env_ipc_perm = perm;
        }else
                 recv->env_ipc_perm = 0;

        recv->env_ipc_recving = 0;
        recv->env_ipc_value = value;
        recv->env_ipc_from = curenv->env_id;
        recv->env_status = ENV_RUNNABLE;

        return 0;
        //*/
	//panic("sys_ipc_try_send not implemented");
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	//panic("sys_ipc_recv not implemented");
        if(curenv->env_ipc_recving) {//it is waiting for a ipc
                panic("curenv is waiting for a ipc already, something wrong\n");
        }else {
                if((int)dstva == -1)
                        curenv->env_ipc_dstva = dstva;
                else if((uint32_t)dstva<UTOP && PGOFF(dstva)) {
                        cprintf("sys_ipc_recv : dstva is illegal.\n");
                        //panic("'");
                        return -E_INVAL;
                }
                curenv->env_ipc_recving = 1;
                curenv->env_ipc_dstva = dstva;
                curenv->env_status = ENV_NOT_RUNNABLE;
                curenv->env_tf.tf_regs.reg_eax = 0;
                //cprintf("begin wait recv,curenv is %08x\n",curenv->env_id);
                sched_yield();
        }
        //*********************cj-code-end
	//panic("sys_ipc_recv not implemented");
        return 0;
}

static void
sys_dump_env(void){
         //print env info;
         cprintf("env_id = %08x\n", curenv->env_id);
         cprintf("env_parent_id = %08x\n", curenv->env_parent_id);
         cprintf("env_runs = %d\n",curenv->env_runs);
         cprintf("env_pgdir = %08x\n",curenv->env_pgdir);
         cprintf("env_cr3 = %08x\n", curenv->env_cr3);
         cprintf("env_syscalls = %d\n", curenv->env_syscalls);
}

static int
sys_ide_read(uint32_t secno, void *dst, size_t nsecs)
{
        //test
        //return ide_pio_read(secno, dst, nsecs);
        return ide_dma_read(secno, dst, nsecs);
}

static int
sys_ide_write(uint32_t secno, void *src, size_t nsecs)
{
        return ide_dma_write(secno, src, nsecs);
        //return ide_pio_write(secno, src, nsecs);
}

static int
sys_time_msec() {
	return (int) time_msec();
}


// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
        curenv->env_syscalls++;
        switch(syscallno){
        case SYS_cputs:
                sys_cputs((char *)a1, (size_t)a2);break;
        case SYS_cgetc:
                sys_cgetc();break;
        case SYS_getenvid:
                return sys_getenvid();
        case SYS_env_destroy:
                return sys_env_destroy((envid_t)a1);
        case SYS_dump_env:
                sys_dump_env();break;
        case SYS_page_alloc:
                return sys_page_alloc((envid_t)a1, (void *)a2, (int)a3);
        case SYS_page_map: {
                return sys_page_map((envid_t)a1, (void *)a2,
                                    (envid_t)a3, (void *)a4, (int)a5);
        }
        case SYS_page_unmap:
                return sys_page_unmap((envid_t)a1, (void *)a2);
        case SYS_exofork:
                return sys_exofork();
        case SYS_env_set_status:
                return sys_env_set_status((envid_t)a1,(int)a2);
        case SYS_env_set_trapframe:
                return sys_env_set_trapframe((envid_t)a1,
                                             (struct Trapframe *)a2);
        case SYS_env_set_pgfault_upcall:
                return sys_env_set_pgfault_upcall((envid_t)a1, (void *)a2);
        case SYS_yield:
                sys_yield();break;//new add syscall for lab4;
        case SYS_ipc_try_send:
                return sys_ipc_try_send((envid_t)a1, (uint32_t)a2,
                                        (void *)a3, (unsigned)a4);
        case SYS_ipc_recv:
                return sys_ipc_recv((void *)a1);
        case SYS_ide_read:
                sys_ide_read((uint32_t)a1, (void *)a2, (size_t)a3);
                break;
        case SYS_ide_write:
                sys_ide_write((uint32_t)a1, (void *)a2, (size_t)a3);
                break;
        case SYS_time_msec:
                return sys_time_msec();
        case NSYSCALLS:
                break;
        default:
                return -E_INVAL;
        }
        return 0;

	//panic("syscall not implemented");
}

