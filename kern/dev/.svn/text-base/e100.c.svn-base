// LAB 6: Your driver code here
#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <kern/pmap.h>
#include <dev/ide.h>
#include <dev/idereg.h>
#include <kern/env.h>
#include <kern/sched.h>

#include "e100.h"

struct e100_channel {
        uint32_t irq;
        uint32_t io_port;

        //CSR Memory Mapped Base Address Register (BAR 0 at offset 10)
        uint32_t mmbar_addr;
        //CSR I/O Mapped Base Address Register (BAR 1 at offset 14)
        uint32_t iombar_addr;
        //Flash Memory Mapped Base Address Register (BAR 2 at offset 18)
        uint32_t fmmbar_addr;
};

struct e100_channel *e100c = NULL;

// Stupid I/O delay routine necessitated by historical PC design flaws
// Copy from kern/console.c;
// Every inb(0x84) takes about 1.25us;
static void
delay(void)
{
	inb(0x84);
	inb(0x84);
	inb(0x84);
	inb(0x84);
}

int
e100_init(struct pci_func *pcif)
{
        cprintf("e100_init:begin.\n");
        struct Page *pp;
        int r = page_alloc(&pp);
        if (r<0)
                return r;

        e100c = page2kva(pp);
        memset(e100c, 0, sizeof(struct e100_channel));

        pci_func_enable(pcif);

        e100c->irq = pcif->irq_line;
        e100c->iombar_addr = pcif->reg_size[1] ?
                (pcif->reg_size[1]&(~3)) : 0;

        // Enable E100 irq:
        cprintf("	Setup E100 interrupt via 8259A\n");
	irq_setmask_8259A(irq_mask_8259A & ~(1<<e100c->irq));
	cprintf("	unmasked E100 interrupt\n");

        //test reset E100
        outl(e100c->iombar_addr + PCI_E100_PORT, E100_PORT_SOFT_RESET);
        //10us delay loop after reset
        delay();
        delay();
        return 0;
}
