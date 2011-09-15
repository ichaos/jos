// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).
        pte_t pte = ((pte_t *)vpt)[PPN(addr)];
        //if(!pte&PTE_COW)
        // panic("pgfault:pte[%08x] prim error\n", pte);

	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.
	// LAB 4: Your code here.
        if(pte&PTE_COW && err&FEC_WR){
                void *cj = (void *)ROUNDDOWN(addr, PGSIZE);
                sys_page_alloc(0, (void *)PFTEMP, PTE_P|PTE_W|PTE_U);
                memmove((void *)PFTEMP, cj, PGSIZE);
                sys_page_map(0, (void *)PFTEMP, 0, cj, PTE_P|PTE_W|PTE_U);
        }else
                panic("pgfault:pte[%08x] @ addr[%08x] prim error\n", pte, addr);
        //------------cj-code-end*
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why mark ours copy-on-write again
// if it was already copy-on-write?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	void *addr;
	pte_t pte;
        //curenvid = sys_getenvid();

        //struct Env *curenv = env;

        addr = (void *)(pn*PGSIZE);
        pte = ((pte_t *)vpt)[pn];

        //pte = (env->env_pgdir[PDX(addr)]&(~0xfff))[PTX(addr)];
        //pte &= 0x000;
        //pte &= PTE_U|PTE_P|PTE_COW;

	// LAB 4: Your code here.
        if(pte&PTE_W || pte&PTE_COW) {
                if(sys_page_map(0, addr, envid, addr, PTE_U|PTE_P|PTE_COW))
                        panic("duppage : page_map failed.\n");
                if(sys_page_map(0, addr, 0, addr, PTE_U|PTE_P|PTE_COW))
                        panic("duppage : page_map failed.2\n");
        }
	//panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "env" and the user exception stack in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
        ///*
        envid_t envid;
        envid = sys_exofork();

        int i,j;

        //cprintf("fork\n");
        set_pgfault_handler(pgfault);
        //if(sys_env_set_pgfault_upcall(sys_getenvid(), pgfault))
        // panic("fork : sys_env_set_pgfault_upcall for self failed\n");

        if(envid>0) {//parent
                //if(sys_env_set_pgfault_upcall(0, pgfault)) {
                //      panic("fork : sys_env_set_pgfault_upcall for self failed\n");
                //}
                ///*
                //cprintf("---%d---\n",(UXSTACKTOP-PGSIZE)/PGSIZE);
                pde_t pde;
                pte_t pte;
                for (i = UTEXT/PTSIZE; i < (UXSTACKTOP)/PTSIZE; i++) {
                        pde = ((pte_t *)vpd)[i];
                        if(pde&PTE_P) {
                                for(j=0; (i*1024+j)<(UXSTACKTOP-PGSIZE)/PGSIZE&&j<1024; j++) {
                                        pte = ((pte_t *)vpt)[i*1024+j];
                                        if( (pte&PTE_P) && (pte&PTE_U)){
                                                //cprintf("%d\t",i*1024+j);
                                                duppage(envid, i*1024+j);
                                        }
                                }
                        }
                }//*/

                //copy stack and xstack
                /*
                if (sys_page_alloc(envid, (void *)(USTACKTOP-PGSIZE), PTE_P|PTE_U|PTE_W) < 0)
                        panic("sys_page_alloc:");
                if (sys_page_map(envid, (void *)(USTACKTOP-PGSIZE), 0, UTEMP, PTE_P|PTE_U|PTE_W) < 0)
                        panic("sys_page_map:");
                        memmove(UTEMP, (void *)(USTACKTOP-PGSIZE), PGSIZE);*/

                if (sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_P|PTE_U|PTE_W) < 0)
                        panic("sys_page_alloc:");
                extern void _pgfault_upcall(void);
                if(sys_env_set_pgfault_upcall(envid, _pgfault_upcall))
                        panic("fork : sys_env_set_pgfault_upcall for child failed\n");

                // Start the child environment running
                if (sys_env_set_status(envid, ENV_RUNNABLE) < 0)
                        panic("fork : sys_env_set_status failed\n");

                return envid;
        }else if(envid<0){
                panic("fork : sys_exofork failed\n");
        }

        //child
        env = &envs[ENVX(sys_getenvid())];
        //cprintf("child env_id is %08x\n",env->env_id);
        return 0;
	//panic("fork not implemented");
        //*/
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
