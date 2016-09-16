#pragma once

// Suppress warnings about overriding return value of kasm87 function
#pragma warning(disable:4055)
#pragma warning(disable:4152)

typedef double (__cdecl*EVALFUNC)(double, ...);
typedef double (__cdecl*EVALFUNCP)(void*, ...);

void* _cdecl kasm87(char* codestring);
long _cdecl kasm87_findfirstfuncparen(char* codestring);
void _cdecl kasm87free(void* func);
void _cdecl kasm87freeall();
void _cdecl kasm87_showdebug(long showflags, char* debuf, long debuflng);
typedef struct { char* nam; void* ptr; } evalextyp;
void _cdecl kasm87addext(evalextyp* daeet, long n);
void kasm87jumpback(void* func, long mode);

extern char kasm87err[256];
extern long kasm87leng, kasm87err0, kasm87err1;
extern long kasm87optimize; //0=disable optimizations(faster compile), 1=optimize (default)

