#include "intrinsics.h"

intrinsic_info intrinsics [] = {
    {
        .symbol = "print_i64_impl",
        .source = 
            "_print_i64:\n"
                "push rbp"
                "mov rbp, rsp"
                "; == Save the length count addr\n"
                "push rax\n"
                "mov r8, 0\n"
                "mov [rsp], r8\n"
                "mov r8, rsp\n"
                "; ==\n"
                "mov rax, rdi\n"
                "mov rbx, 0xA\n"

                "; == Add a line feed to the end\n"
                "sub rsp, 0x1\n"
                "mov rdx, 0xA\n"
                "mov [rsp], dl\n"
                "mov rdx, 0\n"
                "; ==\n"

                ".print_i64_loop:\n"
                    "div rbx\n"
                    "; == increment the string length\n"
                    "mov r11, [r8]\n"
                    "inc r11\n"
                    "mov [r8], r11\n"
                    "; ==\n"
                    "; == push the char equiv. of the remainder\n"
                    "add rdx, 0x30\n"
                    "sub rsp, 0x1\n"
                    "mov [rsp], dl\n"
                    "mov rdx, 0\n"
                    "; ==\n"
                    "cmp rax, 0x0\n"
                    "jz .print_i64_return\n"
                    "jmp .print_i64_loop\n"

                ".print_i64_return:\n"
                    "; == print the string\n"
                    "mov rdi, 1\n"
                    "mov rsi, rsp\n"
                    "mov rdx, [r8]\n"
                    "inc rdx ; increment the length to include the line feed\n"
                    "mov rax, 1\n"
                    "syscall\n"
                    "; ==\n"

                    "; == restore the stack\n"
                    "mov rsp, r8\n"
                    "pop rax\n"
                    "pop rbp"
                    "ret\n"
                    "; ==\n",
            .impl = 1
    },
    {
        .symbol = "print_i64",
        .source = 
            "push rax\n"
            "push rdi\n"
            "call _print_i64\n"
            "pop rdi\n"
            "pop rax\n",
        .impl = 0
    }
};
