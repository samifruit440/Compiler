    .text
    .globl _start
_start:
    movl $0x100000, %esi  # Heap pointer
    movl $0, %eax
    movl %eax, %ebx     # return value
    movl $1, %eax      # exit syscall
    int $0x80
