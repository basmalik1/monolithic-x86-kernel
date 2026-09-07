/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"

#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
  // master
  outb(ICW1, MASTER_8259_PORT);
  outb(ICW2_MASTER, MASTER_8259_DPORT);
  outb(ICW3_MASTER, MASTER_8259_DPORT);
  outb(ICW4, MASTER_8259_DPORT);
  // slave
  outb(ICW1, SLAVE_8259_PORT);
  outb(ICW2_SLAVE, SLAVE_8259_DPORT);
  outb(ICW3_SLAVE, SLAVE_8259_DPORT);
  outb(ICW4, SLAVE_8259_DPORT);
  // mask interrupts
  master_mask = 0xFB;  // 11111011 (IRQ 2 is connected to the slave PIC, so it must be enabled)
  slave_mask = 0xFF;   // (all interrupts disabled)

  outb(master_mask, MASTER_8259_DPORT);
  outb(slave_mask, SLAVE_8259_DPORT);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
  if (irq_num < 8) {                       // master
    master_mask &= ~(1 << irq_num);        // set the bit to 0
    outb(master_mask, MASTER_8259_DPORT);  // write to the master port
  } else if (irq_num < 16) {               // slave
    slave_mask &= ~(1 << (irq_num - 8));   // set the bit to 0 (sub 8 for slave ports)
    outb(slave_mask, SLAVE_8259_DPORT);    // write to the slave port
  } else {
    return;  // invalid irq_num
  }
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
  if (irq_num < 8) {                       // master
    master_mask |= (1 << irq_num);         // set the bit to 1
    outb(master_mask, MASTER_8259_DPORT);  // write to the master port
  } else if (irq_num < 16) {               // slave
    slave_mask |= (1 << (irq_num - 8));    // set the bit to 0 (sub 8 for slave ports)
    outb(slave_mask, SLAVE_8259_DPORT);    // write to the slave port
  } else {
    return;  // invalid irq_num
  }
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
  if (irq_num < 8) {                                  // master
    outb(EOI | irq_num, MASTER_8259_PORT);            // write to the master port
  } else if (irq_num < 16) {                          // slave
    outb(EOI | (irq_num - 8), SLAVE_8259_PORT);       // write to the slave port (sub 8 for slave ports)
    outb(EOI | SLAVE_CASCADE_IRQ, MASTER_8259_PORT);  // tell master that slave is done
  } else {
    return;  // invalid irq_num
  }
}
