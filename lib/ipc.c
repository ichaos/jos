// User-level IPC library routines

#include <inc/lib.h>
// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'fromenv' is nonnull, then store the IPC sender's envid in *fromenv.
// If 'perm' is nonnull, then store the IPC sender's page permission in *perm
//	(this is nonzero iff a page was successfully transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
//
// Hint:
//   Use 'env' to discover the value and who sent it.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
int32_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	// LAB 4: Your code here.
        //cprintf("in recv\n");
        if(!pg)
                pg = (void *)-1;
        int cj = sys_ipc_recv(pg);
        if(cj<0) {
                if(from_env_store)
                        *from_env_store = 0;
                if(perm_store)
                        *perm_store = 0;
                return cj;
        }else {
                if(from_env_store)
                        *from_env_store = env->env_ipc_from;
                if(perm_store)
                        *perm_store = env->env_ipc_perm;

                return env->env_ipc_value;
        }
	//panic("ipc_recv not implemented");
}

// Send 'val' (and 'pg' with 'perm', assuming 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
//
// Hint:

//   Use sys_yield() to be CPU-friendly.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
void
ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
{
	// LAB 4: Your code here.
        int cj;
        if(!pg)
                pg = (void *)-1;
        while((cj=sys_ipc_try_send(to_env, val, pg, perm)) == -E_IPC_NOT_RECV) {
                sys_yield();
        }
        if(cj<0)
                panic("ipc_send :%e\n",cj);
	//panic("ipc_send not implemented");
}

