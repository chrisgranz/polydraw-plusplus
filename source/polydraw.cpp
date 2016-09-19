
#include <windows.h>
#include <process.h>

#include <GL/glew.h>
#include <GL/GL.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <malloc.h>

#include "SciLexer.h"
#include "Scintilla.h"

#include "eval.hpp"

#include "PolyDraw.hpp"

//TODO:
// * Bug: glgettex/glsettex doesn't work with global array?
// * printbez() to allow drawing to graphic window
// * GPGPU: allow texture indices to swap read&write
// * multiple render target (gl_FragData)
// * stencil operations?
// * RScript
// * Support transform feedback?
// * new GL syntax (VBO/VAO)
// * pic()&getpicsiz()
// * fil()&getfilsiz()
// * keyread()
// * playtext()/playsound()/playsong()
// * refresh()
// * allow mouse to resize render window with <-> cursor

static int s_TextSize = 0;
static char* s_Text = nullptr;
static char* s_OText = nullptr;
static char* s_TText = nullptr;
static char* s_Line = nullptr;
static char* s_BadLineBits = nullptr;

//tsec_t g_OTSec[MAX_TEXT_SECTIONS];
//tsec_t g_TSec[MAX_TEXT_SECTIONS];
//int g_OTSecN = 0;
//int g_TSecN = 0;
std::vector<tsec_t> g_OTSec;
std::vector<tsec_t> g_TSec;

int g_CapTextSize = 512;

tex_t g_Textures[MAX_USER_TEXURES + 1] = { 0 }; // +1 for font

static const int MAX_SHADERS = 256;
static int s_Shaders[3][MAX_SHADERS];
static int s_ShaderCount[3] = { 0, 0, 0 };
static int s_geo2blocki[MAX_SHADERS];

static const int MAX_SHADER_PROGRAMS = 256;
static int s_ShaderProgramCount = 0;
int g_ShaderPrograms[MAX_SHADER_PROGRAMS];
int g_CurrentShader = 0;

struct shadprogi_t
{
	int vertex;
	int geometry;
	int fragment;
	int ishw;
};

static shadprogi_t s_ShaderProgramsI[MAX_SHADER_PROGRAMS]; // remember linkages

GLint g_Queries[1];

static char* s_ProgramName = "PolyDraw++";
static int s_oxres = 0;
static int s_oyres = 0;
static int s_xres;
static int s_yres;
static int s_ActiveApp = 1;
static int s_ShiftKeyStatus = 0;
static int s_ShaderStuck = 0;
static int s_ShaderCrashed = 0;
static double s_FOV;

double g_dbstatus = 0.0;
double g_dkeystatus[256] = { 0 };
double g_DNumFrames = 0.0;
__int64 g_qper;
__int64 g_qtim0;

static int s_SongTime = 0;
//static int s_mehax = 0;
static int s_DoRecompile = 0;
static char s_SaveFilename[MAX_PATH] = "";
static char* s_SaveFilenamePtr = 0;

double g_RenderWidth;
double g_RenderHeight;
double g_MouseX;
double g_MouseY;

static HINSTANCE s_HInst;

static HWND s_HWndMain = 0;
static HWND s_HWndRender = 0;
static HWND s_HWndConsole = 0;
static HWND s_HWndEditor = 0;

static HFONT s_HFont = 0;
static HMENU s_HMenu = 0;

enum
{
	MENU_FILENEW = 0, MENU_FILEOPEN = MENU_FILENEW + 4, MENU_FILESAVE, MENU_FILESAVEAS, MENU_FILEEXIT,
	MENU_EDITFIND, MENU_EDITFINDNEXT, MENU_EDITFINDPREV, MENU_EDITREPLACE,
	MENU_COMPCONTENT, MENU_EVALHIGHLIGHT, MENU_RENDPLC, MENU_FULLSCREEN = MENU_RENDPLC + 4, MENU_CLEARBUFFER, MENU_FONT,
	MENU_HELPABOUT
};

///////////////////////////////////////////////////////////////////////////////
static char s_ExeFullPath[MAX_PATH] = "";
static char s_ExeDirOnly[MAX_PATH] = "";
static char s_IniFilename[MAX_PATH] = "";

static popt_t s_popts;
static popt_t s_opopts;

///////////////////////////////////////////////////////////////////////////////
static HMIDIOUT s_HMidiOutPlayNote = 0;

///////////////////////////////////////////////////////////////////////////////
static void LoadIni()
{
	//char tbuf[512];

	s_popts.rendcorn    =   0;
	s_popts.fullscreen  =   0;
    s_popts.clearbuffer =   1;
	s_popts.timeout     = 250;
	s_popts.fontheight  = -13;
	s_popts.fontwidth   =   0;
	s_popts.compctrlent =   0;
	s_popts.sepchar     = '-';
	strcpy(s_popts.fontname, "Courier");

	s_popts.rendcorn    = min(max(        GetPrivateProfileInt("POLYDRAW","rendcorn"   ,s_popts.rendcorn   ,s_IniFilename),    0),   4);
	s_popts.fullscreen  = min(max(        GetPrivateProfileInt("POLYDRAW","fullscreen" ,s_popts.fullscreen ,s_IniFilename),    0),   1);
	s_popts.clearbuffer = min(max(        GetPrivateProfileInt("POLYDRAW","clearbuffer",s_popts.clearbuffer,s_IniFilename),    0),   1);
	s_popts.timeout     = min(max(        GetPrivateProfileInt("POLYDRAW","timeout"    ,s_popts.timeout    ,s_IniFilename),    0),5000);
	s_popts.fontheight  = min(max((signed)GetPrivateProfileInt("POLYDRAW","fontheight" ,s_popts.fontheight ,s_IniFilename),-1000),1000);
	s_popts.fontwidth   = min(max((signed)GetPrivateProfileInt("POLYDRAW","fontwidth"  ,s_popts.fontwidth  ,s_IniFilename),-1000),1000);
	s_popts.compctrlent = min(max(        GetPrivateProfileInt("POLYDRAW","compctrlent",s_popts.compctrlent,s_IniFilename),    0),   1);
	s_popts.sepchar     = min(max(        GetPrivateProfileInt("POLYDRAW","sepchar"    ,s_popts.sepchar    ,s_IniFilename),    0), 255);
	GetPrivateProfileString("POLYDRAW", "fontname", s_popts.fontname, s_popts.fontname, sizeof(s_popts.fontname), s_IniFilename);

	memcpy(&s_opopts, &s_popts, sizeof(s_opopts));
}

///////////////////////////////////////////////////////////////////////////////
static void SaveIni()
{
	char tbuf[512];

	if (!memcmp(&s_opopts, &s_popts, sizeof(s_opopts)))
		return;

	sprintf(tbuf,"%d",s_popts.rendcorn   ); WritePrivateProfileString("POLYDRAW","rendcorn"   ,tbuf,s_IniFilename);
	sprintf(tbuf,"%d",s_popts.fullscreen ); WritePrivateProfileString("POLYDRAW","fullscreen" ,tbuf,s_IniFilename);
	sprintf(tbuf,"%d",s_popts.clearbuffer); WritePrivateProfileString("POLYDRAW","clearbuffer",tbuf,s_IniFilename);
	sprintf(tbuf,"%d",s_popts.timeout    ); WritePrivateProfileString("POLYDRAW","timeout"    ,tbuf,s_IniFilename);
	sprintf(tbuf,"%d",s_popts.fontheight ); WritePrivateProfileString("POLYDRAW","fontheight" ,tbuf,s_IniFilename);
	sprintf(tbuf,"%d",s_popts.fontwidth  ); WritePrivateProfileString("POLYDRAW","fontwidth"  ,tbuf,s_IniFilename);
	sprintf(tbuf,"%d",s_popts.compctrlent); WritePrivateProfileString("POLYDRAW","compctrlent",tbuf,s_IniFilename);
	sprintf(tbuf,"%d",s_popts.sepchar    ); WritePrivateProfileString("POLYDRAW","sepchar"    ,tbuf,s_IniFilename);
	sprintf(tbuf,"%s",s_popts.fontname   ); WritePrivateProfileString("POLYDRAW","fontname"   ,tbuf,s_IniFilename);
}

///////////////////////////////////////////////////////////////////////////////
double SetFOV(double fov, double width, double height)
{
	s_FOV = tan(fov*PI / 360.0)*atan((float)height / (float)width)*360.0 / PI;
	return s_FOV;
}

///////////////////////////////////////////////////////////////////////////////
void kputs(const char* st, int addcr)
{
	static char buf[8192];
	static int bufleng = 0, obufleng;
	int i, j, stleng, iminmod;

	if (!st)
		return;

	stleng = 2;

	for (i = 0; st[i]; i++) // calculate processed string length
	{
		if (st[i] == '\n')
			stleng++;
	}

	stleng += i;

	if (stleng >= sizeof(buf)-1)
		return;

	// Remove lines at the top if necessary
	j = 0;
	iminmod = bufleng;
	obufleng = bufleng;

	while ((bufleng - j + stleng) >= sizeof(buf) - 1)
	{
		for (; j < bufleng; j++)
		{
			if (buf[j] == '\n')
			{
				j++;
				break;
			}
		}
	}

	if (j)
	{
		bufleng -= j;
		memmove(&buf[0], &buf[j], bufleng + 1);
		iminmod = 0;
	}

	for(j = 0; st[j]; j++)
	{
		if (st[j] == '\r')
		{
			while (bufleng > 0 && buf[bufleng-1] != '\n')
				bufleng--;

			if (iminmod)
				iminmod = bufleng;

			continue;
		}

		if (st[j] == '\n')
		{
			buf[bufleng] = '\r';
			bufleng++;
		}

		buf[bufleng] = st[j];
		bufleng++;
	}

	if (addcr)
	{
		buf[bufleng++] = '\r';
		buf[bufleng++] = '\n';
	}

	buf[bufleng] = 0;

	if (!iminmod)
	{
		SendMessage(s_HWndConsole, WM_SETTEXT, 0, (long)buf); //SetWindowText(hWndCons,buf);
		SendMessage(s_HWndConsole, EM_LINESCROLL, 0, 0x7fffffff);
	}
	else
	{
		SendMessage(s_HWndConsole, EM_SETSEL, iminmod, obufleng);
		SendMessage(s_HWndConsole, EM_REPLACESEL, 0, (LPARAM)&buf[iminmod]);
	}
}

///////////////////////////////////////////////////////////////////////////////
static HANDLE safecallhand = 0;
static HANDLE safecallevent[2] = { 0, 0 };
static volatile int safecall_kill = 0;
static volatile double saferetdouble;
static double (__cdecl *quickfunc)();

