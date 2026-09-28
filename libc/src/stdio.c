#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

struct out { char *buf; size_t cap; size_t pos; int fd; };
static void emit(struct out *o,char c){ if(o->buf){if(o->cap&&o->pos+1u<o->cap)o->buf[o->pos]=c;} else if(o->fd>=0){(void)write(o->fd,&c,1u);} ++o->pos; }
static void text(struct out*o,const char*s){if(!s)s="(null)";while(*s)emit(o,*s++);}
static void number(struct out*o,uint64_t v,unsigned base,int upper){char b[32];size_t n=0;const char*d=upper?"0123456789ABCDEF":"0123456789abcdef";if(v==0){emit(o,'0');return;}while(v&&n<sizeof(b)){b[n++]=d[v%base];v/=base;}while(n)emit(o,b[--n]);}
static int format(struct out*o,const char*f,va_list ap){while(*f){if(*f!='%'){emit(o,*f++);continue;}++f;if(*f=='%'){emit(o,'%');++f;continue;}int ll=0;if(*f=='l'){++f;ll=1;if(*f=='l'){++f;ll=2;}}switch(*f){case 'c':emit(o,(char)va_arg(ap,int));break;case 's':text(o,va_arg(ap,const char*));break;case 'd':case 'i':{int64_t v=ll==2?va_arg(ap,long long):(ll==1?va_arg(ap,long):va_arg(ap,int));if(v<0){emit(o,'-');number(o,(uint64_t)(-(v+1))+1u,10,0);}else number(o,(uint64_t)v,10,0);break;}case 'u':{uint64_t v=ll==2?va_arg(ap,unsigned long long):(ll==1?va_arg(ap,unsigned long):va_arg(ap,unsigned int));number(o,v,10,0);break;}case 'x':case 'X':{int up=*f=='X';uint64_t v=ll==2?va_arg(ap,unsigned long long):(ll==1?va_arg(ap,unsigned long):va_arg(ap,unsigned int));number(o,v,16,up);break;}case 'p':text(o,"0x");number(o,(uintptr_t)va_arg(ap,void*),16,0);break;default:emit(o,'%');emit(o,*f);break;}if(*f)++f;}if(o->buf&&o->cap){size_t i=o->pos<o->cap?o->pos:o->cap-1u;o->buf[i]='\0';}return (int)o->pos;}
int putchar(int c){char ch=(char)c;return write(1,&ch,1)==1?(unsigned char)ch:-1;}
int puts(const char*s){long a=write(1,s,strlen(s)),b=write(1,"\n",1);return a<0||b<0?-1:(int)(a+b);}
int vsnprintf(char*b,size_t c,const char*f,va_list a){struct out o={b,c,0u,-1};va_list cp;va_copy(cp,a);int r=format(&o,f,cp);va_end(cp);return r;}
int snprintf(char*b,size_t c,const char*f,...){va_list a;va_start(a,f);int r=vsnprintf(b,c,f,a);va_end(a);return r;}
int printf(const char*f,...){struct out o={0,0u,0u,1};va_list a;va_start(a,f);int r=format(&o,f,a);va_end(a);return r;}
