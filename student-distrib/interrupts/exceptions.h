#include "exception_handlers.h"

#define EXCEPTION_WRAPPER(exception) void exception();

// Table 5-1. Protected-Mode Exceptions and Interrupts
EXCEPTION_WRAPPER(divide_error);
// Intel reserved (1)
EXCEPTION_WRAPPER(nmi);
EXCEPTION_WRAPPER(breakpoint);
EXCEPTION_WRAPPER(overflow);
EXCEPTION_WRAPPER(bound_range_exceeded);
EXCEPTION_WRAPPER(invalid_opcode);
EXCEPTION_WRAPPER(device_not_available);
EXCEPTION_WRAPPER(double_fault);
EXCEPTION_WRAPPER(coprocessor_segment_overrun);
EXCEPTION_WRAPPER(invalid_tss);
EXCEPTION_WRAPPER(segment_not_present);
EXCEPTION_WRAPPER(stack_segment_fault);
EXCEPTION_WRAPPER(general_protection);
EXCEPTION_WRAPPER(page_fault);
// Intel reserved (15)
EXCEPTION_WRAPPER(FPU_error);
EXCEPTION_WRAPPER(alignment_check);
EXCEPTION_WRAPPER(machine_check);
EXCEPTION_WRAPPER(simd_floating_point_exception);
// intel reserved (20-31)

