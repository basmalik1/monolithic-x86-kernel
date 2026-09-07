#define EXCEPTION_HANDLER(exception) \
  void exception##_handler();

// Table 5-1. Protected-Mode Exceptions and Interrupts
EXCEPTION_HANDLER(divide_error);
// Intel reserved (1)
EXCEPTION_HANDLER(nmi);
EXCEPTION_HANDLER(breakpoint);
EXCEPTION_HANDLER(overflow);
EXCEPTION_HANDLER(bound_range_exceeded);
EXCEPTION_HANDLER(invalid_opcode);
EXCEPTION_HANDLER(device_not_available);
EXCEPTION_HANDLER(double_fault);
EXCEPTION_HANDLER(coprocessor_segment_overrun);
EXCEPTION_HANDLER(invalid_tss);
EXCEPTION_HANDLER(segment_not_present);
EXCEPTION_HANDLER(stack_segment_fault);
EXCEPTION_HANDLER(general_protection);
EXCEPTION_HANDLER(page_fault);
// Intel reserved (15)
EXCEPTION_HANDLER(FPU_error);
EXCEPTION_HANDLER(alignment_check);
EXCEPTION_HANDLER(machine_check);
EXCEPTION_HANDLER(simd_floating_point_exception);
// intel reserved (20-31)
