    .text
    .globl _start
_start:
    movl $16655, %eax
    movl %eax, %ebx     # return value
    movl $1, %eax      # exit syscall
    int $0x80
