#define KERNEL_DS 0x0018

.globl syscall_wrapper
syscall_wrapper:
  pushal
  pushw %ds
  movw $KERNEL_DS, %si
  movw %si, %ds
  call handle_syscall
  popw %ds
  movl %eax, 28(%esp)   // replace the return value
  popal
  iret

.globl rtc_wrapper
rtc_wrapper:
  pushal
  pushw %ds
  movw $KERNEL_DS, %si
  movw %si, %ds
  call rtc_handler
  popw %ds
  popal
  iret

.globl keyboard_wrapper
keyboard_wrapper:
  pushal
  pushw %ds
  movw $KERNEL_DS, %si
  movw %si, %ds
  call keyboard_handler
  popw %ds
  popal
  iret
