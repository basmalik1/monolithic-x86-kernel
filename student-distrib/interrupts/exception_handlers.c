#include "../lib.h"

#define EXCEPTION_HANDLER(exception)  \
  void exception##_handler() {        \
    cli();                            \
    printf(#exception " occurred\n"); \
    while (1);                        \
    sti();                            \
  }

// Table 5-1. Protected-Mode Exceptions and Interrupts
EXCEPTION_HANDLER(divide_error);
// Intel reserved (1)
EXCEPTION_HANDLER(nmi);
EXCEPTION_HANDLER(breakpoint);
EXCEPTION_HANDLER(overflow);
EXCEPTION_HANDLER(bound_range_exceeded);
// EXCEPTION_HANDLER(invalid_opcode);
EXCEPTION_HANDLER(device_not_available);
EXCEPTION_HANDLER(double_fault);
EXCEPTION_HANDLER(coprocessor_segment_overrun);
EXCEPTION_HANDLER(invalid_tss);
EXCEPTION_HANDLER(segment_not_present);
EXCEPTION_HANDLER(stack_segment_fault);
EXCEPTION_HANDLER(general_protection);
// EXCEPTION_HANDLER(page_fault);
// Intel reserved (15)
EXCEPTION_HANDLER(FPU_error);
EXCEPTION_HANDLER(alignment_check);
EXCEPTION_HANDLER(machine_check);
EXCEPTION_HANDLER(simd_floating_point_exception);
// intel reserved (20-31)


/* page_fault_handler
 *   DESCRIPTION: handles page fault exception and prints out the error message and the address that caused the fault
 *   INPUTS: addr : the address that caused the page fault
 *           ds, edi, esi, ebp, esp, ebx, edx, ecx, eax :the current state of the registers
 *           error_code: the error code of the page fault
 *           eip, cs, eflags, useresp, ss: the current state of the processor
 *   OUTPUTS: prints an error message to the console
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
void page_fault_handler(
    uint32_t addr, uint16_t ds,
    uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
    uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax,
    uint32_t error_code,
    uint32_t eip, uint16_t cs, uint32_t eflags, uint32_t useresp, uint16_t ss) {
  printf("Page fault (Code: 0x%x) occurred trying to access: 0x%x from 0x%x\n", error_code, addr, eip);
  while (1);
}
/* invalid_opcode_handler
 *   DESCRIPTION: handels invalid opcode exception and prints out the error message
 *   INPUTS: addr : the address that caused the page fault
 *           ds, edi, esi, ebp, esp, ebx, edx, ecx, eax :the current state of the registers
 *           error_code: the error code of the page fault
 *           eip, cs, eflags, useresp, ss: the current state of the processor
 *   OUTPUTS: prints an error message to the console
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
void invalid_opcode_handler(
    uint16_t ds,
    uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
    uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax,
    uint32_t error_code,
    uint32_t eip, uint16_t cs, uint32_t eflags, uint32_t useresp, uint16_t ss) {
  printf("Invalid opcode occurred\n", ds);
  while (1);
}
