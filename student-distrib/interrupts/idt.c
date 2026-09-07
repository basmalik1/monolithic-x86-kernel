
#include "idt.h"

#include "../lib.h"
#include "../x86_desc.h"
#include "exceptions.h"
#include "idt_wrappers.h"

#define SETUP_EXCEPTION(exception, index) \
  SET_IDT_INTR_ENTRY(idt[index], exception, KERNEL_CS, 0);

/* initialize_idt
 *   DESCRIPTION: Initializes the IDT with the appropriate entries
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Initializes the IDT
 *               : Loads the IDT
 */
void initialize_idt() {
  SETUP_EXCEPTION(divide_error, 0);
  SETUP_EXCEPTION(nmi, 2);
  SETUP_EXCEPTION(breakpoint, 3);
  SETUP_EXCEPTION(overflow, 4);
  SETUP_EXCEPTION(bound_range_exceeded, 5);
  SETUP_EXCEPTION(invalid_opcode, 6);
  SETUP_EXCEPTION(device_not_available, 7);
  SETUP_EXCEPTION(double_fault, 8);
  SETUP_EXCEPTION(coprocessor_segment_overrun, 9);
  SETUP_EXCEPTION(invalid_tss, 10);
  SETUP_EXCEPTION(segment_not_present, 11);
  SETUP_EXCEPTION(stack_segment_fault, 12);
  SETUP_EXCEPTION(general_protection, 13);
  SETUP_EXCEPTION(page_fault, 14);
  SETUP_EXCEPTION(FPU_error, 16);
  SETUP_EXCEPTION(alignment_check, 17);
  SETUP_EXCEPTION(machine_check, 18);
  SETUP_EXCEPTION(simd_floating_point_exception, 19);
  // syscall handler will be used for syscalls
  SET_IDT_TRAP_ENTRY(idt[0x80], syscall_wrapper, KERNEL_CS, 3);
  SET_IDT_TRAP_ENTRY(idt[PIC_OFFSET + RTC_IRQ], rtc_wrapper, KERNEL_CS, 0);

  lidt(idt_desc_ptr);  // ldt_desc_ptr is defined in x86_desc.h, provided for us
  
  /* Keyboard Interrupt*/
  SET_IDT_INTR_ENTRY(idt[0x21], keyboard_wrapper, KERNEL_CS, 0);

}
