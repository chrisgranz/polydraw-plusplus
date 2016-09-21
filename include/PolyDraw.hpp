
#pragma once

#include <cstdio>

#include <string>
#include <vector>

#define PI 3.14159265358979323

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

///////////////////////////////////////////////////////////////////////////////
extern "C" {

//KPLIB.H:
// High-level (easy) picture loading function:
extern void kpzload(const char*, int*, int*, int*, int*);
// Low-level PNG/JPG functions:
extern int kpgetdim(const char*, int, int*, int*);
extern int kprender(const char*, int, int*, int, int, int, int, int);
// Ken's ZIP functions:
extern int kzaddstack(const char*);
extern void kzuninit();
extern void kzsetfil(FILE*);
extern int kzopen(const char*);
extern void kzfindfilestart(const char*);
extern int kzfindfile(char*);
extern int kzread(void*, int);
extern int kzfilelength();
extern int kzseek(int, int);
extern int kztell();
extern int kzgetc();
extern int kzeof();
extern void kzclose();

}

///////////////////////////////////////////////////////////////////////////////
enum
{
	KGL_BGRA32 = 0,
	KGL_CHAR,
	KGL_SHORT,
	KGL_INT, //only supported on newest cards
	KGL_FLOAT,
	KGL_VEC4,
	KGL_NUM,
};

///////////////////////////////////////////////////////////////////////////////
enum
{
	KGL_LINEAR = (0<<4),
	KGL_NEAREST = (1<<4),
	KGL_MIPMAP = (2<<4),
	KGL_MIPMAP3 = (2<<4),
	KGL_MIPMAP2 = (3<<4),
	KGL_MIPMAP1 = (4<<4),
	KGL_MIPMAP0 = (5<<4),
};

///////////////////////////////////////////////////////////////////////////////
enum
{
	KGL_REPEAT = (0<<8),
	KGL_MIRRORED_REPEAT = (1<<8),
	KGL_CLAMP = (2<<8),
	KGL_CLAMP_TO_EDGE = (3<<8),
};

///////////////////////////////////////////////////////////////////////////////
struct AppOptions
{
	int renderCorner;
	int fullscreen;
	int clearBufferEachFrame;
	int compileOnCtrlEnter;
	int timeout;
	int fontheight;
	int fontwidth;
	int sepchar;
	char fontname[256];
};

///////////////////////////////////////////////////////////////////////////////
struct TestSection
{
	int i0, i1; // text index range:{i0 <= i < i1} ('@' lines not included)
	int type; // 0=host,1=vert,2=geom,3=frag
	int count; // 0,1,..
	int lineOffset; // absolute starting line (need for error line)
	int nextType; // index to next of same typ

	int geo_in;     // GL_POINTS,GL_LINES,GL_LINES_ADJACENCY,GL_TRIANGLES,GL_TRIANGLES_ADJACENCY
	int geo_out;    // GL_POINTS,GL_LINE_STRIP,GL_TRIANGLE_STRIP
	int geo_nverts; // 1..1024

	char name[256];
};

static const int MAX_TEXT_SECTIONS = 256;
extern std::vector<TestSection> g_OldTextSections;
extern std::vector<TestSection> g_TextSections;

///////////////////////////////////////////////////////////////////////////////
static const int MAX_USER_TEXURES = 256;
extern int g_CapTextSize;

struct Texture
{
	char name[MAX_PATH]; // texture file name
	int target; // which texture unit
	int colorType; // NOTE: also includes other flags besides color
	int sizeX; // x-size (width)
	int sizeY; // y-size (height)
	int sizeZ; // z-size (depth)
};

extern Texture g_Textures[MAX_USER_TEXURES + 1]; // +1 for font texture

extern int g_ShaderPrograms[];
extern int g_CurrentShader;

extern GLint g_Queries[1];

extern double g_DNumFrames;
extern __int64 g_qper;
extern __int64 g_qtim0;

///////////////////////////////////////////////////////////////////////////////
// PolyDraw.cpp
///////////////////////////////////////////////////////////////////////////////
double SetFOV(double fov, double width, double height);

void kputs(const char* st, int addcr);
double __cdecl myprintf(char *fmt, ...);

void printg_init();
double myprintg(double dx, double dy, double dfcol, char *fmt, ...);

double __cdecl setshader_int(int sh0, int sh1, int sh2);

double MIDIPlayNote(double chn, double frq, double vol);

///////////////////////////////////////////////////////////////////////////////
// EvalBindings.cpp
///////////////////////////////////////////////////////////////////////////////
void noiseinit();

void CreateEmptyTexture(int itex, int xs, int ys, int zs, int icoltype);
void SetEvalVars(double dxres, double dyres, double dmousx, double dmousy, double dbstatus, double dkeystatus[256]);
EVALFUNC CompileEVALFunctionWithExt(std::string text);

