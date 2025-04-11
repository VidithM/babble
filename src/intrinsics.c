#include "babble-lang.h"
#include "intrinsics.h"

const intrinsic_info intrinsics [] = {
    {
        .symbol = "print_i64_impl",
        .source = 
            "_print_i64:\n"
                "; == Save the length count addr\n"
                "push r8\n"
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
                    "pop r8\n"
                    "ret\n"
                    "; ==\n",
            .impl = 1
    },
    {
        .symbol = "print_str_impl",
        .source =
            "_print_str:\n"
                "mov r8, 0\n"
                "mov rcx, rdi\n"
                ".print_str_loop:\n"
                    "mov r11, [rcx]\n"
                    "cmp r11, 0x0\n"
                    "jz .print_str_return\n"
                    "inc r8\n"
                    "jmp .print_str_loop\n"
                ".print_str_return:\n"
                    "mov rdi, 1\n"
                    "mov rsi, rdi\n"
                    "mov rdx, r8\n"
                    "mov rax, 1\n"
                    "syscall\n"
                    "ret\n",
        .impl = 1
    },
    {
        .symbol = "print_i64",
        .source = 
            "push rax\n"
            "push rdi\n"
            "push rcx\n"
            "call _print_i64\n"
            "pop rcx\n"
            "pop rdi\n"
            "pop rax\n",
        .impl = 0
    },
    {
        .symbol = "print_str",
        .source = 
            "push rax\n"
            "push rdi\n"
            "push rcx\n"
            "call _print_str\n"
            "pop rcx\n"
            "pop rdi\n"
            "pop rax\n",
        .impl = 0
    }
};

const size_t N_INTRINSICS = 4;