#ifndef JOS_DEV_E100_H
#define JOS_DEV_E100_H 1

#define PCI_E100_VID 0x8086
#define PCI_E100_DID 0x1209

#define PCI_E100_PORT 0x08
#define E100_PORT_SOFT_RESET 0x00

#define E100_IRQ 11

struct cb {
        volatile uint16_t status;
        uint16_t cmd;
        uint32_t link;
};

/************************************************************
 *    +--------------+--------------+
 *    |    CONTROL   |    STATUS    |
 *    +--------------+--------------+
 *    |            LINK             |
 *    +--------------+--------------+
 *    |       TBD ARRAY ADDR        |
 *    +--------------+--------------+
 *    |TBD COUNT|THRS|TCB BYTE COUNT|
 *    +--------------+--------------+
 *    |            DATA             |
 *    +--------------+--------------+
 ************************************************************/

struct tcb {
        struct cb header;
        uint32_t tbd_array_addr;
        char tbd_count;
        char thrs;
        uint16_t tbd_byte_count;
        char data[1518];
};

//DMA ring
struct tcb dma_ring[7];

int e100_init(struct pci_func *pcif);

#endif	// !JOS_DEV_E100_H
