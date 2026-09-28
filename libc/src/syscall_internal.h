#ifndef AXIOM_LIBC_SYSCALL_INTERNAL_H
#define AXIOM_LIBC_SYSCALL_INTERNAL_H
long __axiom_syscall0(long n);
long __axiom_syscall1(long n, long a1);
long __axiom_syscall2(long n, long a1, long a2);
long __axiom_syscall3(long n, long a1, long a2, long a3);
long __axiom_syscall5(long n, long a1, long a2, long a3, long a4, long a5);
#endif