static unsigned __stdcall eval_highlight_safethread(void *_)
{
	while (1)
	{
		WaitForSingleObject(safecallevent[0], INFINITE);

		if (safecall_kill)
			break;

		saferetdouble = quickfunc();
		SetEvent(safecallevent[1]);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static int eval_highlight(char* ptr, int leng)
{
	//double d;
	int i;
	char *quickbuf, tbuf[256];

	quickbuf = (char*)_alloca(leng+3);

	if (!quickbuf)
		return 0;

	quickbuf[0] = '(';
	quickbuf[1] = ')';
	memcpy(&quickbuf[2], ptr, leng);
	quickbuf[leng+2] = 0;
	quickfunc = (double (__cdecl *)()) kasm87(quickbuf);

	if (!quickfunc)
	{
		kputs(kasm87err, 1);
		return 0;
	}

	if (!safecallhand)
	{
		unsigned int win98requiresme;

		for (i = 0; i < 2; i++)
			safecallevent[i] = CreateEvent(0, 0, 0, 0);

		safecall_kill = 0;
		safecallhand = (HANDLE)_beginthreadex(0, 1048576, eval_highlight_safethread, 0, 0, &win98requiresme);
	}

	SetEvent(safecallevent[0]);

	if (WaitForSingleObject(safecallevent[1], 1000) == WAIT_TIMEOUT)
	{
		kasm87jumpback(quickfunc, 0);
		WaitForSingleObject(safecallevent[1], INFINITE);
		kasm87jumpback(quickfunc, 1);
		kasm87free((void*)quickfunc);
		kputs("Ctrl+'=' timeout!",1);
		return 0;
	}

	kasm87free((void *)quickfunc);
	_snprintf(tbuf, sizeof(tbuf), "%.20g", saferetdouble);
	kputs(tbuf, 1);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
static void eval_highlight_kill()
{
	safecall_kill = 1;
	SetEvent(safecallevent[0]);
	WaitForSingleObject(safecallhand, 1000);
}

///////////////////////////////////////////////////////////////////////////////
static HANDLE gthand;
static HANDLE ghevent[3];
static double (__cdecl*s_EvalFunc)() = 0;
static int gevalfuncleng = 0;
static int showtimeout = 0;

///////////////////////////////////////////////////////////////////////////////
static unsigned int __stdcall watchthread(void*)
{
	while (1)
	{
		WaitForSingleObject(ghevent[0], INFINITE);

		// if script takes too long, temporarily apply self-modifying code to force it to finish much faster
		if (WaitForSingleObject(ghevent[1], s_popts.timeout) == WAIT_TIMEOUT)
		{
			showtimeout = 1;
			kasm87jumpback(s_EvalFunc, 0);
			WaitForSingleObject(ghevent[1], INFINITE);
			kasm87jumpback(s_EvalFunc, 1);
		}

		SetEvent(ghevent[2]);
	}
}

///////////////////////////////////////////////////////////////////////////////
static short* menustart(short* sptr)
{
	*sptr++ = 0;
	*sptr++ = 0;
	return sptr;
}

///////////////////////////////////////////////////////////////////////////////
static short* menuadd(short* sptr, char* st, int flags, int id)
{
	*sptr++ = flags; //MENUITEMTEMPLATE

	if (!(flags & MF_POPUP))
		*sptr++ = id;

	sptr += MultiByteToWideChar(CP_ACP, 0, st, -1, (LPWSTR)sptr, strlen(st) + 1);
	return sptr;
}

///////////////////////////////////////////////////////////////////////////////
static int myprintf_check(char* fmt)
{
	int i, inperc, inslash;

	if (!fmt) return(-1);

	inperc = 0; inslash = 0; // Filter out

	for (i = 0; fmt[i]; i++)
	{
		if (inslash) { inslash = 0; continue; }
		if (fmt[i] == '\\') { inslash = 1; continue; }
		if (fmt[i] == '%') { inperc ^= 1; continue; }
		if (!inperc) continue;

		//int types
		if ((fmt[i] == 'c') || (fmt[i] == 'C') || (fmt[i] == 'd') || (fmt[i] == 'i') ||
			 (fmt[i] == 'o') || (fmt[i] == 'u') || (fmt[i] == 'x') || (fmt[i] == 'X'))
			{ kputs("invalid %",1); return(0); }

		//double types
		if ((fmt[i] == 'e') || (fmt[i] == 'E') || (fmt[i] == 'f') || (fmt[i] == 'g') || (fmt[i] == 'G'))
			{ inperc = 0; continue; }

		//pointer types
		if ((fmt[i] == 'n') || (fmt[i] == 'p') || (fmt[i] == 's') || (fmt[i] == 'S') || (fmt[i] == 'Z'))
			{ kputs("invalid %",1); return(0); }
	}
	return(1);
}

///////////////////////////////////////////////////////////////////////////////
static void myprintf_filter(char* st)
{
	int i, j, inslash;

	//Filter \\, \n, etc..
	inslash = 0;
	for (i = 0, j = 0; st[i]; i++)
	{
		if (inslash)
		{
			inslash = 0;
			if (st[i] == 'b') { if (j) j--; continue; }
			if (st[i] == 'r') { st[j++] = 13; continue; }
			if (st[i] == 'n') { st[j++] = 10; continue; }
			if (st[i] == 't') { st[j++] = 9; continue; }
		} else if (st[i] == '\\') { inslash = 1; continue; }
		st[j++] = st[i];
	}
	st[j] = 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl myprintf(char *fmt, ...)
{
	va_list arglist;
	char st[2048];

	if (!myprintf_check(fmt))
		return -1.0;

	va_start(arglist,fmt);
	_vsnprintf(st, sizeof(st), fmt, arglist);
	va_end(arglist);

	myprintf_filter(st);

	kputs(st, 0);

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
// NOTE: font is stored vertically first! (like .ART files)
static const __int64 font6x8[] = //256 DOS chars, from: DOSAPP.FON (tab blank)
{
	0x3E00000000000000, 0x6F6B3E003E455145, 0x1C3E7C3E1C003E6B, 0x3000183C7E3C1800,
	0x7E5C180030367F36, 0x000018180000185C, 0x0000FFFFE7E7FFFF, 0xDBDBC3FF00000000,
	0x0E364A483000FFC3, 0x6000062979290600, 0x0A7E600004023F70, 0x2A1C361C2A003F35,
	0x0800081C3E7F0000, 0x7F361400007F3E1C, 0x005F005F00001436, 0x22007F017F090600,
	0x606060002259554D, 0x14B6FFB614000060, 0x100004067F060400, 0x3E08080010307F30,
	0x08083E1C0800081C, 0x0800404040407800, 0x3F3C3000083E083E, 0x030F3F0F0300303C,
	0x0000000000000000, 0x0003070000065F06, 0x247E247E24000307, 0x630000126A2B2400,
	0x5649360063640813, 0x0000030700005020, 0x00000000413E0000, 0x1C3E080000003E41,
	0x08083E080800083E, 0x0800000060E00000, 0x6060000008080808, 0x0204081020000000,
	0x00003E4549513E00, 0x4951620000407F42, 0x3649494922004649, 0x2F00107F12141800,
	0x494A3C0031494949, 0x0305097101003049, 0x0600364949493600, 0x6C6C00001E294949,
	0x00006CEC00000000, 0x2400004122140800, 0x2241000024242424, 0x0609590102000814,
	0x7E001E555D413E00, 0x49497F007E111111, 0x224141413E003649, 0x7F003E4141417F00,
	0x09097F0041494949, 0x7A4949413E000109, 0x00007F0808087F00, 0x4040300000417F41,
	0x412214087F003F40, 0x7F00404040407F00, 0x04027F007F020402, 0x3E4141413E007F08,
	0x3E00060909097F00, 0x09097F005E215141, 0x3249494926006619, 0x3F0001017F010100,
	0x40201F003F404040, 0x3F403C403F001F20, 0x0700631408146300, 0x4549710007087008,
	0x0041417F00000043, 0x0000201008040200, 0x01020400007F4141, 0x8080808080800402,
	0x2000000007030000, 0x44447F0078545454, 0x2844444438003844, 0x38007F4444443800,
	0x097E080008545454, 0x7CA4A4A418000009, 0x0000007804047F00, 0x8480400000407D00,
	0x004428107F00007D, 0x7C0000407F000000, 0x04047C0078041804, 0x3844444438000078,
	0x380038444444FC00, 0x44784400FC444444, 0x2054545408000804, 0x3C000024443E0400,
	0x40201C00007C2040, 0x3C6030603C001C20, 0x9C00006C10106C00, 0x54546400003C60A0,
	0x0041413E0800004C, 0x0000000077000000, 0x02010200083E4141, 0x3C2623263C000001,
	0x3D001221E1A11E00, 0x54543800007D2040, 0x7855555520000955, 0x2000785554552000,
	0x5557200078545555, 0x1422E2A21C007857, 0x3800085555553800, 0x5555380008555455,
	0x00417C0100000854, 0x0000004279020000, 0x2429700000407C01, 0x782F252F78007029,
	0x3400455554547C00, 0x7F097E0058547C54, 0x0039454538004949, 0x3900003944453800,
	0x21413C0000384445, 0x007C20413D00007D, 0x3D00003D60A19C00, 0x40413C00003D4242,
	0x002466241800003D, 0x29006249493E4800, 0x16097F00292A7C2A, 0x02097E8840001078,
	0x0000785555542000, 0x4544380000417D00, 0x007D21403C000039, 0x7A0000710A097A00,
	0x5555080000792211, 0x004E51514E005E55, 0x3C0020404D483000, 0x0404040404040404,
	0x506A4C0817001C04, 0x0000782A34081700, 0x0014080000307D30, 0x0814000814001408,
	0x55AA114411441144, 0xEEBBEEBB55AA55AA, 0x0000FF000000EEBB, 0x0A0A0000FF080808,
	0xFF00FF080000FF0A, 0x0000F808F8080000, 0xFB0A0000FE0A0A0A, 0xFF00FF000000FF00,
	0x0000FE02FA0A0000, 0x0F0800000F080B0A, 0x0F0A0A0A00000F08, 0x0000F80808080000,
	0x080808080F000000, 0xF808080808080F08, 0x0808FF0000000808, 0x0808080808080808,
	0xFF0000000808FF08, 0x0808FF00FF000A0A, 0xFE000A0A0B080F00, 0x0B080B0A0A0AFA02,
	0x0A0AFA02FA0A0A0A, 0x0A0A0A0AFB00FF00, 0xFB00FB0A0A0A0A0A, 0x0A0A0B0A0A0A0A0A,
	0x0A0A08080F080F08, 0xF808F8080A0AFA0A, 0x08080F080F000808, 0x00000A0A0F000000,
	0xF808F8000A0AFE00, 0x0808FF00FF080808, 0x08080A0AFB0A0A0A, 0xF800000000000F08,
	0xFFFFFFFFFFFF0808, 0xFFFFF0F0F0F0F0F0, 0xFF000000000000FF, 0x0F0F0F0F0F0FFFFF,
	0xFE00241824241800, 0x01017F0000344A4A, 0x027E027E02000003, 0x1800006349556300,
	0x2020FC00041C2424, 0x000478040800001C, 0x3E00085577550800, 0x02724C00003E4949,
	0x0030595522004C72, 0x1800182418241800, 0x2A2A1C0018247E24, 0x003C02023C00002A,
	0x0000002A2A2A2A00, 0x4A4A510000242E24, 0x00514A4A44000044, 0x20000402FC000000,
	0x2A08080000003F40, 0x0012241224000808, 0x0000000609090600, 0x0008000000001818,
	0x02023E4030000000, 0x0900000E010E0100, 0x3C3C3C0000000A0D, 0x000000000000003C,
};

///////////////////////////////////////////////////////////////////////////////
extern void CreateEmptyTexture(int itex, int xs, int ys, int zs, int icoltype);
static unsigned int s_fontid;

///////////////////////////////////////////////////////////////////////////////
static void printg_init()
{
	int j, x, y, xsiz, ysiz, *iptr, *tbuf;

	// Load 6x8(x256) font
	xsiz = 8; ysiz = 8*256;

	if ((tbuf = (int*)malloc(xsiz*ysiz*4)) == NULL)
		return;

	for (y = 0, iptr = tbuf; y < ysiz; y++, iptr += xsiz)
	{
		for (x = 0; x < xsiz; x++)
		{
			if (x < 6)
			{
				if (((char*)font6x8)[(y >> 3) * 6 + (x & 7)] & (1 << (y & 7)))
					j = -1;
				else
					j = 0;
			}
			else
				j = 0;

			iptr[x] = j;
		}
	}

	s_fontid = MAX_USER_TEXURES;
	CreateEmptyTexture(s_fontid, xsiz, ysiz, 1, KGL_NEAREST + KGL_CLAMP_TO_EDGE);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, xsiz, ysiz, GL_BGRA_EXT, GL_UNSIGNED_BYTE, tbuf);

	free((void*)tbuf);
}

///////////////////////////////////////////////////////////////////////////////
double myprintg(double dx, double dy, double dfcol, char *fmt, ...)
{
	va_list arglist;
	unsigned char* cptr, st[2048];
	int ich, intab, x, y, fcol;
	double ocol[4];

	if (!myprintf_check(fmt))
		return -1.0;

	va_start(arglist, fmt);

	if (_vsnprintf((char *)&st, sizeof(st) - 1, fmt, arglist))
		st[sizeof(st) - 1] = 0;

	va_end(arglist);

	myprintf_filter((char*)st);

	x = (int)dx;
	y = (int)dy;
	fcol = (int)dfcol;

	//Need to backup/restore:
	//  PROJECTION matrix  Use glPushMatrix()
	//  MODELVIEW matrix   Use glPushMatrix()
	//  GL_MATRIX_MODE     Use glPushAttrib(GL_MODELVIEW)
	//  GL_DEPTH_TEST      Use glPushAttrib(GL_ENABLE_BIT or GL_DEPTH_BUFFER_BIT)
	//  GL_BLEND           Use glPushAttrib(GL_ENABLE_BIT or GL_COLOR_BUFFER_BIT)
	//  GL_TEXTURE_2D      Use glPushAttrib(GL_ENABLE_BIT)
	//  glColor            Use glGet...glColor

	/*
	if ((!usearbasm) && (!usearbasmonly))
	{
		//((PFNGLUSEPROGRAMPROC)g_glfp[glUseProgram])(0);
		glUseProgram(0);
	}
	else
	{
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}
	*/
	glUseProgram(0);

	//if (g_glfp[glActiveTexture])
	//	((PFNGLACTIVETEXTUREPROC)g_glfp[glActiveTexture])(GL_TEXTURE0);
	glActiveTexture(GL_TEXTURE0);

	glPushAttrib(GL_ENABLE_BIT | GL_MODELVIEW);
	glGetDoublev(GL_CURRENT_COLOR, ocol);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, g_RenderWidth, g_RenderHeight, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glColor3ub((fcol >> 16) & 255, (fcol >> 8) & 255, fcol & 255);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_fontid);
	glBegin(GL_QUADS);
	intab = 0;

	for (cptr = st; cptr[0]; x += 6)
	{
		if (intab)
			intab--;
		else
			ich = *cptr++;

		if (ich == 9)
		{
			intab = 2;
			ich = ' ';
		}

		glTexCoord2f(0.0f, ((float)ich) / 256.0f);
		glVertex2i(x, y);
		glTexCoord2f(0.75f, ((float)ich) / 256.0f);
		glVertex2i(x+6, y);
		glTexCoord2f(0.75f, ((float)(ich + 1)) / 256.0f);
		glVertex2i(x+6, y+8);
		glTexCoord2f(0.0f, ((float)(ich + 1)) / 256.0f);
		glVertex2i(x, y+8);
	}

	glEnd();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glColor4dv(ocol);
	glPopAttrib();

	/*
	if (usearbasm || usearbasmonly)
	{
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
	}
	*/

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER)
static __forceinline void dtol(double f, int *a)
{
	_asm
	{
		mov eax, a
		fld f
		fistp dword ptr [eax]
	}
}
#else
static void dtol(double f, int *a)
{
	a = (int)f;
}
#endif

///////////////////////////////////////////////////////////////////////////////
double __cdecl setshader_int(int sh0, int sh1, int sh2)
{
	char tbuf[4096];
	int i, j;

	/*
	if ((usearbasm) || (usearbasmonly))
	{
		if ((unsigned)sh0 >= (unsigned)shadn[0]) sh0 = 0;
		if ((unsigned)sh2 >= (unsigned)shadn[2]) sh2 = 0;
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, shad[0][sh0]);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shad[2][sh2]);
		return 0.0;
	}
	*/

	for (i = 0; i < s_ShaderProgramCount; i++)
	{
		if (s_ShaderProgramsI[i].vertex == sh0 && s_ShaderProgramsI[i].geometry == sh1 && s_ShaderProgramsI[i].fragment == sh2)
		{
			if (!s_ShaderProgramsI[i].ishw)
			{
				g_CurrentShader = 0;
				return -1.0;
			}

			glUseProgram(g_ShaderPrograms[i]);
			g_CurrentShader = i;
			return 0.0;
		}
	}

	if (s_ShaderProgramCount >= MAX_SHADER_PROGRAMS)
		return 0.0; //silent error :/

	if ((unsigned)sh0 >= (unsigned)s_ShaderCount[0])
		sh0 = 0;

	if ((unsigned)sh1 >= (unsigned)s_ShaderCount[1])
		sh1 =-1;

	if ((unsigned)sh2 >= (unsigned)s_ShaderCount[2])
		sh2 = 0;

	g_CurrentShader = s_ShaderProgramCount;
	s_ShaderProgramsI[s_ShaderProgramCount].vertex = sh0;
	s_ShaderProgramsI[s_ShaderProgramCount].geometry = sh1;
	s_ShaderProgramsI[s_ShaderProgramCount].fragment = sh2;
	s_ShaderProgramsI[s_ShaderProgramCount].ishw = 1;

	g_ShaderPrograms[s_ShaderProgramCount] = glCreateProgram();
	glAttachShader(g_ShaderPrograms[s_ShaderProgramCount], s_Shaders[0][sh0]);

	if (sh1 >= 0)
		glAttachShader(g_ShaderPrograms[s_ShaderProgramCount], s_Shaders[1][sh1]);

	glAttachShader(g_ShaderPrograms[s_ShaderProgramCount], s_Shaders[2][sh2]);

	//if (sh1 >= 0 && g_glfp[glProgramParameteri])

	{
		//Example: @g,GL_TRIANGLES,GL_TRIANGLE_STRIP,1024:myname
		//glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS,&n); //2048
		//glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES,&n); //1024
		//glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS,&n); //1024
		i = s_geo2blocki[sh1];

		glProgramParameteri(g_ShaderPrograms[s_ShaderProgramCount], GL_GEOMETRY_INPUT_TYPE_EXT, g_TSec[i].geo_in);
		glProgramParameteri(g_ShaderPrograms[s_ShaderProgramCount], GL_GEOMETRY_OUTPUT_TYPE_EXT, g_TSec[i].geo_out);
		glProgramParameteri(g_ShaderPrograms[s_ShaderProgramCount], GL_GEOMETRY_VERTICES_OUT_EXT, g_TSec[i].geo_nverts);
	}

	glLinkProgram(g_ShaderPrograms[s_ShaderProgramCount]);
	glGetProgramiv(g_ShaderPrograms[s_ShaderProgramCount], GL_LINK_STATUS, &i);

	// NOTE:must get infolog anyway because driver doesn't consider running in SW an error.
	glGetInfoLogARB(g_ShaderPrograms[s_ShaderProgramCount], sizeof(tbuf), 0, tbuf);

	j = (strstr(tbuf, "software") != 0);

	if (!i || j) // the string of evil..
	{
		if (!i)
			kputs(tbuf,1);

		if (j)
			kputs("Shader won't run in HW! Execution denied. :/", 1);

		s_ShaderProgramsI[s_ShaderProgramCount].ishw = 0;
		s_ShaderProgramCount++;

		return -1.0;
	}

	glUseProgram(g_ShaderPrograms[s_ShaderProgramCount]);

	// Note: Get*Uniform*() must be called after glUseProgram() to work properly
	glUniform1i(glGetUniformLocation(g_ShaderPrograms[s_ShaderProgramCount], "tex0"), 0);
	glUniform1i(glGetUniformLocation(g_ShaderPrograms[s_ShaderProgramCount], "tex1"), 1);
	glUniform1i(glGetUniformLocation(g_ShaderPrograms[s_ShaderProgramCount], "tex2"), 2);
	glUniform1i(glGetUniformLocation(g_ShaderPrograms[s_ShaderProgramCount], "tex3"), 3);

	s_ShaderProgramCount++;
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
static void MIDIUninit()
{
	if ((((long)s_HMidiOutPlayNote) + 1) & 0xfffffffe)
	{
		midiOutClose(s_HMidiOutPlayNote);
		s_HMidiOutPlayNote = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
double MIDIPlayNote(double chn, double frq, double vol)
{
	if (s_HMidiOutPlayNote == (HMIDIOUT)-1)
		return -1.0;

	if (s_HMidiOutPlayNote == 0)
	{
		if (midiOutOpen(&s_HMidiOutPlayNote, MIDI_MAPPER, 0, 0, 0) != MMSYSERR_NOERROR)
		{
			s_HMidiOutPlayNote = (HMIDIOUT)-1;
			return -1.0;
		}
	}

	midiOutShortMsg(s_HMidiOutPlayNote, (min(max((int)vol, 0), 127) << 16) +
		(min(max((int)frq, 0), 127) << 8) +
		(min(max((int)chn, 0), 255)));
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
// Parse script for '@' lines, generating list of sections
static std::vector<tsec_t> Text2Sections(char* text)
{
	std::vector<tsec_t> sections;
	tsec_t sec;

	int i, j, i0, ntyp, n, slast[4], scnt[4], olin, line;
	char* cptr;

	for (i = (4 - 1); i >= 0; i--)
	{
		slast[i] = -1;
		scnt[i] = 0;
	}

	i0 = 0;
	ntyp = 0;
	n = 0;
	olin = 0;
	line = 0;
	sec.name[0] = '\0';

	for (i = 0; text[i] != '\0'; i++)
	{
		// comment
		if (text[i] == '/')
		{
			if (text[i + 1] == '/') // line comment
			{
				for (i += 2; text[i] != '\0' && text[i] != '\n'; i++)
					;

				++line;
				continue;
			}

			if (text[i + 1] == '*') // block comment
			{
				for (i += 2; text[i] != '\0' && (text[i] != '*' || text[i + 1] != '/'); i++)
				{
					if (text[i] == '\n')
						line++;
				}

				if (text[i] == '\0')
					break;

				i++;
				continue;
			}

			continue;
		}

		// line ending
		if (text[i] == '\n')
		{
			line++;
			continue;
		}

		// if not the next section, just continue to next character
		if (text[i] != '@' || (i > 0 && (text[i - 1] != '\r' && text[i - 1] != '\n')))
			continue;

		//if (text[i] == '@' && (i == 0 || (text[i - 1] == '\r' || text[i - 1] == '\n'))) // start of new section
		//{
			// now save previous section in sections collection
			{
				sec.i0 = i0;
				sec.i1 = i;
				sec.type = ntyp;
				sec.count = scnt[ntyp];
				sec.lineOffset = olin;
				sec.nextType = -1;
				sections.push_back(sec);
			}

			if (slast[ntyp] >= 0)
				sections[slast[ntyp]].nextType = n;

			slast[ntyp] = n;
			scnt[ntyp]++;

			i++;

			if (text[i] == 'h') ntyp = 0;
			else if (text[i] == 'v') ntyp = 1; // vertex shader
			else if (text[i] == 'g') ntyp = 2; // geometry shader
			else if (text[i] == 'f') ntyp = 3; // fragment shader
			else i--; // reuse ntyp

			i++;

			if (ntyp == 2 && text[i] == ',') // read options for geometry shader if needed
			{
				i++;

				// default values
				sec.geo_in = GL_TRIANGLES;
				sec.geo_out = GL_TRIANGLE_STRIP;
				sec.geo_nverts = 8; // NOTE: default is 0 in specification

				if ((strlen(&text[i]) >= 9) && (!_memicmp(&text[i], "GL_POINTS", 9))) { i += 9; sec.geo_in = GL_POINTS; }
				if ((strlen(&text[i]) >= 8) && (!_memicmp(&text[i], "GL_LINES", 8))) { i += 8; sec.geo_in = GL_LINES; }
				if ((strlen(&text[i]) >= 18) && (!_memicmp(&text[i], "GL_LINES_ADJACENCY", 18))) { i += 22; sec.geo_in = GL_LINES_ADJACENCY_EXT; }
				if ((strlen(&text[i]) >= 22) && (!_memicmp(&text[i], "GL_LINES_ADJACENCY_EXT", 22))) { i += 22; sec.geo_in = GL_LINES_ADJACENCY_EXT; }
				if ((strlen(&text[i]) >= 12) && (!_memicmp(&text[i], "GL_TRIANGLES", 12))) { i += 12; sec.geo_in = GL_TRIANGLES; }
				if ((strlen(&text[i]) >= 22) && (!_memicmp(&text[i], "GL_TRIANGLES_ADJACENCY", 22))) { i += 26; sec.geo_in = GL_TRIANGLES_ADJACENCY_EXT; }
				if ((strlen(&text[i]) >= 26) && (!_memicmp(&text[i], "GL_TRIANGLES_ADJACENCY_EXT", 26))) { i += 26; sec.geo_in = GL_TRIANGLES_ADJACENCY_EXT; }

				if (text[i] == ',')
				{
					i++;
					if ((strlen(&text[i]) >= 9) && (!_memicmp(&text[i], "GL_POINTS", 9))) { i += 9; sec.geo_out = GL_POINTS; }
					if ((strlen(&text[i]) >= 13) && (!_memicmp(&text[i], "GL_LINE_STRIP", 13))) { i += 13; sec.geo_out = GL_LINE_STRIP; }
					if ((strlen(&text[i]) >= 17) && (!_memicmp(&text[i], "GL_TRIANGLE_STRIP", 17))) { i += 17; sec.geo_out = GL_TRIANGLE_STRIP; }
					if (text[i] == ',')
					{
						i++;
						sec.geo_nverts = strtol(&text[i], &cptr, 0); // ~1..1024
						i = cptr - &text[0];
					}
				}
			}

			if (text[i] == ':' && n < (MAX_TEXT_SECTIONS - 1))
			{
				j = 0;

				for (i++; text[i] && text[i] != '\r' && text[i] != '\n'; i++)
				{
					if (text[i] == '/' && text[i + 1] == '/')
						break;

					if (j < (sizeof(sec.name) - 1))
					{
						sec.name[j] = text[i];
						j++;
					}
				}

				while (j > 0 && sec.name[j - 1] == ' ')
					j--;

				sec.name[j] = '\0';
			}
			else
				sec.name[0] = '\0';

			while (text[i] && text[i] != '\n')
				i++;

			line++;
			olin = line;
			i0 = i + 1;

			++n;

			if (n >= MAX_TEXT_SECTIONS)
				return sections;
		//}
	}

	//ltsec[n].i0 = i0;
	//ltsec[n].i1 = i;
	//ltsec[n].type = ntyp;
	//ltsec[n].count = scnt[ntyp];
	//ltsec[n].lineOffset = olin;
	//ltsec[n].nextType = -1;

	// save last section in sections collection
	{
		sec.i0 = i0;
		sec.i1 = i;
		sec.type = ntyp;
		sec.count = scnt[ntyp];
		sec.lineOffset = olin;
		sec.nextType = -1;
		sections.push_back(sec);
	}

	if (slast[ntyp] >= 0)
		sections[slast[ntyp]].nextType = n;

	//n++;
	return sections;
}

///////////////////////////////////////////////////////////////////////////////
static void glsl_geterrorlines(char* error, int offs)
{
	int j;

	for (int i = 0; error[i]; i++)
	{
		if (i > 0 && error[i-1] != '\n') // Check only at beginning of lines
			continue;

		if (!memcmp(&error[i], "0(", 2)) // NVIDIA style
		{
			j = atol(&error[i + 2]) - 1;

			if ((unsigned)j >= (unsigned)s_TextSize)
				continue;

			j += offs;
			s_BadLineBits[j >> 3] |= (1 << (j & 7));
		}
		else if (!memcmp(&error[i], "(", 1)) // NVIDIA style (old)
		{
			j = atol(&error[i + 1]) - 1;

			if ((unsigned)j >= (unsigned)s_TextSize)
				continue;

			j += offs;
			s_BadLineBits[j >> 3] |= (1 << (j & 7));
		}
		else if (!memcmp(&error[i], "ERROR: ", 7)) // ATI(AMD)/Intel style
		{
			i += 7;

			if (!(error[i] >= '0' && error[i] <= '9'))
				continue;

			i++;

			while (error[i] >= '0' && error[i] <= '9')
				i++;

			if (error[i] != ':')
				continue;

			i++;

			j = atol(&error[i]) - 1;

			if ((unsigned)j >= (unsigned)s_TextSize)
				continue;

			j += offs;
			s_BadLineBits[j >> 3] |= (1 << (j & 7));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
static void SetShaders(HWND h, HWND hWndEdit)
{
	static const char shadnam[3][5] = { "vert", "geom", "frag" };
	static const int shadconst[3] = { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER_EXT, GL_FRAGMENT_SHADER };
	int i, j, k, compiled, needloop = 0;
	bool NeedRecompile;
	size_t tseci;
	char ch;
	char* cptr;
	char tbuf[4096];
	const char* erst;

	if (s_ShaderStuck)
		return;

#if 0
	if (dkeystatus[0x2a]) //debug only!
	{
		dkeystatus[0x2a] = 0;
		sprintf(tbuf,"tsecn=%d",tsecn); kputs(tbuf,1);
		for(i=0;i<tsecn;i++)
		{
			sprintf(tbuf,"%d %d %d %d %d %d [%d %d %d]|%s|",tsec[i].i0,tsec[i].i1,tsec[i].typ,tsec[i].cnt,tsec[i].linofs,tsec[i].nxt,tsec[i].geo_in,tsec[i].geo_out,tsec[i].geo_nverts,tsec[i].nam);
			kputs(tbuf,1);
		}
	}
#endif

	if (s_DoRecompile & 2)
	{
		s_DoRecompile &= ~2;
		NeedRecompile = true;
	}
	else if (s_popts.compctrlent)
		NeedRecompile = false;
	else
	{
		NeedRecompile = false;

		for (tseci = 0; tseci < g_TSec.size(); tseci++)
		{
			//Compare block
			if (!g_TSec[tseci].type)
				continue;

			if (tseci >= g_OTSec.size() || g_TSec[tseci].type != g_OTSec[tseci].type)
			{
				NeedRecompile = true;
				break;
			}

			if ((g_TSec[tseci].i1 - g_TSec[tseci].i0) != (g_OTSec[tseci].i1 - g_OTSec[tseci].i0))
			{
				NeedRecompile = true;
				break;
			}

			if (memcmp(&s_Text[g_TSec[tseci].i0], &s_OText[g_OTSec[tseci].i0], g_TSec[tseci].i1 - g_TSec[tseci].i0) != 0)
			{
				NeedRecompile = true;
				break;
			}
		}
	}

	if (!NeedRecompile)
		return;

	memset(s_BadLineBits, 0, (s_TextSize + 7) >> 3);

	for (i = 0; i < 3; i++)
	{
		for (; s_ShaderCount[i] > 0; s_ShaderCount[i]--)
			glDeleteShader(s_Shaders[i][s_ShaderCount[i] - 1]);
	}

	for(; s_ShaderProgramCount > 0; s_ShaderProgramCount--)
		glDeleteProgram(g_ShaderPrograms[s_ShaderProgramCount - 1]);

	for (tseci = 0; tseci < g_TSec.size(); tseci++)
	{
		if (g_TSec[tseci].type == 0)
			continue;

		//Compare block
		//if ((tseci >= otsecn) || (tsec[tseci].typ != otsec[tseci].typ)) needrecompile = 1;
		//else if (tsec[tseci].i1-tsec[tseci].i0 != otsec[tseci].i1-otsec[tseci].i0) needrecompile = 1;
		//else if (memcmp(&text[tsec[tseci].i0],&otext[otsec[tseci].i0],tsec[tseci].i1-tsec[tseci].i0)) needrecompile = 1;
		//else needrecompile = 0;

		j = g_TSec[tseci].type - 1;

		if (s_Text[g_TSec[tseci].i0] == '!' && s_Text[g_TSec[tseci].i0+1] == '!') // ARB ASM section
		{
			glUseProgram(0);

			glGetError(); //flush errors (could be from script)

			if (g_TSec[tseci].name[0])
				sprintf(tbuf, "Compile %s_asm %s", shadnam[j], g_TSec[tseci].name);
			else
				sprintf(tbuf, "Compile %s_asm#%d", shadnam[j], g_TSec[tseci].count);

			if (g_TSec[tseci].type&1)
				kputs(tbuf, 1);

			if (g_TSec[tseci].type == 1)
				i = GL_VERTEX_PROGRAM_ARB;
			else if (g_TSec[tseci].type == 3)
				i = GL_FRAGMENT_PROGRAM_ARB;
			else
				i = 0;

			if (i)
			{
				glEnable(i);

				glGenProgramsARB(1, (GLuint*)&j);
				glBindProgramARB(i, j);

				if (i == GL_VERTEX_PROGRAM_ARB)
					s_Shaders[0][s_ShaderCount[0]] = j;
				else
					s_Shaders[2][s_ShaderCount[2]] = j;

				glProgramStringARB(i, GL_PROGRAM_FORMAT_ASCII_ARB, g_TSec[tseci].i1 - g_TSec[tseci].i0, &s_Text[g_TSec[tseci].i0]);

				if (glGetError() != GL_NO_ERROR)
				{
					erst = (const char*) glGetString(GL_PROGRAM_ERROR_STRING_ARB);
					kputs(erst, 1);

					for (j = 0; erst[j]; j++)
					{
						if (j && erst[j-1] != '\n')
							continue;

						if (!_memicmp(&erst[j], "line ", 5))
							k = atol(&erst[j + 5]) - 1;
						else if (!_memicmp(&erst[j], "Error, line ", 12))
							k = atol(&erst[j + 12]) - 1;
						else
							continue;

						if ((unsigned)k < (unsigned)s_TextSize)
							s_BadLineBits[(g_TSec[tseci].lineOffset + k) >> 3] |= (1 << ((g_TSec[tseci].lineOffset + k) & 7));
					}

					return;
				}
			}
		}
		//else if (j == 1 && !g_glfp[glProgramParameteri])
		//{
		//	kputs("ERROR: Geometry shader not supported by this hardware :/", 1);
		//}
		else
		{
			//usearbasm = 0;

			if (g_TSec[tseci].name[0])
				sprintf(tbuf, "Compile %s %s", shadnam[j], g_TSec[tseci].name);
			else
				sprintf(tbuf, "Compile %s#%d", shadnam[j], g_TSec[tseci].count);

			kputs(tbuf, 1);

			if (j == 1)
				s_geo2blocki[g_TSec[tseci].count] = tseci; //map shader to block for geometry (to access geo_in, geo_out, geo_nverts)

			i = glCreateShader(shadconst[j]);
			s_Shaders[j][s_ShaderCount[j]] = i;
			cptr = &s_Text[g_TSec[tseci].i0];
			ch = s_Text[g_TSec[tseci].i1];
			s_Text[g_TSec[tseci].i1] = 0;
			glShaderSource(i, 1, (const GLchar**)&cptr, 0);
			s_Text[g_TSec[tseci].i1] = ch;

			glCompileShader(i);
			glGetShaderiv(i, GL_COMPILE_STATUS, &compiled);

			if (!compiled)
			{
				glGetInfoLogARB(i, sizeof(tbuf), 0, tbuf);
				kputs(tbuf, 1);
				glsl_geterrorlines(tbuf, g_TSec[tseci].lineOffset);
				return;
			}
		}

		s_ShaderCount[g_TSec[tseci].type-1]++;
	}

	g_CurrentShader = 0;
	setshader_int(0, -1, 0);
}

///////////////////////////////////////////////////////////////////////////////
// This cover function protects SOME cases
static EXCEPTION_RECORD gexception_record;
static CONTEXT gexception_context;
static int myexception_getaddr(LPEXCEPTION_POINTERS pxi)
{
	memcpy(&gexception_record, pxi->ExceptionRecord, sizeof(EXCEPTION_RECORD));
	memcpy(&gexception_context, pxi->ContextRecord, sizeof(CONTEXT));
	return(EXCEPTION_EXECUTE_HANDLER);
}

///////////////////////////////////////////////////////////////////////////////
void SafeEvalFunc()
{
	__try
	{
		s_EvalFunc();
	}
	__except(myexception_getaddr(GetExceptionInformation()))
	{
		char tbuf[256];
		sprintf(tbuf, "\nShader Exception 0x%08x @ 0x%08x :/", gexception_record.ExceptionCode, gexception_record.ExceptionAddress);
		kputs(tbuf, 1);
		s_ShaderCrashed = 1;
		MessageBeep(16); //evil
	}
}

///////////////////////////////////////////////////////////////////////////////
static void Render(HWND hWnd, HWND hWndEdit)
{       
	bool NeedRecompile = false;
	size_t tseci = 0;
	char ch;

	// Find host block (use only last one if multiple found)
	while (g_TSec[tseci].type != 0)
	{
		if (tseci >= g_TSec.size())
			return;

		++tseci;
	}

	while (g_TSec[tseci].nextType >= 0)
		tseci = g_TSec[tseci].nextType;

	if (tseci >= g_OTSec.size() || g_OTSec[tseci].type || g_OTSec[tseci].nextType >= 0)
		NeedRecompile = true;
	else if (g_TSec[tseci].i1 - g_TSec[tseci].i0 != g_OTSec[tseci].i1 - g_OTSec[tseci].i0)
		NeedRecompile = true;
	else if (memcmp(&s_Text[g_TSec[tseci].i0], &s_OText[g_OTSec[tseci].i0], g_TSec[tseci].i1 - g_TSec[tseci].i0))
		NeedRecompile = true;
	else
		NeedRecompile = false;

	if (s_popts.compctrlent)
		NeedRecompile = false;

	if (s_DoRecompile & 1)
	{
		s_DoRecompile &= ~1;
		NeedRecompile = true;
	}

	if (NeedRecompile)
	{
		s_ShaderCrashed = 0;

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		glDisable(GL_CULL_FACE);

		QueryPerformanceCounter((LARGE_INTEGER*)&g_qtim0);
		g_DNumFrames = 0.0;

		kputs("Compile host program: ", 0);

		if (s_EvalFunc)
			kasm87free(s_EvalFunc);

		ch = s_Text[g_TSec[tseci].i1];
		s_Text[g_TSec[tseci].i1] = 0;
		s_EvalFunc = (double(__cdecl*)())CompileEVALFunctionWithExt(&s_Text[g_TSec[tseci].i0]);
		s_Text[g_TSec[tseci].i1] = ch;
		gevalfuncleng = kasm87leng;

		//NOTE: use tsec[tseci].linofs as offset when adding support for line of error

		kasm87addext(nullptr, 0);

		s_SongTime = 0;

		if (gthand)
		{
			for (int i = 0; i < 3; i++)
				ResetEvent(ghevent[i]);
		}
	}

	// opdate mouse position
	{
		POINT p0, p1;
		p0.x = p0.y = 0;
		p1.x = p1.y = 0;
		ClientToScreen(s_HWndRender, &p0);
		GetCursorPos(&p1);
		g_MouseX = double(p1.x - p0.x);
		g_MouseY = double(p1.y - p0.y);
	}

	if (s_EvalFunc != nullptr && !s_ShaderStuck && !s_ShaderCrashed)
	{
		if (NeedRecompile)
			kputs("OK", 1);

		setshader_int(0, -1, 0);

		if (!gthand)
		{
			unsigned int win98requiresme;
			//gmainthread = GetCurrentThread();
			ghevent[0] = CreateEvent(0, 0, 0, 0);
			ghevent[1] = CreateEvent(0, 0, 0, 0);
			ghevent[2] = CreateEvent(0, 0, 0, 0);
			gthand = (HANDLE)_beginthreadex(0, 4096, watchthread, (void*)0, 0, &win98requiresme);
		}

		SetEvent(ghevent[0]);
		SafeEvalFunc();
		SetEvent(ghevent[1]);

		if (WaitForSingleObject(ghevent[2], 1000) == WAIT_TIMEOUT)
		{
			s_ShaderStuck = 1;
			MessageBeep(16); //evil
			kputs("\nShader stuck! Now would be a good time to save & quit :/",1);

			// auto-restart on deadlock
			HANDLE hpipe;
			unsigned long u;
			char buf[1024];

			hpipe = CreateNamedPipe("\\\\.\\pipe\\txtbuf", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, sizeof(buf), sizeof(buf), 0, 0);

			if (hpipe != INVALID_HANDLE_VALUE)
			{
				PROCESS_INFORMATION pi;
				STARTUPINFO si;
				int scrolly, setsel0, setsel1;

				ZeroMemory(&si, sizeof(STARTUPINFO));
				si.cb = sizeof(STARTUPINFO);
				si.wShowWindow = SW_SHOW;

				scrolly = SendMessage(hWndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
				SendMessage(hWndEdit, EM_GETSEL, (unsigned)&setsel0, (unsigned)&setsel1);

				sprintf(buf, "%s \\\\.\\pipe\\txtbuf /scrolly=%d /setsel0=%d /setsel1=%d /savfil=%s", s_ExeFullPath, scrolly, setsel0, setsel1, s_SaveFilename);
				CreateProcess(0, buf, 0, 0, 1, CREATE_NEW_CONSOLE, 0, 0, &si, &pi);

				if (ConnectNamedPipe(hpipe, 0))
				{
					u = strlen(s_Text);

					if (!WriteFile(hpipe, s_Text, u, &u, 0))
					{
						kputs("WriteFile failed", 1);
						return;
					}

					FlushFileBuffers(hpipe);
					DisconnectNamedPipe(hpipe);
				}

				CloseHandle(hpipe);
				ExitProcess(0);
			}
		}

		if (showtimeout)
		{
			showtimeout = 0;
			kputs("timeout!", 1);
		}

		//FIXME:uninit:
		//   if (ghevent[2] != (HANDLE)-1) { CloseHandle(ghevent[2]); ghevent[2] = (HANDLE)-1; }
		//   if (ghevent[1] != (HANDLE)-1) { CloseHandle(ghevent[1]); ghevent[1] = (HANDLE)-1; }
		//   if (ghevent[0] != (HANDLE)-1) { CloseHandle(ghevent[0]); ghevent[0] = (HANDLE)-1; }
		//   also: close thread: gthand!
	}
	else // there was some error
	{
		if (NeedRecompile)
			kputs(kasm87err, 1);
	}

	g_DNumFrames++;
}

///////////////////////////////////////////////////////////////////////////////
extern void SaveFileDialog(HWND);
static int passasksave()
{
	if (!SendMessage(s_HWndEditor, EM_GETMODIFY, 0, 0))
		return 1;

	switch (MessageBox(s_HWndMain, "Save changes?", s_ProgramName, MB_YESNOCANCEL))
	{
		case IDYES: SaveFileDialog(s_HWndMain); return 1;
		case IDNO: return 1;
		case IDCANCEL: break;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void NewFile(int mode)
{
	if (!passasksave())
		return;

	if (mode == 0)
	{
		SetWindowText(s_HWndEditor, "");
	}
	else if (mode == 1)
	{
		SetWindowText(s_HWndEditor,
			"glquad(1);\r\n"
			"@v\r\n"
			"void main() { gl_Position = ftransform(); }\r\n"
			"@f\r\n"
			"void main() { gl_FragColor = gl_FragCoord*.001; }");
	}
	else if (mode == 2)
	{
		SetWindowText(s_HWndEditor,
			"// Host code (EVAL)\r\n"
			"if (numframes == 0)\r\n"
			"{\r\n"
			"\tglsettex(0,\"earth.jpg\");\r\n"
			"\tstatic env;\r\n"
			"\tenv = glGetUniformLoc(\"env\");\r\n"
			"}\r\n"
			"\r\n"
			"t = klock();\r\n"
			"glbindtexture(0);\r\n"
			"glUniform4f(env, cos(t/2), sin(t/2), 0, 0);\r\n"
			"glUniform4f(env+1, noise(t, 0.5) + 1, noise(t, 1.5) + 1, noise(t, 2.5) + 1, 1);\r\n"
			"glBegin(GL_QUADS);\r\n"
			"glTexCoord(0, 0); glVertex(-2, -1, -6);\r\n"
			"glTexCoord(1, 0); glVertex(+2, -1, -6);\r\n"
			"glTexCoord(1, 1); glVertex(+2, -1, -2);\r\n"
			"glTexCoord(0, 1); glVertex(-2, -1, -2);\r\n"
			"glEnd();\r\n"
			"printg(xres - 64, 0, 0xffffff, \"%.2f fps\", numframes/t);\r\n"
			"\r\n"
			"@v:vertex_shader //================================\r\n"
			"varying vec4 p, v, c, t;\r\n"
			"varying vec3 n;\r\n"
			"void main()\r\n"
			"{\r\n"
			"\tgl_Position = ftransform();\r\n"
			"\tp = gl_Position;\r\n"
			"\tv = gl_Vertex;\r\n"
			"\tn = gl_Normal;\r\n"
			"\tc = gl_Color;\r\n"
			"\tt = gl_MultiTexCoord0;\r\n"
			"}\r\n"
			"\r\n"
			"@f:fragment_shader //================================\r\n"
			"varying vec4 p, v, c, t;\r\n"
			"varying vec3 n;\r\n"
			"uniform sampler2D tex0;\r\n"
			"uniform vec4 env[2];\r\n"
			"void main()\r\n"
			"{\r\n"
			"\tgl_FragColor = texture2D(tex0, t.xy + env[0].xy)*env[1].rgba;\r\n"
			"}\r\n");
	}
	else if (mode == 3)
	{
		SetWindowText(s_HWndEditor,
			"// Host code (EVAL)\r\n"
			"if (numframes == 0)\r\n"
			"{\r\n"
			"   glsettex(0,\"earth.jpg\");\r\n"
			"}\r\n"
			"\r\n"
			"t = klock();\r\n"
			"glbindtexture(0);\r\n"
			"glProgramEnvParam(0,cos(t/2),sin(t/2),0,0);\r\n"
			"glProgramEnvParam(1,noise(t,0.5)+1,noise(t,1.5)+1,noise(t,2.5)+1,1);\r\n"
			"glBegin(GL_QUADS);\r\n"
			"glTexCoord(0,0); glVertex(-2,-1,-6);\r\n"
			"glTexCoord(1,0); glVertex(+2,-1,-6);\r\n"
			"glTexCoord(1,1); glVertex(+2,-1,-2);\r\n"
			"glTexCoord(0,1); glVertex(-2,-1,-2);\r\n"
			"glEnd();\r\n"
			"printg(xres-64,0,0xffffff,\"%.2f fps\",numframes/t);\r\n"
			"\r\n"
			"@v:vertex_shader //================================\r\n"
			"!!ARBvp1.0\r\n"
			"PARAM ModelViewProj[4] = {state.matrix.mvp};\r\n"
			"TEMP temp;\r\n"
			"DP4 temp.x, ModelViewProj[0], vertex.position;\r\n"
			"DP4 temp.y, ModelViewProj[1], vertex.position;\r\n"
			"DP4 temp.z, ModelViewProj[2], vertex.position;\r\n"
			"DP4 temp.w, ModelViewProj[3], vertex.position;\r\n"
			"MOV result.position, temp;\r\n"
			"MOV result.color, vertex.color;\r\n"
			"MOV result.texcoord[0], vertex.texcoord[0];\r\n"
			"END\r\n"
			"\r\n"
			"@f:fragment_shader //================================\r\n"
			"!!ARBfp1.0\r\n"
			"TEMP pos, col;\r\n"
			"ADD pos, fragment.texcoord[0], program.env[0]; #pan texcoord\r\n"
			"TEX col, pos, texture[0], 2D;\r\n"
			"MUL result.color.xyzw, col, program.env[1].xyzw; #scale color\r\n"
			"END");
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	SetFOV(90.0, g_RenderWidth, g_RenderHeight);

	g_CapTextSize = 512;
	s_DoRecompile = 3;
	s_SaveFilename[0] = 0;
	s_SaveFilenamePtr = 0;

	//SendMessage(s_HWndEditor, EM_SETMODIFY, 0, 0);
	SendMessage(s_HWndEditor, SCI_EMPTYUNDOBUFFER, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
static void LoadFile(char* filename, HWND hWndEdit)
{
	int i, j, k, ind[4], leng, fileformat;
	char* buf = 0;

	if (!passasksave())
		return;

	auto fp = fopen(filename, "rb");

	if (fp)
	{
		if (!_memicmp(filename, "\\\\.\\pipe\\", 9)) //load from pipe instead of file
		{
			leng = 0; i = 0; // For pipes, file size is not known in advance

			while (!ferror(fp))
			{
				k = fgetc(fp);

				if (k == EOF)
					break;

				j = i;
				i = (k == '\r' || k == '\n');

				if (i < j)
				{
					s_Text[leng] = 13;
					s_Text[leng+1] = 10;
					leng += 2;
				}

				if (i)
					continue;

				s_Text[leng] = k; leng++;
			}

			s_Text[leng] = 0;
			fileformat = 3;
		}
		else
		{
			char buf5[5];

			strcpy(s_SaveFilename, filename); s_SaveFilenamePtr = 0;

			// Autodetect file format... (0:65536 byte file with tons of 0's, 1:4 null-terminated strings)
			fseek(fp, 0, SEEK_END);
			leng = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			fseek(fp, leng - sizeof(buf5), SEEK_SET);
			fread(buf5, sizeof(buf5), 1, fp);
			fseek(fp, 0, SEEK_SET);

			for (i = sizeof(buf5) - 1; i >= 0; i--)
			{
				if (buf5[i] != 0)
					break;
			}

			if ((leng == 65536) && (i < 0))
				fileformat = 0; // Tigrou's original file format
			else if (i < sizeof(buf5)-1)
				fileformat = 1; // any ASCII 0's is binary
			else
				fileformat = 2;
		}

		switch (fileformat)
		{
			case 0:
				buf = (char*)malloc(65536);
				fread(buf, 1, 65536, fp); // vertex,fragment,eval
				sprintf(s_Text, "%s\r\n\r\n@v: //================================\r\n\r\n%s\r\n\r\n@f: //================================\r\n\r\n%s", &buf[32768], &buf[0], &buf[16384]);

				for (i = 0; s_Text[i]; i++)
				{
					if (s_Text[i] > 32)
						break;
				}

				if (s_Text[i] == '{') // Hack attempting to fix many scripts that lack () at beginning
				{
					memmove(&s_Text[2], s_Text, strlen(s_Text) + 1);
					s_Text[0] = '('; s_Text[1] = ')';
				}

				free(buf);
				break;

			case 1:
				buf = (char*)malloc(leng);
				fread(buf, 1, leng, fp);
				i = 0;
				j = strlen(&buf[i]) + 1; ind[0] = i; i += j;
				j = strlen(&buf[i]) + 1; ind[1] = i; i += j;
				j = strlen(&buf[i]) + 1; ind[2] = i; i += j;
				j = strlen(&buf[i]) + 1; ind[3] = i; i += j;
				sprintf(s_Text,"%s\r\n\r\n@v: //================================\r\n\r\n%s\r\n\r\n@f: //================================\r\n\r\n%s",&buf[ind[2]],&buf[ind[0]],&buf[ind[1]]);
				free(buf);
				break;

			case 2:
				fread(s_Text, 1, leng, fp);
				s_Text[leng] = 0;
				break;

			case 3: // same as format 2, but for pipe
				break;
		}

		fclose(fp);

		// Convert tabs to 3 spaces in-place
		/*
		j = 0;

		for (i = 0; text[i]; i++)
		{
			if (text[i] == '\t')
				j += 2;
		}

		j += i;

		if (j >= (textsiz - 1))
		{
			kputs("file too long", 1);
			MessageBeep(16); //evil
			return;
		}

		text[j] = 0;

		for(i--; i >= 0; i--)
		{
			if (text[i] == '\t')
			{
				j -= 3;
				text[j+2] = ' ';
				text[j+1] = ' ';
				text[j] = ' ';
				continue;
			}

			j--;
			text[j] = text[i];
		}
		*/

		SetWindowText(hWndEdit, s_Text);
	}

	//otext[0] = text[0]^1; otext[1] = 0; //force recompile and reset time

	glActiveTexture(GL_TEXTURE0);

	//qglBindTex(0.0);
	//ksetfov(90.0);
	glBindTexture(GL_TEXTURE_2D, 0);
	SetFOV(90.0, g_RenderWidth, g_RenderHeight);

	g_CapTextSize = 512;
	s_DoRecompile = 3;
	glLineWidth(1.0f);

	/*
	if (g_glfp[wglSwapIntervalEXT])
		((PFNWGLSWAPINTERVALEXTPROC)g_glfp[wglSwapIntervalEXT])(1);
	*/

	{
		char tbuf[512];
		sprintf(tbuf,"\nLoaded '%s'",filename);
		kputs(tbuf, 1);
	}

	SendMessage(hWndEdit, EM_SETMODIFY, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
static void LoadFileDialog(HWND lwnd)
{
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = lwnd;
	ofn.lpstrFilter = "Polydraw Shader Script (*.pss;*.bin)\0*.pss;*.bin\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "pss";

	if (GetOpenFileName(&ofn))
		LoadFile(ofn.lpstrFile,s_HWndEditor);

	s_ShiftKeyStatus = 0;
	memset(g_dkeystatus,0,sizeof(g_dkeystatus));
}

///////////////////////////////////////////////////////////////////////////////
static void SaveFile(char* filename)
{
	FILE* fp = fopen(filename, "wb");

	if (fp)
	{
		strcpy(s_SaveFilename, filename);
		s_SaveFilenamePtr = 0;

		fwrite(s_Text, 1, strlen(s_Text), fp);
		fclose(fp);
		MessageBeep(48);
		SendMessage(s_HWndEditor, EM_SETMODIFY, 0, 0);
	}

	{
		char tbuf[512];
		sprintf(tbuf,"\nSaved '%s'",filename);
		kputs(tbuf,1);
	}
}

///////////////////////////////////////////////////////////////////////////////
static void SaveFileDialog(HWND lwnd)
{
	OPENFILENAME ofn;
	char filnam[MAX_PATH] = "";

	strcpy(filnam, s_SaveFilename);

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = lwnd;
	ofn.lpstrFilter = "PolyDraw Shader Script (*.pss)\0*.pss\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filnam;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "pss";

	if (GetSaveFileName(&ofn))
		SaveFile(ofn.lpstrFile);

	s_ShiftKeyStatus = 0;
	memset(g_dkeystatus, 0, sizeof(g_dkeystatus));
}

///////////////////////////////////////////////////////////////////////////////
static void ResetWindows(int cmdshow);

static void updateshifts(LPARAM lParam, int mode)
{
	if (!mode)
	{
		switch (lParam&0x17f0000)
		{
		case 0x02a0000: s_ShiftKeyStatus &= ~(3<<16); break; //0x2a
		case 0x0360000: s_ShiftKeyStatus &= ~(3<<16); break; //0x36
		case 0x01d0000: s_ShiftKeyStatus &= ~(1<<18); break; //0x1d
		case 0x11d0000: s_ShiftKeyStatus &= ~(1<<19); break; //0x9d
		case 0x0380000: s_ShiftKeyStatus &= ~(1<<20); break; //0x38
		case 0x1380000: s_ShiftKeyStatus &= ~(1<<21); break; //0xb8
		}
	}
	else
	{
		switch (lParam&0x17f0000)
		{
		case 0x02a0000: s_ShiftKeyStatus |= (1<<16); break; //0x2a
		case 0x0360000: s_ShiftKeyStatus |= (1<<17); break; //0x36
		case 0x01d0000: s_ShiftKeyStatus |= (1<<18); break; //0x1d
		case 0x11d0000: s_ShiftKeyStatus |= (1<<19); break; //0x9d
		case 0x0380000: s_ShiftKeyStatus |= (1<<20); break; //0x38
		case 0x1380000: s_ShiftKeyStatus |= (1<<21); break; //0xb8
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
static void helpabout()
{
	char tbuf[1024];
	sprintf(tbuf,"PolyDraw, an Opengl scripting tool. Compiled: %s\r\n"
					 "\r\n"
					 "Get latest version here:\r\n"
					 "   http://advsys.net/ken/download.htm#polydraw\r\n"
					 "\r\n"
					 "Authors:\r\n"
					 "\r\n"
					 "   Ken Silverman (http://advsys.net/ken):\r\n"
					 "      EVAL compiler, GUI cleanup, fixes, enhancements\r\n"
					 "\r\n"
					 "   Tigrou (tigrou.ind@gmail.com):\r\n"
					 "      Original author & concept. His version here:\r\n"
					 "         http://pouet.net/prod.php?which=54245\r\n"
					 "         ftp://ftp.untergrund.net/users/ind/polydraw.zip\r\n"
					 ,__DATE__);
	MessageBox(s_HWndMain, tbuf, s_ProgramName, MB_OK);
}

///////////////////////////////////////////////////////////////////////////////
// Find&replace working info found here:
// ftp://ftp.microsoft.com/developr/drg/WWLive/broadcas/bookchap/nancy/code/cmndlg32/
//   Download:cmndlg32.c,cmndlg32.h,cmndlg32.rc,resource.h; link:user32,gdi32,comdlg32,comctl32
//
// NOTE: 3 hacks must be placed outside this block:
// 1. IsDialogMessage() near PeekMessage/GetMessage.
// 2. if (msg == uFindReplaceMsg) ... in WndProc of edit control.
// 3. Remember to add |ES_NOHIDESEL to 4th parm of edit control's CreateWindow().
///////////////////////////////////////////////////////////////////////////////
static HWND s_HWndFind = 0;
static unsigned int s_FindMsg = 0;
static FINDREPLACE s_FindReplaceInfo; // Must be global/static
static char s_FindText[256];    // Must be global/static and sizeof >= 80
static char s_ReplaceText[256]; // Must be global/static and sizeof >= 80
//static int gfind_inited = 0;

///////////////////////////////////////////////////////////////////////////////
static int findreplace_process(LPFINDREPLACE lpfr)
{
	int i, j, /* textleng,*/ findleng, repleng, findsel0, findsel1;

	if (!lpfr)
		return 0;

	HWND hwnd = s_HWndEditor; //lpfr->hwndOwner;

	if (lpfr->Flags & FR_DIALOGTERM)
	{
		SetFocus(hwnd);
		return 0;
	}

	if (!(lpfr->Flags & (FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL)))
		return 0;

//	GetWindowText(hwnd, s_TText, s_TextSize);
//	textleng = strlen(s_TText);
//	SendMessage(hwnd, SCI_SETTARGETSTART, 0, 0);
//	SendMessage(hwnd, SCI_SETTARGETEND, textleng, 0);

	//int t5, t6, t7;
	//t5 = SendMessage(hwnd, SCI_GETTARGETSTART, 0, 0);
	//t6 = SendMessage(hwnd, SCI_GETTARGETEND, 0, 0);

	findleng = strlen(lpfr->lpstrFindWhat);
	int t7 = SendMessage(hwnd, SCI_SEARCHINTARGET, findleng, (LPARAM)lpfr->lpstrFindWhat);

	SendMessage(hwnd, SCI_SETSEL, t7, t7 + findleng);

	// reset target
	GetWindowText(hwnd, s_TText, s_TextSize);
	auto textleng = strlen(s_TText);
	//auto currentPos = SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	//SendMessage(hwnd, SCI_SETTARGETSTART, currentPos, 0);
	SendMessage(hwnd, SCI_SETTARGETSTART, t7 + findleng, 0);
	SendMessage(hwnd, SCI_SETTARGETEND, textleng, 0);

	return 0;

	/*
	if (lpfr->Flags & (FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL))
	{
		GetWindowText(hwnd, s_TText, s_TextSize);
		textleng = strlen(s_TText);
		findleng = strlen(lpfr->lpstrFindWhat);
		repleng = strlen(lpfr->lpstrReplaceWith);

		SendMessage(hwnd, EM_GETSEL, (WPARAM)&findsel0, (LPARAM)&findsel1);

		if (lpfr->Flags & FR_REPLACEALL)
		{
			for (i = (textleng - findleng); i >= 0; i--)
			{
				if (gfind_mymemcmp(s_TText, i, lpfr->lpstrFindWhat, findleng, lpfr->Flags))
					continue;

				SendMessage(hwnd, EM_SETSEL, i, i + findleng);
				SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)lpfr->lpstrReplaceWith);
				i += 1 - findleng;

				if (findsel0 < i)
					findsel0 += repleng-findleng;

				if (findsel1 < i)
					findsel1 += repleng-findleng;
			}

			SendMessage(hwnd, EM_SETSEL, findsel0, findsel1);
			SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
			SendMessage(g_HWndFind, WM_CLOSE, 0, 0);
			MessageBeep(0);
			return 0;
		}

		i = findsel0;

		if ((lpfr->Flags & FR_REPLACE) && (findsel1 - findsel0 == findleng) && (findsel0 <= textleng - findleng))
		{
			if (!gfind_mymemcmp(s_TText, i, lpfr->lpstrFindWhat, findleng, lpfr->Flags))
			{
				SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)lpfr->lpstrReplaceWith);
				GetWindowText(hwnd, s_TText, textleng); textleng = strlen(s_TText);
			}
		}

		for (j = 0; j < textleng; j++)
		{
			if (lpfr->Flags & FR_DOWN)
			{
				i++;

				if (i >= textleng)
					i -= textleng;
			}
			else
			{
				i--;

				if (i < 0)
					i += textleng;
			}

			if (i <= (textleng - findleng) && !gfind_mymemcmp(s_TText, i, lpfr->lpstrFindWhat, findleng, lpfr->Flags))
				break;
		}

		if (j >= textleng)
		{
			SendMessage(hwnd, EM_SETSEL, findsel0, findsel0);
			MessageBeep(0);
			return 0;
		}

		SendMessage(hwnd, EM_SETSEL, i, i + findleng);
		SendMessage(hwnd, EM_SCROLLCARET, 0, 0);

		{ // Move find/replace dialog out of way if covering highlighted text
			RECT rfind, redit;
			POINT p0, pchar;

			GetWindowRect(g_HWndFind,&rfind);

			p0.x = p0.y = 0;
			ClientToScreen(hwnd, &p0);
			SendMessage(hwnd, EM_GETRECT, 0, (LPARAM)&redit);
			j = SendMessage(hwnd, EM_POSFROMCHAR, i, 0);
			pchar.x = LOWORD(j);
			pchar.y = HIWORD(j);
			j = ((labs(s_popts.fontheight)*20) >> 4); // Estimated character height

			if (p0.y+pchar.y+j >= rfind.top && p0.y+pchar.y < rfind.bottom)
			{
				if ((p0.y + pchar.y + (j >> 1)) * 2 < rfind.top + rfind.bottom)
					MoveWindow(g_HWndFind, rfind.left, p0.y + pchar.y + j, rfind.right - rfind.left, rfind.bottom - rfind.top, 1);
				else
					MoveWindow(g_HWndFind, rfind.left, p0.y + pchar.y + rfind.top - rfind.bottom, rfind.right - rfind.left, rfind.bottom - rfind.top, 1);
			}
		}
	}
	*/

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void findreplace(int isreplace)
{
	if (s_FindMsg == 0) // register message on first call
	{
		s_FindMsg = RegisterWindowMessage(FINDMSGSTRING);
		s_FindText[0] = 0;
		s_ReplaceText[0] = 0;

		s_FindReplaceInfo.lStructSize = sizeof(s_FindReplaceInfo);
		s_FindReplaceInfo.hwndOwner = s_HWndMain;
		s_FindReplaceInfo.hInstance = s_HInst;
		s_FindReplaceInfo.Flags = FR_DOWN;
		s_FindReplaceInfo.lpstrFindWhat = s_FindText;
		s_FindReplaceInfo.lpstrReplaceWith = s_ReplaceText;
		s_FindReplaceInfo.wFindWhatLen = sizeof(s_FindText);
		s_FindReplaceInfo.wReplaceWithLen = sizeof(s_ReplaceText);
		s_FindReplaceInfo.lCustData = 0;
		s_FindReplaceInfo.lpfnHook = 0;
		s_FindReplaceInfo.lpTemplateName = 0;
	}

	//|FR_DOWN     |FR_NOUPDOWN   |FR_HIDEUPDOWN
	//|FR_WHOLEWORD|FR_NOWHOLEWORD|FR_HIDEWHOLEWORD
	//|FR_MATCHCASE|FR_NOMATCHCASE|FR_HIDEMATCHCASE
	//|FR_FINDNEXT|FR_REPLACE|FR_REPLACEALL
	//|FR_DIALOGTERM|FR_SHOWHELP|FR_ENABLEHOOK
	//|FR_ENABLETEMPLATE|FR_ENABLETEMPLATEHANDLE
	s_FindReplaceInfo.Flags &= ~FR_DIALOGTERM; // Need this to prevent bad things on 2nd call

	// create either a find or a replace dialog
	if (!isreplace)
		s_HWndFind = FindText(&s_FindReplaceInfo);
	else
		s_HWndFind = ReplaceText(&s_FindReplaceInfo);

	// setup for initial find/replace
	GetWindowText(s_HWndEditor, s_TText, s_TextSize);
	auto textleng = strlen(s_TText);
	//SendMessage(hwnd, SCI_SETTARGETSTART, 0, 0);

	auto currentPos = SendMessage(s_HWndEditor, SCI_GETCURRENTPOS, 0, 0);
	SendMessage(s_HWndEditor, SCI_SETTARGETSTART, currentPos, 0);

	SendMessage(s_HWndEditor, SCI_SETTARGETEND, textleng, 0);
}

///////////////////////////////////////////////////////////////////////////////
static void findnext(int isnext)
{
	if (s_FindText[0] == '\0' || s_FindMsg == 0)
		return;

	s_FindReplaceInfo.Flags &= ~(FR_DIALOGTERM | FR_REPLACE | FR_REPLACEALL);
	s_FindReplaceInfo.Flags |= FR_FINDNEXT;

	if (isnext)
		s_FindReplaceInfo.Flags |= FR_DOWN;
	else
		s_FindReplaceInfo.Flags &= ~FR_DOWN;

	findreplace_process(&s_FindReplaceInfo);
}

/// Scintilla Colors structure
struct ScintillaStyleColor
{
	int	item;
	COLORREF fg;
	COLORREF bg;

	ScintillaStyleColor(int item, COLORREF fg, COLORREF bg = RGB(0x1e, 0x1e, 0x1e))
		: item(item), fg(fg), bg(bg)
	{ }
};

// A few basic colors
const COLORREF g_Black = RGB(0, 0, 0);
const COLORREF g_White = RGB(0xff, 0xff, 0xff);
const COLORREF g_Gray = RGB(0x1e, 0x1e, 0x1e);
const COLORREF g_LightGray = RGB(0x2a, 0x2a, 0x2a);
const COLORREF g_Green = RGB(0, 0xff, 0);
const COLORREF g_Red = RGB(0xff, 0, 0);
const COLORREF g_Blue = RGB(0, 0, 0xff);
const COLORREF g_Yellow = RGB(0xff, 0xff, 0);
const COLORREF g_Magenta = RGB(0xff, 0, 0xff);
const COLORREF g_Cyan = RGB(0, 0xff, 0xff);

// C++ keywords
// FIXME: we need keywords specific to GLSL and EVAL here...
static const char g_cppKeyWords[] =
// Standard
"asm auto bool break case catch char class const "
"const_cast continue default delete do double "
"dynamic_cast else enum explicit extern false finally "
"float for friend goto if inline int long mutable "
"namespace new operator private protected public "
"register reinterpret_cast register return short signed "
"sizeof static static_cast struct switch template "
"this throw true try typedef typeid typename "
"union unsigned using virtual void volatile "
"wchar_t while "
// a few more
"override final offsetof using "

// Extended
"__asm __asume __based __box __cdecl __declspec "
"__delegate delegate depreciated dllexport dllimport "
"event __event __except __fastcall __finally __forceinline "
"__int8 __int16 __int32 __int64 __int128 __interface "
"interface __leave naked noinline __noop noreturn "
"nothrow novtable nullptr safecast __stdcall "
"__try __except __finally __unaligned uuid __uuidof "
"__virtual_inheritance";

/// Default color scheme
static ScintillaStyleColor g_RGBSyntaxCpp[] =
{
	{ SCE_C_DEFAULT, RGB(0xC8, 0xC8, 0xC8) },

	{ SCE_C_COMMENT, RGB(0x60, 0x8B, 0x4E) },
	{ SCE_C_COMMENTLINE, RGB(0x60, 0x8B, 0x4E) },
	{ SCE_C_COMMENTDOC, RGB(0x60, 0x8B, 0x4E) },
	{ SCE_C_COMMENTLINEDOC, RGB(0x60, 0x8B, 0x4E) },
	{ SCE_C_COMMENTDOCKEYWORD, RGB(0x60, 0x8B, 0x4E) },
	{ SCE_C_COMMENTDOCKEYWORDERROR, RGB(0x60, 0x8B, 0x4E) },

	{ SCE_C_NUMBER, RGB(0xB5, 0xCE, 0xA8) },
	{ SCE_C_STRING, RGB(0xD6, 0x9D, 0x85) },
	{ SCE_C_CHARACTER, RGB(0xD6, 0x9D, 0x85) },
	{ SCE_C_UUID, g_Cyan },
	{ SCE_C_OPERATOR, RGB(0x9B, 0x9B, 0x9B) },
	{ SCE_C_VERBATIM, RGB(0xB5, 0xCE, 0xA8) },
	{ SCE_C_REGEX, RGB(0xD6, 0x9D, 0x85) },
	{ SCE_C_PREPROCESSOR, RGB(0x9B, 0x9B, 0x9B) },
	{ SCE_C_WORD, RGB(0x4E, 0x9C, 0xD6) },

	{ -1, 0 }
};

///////////////////////////////////////////////////////////////////////////////
void SetAStyle(HWND hWnd, int style, COLORREF fore, COLORREF back = g_Gray, int size = -1, const char* face = nullptr)
{
	SendMessage(hWnd, SCI_STYLESETFORE, style, fore);
	SendMessage(hWnd, SCI_STYLESETBACK, style, back);

	if (size >= 1)
		SendMessage(hWnd, SCI_STYLESETSIZE, style, size);

	if (face)
		SendMessage(hWnd, SCI_STYLESETFONT, style, (LPARAM)face);
}

///////////////////////////////////////////////////////////////////////////////
static void ResetWindows(int cmdshow)
{
	static int ooglxres = 0;
	static int ooglyres = 0;
	//RECT r;
	int i, guiflags, x0[3], y0[3], x1[3], y1[3];

	i = ((labs(s_popts.fontheight) * 20) >> 4);

	if (!s_HWndMain)
	{
		RECT rw;

		SystemParametersInfo(SPI_GETWORKAREA, 0, &rw, 0);
		int x = ((rw.right - rw.left - s_xres) >> 1) + rw.left;
		int y = ((rw.bottom - rw.top - s_yres) >> 1) + rw.top;
		s_HWndMain = CreateWindow("PolyDraw", s_ProgramName,
			WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
			x, y, s_xres, s_yres, 0, 0, s_HInst, 0); //|WS_VISIBLE|WS_POPUPWINDOW|WS_CAPTION
		ShowWindow(s_HWndMain, cmdshow);
	}

	guiflags = WS_VISIBLE | WS_CHILD | WS_VSCROLL; //|WS_HSCROLL|WS_CAPTION|WS_SIZEBOX|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU;
	guiflags |= ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL;

	if (!s_popts.fullscreen)
	{
		g_RenderWidth = double((s_xres >> 1) & ~3);
		g_RenderHeight = double((int)(g_RenderWidth * 3) >> 2);
		x1[0] = (int)g_RenderWidth;
		y1[0] = (int)g_RenderHeight;
		x1[1] = (int)g_RenderWidth;
		y1[1] = s_yres - (int)g_RenderHeight;
		x1[2] = s_xres - (int)g_RenderWidth;
		y1[2] = s_yres;

		if (!(s_popts.rendcorn & 1))
		{
			x0[0] = 0;
			x0[1] = 0;
			x0[2] = (int)g_RenderWidth;
		}
		else
		{
			x0[0] = s_xres - (int)g_RenderWidth;
			x0[1] = s_xres - (int)g_RenderWidth;
			x0[2] = 0;
		}

		if (!(s_popts.rendcorn & 2))
		{
			y0[0] = 0;
			y0[1] = (int)g_RenderHeight;
			y0[2] = 0;
		}
		else
		{
			y0[0] = s_yres - (int)g_RenderHeight;
			y0[1] = 0;
			y0[2] = 0;
		}
	}
	else
	{
		g_RenderWidth = (double)s_xres;
		g_RenderHeight = (double)s_yres;
		x0[0] = 0;
		y0[0] = 0;
		x1[0] = (int)g_RenderWidth;
		y1[0] = (int)g_RenderHeight;
		x0[1] = 0;
		y0[1] = (int)g_RenderHeight;
		x1[1] = 0;
		y1[1] = 0;
		x0[2] = (int)g_RenderWidth;
		y0[2] = 0;
		x1[2] = 0;
		y1[2] = 0;
	}

	if (s_HWndRender)
		MoveWindow(s_HWndRender, x0[0], y0[0], x1[0], y1[0], 1);
	else
		s_HWndRender = CreateWindowEx(0, "PolyDraw", "Render", WS_VISIBLE | WS_CHILD, x0[0], y0[0], x1[0], y1[0], s_HWndMain, (HMENU)100, s_HInst, 0);

	if (s_HWndConsole)
		MoveWindow(s_HWndConsole, x0[1], y0[1], x1[1], y1[1], 1);
	else
		s_HWndConsole = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "Console", guiflags | ES_READONLY | WS_HSCROLL, x0[1], y0[1], x1[1], y1[1], s_HWndMain, (HMENU)101, s_HInst, 0);

	if (s_HWndEditor)
		MoveWindow(s_HWndEditor, x0[2], y0[2], x1[2], y1[2], 1);
	else
	{
		s_HWndEditor = CreateWindowEx(WS_EX_WINDOWEDGE, "Scintilla", "Script",
			guiflags | ES_NOHIDESEL,
			x0[3], y0[3], x1[3], y1[3],
			s_HWndMain, (HMENU)102, s_HInst, 0);

		// CPP lexer
		SendMessage(s_HWndEditor, SCI_SETLEXER, SCLEX_CPP, 0);

		// Set number of style bits to use
		SendMessage(s_HWndEditor, SCI_SETSTYLEBITS, 5, 0);

		// Set tab width
		SendMessage(s_HWndEditor, SCI_SETTABWIDTH, 4, 0);

		// Use CPP keywords
		SendMessage(s_HWndEditor, SCI_SETKEYWORDS, 0, (LPARAM)g_cppKeyWords);

		// Set up the global default style. These attributes are used wherever no explicit choices are made.
		SetAStyle(s_HWndEditor, STYLE_DEFAULT, g_White, g_Gray, 10, "Courier New");

		// Set caret foreground color
		SendMessage(s_HWndEditor, SCI_SETCARETFORE, g_White, 0);

		// Set all styles
		SendMessage(s_HWndEditor, SCI_STYLECLEARALL, 0, 0);

		// Set selection color
		SendMessage(s_HWndEditor, SCI_SETSELBACK, TRUE, RGB(0, 0, 255));

		// Set syntax colors
		for (int i = 0; g_RGBSyntaxCpp[i].item != -1; i++)
			SetAStyle(s_HWndEditor, g_RGBSyntaxCpp[i].item, g_RGBSyntaxCpp[i].fg, g_RGBSyntaxCpp[i].bg);

		// Set margin widths
		SendMessage(s_HWndEditor, SCI_SETMARGINWIDTHN, 0, 40);
		SendMessage(s_HWndEditor, SCI_SETMARGINWIDTHN, 1, 10);
		//SendMessage(hWndEdit, SCI_STYLESETBACK, style, RGB(0, 0, 0));

		// Set line numbers in left margin (0), symbols in next (1)
		SendMessage(s_HWndEditor, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER); // line numbers
		SendMessage(s_HWndEditor, SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL); // symbols

		// Set FG/BG colors for margins
		SendMessage(s_HWndEditor, SCI_STYLESETFORE, STYLE_LINENUMBER, g_Green);
		SendMessage(s_HWndEditor, SCI_STYLESETBACK, STYLE_LINENUMBER, g_LightGray);
	}

	if (ooglxres != g_RenderWidth || ooglyres != g_RenderHeight)
	{
		//dorecompile = 1;
		//QueryPerformanceCounter((LARGE_INTEGER *)&qtim0); dnumframes = 0.0; //WinXP/balls.pss needs this!
	}
}

///////////////////////////////////////////////////////////////////////////////
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT p0, p1;
	int i;

	// FIXME: very hacky, needs work
	if (msg == s_FindMsg)
	{
		LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;

		if (lpfr->Flags & FR_DIALOGTERM) // dialog closed
		{
			s_HWndFind = nullptr;
		}
		else if (lpfr->Flags & FR_FINDNEXT)
		{
			if (lpfr->Flags & FR_DOWN) // searching down
			{
				findnext(1);
			}
			else // searching up
			{
				findnext(0);
			}
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:
		if (fmod(g_dbstatus, 2.0) < 1)
			g_dbstatus += 1;

		p0.x = p0.y = 0;
		ClientToScreen(s_HWndRender, &p0);
		GetCursorPos(&p1);

		if (((unsigned)(p1.x - p0.x) < (unsigned)g_RenderWidth) && ((unsigned)(p1.y - p0.y) < (unsigned)g_RenderHeight))
			SetFocus(s_HWndMain);

		break;

	case WM_LBUTTONUP:   if (fmod(g_dbstatus, 2.0) >= 1) g_dbstatus -= 1; break;
	case WM_RBUTTONDOWN: if (fmod(g_dbstatus, 4.0) <  2) g_dbstatus += 2; break;
	case WM_RBUTTONUP:   if (fmod(g_dbstatus, 4.0) >= 2) g_dbstatus -= 2; break;
	case WM_MBUTTONDOWN: if (fmod(g_dbstatus, 8.0) <  4) g_dbstatus += 4; break;
	case WM_MBUTTONUP:   if (fmod(g_dbstatus, 8.0) >= 4) g_dbstatus -= 4; break;

	case WM_KEYUP:
		updateshifts(lParam, 0);
		i = ((lParam >> 16) & 127) + ((lParam >> 17) & 128);

		if (g_dkeystatus[i] != 0.0)
			g_dkeystatus[i] = 0.0;

		break;

	case WM_KEYDOWN:
		updateshifts(lParam, 1);
		i = ((lParam >> 16) & 127) + ((lParam >> 17) & 128);
		//if (i == 1 && s_mehax) PostQuitMessage(0);
		if (g_dkeystatus[i] == 0.0) g_dkeystatus[i] = 1.0;
		if ((wParam & 255) == VK_F1) { helpabout(); return(0); }
		if ((wParam & 255) == VK_F3) { findnext((s_ShiftKeyStatus & 0x30000) == 0); return(0); }
		break;

	case WM_SYSCHAR:
		if ((wParam&255) == VK_RETURN)
		{
			s_popts.fullscreen = !s_popts.fullscreen;
			CheckMenuItem(s_HMenu, MENU_FULLSCREEN, s_popts.fullscreen*MF_CHECKED);
			ResetWindows(SW_NORMAL);
			return 0;
		}

		break;

	case WM_CHAR:
		if ((wParam & 255) == 10) { s_DoRecompile = 3; return 0; } // Ctrl+Enter
		if ((wParam & 255) == 0x0c) { LoadFileDialog(s_HWndMain); return 0; } // Ctrl+L
		if ((wParam & 255) == 0x13) { if (s_SaveFilename[0]) SaveFile(s_SaveFilename); else SaveFileDialog(s_HWndMain); return 0; } // Ctrl+S
		if ((wParam & 255) == 0x06) { findreplace(0); s_ShiftKeyStatus = 0; return 0; } // Ctrl+F
		if ((wParam & 255) == 0x12) { findreplace(1); s_ShiftKeyStatus = 0; return 0; } // Ctrl+R
		break;

	case WM_SIZE:
		if (hWnd != s_HWndMain) break;
		if ((wParam == SIZE_MAXHIDE) || (wParam == SIZE_MINIMIZED)) { s_ActiveApp = 0; break; }
		s_ActiveApp = 1;
		s_xres = LOWORD(lParam);
		s_yres = HIWORD(lParam);
		if ((s_oxres != s_xres) || (s_oyres != s_yres)) { s_oxres = s_xres; s_oyres = s_yres; ResetWindows(SW_NORMAL); }
		break;

	case WM_ACTIVATEAPP:
		s_ActiveApp = (BOOL)wParam;
		s_ShiftKeyStatus = 0;
		break;

#if 0
	case WM_CTLCOLOREDIT:
		SetTextColor(wParam,0xc0c0c0);
		SetBkColor(wParam,0x404040);
		return(GetStockObject(DKGRAY_BRUSH));
	case WM_CTLCOLORSTATIC:
		SetTextColor(wParam,0xc0c0c0);
		SetBkColor(wParam,0x404040);
		return(GetStockObject(DKGRAY_BRUSH));
#endif

	case WM_CLOSE:
		if (!passasksave())
			return 0;

		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) // process menu
		{
		case MENU_FILENEW+0: case MENU_FILENEW+1: case MENU_FILENEW+2: case MENU_FILENEW+3:
			NewFile(LOWORD(wParam) - MENU_FILENEW);
			break;
		case MENU_FILEOPEN:    LoadFileDialog(hWnd); break;
		case MENU_FILESAVE:    if (s_SaveFilename[0]) { SaveFile(s_SaveFilename); break; } //no break intentional
		case MENU_FILESAVEAS:  SaveFileDialog(hWnd); break;
		case MENU_FILEEXIT:    if (passasksave()) { PostQuitMessage(0); } break;
		case MENU_EDITFIND:    /*findreplace(s_HWndEditor, 0);*/ findreplace(0); break;
		case MENU_EDITFINDNEXT: findnext(1); break;
		case MENU_EDITFINDPREV: findnext(0); break;
		case MENU_EDITREPLACE: findreplace(1); break;
		case MENU_COMPCONTENT: s_popts.compctrlent ^= 1; if (!s_popts.compctrlent) s_DoRecompile = 3; CheckMenuItem(s_HMenu,MENU_COMPCONTENT,s_popts.compctrlent*MF_CHECKED); break;
		case MENU_EVALHIGHLIGHT:
			{
				int i0, i1;
				SendMessage(s_HWndEditor, EM_GETSEL, (unsigned)&i0, (unsigned)&i1);

				if (i0 < i1)
				{
					GetWindowText(s_HWndEditor,s_TText,s_TextSize);

					if (eval_highlight(&s_TText[i0], i1 - i0))
						MessageBeep(64);
					else
						MessageBeep(16);
				}
			}

			break;

		case MENU_RENDPLC+0: case MENU_RENDPLC+1: case MENU_RENDPLC+2: case MENU_RENDPLC+3:
			s_popts.rendcorn = LOWORD(wParam) - MENU_RENDPLC;
			s_popts.fullscreen = 0;

			for (i = 0; i < 4; i++)
				CheckMenuItem(s_HMenu, MENU_RENDPLC + i, (LOWORD(wParam) == MENU_RENDPLC + i)*MF_CHECKED);

			CheckMenuItem(s_HMenu, MENU_FULLSCREEN, s_popts.fullscreen*MF_CHECKED);
			ResetWindows(SW_NORMAL);
			break;

		case MENU_FULLSCREEN:
			s_popts.fullscreen = !s_popts.fullscreen;
			CheckMenuItem(s_HMenu, MENU_FULLSCREEN, s_popts.fullscreen*MF_CHECKED);
			ResetWindows(SW_NORMAL);
			break;

		case MENU_CLEARBUFFER:
			s_popts.clearbuffer = !s_popts.clearbuffer;
			CheckMenuItem(s_HMenu, MENU_CLEARBUFFER, s_popts.clearbuffer*MF_CHECKED);
			ResetWindows(SW_NORMAL);
			break;

		case MENU_FONT:
			{
				CHOOSEFONT cf;
				static LOGFONT lf;

				memset(&cf, 0, sizeof(cf));
				cf.lStructSize = sizeof(cf);
				cf.hwndOwner = hWnd;
				cf.lpLogFont = &lf;
				lf.lfHeight = s_popts.fontheight;
				lf.lfWidth = s_popts.fontwidth;
				strcpy(lf.lfFaceName,s_popts.fontname);
				cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_INITTOLOGFONTSTRUCT | CF_NOSTYLESEL;

				if (ChooseFont(&cf))
				{
					if (lf.lfFaceName[0])
					{
						if (s_HFont)
							DeleteObject(s_HFont);

						s_popts.fontheight = lf.lfHeight;
						s_popts.fontwidth = lf.lfWidth;
						strcpy(s_popts.fontname, lf.lfFaceName);

						s_popts.sepchar = '-'; //Many XP fonts do not have solid hyphen char :/
						//     if (!stricmp(popts.fontname,"Consolas"      )) popts.sepchar = 6; //also 151
						//else if (!stricmp(popts.fontname,"Courier"       )) popts.sepchar = 6;
						//else if (!stricmp(popts.fontname,"Courier New"   )) popts.sepchar = 151; //also 6
						//else if (!stricmp(popts.fontname,"Fixedsys"      )) popts.sepchar = 6;
						//else if (!stricmp(popts.fontname,"Lucida Console")) popts.sepchar = 6; //also 151
						//else if (!stricmp(popts.fontname,"Terminal"      )) popts.sepchar = 196;
						//else                                                popts.sepchar = '-';

						s_HFont = CreateFont(s_popts.fontheight, s_popts.fontwidth, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, s_popts.fontname);

						SendMessage(s_HWndConsole, WM_SETFONT, (WPARAM)s_HFont, 0);
						ShowWindow(s_HWndConsole, SW_HIDE);
						UpdateWindow(s_HWndConsole);
						ShowWindow(s_HWndConsole, SW_SHOWNORMAL);
						SendMessage(s_HWndEditor, WM_SETFONT, (WPARAM)s_HFont, 0);
						ShowWindow(s_HWndEditor, SW_HIDE);
						UpdateWindow(s_HWndEditor);
						ShowWindow(s_HWndEditor, SW_SHOWNORMAL);
						ResetWindows(SW_NORMAL);
					}
				}
			}

			break;

		case MENU_HELPABOUT:
			helpabout();
			break;
		}

		switch (HIWORD(wParam))
		{
			//case EN_CHANGE: //break; //Edit control's contents will change
			//case EN_VSCROLL: updatelines(0); break; //doesn't get triggered by mouse dragging; must use WM_PAINT in nhWndEdit instead
			case EN_UPDATE: // Edit control's contents have changed
				//if ((HWND)lParam == hWndEdit)
				//	updatelines(1); //text changed in edit window (would be cheaper than calling strcmp :P)

				break;

			default:
				break;
		}

		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
static void EnableOpenGL(HWND hWnd, HDC* hDC, HGLRC* hRC)
{
	PIXELFORMATDESCRIPTOR pfd;
	int format;

	*hDC = GetDC(hWnd);

	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;
	format = ChoosePixelFormat(*hDC, &pfd);
	SetPixelFormat(*hDC, format, &pfd);

	*hRC = wglCreateContext(*hDC);
	wglMakeCurrent(*hDC,*hRC);
}

///////////////////////////////////////////////////////////////////////////////
static void DisableOpenGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
	wglMakeCurrent(0, 0);
	wglDeleteContext(hRC);
	ReleaseDC(hWnd, hDC);
}

///////////////////////////////////////////////////////////////////////////////
static int CheckGLExtension(char* extnam)
{
	const char* st = (const char*) glGetString(GL_EXTENSIONS);
	st = strstr(st, extnam);

	if (!st)
		return 0;

	return (st[strlen(extnam)] <= 32);
}

///////////////////////////////////////////////////////////////////////////////
static int cmdline2arg(char* cmdline, char** argv)
{
	int i, j, k, inquote, argc;

	// Convert Windows command line into ANSI 'C' command line...
	argv[0] = "exe";
	argc = 1;
	j = inquote = 0;

	for (i = 0; cmdline[i] != '\0'; i++)
	{
		k = ((cmdline[i] != ' ' && cmdline[i] != '\t') || inquote);

		if (cmdline[i] == '\"')
			inquote ^= 1;

		if (j < k)
		{
			argv[argc++] = &cmdline[i + inquote];
			j = inquote + 1;
			continue;
		}

		if (j && !k)
		{
			if (j == 2 && cmdline[i-1] == '\"')
				cmdline[i-1] = 0;

			cmdline[i] = 0; j = 0;
		}
	}

	if (j == 2 && cmdline[i-1] == '\"')
		cmdline[i-1] = 0;

	argv[argc] = 0;
	return argc;
}

///////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if (LoadLibrary("SciLexer.dll") == nullptr)
	{
		MessageBox(s_HWndMain, "Loading Scintilla DLL failed.  Make sure SciLexer.dll is available.", s_ProgramName, MB_OK);
		return 1;
	}

	WNDCLASS wc;
	MSG msg;
	HDC hDC;
	HGLRC hRC;

	__int64 q = 0I64, qlast = 0I64;
	int qnum = 0;
	int i, j, k, z, argc, argfilindex = -1; //, setsel0 = -1, setsel1 = -1, scrolly = -1;
	char buf[1024];
	char* argv[MAX_PATH >> 1];
	//char* savfilnam = 0;

	//osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	//GetVersionEx(&osvi);

	{
		RECT rw;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rw, 0);
		s_xres = (((rw.right - rw.left) * 3) >> 2);
		s_yres = (((rw.bottom - rw.top) * 3) >> 2);
		nCmdShow = SW_MAXIMIZE;

		GetModuleFileName(0, s_ExeFullPath, sizeof(s_ExeFullPath));

		for (i = 0, j = -1; s_ExeFullPath[i]; i++)
		{
			if (s_ExeFullPath[i] == '\\' || s_ExeFullPath[i] == '/')
				j = i;
		}

		strcpy(s_ExeDirOnly, s_ExeFullPath);
		s_ExeDirOnly[j + 1] = '\0';
		sprintf(s_IniFilename, "%spolydraw.ini", s_ExeDirOnly);
		LoadIni();

		kzaddstack(s_ExeDirOnly);
	}

	argc = cmdline2arg(lpCmdLine, argv);

	for (i = argc - 1; i > 0; i--)
	{
		if (argv[i][0] != '/' && argv[i][0] != '-')
		{
			argfilindex = i;
			continue;
		}

		/*
		if (!_memicmp(&argv[i][1], "setsel0",7)) // hack for seamless pipe restart
		{
			setsel0 = atol(&argv[i][9]);
			continue;
		}

		if (!_memicmp(&argv[i][1], "setsel1",7)) // hack for seamless pipe restart
		{
			setsel1 = atol(&argv[i][9]);
			continue;
		}

		if (!_memicmp(&argv[i][1], "scrolly",7)) // hack for seamless pipe restart
		{
			scrolly = atol(&argv[i][9]);
			continue;
		}

		if (!_memicmp(&argv[i][1], "savfil",6)) // hack for seamless pipe restart
		{
			savfilnam = &argv[i][8];
			continue;
		}
		*/

		if (argv[i][1] >= '0' && argv[i][1] <= '9')
		{
			k = 0;
			z = 0;

			for (j = 1; ; j++)
			{
				if (argv[i][j] >= '0' && argv[i][j] <= '9')
				{
					k = (k * 10 + argv[i][j] - '0');
					continue;
				}

				switch (z)
				{
					case 0: s_xres = k; nCmdShow = SW_NORMAL; break;
					case 1: s_yres = k; break;
				}

				if (!argv[i][j])
					break;

				z++;

				if (z > 2)
					break;

				k = 0;
			}
		}
	}

	s_HInst = hInstance;

	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = "PolyDraw";
	RegisterClass(&wc);

	//if (osvi.dwPlatformId < 2) textsiz = 32768;/*<NT*/ else textsiz = 65536;
	s_TextSize = 65536;
	s_Text = (char*)malloc(s_TextSize); if (!s_Text) { MessageBox(s_HWndMain,"malloc failed",s_ProgramName,MB_OK); return 1; }
	s_OText = (char*)malloc(s_TextSize); if (!s_OText) { MessageBox(s_HWndMain, "malloc failed", s_ProgramName, MB_OK); return 1; }
	s_TText = (char*)malloc(s_TextSize); if (!s_TText) { MessageBox(s_HWndMain, "malloc failed", s_ProgramName, MB_OK); return 1; }
	s_Line = (char*)malloc(s_TextSize); if (!s_Line) { MessageBox(s_HWndMain, "malloc failed", s_ProgramName, MB_OK); return 1; }
	s_BadLineBits = (char*)malloc((s_TextSize + 7) >> 3); if (!s_BadLineBits) { MessageBox(s_HWndMain, "malloc failed", s_ProgramName, MB_OK); return 1; }

	memset(s_BadLineBits, 0, (s_TextSize + 7) >> 3);

	ResetWindows(nCmdShow);

	s_HFont = CreateFont(s_popts.fontheight, s_popts.fontwidth, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, s_popts.fontname);
	SendMessage(s_HWndConsole, WM_SETFONT, (WPARAM)s_HFont, 0);
	SendMessage(s_HWndEditor, WM_SETFONT, (WPARAM)s_HFont, 0);
	//SendMessage(hWndLine, WM_SETFONT, (WPARAM)hfont, 0);

	//Use MF_POPUP for top entries
	//Use MF_END for last (top or pulldown) entry
	//MF_GRAYED|MF_DISABLED|4=right_justify|MF_CHECKED|MF_MENUBARBREAK|MF_MENUBREAK|MF_OWNERDRAW
	{
		short sbuf[4096];
		short* sptr;
		sptr = menustart(sbuf);
		sptr = menuadd(sptr, "&File", MF_POPUP, 0);
		sptr = menuadd(sptr, "&New", MF_POPUP, 0);
		sptr = menuadd(sptr, "Blank", 0, MENU_FILENEW + 0);
		sptr = menuadd(sptr, "GLSL (minimal)", 0, MENU_FILENEW + 1);
		sptr = menuadd(sptr, "GLSL", 0, MENU_FILENEW + 2);
		sptr = menuadd(sptr, "ARB ASM", MF_END, MENU_FILENEW + 3);
		sptr = menuadd(sptr, "&Open\tCtrl+L", 0, MENU_FILEOPEN);
		sptr = menuadd(sptr, "&Save\tCtrl+S", 0, MENU_FILESAVE);
		sptr = menuadd(sptr, "Save &As", 0, MENU_FILESAVEAS);
		sptr = menuadd(sptr, "", MF_SEPARATOR, 0);
		sptr = menuadd(sptr, "E&xit\tAlt+F4", MF_END, MENU_FILEEXIT);
		sptr = menuadd(sptr, "&Edit", MF_POPUP, 0);
		sptr = menuadd(sptr, "&Find...\tCtrl+F", 0, MENU_EDITFIND);
		sptr = menuadd(sptr, "Find &Next\tF3", 0, MENU_EDITFINDNEXT);
		sptr = menuadd(sptr, "Find &Previous\tShift+F3", 0, MENU_EDITFINDPREV);
		sptr = menuadd(sptr, "&Replace...\tCtrl+R", MF_END, MENU_EDITREPLACE);
		sptr = menuadd(sptr, "&Options", MF_POPUP, 0);
		sptr = menuadd(sptr, "Compile on Ctrl+Enter", s_popts.compctrlent*MF_CHECKED, MENU_COMPCONTENT);
		sptr = menuadd(sptr, "Evaluate highlighted text\tCtrl+'='", 0, MENU_EVALHIGHLIGHT);
		sptr = menuadd(sptr, "Select Render corner", MF_POPUP, 0);
		sptr = menuadd(sptr, "Top Left", (s_popts.rendcorn == 0)*MF_CHECKED, MENU_RENDPLC + 0);
		sptr = menuadd(sptr, "Top Right", (s_popts.rendcorn == 1)*MF_CHECKED, MENU_RENDPLC + 1);
		sptr = menuadd(sptr, "Bottom Left", (s_popts.rendcorn == 2)*MF_CHECKED, MENU_RENDPLC + 2);
		sptr = menuadd(sptr, "Bottom Right", (s_popts.rendcorn == 3)*MF_CHECKED | MF_END, MENU_RENDPLC + 3);
		sptr = menuadd(sptr, "Fullscreen Render\tAlt+Enter", (s_popts.fullscreen != 0)*MF_CHECKED, MENU_FULLSCREEN);
		sptr = menuadd(sptr, "Clear screen every frame", (s_popts.clearbuffer != 0)*MF_CHECKED, MENU_CLEARBUFFER);
		sptr = menuadd(sptr, "Select &Font..", MF_END, MENU_FONT);
		sptr = menuadd(sptr, "&Help", MF_POPUP | MF_END, 0);
		sptr = menuadd(sptr, "   &About\tF1", MF_END, MENU_HELPABOUT);
		s_HMenu = LoadMenuIndirect(sbuf);
		SetMenu(s_HWndMain, s_HMenu);
	}

	// create OpenGL context
	EnableOpenGL(s_HWndRender, &hDC, &hRC);

	// initialize GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// FIXME: check for minimum OpenGL version here

	// report OpenGL version
	kputs("OpenGL Vendor:    ", 0);
	kputs((const char*)glGetString(GL_VENDOR), 1);
	kputs("OpenGL Renderer:  ", 0);
	kputs((const char*)glGetString(GL_RENDERER), 1);
	kputs("OpenGL Version:   ", 0);
	kputs((const char*)glGetString(GL_VERSION), 1);
	kputs("GLSL Version:     ", 0);
	kputs((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION /* 0x8B8C */), 1);

	// load a default file
	NewFile(2);

	//glGetIntegerv(0x84e2 /*GL_MAX_TEXTURE_UNITS*/,&i); sprintf(buf,"GL_MAX_TEXTURE_UNITS=%d",i); kputs(buf,1); //4 (obsolete/wrong/never use :P)
	//glGetIntegerv(0x8872 /*GL_MAX_TEXTURE_IMAGE_UNITS*/,&i); sprintf(buf,"GL_MAX_TEXTURE_IMAGE_UNITS=%d",i); kputs(buf,1); //32
	//glGetIntegerv(0x8824 /*GL_MAX_DRAW_BUFFERS*/,&i); sprintf(buf,"GL_MAX_DRAW_BUFFERS=%d",i); kputs(buf,1); //8
	//glGetIntegerv(0x8871 /*GL_MAX_TEXTURE_COORDS*/,&i); sprintf(buf,"GL_MAX_TEXTURE_COORDS=%d",i); kputs(buf,1); //8
	//if (!memicmp(glGetString(GL_VENDOR),"NVIDIA",6))
	//{
	//   glGetIntegerv(0x9048 /*NV tot mem kb*/,&i); sprintf(buf,"NV_TOT_MEM=%dKBy",i); kputs(buf,1); //1048576
	//   glGetIntegerv(0x9049 /*NV cur mem kb*/,&i); sprintf(buf,"NV_CUR_MEM=%dKBy",i); kputs(buf,1); //991504
	//}

	//kputs(glGetString(GL_EXTENSIONS),1); //List too long!

	kputs("----------------------------------------", 1);

	//supporttimerquery = (checkext("GL_EXT_timer_query") || checkext("GL_ARB_timer_query"));

	glGenQueries(1, (GLuint*)g_Queries);

	//if (g_glfp[wglSwapIntervalEXT])
	//	((PFNWGLSWAPINTERVALEXTPROC)g_glfp[wglSwapIntervalEXT])(1);

	noiseinit();

	s_Text[0] = 0;
	s_OText[0] = 0;

	if (argfilindex >= 0)
		LoadFile(argv[argfilindex], s_HWndEditor);

	SetFocus(s_HWndEditor);

	/*
	if (scrolly >= 0 || setsel0 >= 0 || setsel1 >= 0)
	{
		if (scrolly >= 0)
			SendMessage(s_HWndEditor, EM_LINESCROLL, 0, scrolly);

		if (setsel0 >= 0 && setsel1 >= 0)
			SendMessage(s_HWndEditor, EM_SETSEL, setsel0, setsel1);
	}

	if (savfilnam)
	{
		strcpy(s_SaveFilename, savfilnam);
		s_SaveFilenamePtr = 0;
		kputs("File name is: ", 0);
		kputs(s_SaveFilename, 1);
	}
	*/

	QueryPerformanceFrequency((LARGE_INTEGER*)&g_qper);
	QueryPerformanceCounter((LARGE_INTEGER*)&g_qtim0);

	printg_init();
	//CreateEmptyTexture(0,32,32,1,KGL_BGRA32); //avoid harmless gl error at glUniform1i(..("tex0")..,0)

	while (1)
	{
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				goto quitit;

			if (s_HWndFind && IsWindow(s_HWndFind) && IsDialogMessage(s_HWndFind, &msg))
				continue; // Needed for FindText/ReplaceText (keyboard shortcuts)

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!s_ActiveApp)
		{
			Sleep(100);
			continue;
		}

		glClearColor(0.f, 0.f, 0.f, 0.f);

		if (s_popts.clearbuffer)
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, (int)g_RenderWidth, (int)g_RenderHeight);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// update perspective
		//mygluPerspective(gfov, (float)oglxres / (float)oglyres, 0.1, 1000.0);

		{
			double fovy = s_FOV;
			double xy = g_RenderWidth / g_RenderHeight;
			double z0 = 0.1;
			double z1 = 1000.0;

			fovy = tan(fovy*(PI / 360.0))*z0;
			xy *= fovy;
			glFrustum(-xy, xy, -fovy, fovy, z0, z1);
		}

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		GetWindowText(s_HWndEditor, s_Text, s_TextSize);
		g_TSec = Text2Sections(s_Text);

		SetShaders(s_HWndMain, s_HWndEditor);

		if (s_ShaderCount[2])
			Render(s_HWndMain, s_HWndEditor);

		if (!s_ShaderCount[2] || !s_EvalFunc)
		{
			Sleep(1);

			if (s_popts.rendcorn == 4) //&& s_mehax)
			{
				CheckMenuItem(s_HMenu, MENU_FULLSCREEN, 0);
				s_popts.fullscreen = 0;
				ResetWindows(SW_NORMAL);
			}
		}

		strcpy(s_OText, s_Text);

		//g_OTSecN = g_TSecN;
		//memcpy(g_OTSec, g_TSec, g_TSecN*sizeof(tsec_t));
		g_OTSec = g_TSec;

		SwapBuffers(hDC);

		QueryPerformanceCounter((LARGE_INTEGER*)&q);
		qnum++;

		if ((q - qlast) > g_qper || !s_SaveFilenamePtr)
		{
			s_SaveFilenamePtr = s_SaveFilename;

			for (i = 0; s_SaveFilename[i]; i++)
			{
				if (s_SaveFilename[i] == '\\')
					s_SaveFilenamePtr = &s_SaveFilename[i + 1];
			}

			i = sprintf(buf, "%s", s_ProgramName);

			if (s_SaveFilename[0])
				i += sprintf(&buf[i], " - %s", s_SaveFilenamePtr);

			if (SendMessage(s_HWndEditor, EM_GETMODIFY, 0, 0))
				i += sprintf(&buf[i], " *");

			i += sprintf(&buf[i], " (%.1f fps)", ((double)g_qper)*((double)qnum) / ((double)(q - qlast)));

			qlast = q;
			qnum = 0;
			SetWindowText(s_HWndMain, buf);
		}
	}

quitit:
	//passasksave();

	glDeleteQueries(1, (GLuint*)g_Queries);

	MIDIUninit();
	DisableOpenGL(s_HWndRender, hDC, hRC);
	DestroyWindow(s_HWndMain);

	//if (!s_mehax)
	SaveIni();

	if (s_BadLineBits)
	{
		free(s_BadLineBits);
		s_BadLineBits = 0;
	}

	if (s_Line)
	{
		free(s_Line);
		s_Line = 0;
	}

	if (s_TText)
	{
		free(s_TText);
		s_TText = 0;
	}

	if (s_OText)
	{
		free(s_OText);
		s_OText = 0;
	}

	if (s_Text)
	{
		free(s_Text);
		s_Text = 0;
	}

	return 0;
}

