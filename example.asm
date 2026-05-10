global _start

section .data
msg db "Hello, World!", 10
len equ 14

section .text

 _start:
    mov rax, 1        ; syscall: write
    mov rdi, 1        ; fd = stdout
    mov rsi, msg       
    mov rdx, len      ; length
    syscall

   ; mul rax, 20

    mov rax, 60       ; syscall: exit
    mov rdi, 0
    syscall
