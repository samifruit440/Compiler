    .text
    .globl _start
_start:
    movl $0x100000, %esi  # Heap pointer
    movl $63, %eax
    cmpl $0x1f, %eax
    je .L0
    movl $20, %eax
    jmp .L1
.L0:
    movl $40, %eax
.L1:
    movl %eax, -4(%esp)
    movl $80, %eax
    movl %eax, -8(%esp)
    movl %esp, %eax
    addl $-8, %eax
    orl $1, %eax
    subl $1, %eax
    movl 4(%eax), %eax
    movl %eax, %ebx     # return value
    movl $1, %eax      # exit syscall
    int $0x80
