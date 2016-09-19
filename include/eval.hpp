#pragma once

// Suppress warnings about overriding return value of kasm87 function
//#pragma warning(disable:4055)
//#pragma warning(disable:4152)

typedef double (__cdecl*EVALFUNC)(double, ...);
typedef double (__cdecl*EVALFUNCP)(void*, ...);

void* _cdecl kasm87(const char* codestring);
long _cdecl kasm87_findfirstfuncparen(char* codestring);
void _cdecl kasm87free(void* func);
void _cdecl kasm87freeall();
void _cdecl kasm87_showdebug(long showflags, char* debuf, long debuflng);

struct evalextyp
{
	char* nam;
	void* ptr;
};

void _cdecl kasm87addext(evalextyp* daeet, long n);
void kasm87jumpback(void* func, long mode);

extern char kasm87err[256];
extern long kasm87leng;
extern long kasm87err0;
extern long kasm87err1;
extern long kasm87optimize; //0=disable optimizations(faster compile), 1=optimize (default)

