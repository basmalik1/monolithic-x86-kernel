#define SYSCALL_WRAPPER(syscall)                 \
  .globl syscall ## _handler_S;                  \
  syscall ## _handler_S:;                        \
  pushl %edx;                                    \
  pushl %ecx;                                    \
  pushl %ebx;                                    \
  call syscall ## _handler;                      \
  addl $12, %esp;                                \
  ret


SYSCALL_WRAPPER(halt);
SYSCALL_WRAPPER(execute);
SYSCALL_WRAPPER(read);
SYSCALL_WRAPPER(write);
SYSCALL_WRAPPER(open);
SYSCALL_WRAPPER(close);
SYSCALL_WRAPPER(getargs);
SYSCALL_WRAPPER(vidmap);
SYSCALL_WRAPPER(set_handler);
SYSCALL_WRAPPER(sigreturn);

# Setup the jump table
mp3_syscall_table:
.long halt_handler_S, execute_handler_S, read_handler_S, write_handler_S, open_handler_S, close_handler_S, getargs_handler_S, vidmap_handler_S, set_handler_handler_S, sigreturn_handler_S





.globl handle_syscall
handle_syscall:
    # Validate the input
    # Jump if less than 1
    cmp $1, %eax
    jb BAD_SYSCALL
    # Jump if greater than 10
    cmp $10, %eax
    ja BAD_SYSCALL
    dec %eax
    jmp *mp3_syscall_table(,%eax,4)

    BAD_SYSCALL:
    movl $-1, %eax
    ret

