    .text
    .globl _start
_start:
    movl $0x100000, %esi  # Heap pointer
    movl $40, %eax
    movl %eax, -4(%esp)
    movl $8, %eax
    movl %eax, -8(%esp)
    movl -4(%esp), %eax
    movl -8(%esp), %ecx
    imull %ecx, %eax
    sarl $2, %eax
    movl %eax, %ebx     # return value
    movl $1, %eax      # exit syscall
    int $0x80
