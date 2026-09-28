#include <string.h>
#include <stdint.h>

size_t strlen(const char *s) { size_t n = 0; if (!s) return 0; while (s[n]) ++n; return n; }
int strcmp(const char *a, const char *b) { while (*a && *a == *b) { ++a; ++b; } return (unsigned char)*a - (unsigned char)*b; }
int strncmp(const char *a, const char *b, size_t n) { size_t i; for (i=0;i<n;++i) { unsigned char ac=(unsigned char)a[i], bc=(unsigned char)b[i]; if (ac!=bc) return ac-bc; if (!ac) return 0; } return 0; }
char *strcpy(char *d, const char *s) { char *r=d; while ((*d++=*s++)!='\0') {} return r; }
char *strncpy(char *d, const char *s, size_t n) { size_t i=0; for (;i<n && s[i];++i) d[i]=s[i]; for (;i<n;++i) d[i]='\0'; return d; }
void *memcpy(void *d,const void *s,size_t n){ unsigned char *o=d; const unsigned char *i=s; size_t k; for(k=0;k<n;++k)o[k]=i[k]; return d; }
void *memmove(void *d,const void *s,size_t n){ unsigned char *o=d; const unsigned char *i=s; size_t k; if(o<i){for(k=0;k<n;++k)o[k]=i[k];}else if(o>i){for(k=n;k>0;--k)o[k-1]=i[k-1];} return d; }
void *memset(void *d,int v,size_t n){ unsigned char *o=d; size_t k; for(k=0;k<n;++k)o[k]=(unsigned char)v; return d; }
int memcmp(const void *a,const void *b,size_t n){ const unsigned char *x=a,*y=b; size_t k; for(k=0;k<n;++k) if(x[k]!=y[k]) return x[k]-y[k]; return 0; }
char *strchr(const char *s,int c){ char ch=(char)c; while(*s){ if(*s==ch) return (char*)s; ++s;} return ch=='\0'?(char*)s:0; }
