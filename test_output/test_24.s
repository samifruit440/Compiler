    .text
    .globl _start
_start:
    movl $0x100000, %esi  # Heap pointer
    movl $28, %eax
    movl %eax, -4(%esp)
    movl $8, %eax
    movl %eax, -8(%esp)
    movl -4(%esp), %eax
    subl -8(%esp), %eax
    movl %eax, %ebx     # return value
    movl $1, %eax      # exit syscall
    int $0x80
