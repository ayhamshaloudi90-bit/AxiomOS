#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <axiom/syscalls.h>
#include <axiom/abi/syscall.h>
#include "syscall_internal.h"

long write(int fd,const void *b,size_t n){return __axiom_syscall3(AXIOM_SYS_WRITE,fd,(long)(uintptr_t)b,(long)n);}
long read(int fd,void *b,size_t n){return __axiom_syscall3(AXIOM_SYS_READ,fd,(long)(uintptr_t)b,(long)n);}
_Noreturn void _exit(int s){(void)__axiom_syscall1(AXIOM_SYS_EXIT,s);for(;;){__asm__ volatile("ud2");}}
long sleep(uint64_t ms){return __axiom_syscall1(AXIOM_SYS_SLEEP,(long)ms);}
long getpid(void){return __axiom_syscall0(AXIOM_SYS_GETPID);}
long getppid(void){return __axiom_syscall0(AXIOM_SYS_GETPPID);}
long getuid(void){return __axiom_syscall0(AXIOM_SYS_GETUID);}
long getgid(void){return __axiom_syscall0(AXIOM_SYS_GETGID);}
long yield(void){return __axiom_syscall0(AXIOM_SYS_YIELD);}
long fork(void){return __axiom_syscall0(AXIOM_SYS_FORK);}
long exec(const char *p){return __axiom_syscall1(AXIOM_SYS_EXEC,(long)(uintptr_t)p);}
int open(const char *p,int f){return (int)__axiom_syscall2(AXIOM_SYS_OPEN,(long)(uintptr_t)p,f);}
long close(int fd){return __axiom_syscall1(AXIOM_SYS_CLOSE,fd);}
long lseek(int fd,long o,int w){return __axiom_syscall3(AXIOM_SYS_LSEEK,fd,o,w);}
int stat(const char *p,struct axiom_stat *s){return (int)__axiom_syscall2(AXIOM_SYS_STAT,(long)(uintptr_t)p,(long)(uintptr_t)s);}
long waitpid(long pid,long *status){return __axiom_syscall2(AXIOM_SYS_WAITPID,pid,(long)(uintptr_t)status);}
long axiom_readdir(const char*p,unsigned long i,struct axiom_dirent*e){return __axiom_syscall3(AXIOM_SYS_READDIR,(long)(uintptr_t)p,(long)i,(long)(uintptr_t)e);}
long axiom_mkdir(const char*p){return __axiom_syscall1(AXIOM_SYS_MKDIR,(long)(uintptr_t)p);}
long axiom_spawn(const char*p){return __axiom_syscall1(AXIOM_SYS_SPAWN,(long)(uintptr_t)p);}
long axiom_clear(void){return __axiom_syscall0(AXIOM_SYS_CLEAR);}
long axiom_kbdstats(struct axiom_keyboard_stats*s){return __axiom_syscall1(AXIOM_SYS_KBDSTATS,(long)(uintptr_t)s);}
long axiom_procinfo(unsigned long i,struct axiom_process_info*p){return __axiom_syscall2(AXIOM_SYS_PROCINFO,(long)i,(long)(uintptr_t)p);}
long axiom_kill(long p,unsigned long s){return __axiom_syscall2(AXIOM_SYS_KILL,p,(long)s);}
long axiom_netinfo(struct axiom_net_info*i){return __axiom_syscall1(AXIOM_SYS_NETINFO,(long)(uintptr_t)i);}
long axiom_ping(const char*t,struct axiom_ping_result*r){return __axiom_syscall2(AXIOM_SYS_PING,(long)(uintptr_t)t,(long)(uintptr_t)r);}
long axiom_dns(const char*h,uint32_t*a){return __axiom_syscall2(AXIOM_SYS_DNS,(long)(uintptr_t)h,(long)(uintptr_t)a);}
long axiom_httpget(const char*h,const char*p,void*b,size_t c,struct axiom_http_result*r){return __axiom_syscall5(AXIOM_SYS_HTTPGET,(long)(uintptr_t)h,(long)(uintptr_t)p,(long)(uintptr_t)b,(long)c,(long)(uintptr_t)r);}
long axiom_httpserve(uint16_t p,const void*b,size_t n,struct axiom_http_server_result*r){return __axiom_syscall5(AXIOM_SYS_HTTPSERVE,(long)p,(long)(uintptr_t)b,(long)n,(long)(uintptr_t)r,0);}
void *axiom_mmap(size_t length){long r=__axiom_syscall1(AXIOM_SYS_MMAP,(long)length);return r<0?0:(void*)(uintptr_t)r;}

long axiom_getpriority(void){return __axiom_syscall0(AXIOM_SYS_GETPRIORITY);}
long axiom_setpriority(unsigned long p){return __axiom_syscall1(AXIOM_SYS_SETPRIORITY,(long)p);}
long axiom_getaffinity(void){return __axiom_syscall0(AXIOM_SYS_GETAFFINITY);}
long axiom_setaffinity(uint64_t m){return __axiom_syscall1(AXIOM_SYS_SETAFFINITY,(long)m);}
long axiom_schedstats(struct axiom_scheduler_stats*s){return __axiom_syscall1(AXIOM_SYS_SCHEDSTATS,(long)(uintptr_t)s);}

long axiom_pipe_create(void){return __axiom_syscall0(AXIOM_SYS_PIPE_CREATE);}
long axiom_pipe_write(long p,const void*b,size_t n){return __axiom_syscall3(AXIOM_SYS_PIPE_WRITE,p,(long)(uintptr_t)b,(long)n);}
long axiom_pipe_read(long p,void*b,size_t n){return __axiom_syscall3(AXIOM_SYS_PIPE_READ,p,(long)(uintptr_t)b,(long)n);}
long axiom_pipe_close(long p){return __axiom_syscall1(AXIOM_SYS_PIPE_CLOSE,p);}
void *axiom_shm_attach(uint64_t k){long r=__axiom_syscall1(AXIOM_SYS_SHM_ATTACH,(long)k);return r<0?0:(void*)(uintptr_t)r;}
long axiom_shm_detach(void*a){return __axiom_syscall1(AXIOM_SYS_SHM_DETACH,(long)(uintptr_t)a);}
long axiom_msgq_open(uint64_t k){return __axiom_syscall1(AXIOM_SYS_MSGQ_OPEN,(long)k);}
long axiom_msgq_send(long q,const void*m,size_t n){return __axiom_syscall3(AXIOM_SYS_MSGQ_SEND,q,(long)(uintptr_t)m,(long)n);}
long axiom_msgq_receive(long q,void*b,size_t n){return __axiom_syscall3(AXIOM_SYS_MSGQ_RECV,q,(long)(uintptr_t)b,(long)n);}
long axiom_msgq_close(long q){return __axiom_syscall1(AXIOM_SYS_MSGQ_CLOSE,q);}
long axiom_ipcstats(struct axiom_ipc_stats*s){return __axiom_syscall1(AXIOM_SYS_IPCSTATS,(long)(uintptr_t)s);}

long axiom_mousestate(struct axiom_mouse_state *state){return __axiom_syscall1(AXIOM_SYS_MOUSESTATE,(long)(uintptr_t)state);}
