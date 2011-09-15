#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	// Implement simple round-robin scheduling.
	// Search through 'envs' for a runnable environment,
	// in circular fashion starting after the previously running env,
	// and switch to the first such environment found.
	// It's OK to choose the previously running env if no other env
	// is runnable.
	// But never choose envs[0], the idle environment,
	// unless NOTHING else is runnable.

    // Lab5 test specific: don't delete the following 4 lines
    // Break into the JOS kernel monitor when only 'fs' and 'idle'
    // are alive in the system.
    // A real, "production" OS of course would NOT do this -
    // it would just endlessly loop waiting for hardware interrupts
    // to cause other environments to become runnable.
    // However, in JOS it is easier for testing and grading
    // if we invoke the kernel monitor after each iteration,
    // because the first invocation of the idle environment
    // usually means everything else has run to completion.
    if(get_allocated_envs_n() == 2) {
        assert(envs[0].env_status == ENV_RUNNABLE);
        breakpoint();
    }

	// LAB 4: Your code here.
        //first find the env_id of previously running env
        //damned! curenv is null now maybe.
        if(curenv == NULL)
                curenv = &envs[0];
        envid_t envid = curenv->env_id;
        int i;
        //cprintf("curenv->env_id is %08x.\n",curenv->env_id);
        for(i=1;i<=NENV;i++){
                //cprintf("i is %d now.\n",i);
                if(ENVX(envid+i)==0)
                        continue;
                if(envs[ENVX(envid+i)].env_status == ENV_RUNNABLE){
                        //cprintf("--ENVX(envid+i) is %08x\n", ENVX(envid+i));
                        env_run(&envs[ENVX(envid+i)]);
                }
        }

	// Run the special idle environment when nothing else is runnable.
 idle:
	if (envs[0].env_status == ENV_RUNNABLE)
		env_run(&envs[0]);
	else {
		cprintf("Destroyed all environments - nothing more to do!\n");
		while (1)
			monitor(NULL);
	}
}
