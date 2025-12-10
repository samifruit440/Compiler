    .text
    .globl _start
_start:
    movl $0x100000, %esi  # Heap pointer
    movl $63, %eax
    cmpl $0x1f, %eax
    je .L0
    movl $40, %eax
    jmp .L1
.L0:
    movl $20, %eax
.L1:
    movl %eax, %ebx     # return value
    movl $1, %eax      # exit syscall
    int $0x80
