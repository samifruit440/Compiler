    .text
    .globl _start
_start:
    movl $0x100000, %esi  # Heap pointer
    movl $12, %eax
    movl %eax, -4(%esp)
    movl $20, %eax
    movl %eax, -8(%esp)
    movl -4(%esp), %eax
    addl -8(%esp), %eax
    movl %eax, %ebx     # return value
    movl $1, %eax      # exit syscall
    int $0x80
