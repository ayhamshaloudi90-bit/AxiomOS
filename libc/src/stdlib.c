#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <axiom/syscalls.h>

#define ALIGN16(x) (((x)+15u)&~(size_t)15u)
#define HEAP_CHUNK_MIN 4096u
#define BLOCK_MAGIC 0xA1190C19u

struct block { size_t size; struct block *next; uint32_t magic; uint32_t free; uint64_t reserved; };
_Static_assert((sizeof(struct block) % 16u) == 0u, "malloc block header alignment");
static struct block *head;

static struct block *grow_heap(size_t need)
{
    size_t total=ALIGN16(need+sizeof(struct block));
    size_t chunk=(total+4095u)&~(size_t)4095u;
    struct block *b;
    if(chunk<HEAP_CHUNK_MIN) chunk=HEAP_CHUNK_MIN;
    b=(struct block*)axiom_mmap(chunk);
    if(!b) return 0;
    b->size=chunk-sizeof(*b); b->next=0; b->magic=BLOCK_MAGIC; b->free=1u; b->reserved=0u;
    if(!head) head=b; else { struct block *p=head; while(p->next)p=p->next; p->next=b; }
    return b;
}

static void split_block(struct block *b,size_t n)
{
    size_t wanted=ALIGN16(n);
    if(b->size>=wanted+sizeof(struct block)+16u){
        struct block *tail=(struct block*)((uint8_t*)(b+1)+wanted);
        tail->size=b->size-wanted-sizeof(*tail); tail->next=b->next; tail->magic=BLOCK_MAGIC; tail->free=1u; tail->reserved=0u;
        b->size=wanted; b->next=tail;
    }
}

static void coalesce(void)
{
    struct block *b=head;
    while(b&&b->next){
        uint8_t *end=(uint8_t*)(b+1)+b->size;
        if(b->free&&b->next->free&&end==(uint8_t*)b->next){
            b->size+=sizeof(struct block)+b->next->size; b->next=b->next->next;
        } else b=b->next;
    }
}

void *malloc(size_t size)
{
    struct block *b; if(!size) return 0; size=ALIGN16(size);
    for(;;){ for(b=head;b;b=b->next) if(b->magic==BLOCK_MAGIC&&b->free&&b->size>=size){split_block(b,size);b->free=0u;return b+1;} if(!grow_heap(size)) return 0; }
}
void free(void *ptr){struct block*b;if(!ptr)return;b=((struct block*)ptr)-1;if(b->magic!=BLOCK_MAGIC||b->free)return;b->free=1u;coalesce();}
void *calloc(size_t count,size_t size){size_t total;void*p;if(size&&count>SIZE_MAX/size)return 0;total=count*size;p=malloc(total);if(p)memset(p,0,total);return p;}
void *realloc(void *ptr,size_t size){struct block*b;void*n;if(!ptr)return malloc(size);if(!size){free(ptr);return 0;}b=((struct block*)ptr)-1;if(b->magic!=BLOCK_MAGIC)return 0;if(b->size>=size){split_block(b,size);return ptr;}n=malloc(size);if(!n)return 0;memcpy(n,ptr,b->size);free(ptr);return n;}

long strtol(const char *s,char **end,int base)
{
    long sign=1,value=0; const char *p=s; int d;
    while(isspace((unsigned char)*p))++p;
    if(*p=='-'||*p=='+'){if(*p=='-')sign=-1;++p;}
    if(base==0){ if(p[0]=='0'&&(p[1]=='x'||p[1]=='X')){base=16;p+=2;} else if(*p=='0'){base=8;++p;} else base=10; }
    else if(base==16&&p[0]=='0'&&(p[1]=='x'||p[1]=='X'))p+=2;
    if(base<2||base>36){if(end)*end=(char*)s;return 0;}
    while(*p){ if(*p>='0'&&*p<='9')d=*p-'0'; else if(*p>='a'&&*p<='z')d=*p-'a'+10; else if(*p>='A'&&*p<='Z')d=*p-'A'+10; else break; if(d>=base)break; value=value*base+d;++p; }
    if(end)*end=(char*)p; return value*sign;
}
int atoi(const char *s){return (int)strtol(s,0,10);}
