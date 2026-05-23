.global _start

.extern main

.section .text

_start:
    call main

    mov %eax, %edi
    mov $60, %eax
    syscall
