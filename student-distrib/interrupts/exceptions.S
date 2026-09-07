#define KERNEL_DS 0x0018

// https://wiki.osdev.org/Interrupt_Service_Routines#Two-Stage_Assembly_Wrapping
#define EXCEPTION_WRAPPER(exception) \
  .globl exception;                  \
  exception:;                        \
  pushl $0;                          \
  pushal;                            \
  pushl %ds;                         \
  movw $KERNEL_DS, %si;              \
  movw %si, %ds;                     \
  call exception ## _handler;        \
  popl %ds;                          \
  popal;                             \
  addl $4, %esp;                     \
  iret

// need to pop the error code off the stack after use
#define ERR_EXCEPTION_WRAPPER(exception) \
  .globl exception;                      \
  exception:;                            \
  pushal;                                \
  pushl %ds;                             \
  movw $KERNEL_DS, %si;                  \
  movw %si, %ds;                         \
  call exception ## _handler;            \
  popl %ds;                              \
  popal;                                 \
  addl $4, %esp;                         \
  iret


// Table 5-1. Protected-Mode Exceptions and Interrupts
EXCEPTION_WRAPPER(divide_error);
// Intel reserved (1)
EXCEPTION_WRAPPER(nmi);
EXCEPTION_WRAPPER(breakpoint);
EXCEPTION_WRAPPER(overflow);
EXCEPTION_WRAPPER(bound_range_exceeded);
EXCEPTION_WRAPPER(invalid_opcode);
EXCEPTION_WRAPPER(device_not_available);
ERR_EXCEPTION_WRAPPER(double_fault);
EXCEPTION_WRAPPER(coprocessor_segment_overrun);
ERR_EXCEPTION_WRAPPER(invalid_tss);
ERR_EXCEPTION_WRAPPER(segment_not_present);
ERR_EXCEPTION_WRAPPER(stack_segment_fault);
ERR_EXCEPTION_WRAPPER(general_protection);

.globl page_fault
page_fault:
  pushal
  pushl %ds
  movw $KERNEL_DS, %si
  movw %si, %ds
  movl %cr2, %eax
  pushl %eax
  call page_fault_handler
  addl $4, %esp
  popl %ds
  popal
  addl $4, %esp
  iret

// Intel reserved (15)
EXCEPTION_WRAPPER(FPU_error);
ERR_EXCEPTION_WRAPPER(alignment_check);
EXCEPTION_WRAPPER(machine_check);
EXCEPTION_WRAPPER(simd_floating_point_exception);
// intel reserved (20-31)
