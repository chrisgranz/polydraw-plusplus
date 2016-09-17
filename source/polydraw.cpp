
#include <windows.h>
#include <process.h>
#include <gl/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <malloc.h>

#include "SciLexer.h"
#include "Scintilla.h"

#include "eval.hpp"

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

extern "C" {

//KPLIB.H:
//High-level (easy) picture loading function:
extern void kpzload(const char*, int*, int*, int*, int*);
//Low-level PNG/JPG functions:
extern int kpgetdim(const char*, int, int*, int*);
extern int kprender(const char*, int, INT_PTR, int, int, int, int, int);
//Ken's ZIP functions:
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

#define GLAPIENTRY APIENTRY
#define GL_TEXTURE_WRAP_R           0x8072
#define GL_MIRRORED_REPEAT          0x8370
#define GL_CLAMP_TO_EDGE            0x812F
#define GL_TEXTURE_CUBE_MAP         0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_CONSTANT_COLOR                 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR       0x8002
#define GL_CONSTANT_ALPHA                 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA       0x8004
#define GL_TEXTURE_3D               0x806F
#define GL_FRAGMENT_SHADER          0x8B30
#define GL_VERTEX_SHADER            0x8B31
#define GL_COMPILE_STATUS           0x8B81
#define GL_LINK_STATUS              0x8B82
#define GL_VERTEX_PROGRAM_ARB       0x8620
#define GL_FRAGMENT_PROGRAM_ARB     0x8804
#define GL_PROGRAM_ERROR_STRING_ARB 0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB 0x8875
#define GL_TIME_ELAPSED_EXT         0x88BF
#define GL_QUERY_RESULT             0x8866
#define GL_QUERY_RESULT_AVAILABLE   0x8867
#define GL_TEXTURE0                 0x84c0
#define GL_FRAMEBUFFER_EXT                0x8D40
#define GL_COLOR_ATTACHMENT0_EXT          0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT          0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT          0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT          0x8CE3
#define GL_LINES_ADJACENCY_EXT                 0x000A
#define GL_LINE_STRIP_ADJACENCY_EXT            0x000B
#define GL_TRIANGLES_ADJACENCY_EXT             0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY_EXT        0x000D
#define GL_PROGRAM_POINT_SIZE_EXT              0x8642
#define GL_MAX_PROGRAM_OUTPUT_VERTICES         0x8C27
#define GL_MAX_PROGRAM_TOTAL_OUTPUT_COMPONENTS 0x8C28
#define GL_GEOMETRY_SHADER_EXT                 0x8DD9
#define GL_GEOMETRY_VERTICES_OUT_EXT           0x8DDA
#define GL_GEOMETRY_INPUT_TYPE_EXT             0x8DDB
#define GL_GEOMETRY_OUTPUT_TYPE_EXT            0x8DDC
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_FLOAT                          0x1406
#define GL_TEXTURE_RECTANGLE_ARB          0x84F5
#define GL_RGBA32F_ARB                    0x8814
#define GL_LUMINANCE32F_ARB               0x8818
#define GL_LUMINANCE32I_EXT               0x8D86

typedef int GLsizei;
typedef char GLchar;
typedef char GLcharARB;
typedef unsigned int GLhandleARB;
typedef unsigned __int64 GLuint64EXT;
typedef GLuint (GLAPIENTRY *PFNGLCREATESHADERPROC      )(GLenum type);
typedef GLuint (GLAPIENTRY *PFNGLCREATEPROGRAMPROC     )(void);
typedef void   (GLAPIENTRY *PFNGLSHADERSOURCEPROC      )(GLuint shader, GLsizei count, const GLchar **strings, const GLint *lengths);
typedef void   (GLAPIENTRY *PFNGLCOMPILESHADERPROC     )(GLuint shader);
typedef void   (GLAPIENTRY *PFNGLATTACHSHADERPROC      )(GLuint program, GLuint shader);
typedef void   (GLAPIENTRY *PFNGLLINKPROGRAMPROC       )(GLuint program);
typedef void   (GLAPIENTRY *PFNGLUSEPROGRAMPROC        )(GLuint program);
typedef void   (GLAPIENTRY *PFNGLGETPROGRAMIVPROC      )(GLuint program, GLenum pname, GLint *param);
typedef void   (GLAPIENTRY *PFNGLGETSHADERIVPROC       )(GLuint shader, GLenum pname, GLint *param);
typedef void   (GLAPIENTRY *PFNGLGETINFOLOGARBPROC     )(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void   (GLAPIENTRY *PFNGLDETACHSHADERPROC      )(GLuint program, GLuint shader);
typedef void   (GLAPIENTRY *PFNGLDELETEPROGRAMPROC     )(GLuint program);
typedef void   (GLAPIENTRY *PFNGLDELETESHADERPROC      )(GLuint shader);
typedef GLint  (GLAPIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void   (GLAPIENTRY *PFNGLUNIFORM1FPROC         )(GLint location, GLfloat v0);
typedef void   (GLAPIENTRY *PFNGLUNIFORM2FPROC         )(GLint location, GLfloat v0, GLfloat v1);
typedef void   (GLAPIENTRY *PFNGLUNIFORM3FPROC         )(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void   (GLAPIENTRY *PFNGLUNIFORM4FPROC         )(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void   (GLAPIENTRY *PFNGLUNIFORM1IPROC         )(GLint location, GLint v0);
typedef void   (GLAPIENTRY *PFNGLUNIFORM2IPROC         )(GLint location, GLint v0, GLint v1);
typedef void   (GLAPIENTRY *PFNGLUNIFORM3IPROC         )(GLint location, GLint v0, GLint v1, GLint v2);
typedef void   (GLAPIENTRY *PFNGLUNIFORM4IPROC         )(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void   (GLAPIENTRY *PFNGLUNIFORM1FVPROC        )(GLint location, GLsizei count, const GLfloat *value);
typedef void   (GLAPIENTRY *PFNGLUNIFORM2FVPROC        )(GLint location, GLsizei count, const GLfloat *value);
typedef void   (GLAPIENTRY *PFNGLUNIFORM3FVPROC        )(GLint location, GLsizei count, const GLfloat *value);
typedef void   (GLAPIENTRY *PFNGLUNIFORM4FVPROC        )(GLint location, GLsizei count, const GLfloat *value);
typedef void   (GLAPIENTRY *PFNGLUNIFORM1IVPROC        )(GLint location, GLsizei count, const GLint *value);
typedef void   (GLAPIENTRY *PFNGLUNIFORM2IVPROC        )(GLint location, GLsizei count, const GLint *value);
typedef void   (GLAPIENTRY *PFNGLUNIFORM3IVPROC        )(GLint location, GLsizei count, const GLint *value);
typedef void   (GLAPIENTRY *PFNGLUNIFORM4IVPROC        )(GLint location, GLsizei count, const GLint *value);
typedef GLint  (GLAPIENTRY *PFNGLGETATTRIBLOCATIONPROC )(GLuint program, const GLchar *name);
typedef void   (GLAPIENTRY *PFNGLVERTEXATTRIB1FPROC    )(GLuint index, GLfloat v0);
typedef void   (GLAPIENTRY *PFNGLVERTEXATTRIB2FPROC    )(GLuint index, GLfloat v0, GLfloat v1);
typedef void   (GLAPIENTRY *PFNGLVERTEXATTRIB3FPROC    )(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void   (GLAPIENTRY *PFNGLVERTEXATTRIB4FPROC    )(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void   (GLAPIENTRY *PFNGLACTIVETEXTUREPROC     )(GLuint texture);
typedef void   (GLAPIENTRY *PFNGLTEXIMAGE3DPROC        )(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
typedef void   (GLAPIENTRY *PFNGLTEXSUBIMAGE3DPROC     )(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
typedef GLint  (GLAPIENTRY *PFNWGLSWAPINTERVALEXTPROC  )(GLint interval);
typedef void   (GLAPIENTRY *PFNGLGENPROGRAMSARBPROC            )(GLsizei n, GLuint *programs);
typedef void   (GLAPIENTRY *PFNGLBINDPROGRAMARBPROC            )(GLenum target, GLuint program);
typedef void   (GLAPIENTRY *PFNGLGETPROGRAMSTRINGARBPROC       )(GLenum target, GLenum pname, GLvoid *string);
typedef void   (GLAPIENTRY *PFNGLPROGRAMSTRINGARBPROC          )(GLenum target, GLenum format, GLsizei len, const GLvoid *string);
typedef void   (GLAPIENTRY *PFNGLPROGRAMLOCALPARAMETER4FARBPROC)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void   (GLAPIENTRY *PFNGLPROGRAMENVPARAMETER4FARBPROC  )(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void   (GLAPIENTRY *PFNGLPROGRAMPARAMETERIEXTPROC) (GLuint program, GLenum pname, GLint value);
typedef void   (GLAPIENTRY *PFNGLDELETEPROGRAMSARBPROC         )(GLsizei n, const GLuint *programs);
typedef void   (GLAPIENTRY *PFNGLGENQUERIESPROC)             (GLsizei n, GLuint *ids);
typedef void   (GLAPIENTRY *PFNGLDELETEQUERIESPROC)          (GLsizei n, const GLuint *ids);
typedef void   (GLAPIENTRY *PFNGLBEGINQUERYPROC)             (GLenum target, GLuint id);
typedef void   (GLAPIENTRY *PFNGLENDQUERYPROC)               (GLenum target);
typedef void   (GLAPIENTRY *PFNGLGETQUERYIVPROC)             (GLenum target, GLenum pname, GLint *params);
typedef void   (GLAPIENTRY *PFNGLGETQUERYOBJECTIVPROC)       (GLuint id, GLenum pname, GLint *params);
typedef void   (GLAPIENTRY *PFNGLGETQUERYOBJECTUIVPROC)      (GLuint id, GLenum pname, GLuint *params);
typedef void   (GLAPIENTRY *PFNGLGETQUERYOBJECTUI64VEXTPROC) (GLuint id, GLenum pname, GLuint64EXT *params);
typedef void   (GLAPIENTRY *PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void   (GLAPIENTRY *PFNGLBINDFRAMEBUFFEREXTPROC)     (GLenum, GLuint);
typedef void   (GLAPIENTRY *PFNGLGENFRAMEBUFFERSEXTPROC)     (GLsizei, GLuint *);

//         char  1 *        GL_LUMINANCE8  GL_LUMINANCE    GL_SIGNED_BYTE
//        cvec4  4 *  4 (GL_SIGNED_RGBA8)       GL_RGBA    GL_SIGNED_BYTE
//        uchar  1 *        GL_LUMINANCE8  GL_LUMINANCE  GL_UNSIGNED_BYTE
//       ucvec4  4 *         4 (GL_RGBA8)       GL_RGBA  GL_UNSIGNED_BYTE
//argb32         4 *         4 (GL_RGBA8)   GL_BGRA_EXT  GL_UNSIGNED_BYTE
//        short  2 *       GL_LUMINANCE16  GL_LUMINANCE   GL_SIGNED_SHORT
//        svec4  8             GL_RGBA16I       GL_RGBA   GL_SIGNED_SHORT //new gpu only
//       ushort  2 *       GL_LUMINANCE16  GL_LUMINANCE GL_UNSIGNED_SHORT
//       usvec4  8            GL_RGBA16UI       GL_RGBA GL_UNSIGNED_SHORT //new gpu only
//    *     int  4 *  GL_LUMINANCE32I_EXT  GL_LUMINANCE     GL_SIGNED_INT //new gpu only
//    *   ivec4 16             GL_RGBA32I       GL_RGBA     GL_SIGNED_INT //new gpu only
//    *    uint  4 * GL_LUMINANCE32UI_EXT  GL_LUMINANCE   GL_UNSIGNED_INT //new gpu only
//    *   uvec4 16            GL_RGBA32UI       GL_RGBA   GL_UNSIGNED_INT //new gpu only
//    *   float  4 *  GL_LUMINANCE32F_ARB  GL_LUMINANCE          GL_FLOAT
//    *    vec4 16 *       GL_RGBA32F_ARB       GL_RGBA          GL_FLOAT
//
//public: KGL_BGRA32, KGL_FLOAT, KGL_VEC4

enum { KGL_BGRA32=0, KGL_CHAR, KGL_SHORT, KGL_INT/*only supported on newest cards*/, KGL_FLOAT, KGL_VEC4, KGL_NUM};
enum { KGL_LINEAR = (0<<4), KGL_NEAREST = (1<<4), KGL_MIPMAP = (2<<4),
		 KGL_MIPMAP3 = (2<<4), KGL_MIPMAP2 = (3<<4), KGL_MIPMAP1 = (4<<4), KGL_MIPMAP0 = (5<<4)};
enum { KGL_REPEAT = (0<<8), KGL_MIRRORED_REPEAT = (1<<8), KGL_CLAMP = (2<<8), KGL_CLAMP_TO_EDGE = (3<<8)};

static int usearbasm = 0;
static int usearbasmonly = 0; //1 if "!!" is detected
static int useoldglfuncs = 0;

const static char* glnames[] =
{
	"glGenProgramsARB","glBindProgramARB",                      //ARB ASM...
	"glGetProgramStringARB","glProgramStringARB",
	"glProgramLocalParameter4fARB",
	"glProgramEnvParameter4fARB",
	"glDeleteProgramsARB",

	"glActiveTexture","glTexImage3D","glTexSubImage3D",         //multi/extended texture
	"wglSwapIntervalEXT",                                       //limit refresh/sleep

	"glCreateShader","glCreateProgram",                         //compile
	"glShaderSource","glCompileShader",                         //
	"glAttachShader","glLinkProgram","glUseProgram",            //link
	"glGetShaderiv","glGetProgramiv","glGetInfoLogARB",         //get info
	"glDetachShader","glDeleteProgram","glDeleteShader",        //decompile
	"glGetUniformLocation","glGetAttribLocation",               //host->shader
	"glUniform1f" ,"glUniform2f" ,"glUniform3f" ,"glUniform4f",
	"glUniform1i" ,"glUniform2i" ,"glUniform3i" ,"glUniform4i",
	"glUniform1fv","glUniform2fv","glUniform3fv","glUniform4fv",
	"glUniform1iv","glUniform2iv","glUniform3iv","glUniform4iv",
	"glVertexAttrib1f","glVertexAttrib2f","glVertexAttrib3f","glVertexAttrib4f",

	"glGenQueries","glDeleteQueries",
	"glBeginQuery","glEndQuery",
	"glGetQueryObjectiv",
	"glGetQueryObjectuiv",

	"glFramebufferTexture2DEXT","glBindFramebufferEXT","glGenFramebuffersEXT",

	"glGetQueryObjectui64vEXT",

	"glProgramParameteriEXT",
};
const static char *glnames_old[] =
{
	"glGenProgramsARB","glBindProgramARB",                          //ARB ASM...
	"glGetProgramStringARB","glProgramStringARB",
	"glProgramLocalParameter4fARB",
	"glProgramEnvParameter4fARB",
	"glDeleteProgramsARB",

	"glActiveTexture","glTexImage3D","glTexSubImage3D",             //texture unit
	"wglSwapIntervalEXT",                                           //limit refresh/sleep

	"glCreateShaderObjectARB","glCreateProgramObjectARB",           //compile
	"glShaderSourceARB","glCompileShaderARB",                       //
	"glAttachObjectARB","glLinkProgramARB","glUseProgramObjectARB", //link
	"glGetObjectParameterivARB","glGetObjectParameterivARB","glGetInfoLogARB", //get info
	"glDetachObjectARB","glDeleteObjectARB","glDeleteObjectARB",    //decompile
	"glGetUniformLocationARB","glGetAttribLocationARB",             //host->shader
	"glUniform1fARB" ,"glUniform2fARB" ,"glUniform3fARB" ,"glUniform4fARB",
	"glUniform1iARB" ,"glUniform2iARB" ,"glUniform3iARB" ,"glUniform4iARB",
	"glUniform1fvARB","glUniform2fvARB","glUniform3fvARB","glUniform4fvARB",
	"glUniform1ivARB","glUniform2ivARB","glUniform3ivARB","glUniform4ivARB",
	"glVertexAttrib1fARB","glVertexAttrib2fARB","glVertexAttrib3fARB","glVertexAttrib4fARB",

	"glGenQueriesARB","glDeleteQueriesARB",
	"glBeginQueryARB","glEndQueryARB",
	"glGetQueryObjectivARB",
	"glGetQueryObjectuivARB",

	"glFramebufferTexture2DEXT","glBindFramebufferEXT","glGenFramebuffersEXT",

	"glGetQueryObjectui64vEXT",

	"glProgramParameteriEXT",
};
enum
{
	glGenProgramsARB,glBindProgramARB,             //ARB ASM...
	glGetProgramStringARB,glProgramStringARB,
	glProgramLocalParameter4fARB,
	glProgramEnvParameter4fARB,
	glDeleteProgramsARB,

	glActiveTexture,glTexImage3D,glTexSubImage3D,  //multi/extended texture
	wglSwapIntervalEXT,                            //limit refesh/sleep

	glCreateShader,glCreateProgram,                //compile
	glShaderSource,glCompileShader,                //
	glAttachShader,glLinkProgram,glUseProgram,     //link
	glGetShaderiv,glGetProgramiv,glGetInfoLogARB,  //get info
	glDetachShader,glDeleteProgram,glDeleteShader, //decompile
	glGetUniformLocation,glGetAttribLocation,      //host->shader
	glUniform1f, glUniform2f, glUniform3f, glUniform4f,
	glUniform1i, glUniform2i, glUniform3i, glUniform4i,
	glUniform1fv,glUniform2fv,glUniform3fv,glUniform4fv,
	glUniform1iv,glUniform2iv,glUniform3iv,glUniform4iv,
	glVertexAttrib1f,glVertexAttrib2f,glVertexAttrib3f,glVertexAttrib4f,

	glGenQueries,glDeleteQueries,
	glBeginQuery,glEndQuery,
	glGetQueryObjectiv,
	glGetQueryObjectuiv,

	glFramebufferTexture2DEXT,glBindFramebufferEXT,glGenFramebuffersEXT,

	glGetQueryObjectui64vEXT,

	glProgramParameteri,

	NUMGLFUNC
};

using glfp_t = void(*)();
static glfp_t glfp[NUMGLFUNC] = {nullptr};

static int textsiz = 0;
static char* text = nullptr;
static char* otext = nullptr;
static char* ttext = nullptr;
static char* line = nullptr;
static char* badlinebits = nullptr;

struct tsec_t
{
	int i0, i1; //text index range:{i0<=i<i1} ('@' lines not included)
	int typ; //0=host,1=vert,2=geom,3=frag
	int cnt; //0,1,..
	int linofs; //absolute starting line (need for error line)
	int nxt; //index to next of same typ

	int geo_in;     //GL_POINTS,GL_LINES,GL_LINES_ADJACENCY,GL_TRIANGLES,GL_TRIANGLES_ADJACENCY
	int geo_out;    //GL_POINTS,GL_LINE_STRIP,GL_TRIANGLE_STRIP
	int geo_nverts; //1..1024

	char nam[64];
};

#define TSECMAX 256
static tsec_t otsec[TSECMAX];
static tsec_t tsec[TSECMAX];
static int otsecn = 0;
static int tsecn = 0;

#define MAXUSERTEX 256
static int captexsiz = 512;
struct tex_t
{
	char nam[MAX_PATH];
	int tar;
	int coltype;
	int sizx;
	int sizy;
	int sizz;
};

static tex_t tex[MAXUSERTEX + 1/*+1 for font*/] = { 0 };
static char* gbmp = 0;
static int gbmpmal = 0;

#define SHADMAX 256
static int shad[3][SHADMAX];
static int shadn[3] = { 0, 0, 0 };
static int geo2blocki[SHADMAX];
#define PROGMAX 256
static int shadprog[PROGMAX];
static int shadprogn = 0;
static int gcurshader = 0;

struct shadprogi_t
{
	int v;
	int g;
	int f;
	int ishw;
};

static shadprogi_t shadprogi[PROGMAX]; //remember linkages

//static OSVERSIONINFO osvi;
static int supporttimerquery = 1;
static GLint queries[1];

static char* prognam = "PolyDraw";
static int oxres = 0, oyres = 0, xres, yres, ActiveApp = 1, shkeystatus = 0;
static int gshaderstuck = 0, gshadercrashed = 0;
static double gfov, dbstatus = 0.0, dkeystatus[256] = {0}, dnumframes = 0.0;
static __int64 qper, qtim0;
static int oglxres, oglyres, songtime = 0, gmehax = 0, dorecompile = 0;
static char gsavfilnam[MAX_PATH] = "", *gsavfilnamptr = 0;
static HWND ghwnd = 0, hWndDraw = 0, hWndCons = 0, hWndLine = 0, hWndEdit = 0;
static HFONT hfont = 0;
static HINSTANCE ghinst;

enum
{
	MENU_FILENEW=0,MENU_FILEOPEN=MENU_FILENEW+4,MENU_FILESAVE,MENU_FILESAVEAS,MENU_FILEEXIT,
	MENU_EDITFIND,MENU_EDITFINDNEXT,MENU_EDITFINDPREV,MENU_EDITREPLACE,
	MENU_COMPCONTENT,MENU_EVALHIGHLIGHT,MENU_RENDPLC,MENU_FULLSCREEN=MENU_RENDPLC+4,MENU_CLEARBUFFER,MENU_FONT,
	MENU_HELPTEXT,MENU_HELPABOUT
};

///////////////////////////////////////////////////////////////////////////////
static char gexefullpath[MAX_PATH] = "", gexedironly[MAX_PATH] = "", ginifilnam[MAX_PATH] = "";

typedef struct
{
	int rendcorn, fullscreen, clearbuffer, timeout, fontheight, fontwidth, compctrlent, sepchar;
	char fontname[256];
} popt_t;
static popt_t popts, opopts;

///////////////////////////////////////////////////////////////////////////////
static void loadini (void)
{
	//char tbuf[512];

	popts.rendcorn    =   0;
	popts.fullscreen  =   0;
    popts.clearbuffer =   1;
	popts.timeout     = 250;
	popts.fontheight  = -13;
	popts.fontwidth   =   0;
	popts.compctrlent =   0;
	popts.sepchar     = '-';
	strcpy(popts.fontname,"Courier");

	popts.rendcorn    = min(max(        GetPrivateProfileInt("POLYDRAW","rendcorn"   ,popts.rendcorn   ,ginifilnam),    0),   4);
	popts.fullscreen  = min(max(        GetPrivateProfileInt("POLYDRAW","fullscreen" ,popts.fullscreen ,ginifilnam),    0),   1);
	popts.clearbuffer = min(max(        GetPrivateProfileInt("POLYDRAW","clearbuffer",popts.clearbuffer,ginifilnam),    0),   1);
	popts.timeout     = min(max(        GetPrivateProfileInt("POLYDRAW","timeout"    ,popts.timeout    ,ginifilnam),    0),5000);
	popts.fontheight  = min(max((signed)GetPrivateProfileInt("POLYDRAW","fontheight" ,popts.fontheight ,ginifilnam),-1000),1000);
	popts.fontwidth   = min(max((signed)GetPrivateProfileInt("POLYDRAW","fontwidth"  ,popts.fontwidth  ,ginifilnam),-1000),1000);
	popts.compctrlent = min(max(        GetPrivateProfileInt("POLYDRAW","compctrlent",popts.compctrlent,ginifilnam),    0),   1);
	popts.sepchar     = min(max(        GetPrivateProfileInt("POLYDRAW","sepchar"    ,popts.sepchar    ,ginifilnam),    0), 255);
	GetPrivateProfileString("POLYDRAW","fontname",popts.fontname,popts.fontname,sizeof(popts.fontname),ginifilnam);

	memcpy(&opopts, &popts, sizeof(opopts));
}

///////////////////////////////////////////////////////////////////////////////
static void saveini (void)
{
	char tbuf[512];

	if (!memcmp(&opopts,&popts,sizeof(opopts))) return;
	sprintf(tbuf,"%d",popts.rendcorn   ); WritePrivateProfileString("POLYDRAW","rendcorn"   ,tbuf,ginifilnam);
	sprintf(tbuf,"%d",popts.fullscreen ); WritePrivateProfileString("POLYDRAW","fullscreen" ,tbuf,ginifilnam);
	sprintf(tbuf,"%d",popts.clearbuffer); WritePrivateProfileString("POLYDRAW","clearbuffer",tbuf,ginifilnam);
	sprintf(tbuf,"%d",popts.timeout    ); WritePrivateProfileString("POLYDRAW","timeout"    ,tbuf,ginifilnam);
	sprintf(tbuf,"%d",popts.fontheight ); WritePrivateProfileString("POLYDRAW","fontheight" ,tbuf,ginifilnam);
	sprintf(tbuf,"%d",popts.fontwidth  ); WritePrivateProfileString("POLYDRAW","fontwidth"  ,tbuf,ginifilnam);
	sprintf(tbuf,"%d",popts.compctrlent); WritePrivateProfileString("POLYDRAW","compctrlent",tbuf,ginifilnam);
	sprintf(tbuf,"%d",popts.sepchar    ); WritePrivateProfileString("POLYDRAW","sepchar"    ,tbuf,ginifilnam);
	sprintf(tbuf,"%s",popts.fontname   ); WritePrivateProfileString("POLYDRAW","fontname"   ,tbuf,ginifilnam);
}

///////////////////////////////////////////////////////////////////////////////
static void kputs (const char* st, int addcr)
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
		SendMessage(hWndCons, WM_SETTEXT, 0, (long)buf); //SetWindowText(hWndCons,buf);
		SendMessage(hWndCons, EM_LINESCROLL, 0, 0x7fffffff);
	}
	else
	{
		SendMessage(hWndCons, EM_SETSEL, iminmod, obufleng);
		SendMessage(hWndCons, EM_REPLACESEL, 0, (LPARAM)&buf[iminmod]);
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
			safecallevent[i] = CreateEvent(0,0,0,0);

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
static HANDLE /*gmainthread,*/ gthand;
static HANDLE ghevent[3];
static double (__cdecl*gevalfunc)() = 0;
static int gevalfuncleng = 0;
static int showtimeout = 0;

///////////////////////////////////////////////////////////////////////////////
static unsigned int __stdcall watchthread(void *_)
{
	while (1)
	{
		WaitForSingleObject(ghevent[0], INFINITE);

			//if script takes too long, temporarily apply self-modifying code to force it to finish much faster
		if (WaitForSingleObject(ghevent[1], popts.timeout) == WAIT_TIMEOUT)
		{
			showtimeout = 1;
			kasm87jumpback(gevalfunc, 0);
			WaitForSingleObject(ghevent[1], INFINITE);
			kasm87jumpback(gevalfunc, 1);
		}

		SetEvent(ghevent[2]);
	}
}

///////////////////////////////////////////////////////////////////////////////
HMENU gmenu = 0;

///////////////////////////////////////////////////////////////////////////////
static short* menustart(short* sptr)
{
	*sptr++ = 0;
	*sptr++ = 0;
	return sptr;
} //MENUITEMTEMPLATEHEADER

///////////////////////////////////////////////////////////////////////////////
static short* menuadd(short* sptr, char* st, int flags, int id)
{
	*sptr++ = flags; //MENUITEMTEMPLATE

	if (!(flags&MF_POPUP))
		*sptr++ = id;

	sptr += MultiByteToWideChar(CP_ACP, 0, st, -1, (LPWSTR)sptr, strlen(st) + 1);
	return(sptr);
}

///////////////////////////////////////////////////////////////////////////////
// glu32.lib replacements..
static void gluPerspective(double fovy, double xy, double z0, double z1)
{
	fovy = tan(fovy*(PI / 360.0))*z0;
	xy *= fovy;
	glFrustum(-xy, xy, -fovy, fovy, z0, z1);
}

///////////////////////////////////////////////////////////////////////////////
static void gluLookAt(double px, double py, double pz, double fx, double fy, double fz, double ux, double uy, double uz)
{
	double t, r[3], d[3], f[3], mat[16];

	f[0] = px-fx; f[1] = py-fy; f[2] = pz-fz;
	t = 1.0/sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]); f[0] *= t; f[1] *= t; f[2] *= t;

	r[0] = f[2]*uy - f[1]*uz;
	r[1] = f[0]*uz - f[2]*ux;
	r[2] = f[1]*ux - f[0]*uy;
	t = 1.0/sqrt(r[0]*r[0] + r[1]*r[1] + r[2]*r[2]); r[0] *= t; r[1] *= t; r[2] *= t;

	d[0] = f[1]*r[2] - f[2]*r[1];
	d[1] = f[2]*r[0] - f[0]*r[2];
	d[2] = f[0]*r[1] - f[1]*r[0];

	mat[0] = r[0]; mat[4] = r[1]; mat[ 8] = r[2]; mat[12] = -(mat[0]*px + mat[4]*py + mat[ 8]*pz);
	mat[1] = d[0]; mat[5] = d[1]; mat[ 9] = d[2]; mat[13] = -(mat[1]*px + mat[5]*py + mat[ 9]*pz);
	mat[2] = f[0]; mat[6] = f[1]; mat[10] = f[2]; mat[14] = -(mat[2]*px + mat[6]*py + mat[10]*pz);
	mat[3] =  0.0; mat[7] =  0.0; mat[11] =  0.0; mat[15] = 1.0;
	glLoadMatrixd(mat);
}

///////////////////////////////////////////////////////////////////////////////
static int gluBuild2DMipmaps(GLenum target, GLint components, GLint xs, GLint ys, GLenum format, GLenum type, const void *data)
{
	unsigned char *wptr, *rptr, *rptr2;
	int i, x, y,  nxs, nys, xs4, nxs4;

	for (i = 1; (xs | ys) & ~1; i++, xs = nxs, ys = nys)
	{
		nxs = max(xs >> 1, 1); nys = max(ys >> 1, 1); xs4 = (xs << 2); nxs4 = (nxs << 2); //from GL_ARB_texture_non_power_of_two spec
		wptr = (unsigned char *)data; rptr = (unsigned char *)data;

		for (y = 0; y < nys; y++, wptr += nxs4, rptr += xs4 * 2)
		{
			for (x = 0; x < nxs4; x++)
			{
				rptr2 = &rptr[(x&~3) + x];
				wptr[x] = (((int)rptr2[0] + (int)rptr2[4] + (int)rptr2[xs4] + (int)rptr2[xs4 + 4] + 2) >> 2);
			}
		}

		glTexImage2D(target, i, 4, nxs, nys, 0, format, type, data); //loading 1st time
	 //glTexSubImage2D(target,i,0,0,nxs,nys  ,format,type,data); //overwrite old texture
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglVertex2d    (double x, double y)                     { glVertex2d(x,y);          return(0.0); }
double __cdecl qglVertex3d    (double x, double y, double z)           { glVertex3d(x,y,z);        return(0.0); }
double __cdecl qglVertex4d    (double x, double y, double z, double w) { glVertex4d(x,y,z,w);      return(0.0); }
double __cdecl qglTexCoord2d  (double u, double v)                     { glTexCoord2d(u,v);        return(0.0); }
double __cdecl qglTexCoord3d  (double u, double v, double s)           { glTexCoord3d(u,v,s);      return(0.0); }
double __cdecl qglTexCoord4d  (double u, double v, double s, double t) { glTexCoord4d(u,v,s,t);    return(0.0); }
double __cdecl qglColor3d     (double x, double y, double z)           { glColor3d(x,y,z);         return(0.0); }
double __cdecl qglColor4d     (double x, double y, double z, double w) { glColor4d(x,y,z,w);       return(0.0); }
double __cdecl qglNormal3d    (double x, double y, double z)           { glNormal3d(x,y,z);        return(0.0); }

double __cdecl qglClear       (double mask)                            { if (mask) glClear(mask);
                                                                         else glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);  
                                                                         return(0.0); }
double __cdecl qglBegin       (double mode)                            { glBegin((int)mode);       return(0.0); }
double __cdecl qglEnd         (double _)                               { glEnd();                  return(0.0); }
double __cdecl qglPushMatrix  (double _)                               { glPushMatrix();           return(0.0); }
double __cdecl qglPopMatrix   (double _)                               { glPopMatrix();            return(0.0); }
double __cdecl qglMultMatrixd (double *m)                              { if (m) glMultMatrixd(m);  return(0.0); }
double __cdecl qglTranslated  (double x, double y, double z)           { glTranslated(x,y,z);      return(0.0); }
double __cdecl qglRotated     (double t, double x, double y, double z) { glRotated(t,x,y,z);       return(0.0); }
double __cdecl qglScaled      (double x, double y, double z)           { glScaled(x,y,z);          return(0.0); }
double __cdecl qglEndTex      (double _)                               {                           return(0.0); }
double __cdecl qglLineWidth   (double size)                            { glLineWidth(size);        return(0.0); }

double __cdecl kmyrgb         (double r, double g, double b)           { return((double)((min(max((int)r,0),255)<<16) + (min(max((int)g,0),255)<<8) + min(max((int)b,0),255))); }
double __cdecl kmyrgba        (double r, double g, double b, double a) { return((double)((min(max((int)a,0),255)<<24) + (min(max((int)r,0),255)<<16) + (min(max((int)g,0),255)<<8) + min(max((int)b,0),255))); }

///////////////////////////////////////////////////////////////////////////////
static int myprintf_check(char *fmt)
{
	int i, inperc, inslash;

	if (!fmt) return(-1);

	inperc = 0; inslash = 0; // Filter out

	for(i=0;fmt[i];i++)
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
static void myprintf_filter (char *st)
{
	int i, j, inslash;

	//Filter \\, \n, etc..
	inslash = 0;
	for(i=0,j=0;st[i];i++)
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
		return(-1.0);

	va_start(arglist,fmt);
	_vsnprintf(st,sizeof(st),fmt,arglist);
	va_end(arglist);

	myprintf_filter(st);

	kputs(st,0);

	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
// NOTE: font is stored vertically first! (like .ART files)
static const __int64 font6x8[] = //256 DOS chars, from: DOSAPP.FON (tab blank)
{
	0x3E00000000000000,0x6F6B3E003E455145,0x1C3E7C3E1C003E6B,0x3000183C7E3C1800,
	0x7E5C180030367F36,0x000018180000185C,0x0000FFFFE7E7FFFF,0xDBDBC3FF00000000,
	0x0E364A483000FFC3,0x6000062979290600,0x0A7E600004023F70,0x2A1C361C2A003F35,
	0x0800081C3E7F0000,0x7F361400007F3E1C,0x005F005F00001436,0x22007F017F090600,
	0x606060002259554D,0x14B6FFB614000060,0x100004067F060400,0x3E08080010307F30,
	0x08083E1C0800081C,0x0800404040407800,0x3F3C3000083E083E,0x030F3F0F0300303C,
	0x0000000000000000,0x0003070000065F06,0x247E247E24000307,0x630000126A2B2400,
	0x5649360063640813,0x0000030700005020,0x00000000413E0000,0x1C3E080000003E41,
	0x08083E080800083E,0x0800000060E00000,0x6060000008080808,0x0204081020000000,
	0x00003E4549513E00,0x4951620000407F42,0x3649494922004649,0x2F00107F12141800,
	0x494A3C0031494949,0x0305097101003049,0x0600364949493600,0x6C6C00001E294949,
	0x00006CEC00000000,0x2400004122140800,0x2241000024242424,0x0609590102000814,
	0x7E001E555D413E00,0x49497F007E111111,0x224141413E003649,0x7F003E4141417F00,
	0x09097F0041494949,0x7A4949413E000109,0x00007F0808087F00,0x4040300000417F41,
	0x412214087F003F40,0x7F00404040407F00,0x04027F007F020402,0x3E4141413E007F08,
	0x3E00060909097F00,0x09097F005E215141,0x3249494926006619,0x3F0001017F010100,
	0x40201F003F404040,0x3F403C403F001F20,0x0700631408146300,0x4549710007087008,
	0x0041417F00000043,0x0000201008040200,0x01020400007F4141,0x8080808080800402,
	0x2000000007030000,0x44447F0078545454,0x2844444438003844,0x38007F4444443800,
	0x097E080008545454,0x7CA4A4A418000009,0x0000007804047F00,0x8480400000407D00,
	0x004428107F00007D,0x7C0000407F000000,0x04047C0078041804,0x3844444438000078,
	0x380038444444FC00,0x44784400FC444444,0x2054545408000804,0x3C000024443E0400,
	0x40201C00007C2040,0x3C6030603C001C20,0x9C00006C10106C00,0x54546400003C60A0,
	0x0041413E0800004C,0x0000000077000000,0x02010200083E4141,0x3C2623263C000001,
	0x3D001221E1A11E00,0x54543800007D2040,0x7855555520000955,0x2000785554552000,
	0x5557200078545555,0x1422E2A21C007857,0x3800085555553800,0x5555380008555455,
	0x00417C0100000854,0x0000004279020000,0x2429700000407C01,0x782F252F78007029,
	0x3400455554547C00,0x7F097E0058547C54,0x0039454538004949,0x3900003944453800,
	0x21413C0000384445,0x007C20413D00007D,0x3D00003D60A19C00,0x40413C00003D4242,
	0x002466241800003D,0x29006249493E4800,0x16097F00292A7C2A,0x02097E8840001078,
	0x0000785555542000,0x4544380000417D00,0x007D21403C000039,0x7A0000710A097A00,
	0x5555080000792211,0x004E51514E005E55,0x3C0020404D483000,0x0404040404040404,
	0x506A4C0817001C04,0x0000782A34081700,0x0014080000307D30,0x0814000814001408,
	0x55AA114411441144,0xEEBBEEBB55AA55AA,0x0000FF000000EEBB,0x0A0A0000FF080808,
	0xFF00FF080000FF0A,0x0000F808F8080000,0xFB0A0000FE0A0A0A,0xFF00FF000000FF00,
	0x0000FE02FA0A0000,0x0F0800000F080B0A,0x0F0A0A0A00000F08,0x0000F80808080000,
	0x080808080F000000,0xF808080808080F08,0x0808FF0000000808,0x0808080808080808,
	0xFF0000000808FF08,0x0808FF00FF000A0A,0xFE000A0A0B080F00,0x0B080B0A0A0AFA02,
	0x0A0AFA02FA0A0A0A,0x0A0A0A0AFB00FF00,0xFB00FB0A0A0A0A0A,0x0A0A0B0A0A0A0A0A,
	0x0A0A08080F080F08,0xF808F8080A0AFA0A,0x08080F080F000808,0x00000A0A0F000000,
	0xF808F8000A0AFE00,0x0808FF00FF080808,0x08080A0AFB0A0A0A,0xF800000000000F08,
	0xFFFFFFFFFFFF0808,0xFFFFF0F0F0F0F0F0,0xFF000000000000FF,0x0F0F0F0F0F0FFFFF,
	0xFE00241824241800,0x01017F0000344A4A,0x027E027E02000003,0x1800006349556300,
	0x2020FC00041C2424,0x000478040800001C,0x3E00085577550800,0x02724C00003E4949,
	0x0030595522004C72,0x1800182418241800,0x2A2A1C0018247E24,0x003C02023C00002A,
	0x0000002A2A2A2A00,0x4A4A510000242E24,0x00514A4A44000044,0x20000402FC000000,
	0x2A08080000003F40,0x0012241224000808,0x0000000609090600,0x0008000000001818,
	0x02023E4030000000,0x0900000E010E0100,0x3C3C3C0000000A0D,0x000000000000003C,
};

///////////////////////////////////////////////////////////////////////////////
extern void CreateEmptyTexture(int itex, int xs, int ys, int zs, int icoltype);
static unsigned fontid;

///////////////////////////////////////////////////////////////////////////////
static void printg_init()
{
	int j, x, y, xsiz, ysiz, *iptr, *tbuf;

	//Load 6x8(x256) font
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

	fontid = MAXUSERTEX;
	CreateEmptyTexture(fontid,xsiz,ysiz,1,KGL_NEAREST+KGL_CLAMP_TO_EDGE);
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,xsiz,ysiz,GL_BGRA_EXT,GL_UNSIGNED_BYTE,tbuf);

	free((void*)tbuf);
}

///////////////////////////////////////////////////////////////////////////////
static double myprintg(double dx, double dy, double dfcol, char *fmt, ...)
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

	if ((!usearbasm) && (!usearbasmonly))
	{
		((PFNGLUSEPROGRAMPROC)glfp[glUseProgram])(0);
	}
	else
	{
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}

	if (glfp[glActiveTexture])
		((PFNGLACTIVETEXTUREPROC)glfp[glActiveTexture])(GL_TEXTURE0);

	glPushAttrib(GL_ENABLE_BIT | GL_MODELVIEW);
	glGetDoublev(GL_CURRENT_COLOR, ocol);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, oglxres, oglyres, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glColor3ub((fcol >> 16) & 255, (fcol >> 8) & 255, fcol & 255);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fontid);
	glBegin(GL_QUADS);
	intab = 0;

	for(cptr = st; cptr[0]; x += 6)
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

		glTexCoord2f(0.0f,((float)ich) / 256.0f);
		glVertex2i(x, y);
		glTexCoord2f(0.75f,((float)ich) / 256.0f);
		glVertex2i(x+6, y);
		glTexCoord2f(0.75f,((float)(ich + 1))/256.0f);
		glVertex2i(x+6, y+8);
		glTexCoord2f(0.0f,((float)(ich + 1))/256.0f);
		glVertex2i(x, y+8);
	}

	glEnd();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glColor4dv(ocol);
	glPopAttrib();

	if (usearbasm || usearbasmonly)
	{
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
	}

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
// Tom Dobrowolski's noise algo
///////////////////////////////////////////////////////////////////////////////
static __forceinline float fgrad(int h, float x, float y, float z)
{
	switch (h & 15)
	{
		case  0: return ( x+y  );
		case  1: return (-x+y  );
		case  2: return ( x-y  );
		case  3: return (-x-y  );
		case  4: return ( x  +z);
		case  5: return (-x  +z);
		case  6: return ( x  -z);
		case  7: return (-x  -z);
		case  8: return (   y+z);
		case  9: return (  -y+z);
		case 10: return (   y-z);
		case 11: return (  -y-z);
		case 12: return ( x+y  );
		case 13: return (-x+y  );
		case 14: return (   y-z);
		case 15: return (  -y-z);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static int noisep[512], lut3m2[1024];
static void noiseinit()
{
	for (int i = 256 - 1; i >= 0; i--)
		noisep[i] = i;

	for (int i = 256 - 1; i > 0; i--)
	{
		int j = ((rand()*(i + 1))>>15);
		int k = noisep[i];
		noisep[i] = noisep[j];
		noisep[j] = k;
	}

	for (int i = 256 - 1; i >= 0; i--)
		noisep[i + 256] = noisep[i];

	for (int i = 1024 - 1; i >= 0; i--)
	{
		float f = ((float)i) / 1024.0f;
		lut3m2[i] = (int)(((3.0f - 2.0f*f)*f*f)*1024.0f);
	}
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
static double noise1d(double fx)
{
	int l[1];
	float p[1], t[1], f[2];

	dtol(fx-0.5,&l[0]);
	p[0] = fx-((float)l[0]);
	l[0] &= 255;
	t[0] = (3.0 - 2.0*p[0])*p[0]*p[0];
	f[0] = fgrad(noisep[noisep[noisep[l[0]  ]]],p[0]  ,0,0);
	f[1] = fgrad(noisep[noisep[noisep[l[0]+1]]],p[0]-1,0,0);

	return ((f[1]-f[0])*t[0] + f[0]);
}

///////////////////////////////////////////////////////////////////////////////
static double noise2d(double fx, double fy)
{
	int i, l[2], a[4];
	float p[2], t[2], f[4];
	dtol(fx-.5,&l[0]); p[0] = fx-((float)l[0]); l[0] &= 255; t[0] = (3.0 - 2.0*p[0])*p[0]*p[0];
	dtol(fy-.5,&l[1]); p[1] = fy-((float)l[1]); l[1] &= 255; t[1] = (3.0 - 2.0*p[1])*p[1]*p[1];
	i = noisep[l[0]  ]; a[0] = noisep[i+l[1]]; a[2] = noisep[i+l[1]+1];
	i = noisep[l[0]+1]; a[1] = noisep[i+l[1]]; a[3] = noisep[i+l[1]+1];
	f[0] = fgrad(noisep[a[0]],p[0]  ,p[1],0);
	f[1] = fgrad(noisep[a[1]],p[0]-1,p[1],0); p[1]--;
	f[2] = fgrad(noisep[a[2]],p[0]  ,p[1],0);
	f[3] = fgrad(noisep[a[3]],p[0]-1,p[1],0);
	f[0] = (f[1]-f[0])*t[0] + f[0];
	f[1] = (f[3]-f[2])*t[0] + f[2];
	return((f[1]-f[0])*t[1] + f[0]);
}

///////////////////////////////////////////////////////////////////////////////
static double noise3d(double fx, double fy, double fz)
{
	int i, l[3], a[4];
	float p[3], t[3], f[8];

	dtol(fx-.5,&l[0]); p[0] = fx-((float)l[0]); l[0] &= 255; t[0] = (3.0 - 2.0*p[0])*p[0]*p[0];
	dtol(fy-.5,&l[1]); p[1] = fy-((float)l[1]); l[1] &= 255; t[1] = (3.0 - 2.0*p[1])*p[1]*p[1];
	dtol(fz-.5,&l[2]); p[2] = fz-((float)l[2]); l[2] &= 255; t[2] = (3.0 - 2.0*p[2])*p[2]*p[2];
	i = noisep[l[0]  ]; a[0] = noisep[i+l[1]]; a[2] = noisep[i+l[1]+1];
	i = noisep[l[0]+1]; a[1] = noisep[i+l[1]]; a[3] = noisep[i+l[1]+1];
	f[0] = fgrad(noisep[a[0]+l[2]  ],p[0]  ,p[1]  ,p[2]);
	f[1] = fgrad(noisep[a[1]+l[2]  ],p[0]-1,p[1]  ,p[2]);
	f[2] = fgrad(noisep[a[2]+l[2]  ],p[0]  ,p[1]-1,p[2]);
	f[3] = fgrad(noisep[a[3]+l[2]  ],p[0]-1,p[1]-1,p[2]); p[2]--;
	f[4] = fgrad(noisep[a[0]+l[2]+1],p[0]  ,p[1]  ,p[2]);
	f[5] = fgrad(noisep[a[1]+l[2]+1],p[0]-1,p[1]  ,p[2]);
	f[6] = fgrad(noisep[a[2]+l[2]+1],p[0]  ,p[1]-1,p[2]);
	f[7] = fgrad(noisep[a[3]+l[2]+1],p[0]-1,p[1]-1,p[2]);
	f[0] = (f[1]-f[0])*t[0] + f[0];
	f[1] = (f[3]-f[2])*t[0] + f[2];
	f[2] = (f[5]-f[4])*t[0] + f[4];
	f[3] = (f[7]-f[6])*t[0] + f[6];
	f[0] = (f[1]-f[0])*t[1] + f[0];
	f[1] = (f[3]-f[2])*t[1] + f[2];
	return((f[1]-f[0])*t[2] + f[0]);
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglAlphaEnable(double d)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglAlphaDisable(double d)
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglQuad(double alpha)
{
	glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0,oglxres,oglyres,0,-1,1);
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	if (alpha == 0.0) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); }
	if (alpha == 1.0) { glDisable(GL_BLEND); }

	glBegin(GL_QUADS);
	glTexCoord2f(0,1); glVertex2f(      0,      0);
	glTexCoord2f(1,1); glVertex2f(oglxres,      0);
	glTexCoord2f(1,0); glVertex2f(oglxres,oglyres);
	glTexCoord2f(0,0); glVertex2f(      0,oglyres);
	glEnd();

	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW); glPopMatrix();
	glPopAttrib();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static double __cdecl setshader_int(int sh0, int sh1, int sh2)
{
	char tbuf[4096];
	int i, j;

	if ((usearbasm) || (usearbasmonly))
	{
		if ((unsigned)sh0 >= (unsigned)shadn[0]) sh0 = 0;
		if ((unsigned)sh2 >= (unsigned)shadn[2]) sh2 = 0;
		((PFNGLBINDPROGRAMARBPROC)glfp[glBindProgramARB])(GL_VERTEX_PROGRAM_ARB  ,shad[0][sh0]);
		((PFNGLBINDPROGRAMARBPROC)glfp[glBindProgramARB])(GL_FRAGMENT_PROGRAM_ARB,shad[2][sh2]);
		return(0.0);
	}

	for(i=0;i<shadprogn;i++)
		if ((shadprogi[i].v == sh0) && (shadprogi[i].g == sh1) && (shadprogi[i].f == sh2))
		{
			if (!shadprogi[i].ishw) { gcurshader = 0; return(-1.0); }
			((PFNGLUSEPROGRAMPROC)glfp[glUseProgram])(shadprog[i]); gcurshader = i; return(0.0);
		}
	if (shadprogn >= PROGMAX) return(0.0); //silent error :/

	if ((unsigned)sh0 >= (unsigned)shadn[0]) sh0 = 0;
	if ((unsigned)sh1 >= (unsigned)shadn[1]) sh1 =-1;
	if ((unsigned)sh2 >= (unsigned)shadn[2]) sh2 = 0;

	gcurshader = shadprogn;
	shadprogi[shadprogn].v = sh0;
	shadprogi[shadprogn].g = sh1;
	shadprogi[shadprogn].f = sh2;
	shadprogi[shadprogn].ishw = 1;

	shadprog[shadprogn] = ((PFNGLCREATEPROGRAMPROC)glfp[glCreateProgram])();
					  ((PFNGLATTACHSHADERPROC)glfp[glAttachShader])(shadprog[shadprogn],shad[0][sh0]);
	if (sh1 >= 0) ((PFNGLATTACHSHADERPROC)glfp[glAttachShader])(shadprog[shadprogn],shad[1][sh1]);
					  ((PFNGLATTACHSHADERPROC)glfp[glAttachShader])(shadprog[shadprogn],shad[2][sh2]);

	if ((sh1 >= 0) && (glfp[glProgramParameteri]))
	{
		//Example: @g,GL_TRIANGLES,GL_TRIANGLE_STRIP,1024:myname
		//glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS,&n); //2048
		//glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES,&n); //1024
		//glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS,&n); //1024
		i = geo2blocki[sh1];
		((PFNGLPROGRAMPARAMETERIEXTPROC)glfp[glProgramParameteri])(shadprog[shadprogn],GL_GEOMETRY_INPUT_TYPE_EXT,tsec[i].geo_in); //GL_POINTS/GL_LINES/GL_LINES_ADJACENCY/GL_TRIANGLES/GL_TRIANGLES_ADJACENCY
		((PFNGLPROGRAMPARAMETERIEXTPROC)glfp[glProgramParameteri])(shadprog[shadprogn],GL_GEOMETRY_OUTPUT_TYPE_EXT,tsec[i].geo_out); //GL_POINTS/GL_LINE_STRIP/GL_TRIANGLE_STRIP
		((PFNGLPROGRAMPARAMETERIEXTPROC)glfp[glProgramParameteri])(shadprog[shadprogn],GL_GEOMETRY_VERTICES_OUT_EXT,tsec[i].geo_nverts); //min max=1024 ?
	}

	((PFNGLLINKPROGRAMPROC)glfp[glLinkProgram])(shadprog[shadprogn]);
	((PFNGLGETPROGRAMIVPROC)glfp[glGetProgramiv])(shadprog[shadprogn],GL_LINK_STATUS,&i);
	//NOTE:must get infolog anyway because driver doesn't consider running in SW an error.
	((PFNGLGETINFOLOGARBPROC)glfp[glGetInfoLogARB])(shadprog[shadprogn],sizeof(tbuf),0,tbuf);
	j = (strstr(tbuf,"software") != 0);

	if (!i || j) //the string of evil..
	{
		if (!i) kputs(tbuf,1);
		if (j) kputs("Shader won't run in HW! Execution denied. :/",1);
		shadprogi[shadprogn].ishw = 0;
		shadprogn++;
		return(-1.0);
	}

	((PFNGLUSEPROGRAMPROC)glfp[glUseProgram])(shadprog[shadprogn]);

		//Note: Get*Uniform*() must be called after glUseProgram() to work properly
	((PFNGLUNIFORM1IPROC)glfp[glUniform1i])(((PFNGLGETUNIFORMLOCATIONPROC)glfp[glGetUniformLocation])(shadprog[shadprogn],"tex0"),0);
	((PFNGLUNIFORM1IPROC)glfp[glUniform1i])(((PFNGLGETUNIFORMLOCATIONPROC)glfp[glGetUniformLocation])(shadprog[shadprogn],"tex1"),1);
	((PFNGLUNIFORM1IPROC)glfp[glUniform1i])(((PFNGLGETUNIFORMLOCATIONPROC)glfp[glGetUniformLocation])(shadprog[shadprogn],"tex2"),2);
	((PFNGLUNIFORM1IPROC)glfp[glUniform1i])(((PFNGLGETUNIFORMLOCATIONPROC)glfp[glGetUniformLocation])(shadprog[shadprogn],"tex3"),3);

	shadprogn++;
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglsetshader(double d)
{
	return(setshader_int(0,-1,(int)d));
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsetshader3(char *st0, char *st1, char *st2)
{
	int i, j, shi[3] = {-1,-1,-1};
	for(i=0;i<tsecn;i++)
	{
		j = ((int)tsec[i].typ)-1; if (j < 0) continue;
		if (((j == 0) && (!_stricmp(tsec[i].nam,st0))) ||
			 ((j == 1) && (!_stricmp(tsec[i].nam,st1))) ||
			 ((j == 2) && (!_stricmp(tsec[i].nam,st2)))) shi[j] = tsec[i].cnt;
	}
	return (setshader_int(shi[0],shi[1],shi[2]));
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsetshader2(char *st0, char *st1)
{
	return (kglsetshader3(st0,"",st1));
}

///////////////////////////////////////////////////////////////////////////////
static const int cubemapconst[6] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

///////////////////////////////////////////////////////////////////////////////
static const int cubemapindex[6] = {1,3,4,5,0,2};
static void CreateEmptyTexture(int itex, int xs, int ys, int zs, int icoltype)
{
	int i, internalFormat, format, type;

	tex[itex].tar = GL_TEXTURE_3D;

	if (zs == 1)
	{
		tex[itex].tar = GL_TEXTURE_2D;

		if (xs*6 == ys)
		{
			tex[itex].tar = GL_TEXTURE_CUBE_MAP;
			icoltype = (icoltype&~0xf00)|KGL_CLAMP_TO_EDGE;

			if ((icoltype&0xf0) >= KGL_MIPMAP)
				icoltype = (icoltype&~0xf0)|KGL_LINEAR;
		}
		else if (ys == 1)
			tex[itex].tar = GL_TEXTURE_1D;
	}

	tex[itex].sizx = xs;
	tex[itex].sizy = ys;
	tex[itex].sizz = zs;
	tex[itex].coltype = icoltype;

	glBindTexture(tex[itex].tar,itex);

	switch (icoltype&0xf0)
	{
		case KGL_LINEAR: default:
			glTexParameteri(tex[itex].tar,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			break;

		case KGL_NEAREST:
			glTexParameteri(tex[itex].tar,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			break;

		case KGL_MIPMAP0: case KGL_MIPMAP1: case KGL_MIPMAP2: case KGL_MIPMAP3:
			switch(icoltype&0xf0)
			{
				case KGL_MIPMAP0: glTexParameteri(tex[itex].tar,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST); break;
				case KGL_MIPMAP1: glTexParameteri(tex[itex].tar,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR); break;
				case KGL_MIPMAP2: glTexParameteri(tex[itex].tar,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST); break;
				case KGL_MIPMAP3: glTexParameteri(tex[itex].tar,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); break;
			}

			//#define GL_TEXTURE_MIN_LOD      0x813A
			//#define GL_TEXTURE_MAX_LOD      0x813B
			//#define GL_TEXTURE_BASE_LEVEL   0x813C
			//#define GL_TEXTURE_MAX_LEVEL    0x813D
			//#define GL_MAX_TEXTURE_LOD_BIAS 0x84FD
			//#define GL_TEXTURE_LOD_BIAS     0x8501
			//#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
			//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
			//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,1);
			//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_LOD,0);
			//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LOD,1);
			//glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_LOD_BIAS,-4);
			//glTexEnvi(GL_TEXTURE_ENV,GL_MAX_TEXTURE_LOD_BIAS,-4);
			//glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,1.0);

			glTexParameteri(tex[itex].tar,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			break;
	}

	switch(icoltype&0xf00)
	{
		case KGL_REPEAT: default:
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_S,GL_REPEAT);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_T,GL_REPEAT);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_R,GL_REPEAT);
			break;

		case KGL_MIRRORED_REPEAT:
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_R,GL_MIRRORED_REPEAT);
			break;

		case KGL_CLAMP:
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_S,GL_CLAMP);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_T,GL_CLAMP);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_R,GL_CLAMP);
			break;

		case KGL_CLAMP_TO_EDGE:
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
			glTexParameteri(tex[itex].tar,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
			break;
	}

	switch(icoltype&15)
	{
		case KGL_BGRA32: internalFormat =                   4; format =  GL_BGRA_EXT; type = GL_UNSIGNED_BYTE; break;
		case KGL_CHAR:   internalFormat =       GL_LUMINANCE8; format = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
		case KGL_SHORT:  internalFormat =      GL_LUMINANCE16; format = GL_LUMINANCE; type =GL_UNSIGNED_SHORT; break;
		case KGL_INT:    internalFormat = GL_LUMINANCE32I_EXT; format = GL_LUMINANCE; type =  GL_UNSIGNED_INT; break;
		case KGL_FLOAT:  internalFormat = GL_LUMINANCE32F_ARB; format = GL_LUMINANCE; type =         GL_FLOAT; break;
		case KGL_VEC4:   internalFormat =      GL_RGBA32F_ARB; format =      GL_RGBA; type =         GL_FLOAT; break;
	}

	switch(tex[itex].tar)
	{
		case GL_TEXTURE_1D: glTexImage1D(tex[itex].tar,0,internalFormat,tex[itex].sizx,               0,format,type,0); break;
		case GL_TEXTURE_2D: glTexImage2D(tex[itex].tar,0,internalFormat,tex[itex].sizx,tex[itex].sizy,0,format,type,0); break;
		case GL_TEXTURE_3D: ((PFNGLTEXIMAGE3DPROC)glfp[glTexImage3D])(tex[itex].tar,0,internalFormat,tex[itex].sizx,tex[itex].sizy,tex[itex].sizz,0,format,type,0); break;
		case GL_TEXTURE_CUBE_MAP:
			for(i=0;i<6;i++) { glTexImage2D(cubemapconst[i],0,internalFormat,tex[itex].sizx,tex[itex].sizx,0,format,type,0); } break;
	}
}

///////////////////////////////////////////////////////////////////////////////
static int glastcap = 0;
double __cdecl qglCapture(double dcaptexsiz)
{
	int i;
	i = (int)dcaptexsiz; if ((i > 0) && (i <= 8192)) captexsiz = i;
	i = min(oglxres,oglyres);

	if (i < captexsiz) //FIXME:ugly hack; use FBO to support full requested size?
	{
		for (captexsiz = 1; (captexsiz <<1 ) <= i; captexsiz <<= 1);
	}

	glViewport(0,0,captexsiz,captexsiz);

	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluPerspective(45,1,0.1,1000.0);
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

	glScalef(oglyres/(float)oglxres,1,1);
	glastcap = 0;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
GLuint gmyfb = 0; // Nice article about GPGPU in shaders: http://www.mathematik.tu-dortmund.de/~goeddeke/gpgpu/tutorial.html
double __cdecl kglCapture(double dtex, double xsiz, double ysiz, double dcoltype)
{
	int itex, xs, ys, icoltype;

	itex = (int)dtex; icoltype = (int)dcoltype; if ((icoltype&15) >= KGL_NUM) icoltype &= ~15;
	if ((xsiz < 1.0) || (ysiz < 1.0) || (xsiz*ysiz > 67108864.0)) return(-1.0);
	xs = (int)xsiz; ys = (int)ysiz;
	if ((!glfp[glGenFramebuffersEXT]) || (!glfp[glBindFramebufferEXT]) || (!glfp[glFramebufferTexture2DEXT]))
	{
		kputs("Sorry, this HW doesn't support glcapture(,,,,) :/",1);
		gshadercrashed = 1; return(-1.0);
	}

	if (!gmyfb)
		((PFNGLGENFRAMEBUFFERSEXTPROC)glfp[glGenFramebuffersEXT])(1,&gmyfb); //create FBO/offscreen framebuf

	((PFNGLBINDFRAMEBUFFEREXTPROC)glfp[glBindFramebufferEXT])(GL_FRAMEBUFFER_EXT,gmyfb);

	if ((tex[itex].sizx != captexsiz) || (tex[itex].sizy != captexsiz) || (tex[itex].sizz != 1) || (tex[itex].coltype != icoltype))
		CreateEmptyTexture(itex,xs,ys,1,icoltype);

	//tex[itex].tar = GL_TEXTURE_RECTANGLE_ARB;
	//glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE); //necessary?

	//glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	((PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)glfp[glFramebufferTexture2DEXT])(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT/*0..3*/,tex[itex].tar,itex,0);

	glViewport(0,0,xs,ys);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(45,1,0.1,1000.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glScalef(ys / (float)xs, 1, 1);

	glastcap = 1;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglEndCapture(double dtex)
{
	int itex;
	itex = (int)dtex; if ((unsigned)itex >= (unsigned)MAXUSERTEX) return(-1.0);
	tex[itex].nam[0] = 0;

	if (glastcap)
	{
		glViewport(0,0,oglxres,oglyres);
		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
		((PFNGLBINDFRAMEBUFFEREXTPROC)glfp[glBindFramebufferEXT])(GL_FRAMEBUFFER_EXT,0); //restore
		return(0.0);
	}

	if ((tex[itex].sizx != captexsiz) || (tex[itex].sizy != captexsiz) || (tex[itex].sizz != 1) || (tex[itex].coltype != KGL_BGRA32))
		CreateEmptyTexture(itex,captexsiz,captexsiz,1,KGL_BGRA32);

	glBindTexture(tex[itex].tar,(int)itex);
	glCopyTexImage2D(tex[itex].tar,0,GL_RGBA,0,0,tex[itex].sizx,tex[itex].sizy,0);

	tex[itex].nam[0] = 0;

	glViewport(0,0,oglxres,oglyres);
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW); glPopMatrix();

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsettex2(double dtex, char *st, double dcolmode)
{
	int i, x, y, gotpic, itex, icolmode, leng, xsiz, ysiz;
	char *buf = NULL;

	itex = (int)dtex; if ((unsigned)itex >= (unsigned)MAXUSERTEX) return(-1.0);
	icolmode = ((int)dcolmode)&~15;
	if (strlen(st) > MAX_PATH-1) return(-2.0);
	if ((!_stricmp(tex[itex].nam,st)) && (tex[itex].coltype == icolmode)) return(0.0);
	strcpy(tex[itex].nam,st);

	gotpic = 0; xsiz = 32; ysiz = 32;

	do
	{
		if (!kzopen(st))
			break;

		leng = kzfilelength();
		buf = (char*)malloc(leng);

		if (!buf)
		{
			kzclose();
			break;
		}

		if (kzread(buf, leng) < leng)
		{
			free(buf);
			kzclose();
			break;
		}

		kzclose();
		gotpic = kpgetdim(buf,leng,&xsiz,&ysiz);
	}
	while (0);

	if ((tex[itex].sizx != xsiz) || (tex[itex].sizy != ysiz) || (tex[itex].sizz != 1) || (tex[itex].coltype != icolmode))
		CreateEmptyTexture(itex,xsiz,ysiz,1,icolmode);

	i = xsiz*ysiz*4;
	if (i > gbmpmal) { gbmpmal = i; gbmp = (char *)realloc(gbmp,gbmpmal); }

	if (!gotpic)
	{
		static const int imagenotfoundbmp[32] = //generate placeholder image
		{
			0x7ce39138,0x05145b10,0x04145510,0x3dd45110,0x0517d110,0x05145110,0x7de45138,0x00000000, //"IMAGE"
			0x01f39100,0x00445300,0x00445500,0x00445900,0x00445100,0x00445100,0x00439100,0x00000000, //" NOT "
			0x3d144e7c,0x45345104,0x45545104,0x4594513c,0x45145104,0x45145104,0x3d138e04,0x00000000, //"FOUND"
			0x00400000,0x00200000,0x00200600,0x0027c600,0x00200000,0x00200600,0x00400600,0x00000000, //" :-( "
		};

		buf = gbmp;
		for(y=0;y<ysiz;y++)
			for(x=0;x<xsiz;x++,buf+=4)
			{
				if (imagenotfoundbmp[y]&(1<<x)) { *(int *)buf = 0xf0102030; continue; }
				*(int *)gbmp = (((rand()<<15)+rand())&0x1f1f1f)+0xff506070;
			}
	}
	else
	{
		kprender(buf,leng,(INT_PTR)gbmp,tex[itex].sizx*4,min(xsiz,tex[itex].sizx),min(ysiz,tex[itex].sizy),0,0);
	}

	glBindTexture(tex[itex].tar,itex);
	switch(tex[itex].tar)
	{
		case GL_TEXTURE_2D:
			glTexSubImage2D(tex[itex].tar,0,0,0,tex[itex].sizx,tex[itex].sizy,GL_BGRA_EXT,GL_UNSIGNED_BYTE,gbmp);
			if ((icolmode&0xf0) >= KGL_MIPMAP) gluBuild2DMipmaps(tex[itex].tar,4,tex[itex].sizx,tex[itex].sizy,GL_BGRA_EXT,GL_UNSIGNED_BYTE,gbmp);
			break;
		case GL_TEXTURE_CUBE_MAP:
			for(i=0;i<6;i++) glTexSubImage2D(cubemapconst[i],0,0,0,tex[itex].sizx,tex[itex].sizx,GL_BGRA_EXT,GL_UNSIGNED_BYTE,gbmp+tex[itex].sizx*tex[itex].sizx*4*cubemapindex[i]);
			break;
	}

	if (gotpic) free(buf);

	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsettex(double dtex, char *st)
{
	return(kglsettex2(dtex,st,(double)(KGL_MIPMAP+KGL_REPEAT)));
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglgettexarray2(double dtex, double *p, double dxsiz, double dysiz, double coltype)
{
	int i, xs, ys, itex, evalvalperpix, glbyteperpix, internalFormat, format, type;

	itex = (int)dtex; if ((unsigned)itex >= (unsigned)MAXUSERTEX) return(-1.0);
	if ((dxsiz < 1.0) || (dysiz < 1.0) || (dxsiz*dysiz > 67108864.0)) return(-1.0);
	xs = (int)dxsiz; ys = (int)dysiz;
	if (xs*ys > tex[itex].sizx*tex[itex].sizy) return(-1.0);

	switch(tex[itex].coltype&15)
	{
		case KGL_BGRA32: evalvalperpix = 1; glbyteperpix = 4; break;
		case KGL_CHAR:   evalvalperpix = 1; glbyteperpix = 1; break;
		case KGL_SHORT:  evalvalperpix = 1; glbyteperpix = 2; break;
		case KGL_INT:    evalvalperpix = 1; glbyteperpix = 4; break;
		case KGL_FLOAT:  evalvalperpix = 1; glbyteperpix = 4; break;
		case KGL_VEC4:   evalvalperpix = 4; glbyteperpix =16; break;
	}

	if ((((int)p) < ((int)gevalfunc)) || (((int)p)+((xs*ys*evalvalperpix)<<3) > ((int)gevalfunc)+gevalfuncleng)) return(-1.0);

	i = xs*ys*glbyteperpix;
	if (i > gbmpmal) { gbmpmal = i; gbmp = (char *)realloc(gbmp,gbmpmal); }

	glBindTexture(tex[itex].tar,itex);
	switch(tex[itex].coltype&15)
	{
		case KGL_BGRA32: internalFormat =                   4; format =  GL_BGRA_EXT; type = GL_UNSIGNED_BYTE; break;
		case KGL_CHAR:   internalFormat =       GL_LUMINANCE8; format = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
		case KGL_SHORT:  internalFormat =      GL_LUMINANCE16; format = GL_LUMINANCE; type =GL_UNSIGNED_SHORT; break;
		case KGL_INT:    internalFormat = GL_LUMINANCE32I_EXT; format = GL_LUMINANCE; type =  GL_UNSIGNED_INT; break;
		case KGL_FLOAT:  internalFormat = GL_LUMINANCE32F_ARB; format = GL_LUMINANCE; type =         GL_FLOAT; break;
		case KGL_VEC4:   internalFormat =      GL_RGBA32F_ARB; format =      GL_RGBA; type =         GL_FLOAT; break;
	}
	glGetTexImage(tex[itex].tar,0,format,type,gbmp);

		//preferred method .. doesn't work :/
	//glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	//glReadPixels(0,0,tex[itex].sizx,tex[itex].sizy,GL_BGRA_EXT,GL_UNSIGNED_BYTE,gbmp);

	switch(tex[itex].coltype&15)
	{
		case KGL_BGRA32: for(i=xs*ys  -1;i>=0;i--) p[i] = (double)*(unsigned int *)((i<<2) + gbmp); break;
		case KGL_CHAR:   for(i=xs*ys  -1;i>=0;i--) p[i] = (double)*(unsigned char *)(i     + gbmp); break;
		case KGL_SHORT:  for(i=xs*ys  -1;i>=0;i--) p[i] = (double)*(unsigned short *)((i<<1)+gbmp); break;
		case KGL_INT:    for(i=xs*ys  -1;i>=0;i--) p[i] = (double)*(unsigned int *)((i<<2) + gbmp); break;
		case KGL_FLOAT:  for(i=xs*ys  -1;i>=0;i--) p[i] = (double)*(       float *)((i<<2) + gbmp); break;
		case KGL_VEC4:   for(i=xs*ys*4-1;i>=0;i--) p[i] = (double)*(       float *)((i<<2) + gbmp); break;
	}
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsettexarray3(double dtex, double *p, double dxsiz, double dysiz, double dzsiz, double dcoltype)
{
	int i, xs, ys, zs, itex, icoltype, evalvalperpix, glbyteperpix, internalFormat, format, type;

	itex = (int)dtex; if ((unsigned)itex >= (unsigned)MAXUSERTEX) return(-1.0);
	icoltype = (int)dcoltype;
	tex[itex].nam[0] = 0;
	if ((dxsiz < 1.0) || (dysiz < 1.0) || (dzsiz < 1.0) || (dxsiz*dysiz*dzsiz > 67108864.0)) return(-1.0);
	xs = (int)dxsiz; ys = (int)dysiz; zs = (int)dzsiz;
	if ((tex[itex].sizx != xs) || (tex[itex].sizy != ys) || (tex[itex].sizz != zs) || (tex[itex].coltype != icoltype))
		CreateEmptyTexture(itex,xs,ys,zs,icoltype);

	switch(icoltype&15)
	{
		case KGL_BGRA32: evalvalperpix = 1; glbyteperpix = 4; break;
		case KGL_CHAR:   evalvalperpix = 1; glbyteperpix = 1; break;
		case KGL_SHORT:  evalvalperpix = 1; glbyteperpix = 2; break;
		case KGL_INT:    evalvalperpix = 1; glbyteperpix = 4; break;
		case KGL_FLOAT:  evalvalperpix = 1; glbyteperpix = 4; break;
		case KGL_VEC4:   evalvalperpix = 4; glbyteperpix =16; break;
	}

	i = xs*ys*zs*glbyteperpix;
	if (i > gbmpmal) { gbmpmal = i; gbmp = (char *)realloc(gbmp,gbmpmal); }

	if ((((int)p) < ((int)gevalfunc)) || (((int)p)+((xs*ys*zs*evalvalperpix)<<3) > ((int)gevalfunc)+gevalfuncleng)) return(-1.0);

	switch(icoltype&15)
	{
		case KGL_BGRA32: for(i=xs*ys*zs  -1;i>=0;i--) *(unsigned int *)((i<<2) + gbmp) = (unsigned int)(p[i]); break;
		case KGL_CHAR:   for(i=xs*ys*zs  -1;i>=0;i--) *(unsigned char *)( i    + gbmp) = (unsigned char)(p[i]); break;
		case KGL_SHORT:  for(i=xs*ys*zs  -1;i>=0;i--) *(unsigned short *)((i<<1)+gbmp) = (unsigned short)(p[i]); break;
		case KGL_INT:    for(i=xs*ys*zs  -1;i>=0;i--) *(unsigned int *)((i<<2) + gbmp) = (unsigned int)(p[i]); break;
		case KGL_FLOAT:  for(i=xs*ys*zs  -1;i>=0;i--) *(       float *)((i<<2) + gbmp) = (       float)(p[i]); break;
		case KGL_VEC4:   for(i=xs*ys*zs*4-1;i>=0;i--) *(       float *)((i<<2) + gbmp) = (       float)(p[i]); break;
	}

	glBindTexture(tex[itex].tar,itex);
	switch(icoltype&15)
	{
		case KGL_BGRA32: internalFormat =                   4; format =  GL_BGRA_EXT; type = GL_UNSIGNED_BYTE; break;
		case KGL_CHAR:   internalFormat =       GL_LUMINANCE8; format = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
		case KGL_SHORT:  internalFormat =      GL_LUMINANCE16; format = GL_LUMINANCE; type =GL_UNSIGNED_SHORT; break;
		case KGL_INT:    internalFormat = GL_LUMINANCE32I_EXT; format = GL_LUMINANCE; type =  GL_UNSIGNED_INT; break;
		case KGL_FLOAT:  internalFormat = GL_LUMINANCE32F_ARB; format = GL_LUMINANCE; type =         GL_FLOAT; break;
		case KGL_VEC4:   internalFormat =      GL_RGBA32F_ARB; format =      GL_RGBA; type =         GL_FLOAT; break;
	}
	switch(tex[itex].tar)
	{
	case GL_TEXTURE_1D:
		glTexSubImage1D(tex[itex].tar, 0, 0, tex[itex].sizx, format, type, gbmp);
		break;

	case GL_TEXTURE_2D:
		glTexSubImage2D(tex[itex].tar, 0, 0, 0, tex[itex].sizx, tex[itex].sizy, format, type, gbmp);

		if ((icoltype & 0xf0) >= KGL_MIPMAP && (icoltype & 15) == KGL_BGRA32)
			gluBuild2DMipmaps(tex[itex].tar, 4, tex[itex].sizx, tex[itex].sizy, GL_BGRA_EXT, GL_UNSIGNED_BYTE, gbmp);

		break;

	case GL_TEXTURE_3D:
		((PFNGLTEXSUBIMAGE3DPROC)glfp[glTexSubImage3D])(tex[itex].tar, 0, 0, 0, 0, tex[itex].sizx, tex[itex].sizy, tex[itex].sizz, format, type, gbmp);;
		break;

	case GL_TEXTURE_CUBE_MAP:
		for (i = 0; i < 6; i++)
			glTexSubImage2D(cubemapconst[i], 0, 0, 0, tex[itex].sizx, tex[itex].sizx, format, type, gbmp + tex[itex].sizx*tex[itex].sizx*glbyteperpix*cubemapindex[i]);

		break;
	}

	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsettexarray2(double dtex, double *p, double dxsiz, double dysiz, double coltype) { return(kglsettexarray3(dtex,p,dxsiz,dysiz,1.0,coltype)); }
double __cdecl kglsettexarray1(double dtex, double *p, double dxsiz, double coltype)               { return(kglsettexarray3(dtex,p,dxsiz,  1.0,1.0,coltype)); }

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglBindTex(double dtex)
{
	int itex;
	itex = (int)dtex; if ((unsigned)itex >= (unsigned)MAXUSERTEX) return(-1.0);
	glBindTexture(tex[itex].tar,itex);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglActiveTex(double texunit)
{
	int itexunit = ((int)texunit)&3;
	if (glfp[glActiveTexture]) ((PFNGLACTIVETEXTUREPROC)glfp[glActiveTexture])(itexunit+GL_TEXTURE0);
	return(0);
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kgluPerspective(double fovy, double xy, double z0, double z1)
{
	glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(fovy,xy,z0,z1);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qgluLookAt(double x, double y, double z, double px, double py, double pz, double ux, double uy, double uz)
{
	gluLookAt(x,y,z,px,py,pz,ux,uy,uz);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double ksetfov(double fov) { gfov = tan(fov*PI/360.0)*atan((float)oglyres/(float)oglxres)*360.0/PI; return(gfov); }

///////////////////////////////////////////////////////////////////////////////
double kglProgramLocalParam(double ind, double a, double b, double c, double d)
{
	((PFNGLPROGRAMLOCALPARAMETER4FARBPROC)glfp[glProgramLocalParameter4fARB])(GL_VERTEX_PROGRAM_ARB,(unsigned)ind,a,b,c,d);
	((PFNGLPROGRAMLOCALPARAMETER4FARBPROC)glfp[glProgramLocalParameter4fARB])(GL_FRAGMENT_PROGRAM_ARB,(unsigned)ind,a,b,c,d);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double kglProgramEnvParam(double ind, double a, double b, double c, double d)
{
	((PFNGLPROGRAMENVPARAMETER4FARBPROC)glfp[glProgramEnvParameter4fARB])(GL_VERTEX_PROGRAM_ARB,(unsigned)ind,a,b,c,d);
	((PFNGLPROGRAMENVPARAMETER4FARBPROC)glfp[glProgramEnvParameter4fARB])(GL_FRAGMENT_PROGRAM_ARB,(unsigned)ind,a,b,c,d);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double kglGetUniformLoc(char *shadvarnam)
{
	int i;
	i = ((PFNGLGETUNIFORMLOCATIONPROC)glfp[glGetUniformLocation])(shadprog[gcurshader],shadvarnam);
	return((double)i);
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform1f(double sh, double v0)                                  { ((PFNGLUNIFORM1FPROC)glfp[glUniform1f])((int)sh,(float)v0);                               return(0.0); }
double kglUniform2f(double sh, double v0, double v1)                       { ((PFNGLUNIFORM2FPROC)glfp[glUniform2f])((int)sh,(float)v0,(float)v1);                     return(0.0); }
double kglUniform3f(double sh, double v0, double v1, double v2)            { ((PFNGLUNIFORM3FPROC)glfp[glUniform3f])((int)sh,(float)v0,(float)v1,(float)v2);           return(0.0); }
double kglUniform4f(double sh, double v0, double v1, double v2, double v3) { ((PFNGLUNIFORM4FPROC)glfp[glUniform4f])((int)sh,(float)v0,(float)v1,(float)v2,(float)v3); return(0.0); }
double kglUniform1i(double sh, double v0)                                  { ((PFNGLUNIFORM1IPROC)glfp[glUniform1i])((int)sh,(int)v0);                                 return(0.0); }
double kglUniform2i(double sh, double v0, double v1)                       { ((PFNGLUNIFORM2IPROC)glfp[glUniform2i])((int)sh,(int)v0,(int)v1);                         return(0.0); }
double kglUniform3i(double sh, double v0, double v1, double v2)            { ((PFNGLUNIFORM3IPROC)glfp[glUniform3i])((int)sh,(int)v0,(int)v1,(int)v2);                 return(0.0); }
double kglUniform4i(double sh, double v0, double v1, double v2, double v3) { ((PFNGLUNIFORM4IPROC)glfp[glUniform4i])((int)sh,(int)v0,(int)v1,(int)v2,(int)v3);         return(0.0); }

///////////////////////////////////////////////////////////////////////////////
#define MAXUNIFVALNUM 4096
double kglUniform1fv(double sh, double num, double *vals)
{
	int i, inum; float *fvals;
	inum = min((int)num,MAXUNIFVALNUM); if (inum < 0) return(0.0);
	fvals = (float *)_alloca(inum*sizeof(fvals[0])); if (!fvals) return(0.0);
	for(i=0;i<inum;i++) fvals[i] = (float)vals[i];
	((PFNGLUNIFORM1FVPROC)glfp[glUniform1fv])((int)sh,inum,fvals);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform2fv(double sh, double num, double *vals)
{
	int i, inum; float *fvals;
	inum = min((int)num,MAXUNIFVALNUM); if (inum < 0) return(0.0);
	fvals = (float *)_alloca(inum*sizeof(fvals[0])); if (!fvals) return(0.0);
	for(i=0;i<inum;i++) fvals[i] = (float)vals[i];
	((PFNGLUNIFORM2FVPROC)glfp[glUniform2fv])((int)sh,inum,fvals);
	return(0.0);
}
///////////////////////////////////////////////////////////////////////////////
double kglUniform3fv(double sh, double num, double *vals)
{
	int i, inum; float *fvals;
	inum = min((int)num, MAXUNIFVALNUM); if (inum < 0) return(0.0);
	fvals = (float *)_alloca(inum*sizeof(fvals[0])); if (!fvals) return(0.0);
	for (i = 0; i < inum; i++) fvals[i] = (float)vals[i];
	((PFNGLUNIFORM3FVPROC)glfp[glUniform3fv])((int)sh, inum, fvals);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform4fv(double sh, double num, double *vals)
{
	int i, inum; float *fvals;
	inum = min((int)num,MAXUNIFVALNUM); if (inum < 0) return(0.0);
	fvals = (float *)_alloca(inum*sizeof(fvals[0])); if (!fvals) return(0.0);
	for(i=0;i<inum;i++) fvals[i] = (float)vals[i];
	((PFNGLUNIFORM4FVPROC)glfp[glUniform4fv])((int)sh,inum,fvals);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform1iv(double sh, double num, double* vals)
{
	int i, inum; int *ivals;
	inum = min((int)num,MAXUNIFVALNUM); if (inum < 0) return(0.0);
	ivals = (int *)_alloca(inum*sizeof(ivals[0])); if (!ivals) return(0.0);
	for(i=0;i<inum;i++) ivals[i] = (int)vals[i];
	((PFNGLUNIFORM1IVPROC)glfp[glUniform1iv])((int)sh,inum,ivals);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform2iv(double sh, double num, double* vals)
{
	int i, inum;
	int* ivals;

	inum = min((int)num, MAXUNIFVALNUM);

	if (inum < 0)
		return 0.0;

	ivals = (int*)_alloca(inum*sizeof(ivals[0]));

	if (!ivals)
		return 0.0;

	for (i = 0; i < inum; i++)
		ivals[i] = (int)vals[i];

	((PFNGLUNIFORM2IVPROC)glfp[glUniform2iv])((int)sh, inum, ivals);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform3iv(double sh, double num, double* vals)
{
	int i, inum;
	int* ivals;

	inum = min((int)num, MAXUNIFVALNUM);

	if (inum < 0)
		return 0.0;

	ivals = (int*)_alloca(inum*sizeof(ivals[0]));

	if (!ivals)
		return 0.0;

	for (i = 0; i < inum; i++)
		ivals[i] = (int)vals[i];

	((PFNGLUNIFORM3IVPROC)glfp[glUniform3iv])((int)sh, inum, ivals);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform4iv(double sh, double num, double* vals)
{
	int i, inum;
	int* ivals;

	inum = min((int)num, MAXUNIFVALNUM);

	if (inum < 0)
		return 0.0;

	ivals = (int*)_alloca(inum*sizeof(ivals[0]));

	if (!ivals)
		return 0.0;

	for (i = 0; i < inum; i++)
		ivals[i] = (int)vals[i];

	((PFNGLUNIFORM4IVPROC)glfp[glUniform4iv])((int)sh, inum, ivals);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglGetAttribLoc(char *shadvarnam)
{
	int i;
	i = ((PFNGLGETATTRIBLOCATIONPROC)glfp[glGetAttribLocation])(shadprog[gcurshader], shadvarnam);
	return((double)i);
}

///////////////////////////////////////////////////////////////////////////////
double kglVertexAttrib1f(double sh, double v0)                                  { ((PFNGLVERTEXATTRIB1FPROC)glfp[glVertexAttrib1f])((int)sh,(float)v0);                               return(0.0); }
double kglVertexAttrib2f(double sh, double v0, double v1)                       { ((PFNGLVERTEXATTRIB2FPROC)glfp[glVertexAttrib2f])((int)sh,(float)v0,(float)v1);                     return(0.0); }
double kglVertexAttrib3f(double sh, double v0, double v1, double v2)            { ((PFNGLVERTEXATTRIB3FPROC)glfp[glVertexAttrib3f])((int)sh,(float)v0,(float)v1,(float)v2);           return(0.0); }
double kglVertexAttrib4f(double sh, double v0, double v1, double v2, double v3) { ((PFNGLVERTEXATTRIB4FPROC)glfp[glVertexAttrib4f])((int)sh,(float)v0,(float)v1,(float)v2,(float)v3); return(0.0); }

///////////////////////////////////////////////////////////////////////////////
double kglCullFace(double mode)
{
	int imode = (int)mode;

	if (imode == GL_NONE)
	{
		glDisable(GL_CULL_FACE);
		return 0.0;
	}

	glEnable(GL_CULL_FACE);
	glCullFace(imode);
	glFrontFace(GL_CW);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglBlendFunc(double sfactor, double dfactor)
{
	glEnable(GL_BLEND);
	glBlendFunc(sfactor, dfactor);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglEnable(double d) { glEnable(d); return(0); }
double kglDisable(double d) { glDisable(d); return(0); }

///////////////////////////////////////////////////////////////////////////////
double mysleep(double ms)
{
	int i = ((int)ms);
	i = min(max(i, 0), 1000);
	Sleep(i);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double glswapinterval(double val)
{
	((PFNWGLSWAPINTERVALEXTPROC)glfp[wglSwapIntervalEXT])((int)val);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
static int ginstartklock = 0;
double glklockstart(double _)
{
	if (supporttimerquery)
	{
		if (ginstartklock)
			((PFNGLENDQUERYPROC)glfp[glEndQuery])(GL_TIME_ELAPSED_EXT);
		else
			ginstartklock = 1;

		((PFNGLBEGINQUERYPROC)glfp[glBeginQuery])(GL_TIME_ELAPSED_EXT, queries[0]);
	}

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double glklockelapsed(double _)
{
	GLuint64EXT qdtim;
	GLuint dtim;
	GLint got;

	if (!supporttimerquery)
		return -1.0;

	if (!ginstartklock)
		return -2.0;

	ginstartklock = 0;
	((PFNGLENDQUERYPROC)glfp[glEndQuery])(GL_TIME_ELAPSED_EXT);

	do
	{
		((PFNGLGETQUERYOBJECTIVPROC)glfp[glGetQueryObjectiv])(queries[0], GL_QUERY_RESULT_AVAILABLE, &got);
	}
	while (!got);

	if (glfp[glGetQueryObjectui64vEXT])
	{
		((PFNGLGETQUERYOBJECTUI64VEXTPROC)glfp[glGetQueryObjectui64vEXT])(queries[0], GL_QUERY_RESULT, &qdtim);
		return(((double)qdtim)*1e-9);
	}

	((PFNGLGETQUERYOBJECTUIVPROC)glfp[glGetQueryObjectuiv])(queries[0], GL_QUERY_RESULT, &dtim);
	return(((double)dtim)*1e-9);
}

///////////////////////////////////////////////////////////////////////////////
double myklock(double d)
{
	__int64 q;
	int i = (int)d;
	if (!i)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&q);
		return(((double)(q - qtim0)) / ((double)qper));
	}

	if (labs(i) < 10)
	{
			//t = klock(0); //0=seconds since compile, <0=UTC time, >0=local time
			//For example: 2009070414301725 is: July 4, 2009, 2:30pm + 17.25 seconds
			//Nice test program:
			//   cls(0); moveto(0,100);
			//   for(i=-9;i<=9;i++) printf("klock(%+2g) = %f\n",i,klock(i));
		SYSTEMTIME tim;
		if (i < 0)
		{
			GetSystemTime(&tim);
			i = -i;
		}
		else
			GetLocalTime(&tim);

		switch(i)
		{
			case 1:
			{
				__int64 q = ((__int64)tim.wYear        )*10000000000000I64 +
								((__int64)tim.wMonth       )*100000000000I64 +
								((__int64)tim.wDay         )*1000000000I64 +
								((__int64)tim.wHour        )*10000000I64 +
								((__int64)tim.wMinute      )*100000I64 +
								((__int64)tim.wSecond      )*1000I64 +
								((__int64)tim.wMilliseconds);
				 return(((double)q)*.001); //YYYYMMDDHHMMSS.sss
			}
			case 2: return((double)tim.wYear);
			case 3: return((double)tim.wMonth);
			case 4: return((double)tim.wDayOfWeek);
			case 5: return((double)tim.wDay);
			case 6: return((double)tim.wHour);
			case 7: return((double)tim.wMinute);
			case 8: return((double)tim.wSecond);
			case 9: return((double)tim.wMilliseconds);
		}
	}

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
static HMIDIOUT hmidoplaynote = 0;
static void playnoteuninit() { if ((((long)hmidoplaynote)+1)&0xfffffffe) { midiOutClose(hmidoplaynote); hmidoplaynote = 0; } }
double myplaynote(double chn, double frq, double vol)
{
	if (hmidoplaynote == (HMIDIOUT)-1)
		return -1.0;

	if (hmidoplaynote == 0)
	{
		if (midiOutOpen(&hmidoplaynote, MIDI_MAPPER, 0, 0, 0) != MMSYSERR_NOERROR)
		{
			hmidoplaynote = (HMIDIOUT)-1;
			return -1.0;
		}
	}

	midiOutShortMsg(hmidoplaynote, (min(max((int)vol, 0), 127) << 16) +
		(min(max((int)frq, 0), 127) << 8) +
		(min(max((int)chn, 0), 255)));
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
#define MAXZIPS 256 //FIXME: should be dynamic allocation!
static char zipnam[MAXZIPS][MAX_PATH+4];
static long numzips = 0;
double mykzaddstack(char *filnam)
{
	long i;

	for (i = numzips - 1; i >= 0; i--)
	{
		if (!_stricmp(zipnam[i], filnam))
			return 0.0;
	}

	i = strlen(filnam);

	if (numzips >= MAXZIPS || i >= MAX_PATH+4)
		return -1.0;

	memcpy(&zipnam[numzips], filnam, i + 1);
	numzips++;
	kzaddstack(filnam);
	return(0.0);
}

///////////////////////////////////////////////////////////////////////////////
extern void ksrand(long);

double mysrand(double val)
{
	ksrand((int)val);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
// Parse script for '@' lines, generating list of sections
static int txt2sec(char* t, tsec_t *ltsec)
{
	int i, j, i0, ntyp, n, slast[4], scnt[4], olin, lin;
	char* cptr;

	for (i = 4 - 1; i >= 0; i--)
	{
		slast[i] = -1;
		scnt[i] = 0;
	}

	i0 = 0;
	ntyp = 0;
	n = 0;
	olin = 0;
	lin = 0;
	ltsec[0].nam[0] = 0;

	for (i = 0; t[i]; i++)
	{
		if (t[i] == '/')
		{
			if (t[i+1] == '/')
			{
				for (i += 2; (t[i]) && (t[i] != '\n'); i++)
					;

				lin++;
			}
			else if (t[i+1] == '*')
			{
				for (i += 2; t[i] && (t[i] != '*' || t[i + 1] != '/'); i++)
				{
					if (t[i] == '\n')
						lin++;
				}

				if (!t[i])
					break;

				i++;
			}
		}
		else if (t[i] == '\n')
			lin++;
		else if (t[i] == '@' && (!i || (t[i-1] == '\r' || t[i-1] == '\n')))
		{
			ltsec[n].i0 = i0;
			ltsec[n].i1 = i;
			ltsec[n].typ = ntyp;
			ltsec[n].cnt = scnt[ntyp];
			ltsec[n].linofs = olin;
			ltsec[n].nxt = -1;

			if (slast[ntyp] >= 0)
				ltsec[slast[ntyp]].nxt = n;

			slast[ntyp] = n;
			scnt[ntyp]++;

			i++; if (t[i] == 'h') ntyp = 0;
			else if (t[i] == 'v') ntyp = 1;
			else if (t[i] == 'g') ntyp = 2;
			else if (t[i] == 'f') ntyp = 3;
			else i--; //reuse ntyp
			i++;

			if ((ntyp == 2) && (t[i] == ',') && (n < TSECMAX-1)) //read options for geometry shader
			{
				i++;
				ltsec[n+1].geo_in     = GL_TRIANGLES;
				ltsec[n+1].geo_out    = GL_TRIANGLE_STRIP;
				ltsec[n+1].geo_nverts = 8; //NOTE:default is 0 in specification

				if ((strlen(&t[i]) >=  9) && (!_memicmp(&t[i],"GL_POINTS"                 , 9))) { i +=  9; ltsec[n+1].geo_in = GL_POINTS; }
				if ((strlen(&t[i]) >=  8) && (!_memicmp(&t[i],"GL_LINES"                  , 8))) { i +=  8; ltsec[n+1].geo_in = GL_LINES; }
				if ((strlen(&t[i]) >= 18) && (!_memicmp(&t[i],"GL_LINES_ADJACENCY"        ,18))) { i += 22; ltsec[n+1].geo_in = GL_LINES_ADJACENCY_EXT; }
				if ((strlen(&t[i]) >= 22) && (!_memicmp(&t[i],"GL_LINES_ADJACENCY_EXT"    ,22))) { i += 22; ltsec[n+1].geo_in = GL_LINES_ADJACENCY_EXT; }
				if ((strlen(&t[i]) >= 12) && (!_memicmp(&t[i],"GL_TRIANGLES"              ,12))) { i += 12; ltsec[n+1].geo_in = GL_TRIANGLES; }
				if ((strlen(&t[i]) >= 22) && (!_memicmp(&t[i],"GL_TRIANGLES_ADJACENCY"    ,22))) { i += 26; ltsec[n+1].geo_in = GL_TRIANGLES_ADJACENCY_EXT; }
				if ((strlen(&t[i]) >= 26) && (!_memicmp(&t[i],"GL_TRIANGLES_ADJACENCY_EXT",26))) { i += 26; ltsec[n+1].geo_in = GL_TRIANGLES_ADJACENCY_EXT; }
				if (t[i] == ',')
				{
					i++;
					if ((strlen(&t[i]) >=  9) && (!_memicmp(&t[i],"GL_POINTS"                 , 9))) { i +=  9; ltsec[n+1].geo_out = GL_POINTS; }
					if ((strlen(&t[i]) >= 13) && (!_memicmp(&t[i],"GL_LINE_STRIP"             ,13))) { i += 13; ltsec[n+1].geo_out = GL_LINE_STRIP; }
					if ((strlen(&t[i]) >= 17) && (!_memicmp(&t[i],"GL_TRIANGLE_STRIP"         ,17))) { i += 17; ltsec[n+1].geo_out = GL_TRIANGLE_STRIP; }
					if (t[i] == ',')
					{
						i++;
						ltsec[n+1].geo_nverts = strtol(&t[i],&cptr,0); //~1..1024
						i = cptr-t;
					}
				}
			}

			if (t[i] == ':' && n < TSECMAX-1)
			{
				j = 0;

				for (i++; t[i] && t[i]!='\r' && t[i]!='\n'; i++)
				{
					if (t[i] == '/' && t[i+1] == '/')
						break;

					if (j < sizeof(ltsec[0].nam)-1)
					{
						ltsec[n+1].nam[j] = t[i];
						j++;
					}
				}

				while (j > 0 && ltsec[n+1].nam[j-1] == ' ')
					j--;

				ltsec[n+1].nam[j] = 0;
			}
			else
				ltsec[n+1].nam[0] = 0;

			while ((t[i]) && (t[i] != '\n'))
				i++;

			lin++;
			olin = lin;
			i0 = i+1;

			n++;

			if (n >= TSECMAX)
				return(n);
		}
	}

	ltsec[n].i0 = i0;
	ltsec[n].i1 = i;
	ltsec[n].typ = ntyp;
	ltsec[n].cnt = scnt[ntyp];
	ltsec[n].linofs = olin;
	ltsec[n].nxt = -1;

	if (slast[ntyp] >= 0)
		ltsec[slast[ntyp]].nxt = n;

	n++;

	return(n);
}

///////////////////////////////////////////////////////////////////////////////
static void glsl_geterrorlines(char* error, int offs)
{
	int i, j;

	for (i = 0; error[i]; i++)
	{
		if (i && error[i-1] != '\n') continue; // Check only at beginning of lines

		if (!memcmp(&error[i], "0(",2)) // NVIDIA style
		{
			j = atol(&error[i + 2]) - 1; if ((unsigned)j >= (unsigned)textsiz) continue;
			j += offs; badlinebits[j >> 3] |= (1 << (j & 7));
		}
		else if (!memcmp(&error[i], "(",1)) // NVIDIA style (old)
		{
			j = atol(&error[i + 1]) - 1; if ((unsigned)j >= (unsigned)textsiz) continue;
			j += offs; badlinebits[j >> 3] |= (1 << (j & 7));
		}
		else if (!memcmp(&error[i], "ERROR: ",7)) // ATI(AMD)/Intel style
		{
			i += 7;
			if (!((error[i] >= '0') && (error[i] <= '9'))) { continue; } i++;
			while ((error[i] >= '0') && (error[i] <= '9')) i++;
			if (error[i] != ':') { continue; } i++;
			j = atol(&error[i]) - 1; if ((unsigned)j >= (unsigned)textsiz) continue;
			j += offs; badlinebits[j >> 3] |= (1 << (j & 7));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
extern void updatelines(int);

static void setShaders(HWND h, HWND hWndEdit)
{
	static const char shadnam[3][5] = { "vert", "geom", "frag" };
	static const int shadconst[3] = { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER_EXT, GL_FRAGMENT_SHADER };
	int i, j, k, compiled, needloop = 0, needrecompile, tseci;
	char ch;
	char* cptr;
	char tbuf[4096];
	const char* erst;

	if (gshaderstuck) return;

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

	if (dorecompile&2)
	{
		dorecompile &= ~2;
		needrecompile = 1;
	}
	else if (popts.compctrlent)
		needrecompile = 0;
	else
	{
		needrecompile = 0;

		for(tseci = 0; tseci < tsecn; tseci++)
		{
			//Compare block
			if (!tsec[tseci].typ)
				continue;

			if ((tseci >= otsecn) || (tsec[tseci].typ != otsec[tseci].typ))
			{
				needrecompile = 1;
				break;
			}

			if (tsec[tseci].i1-tsec[tseci].i0 != otsec[tseci].i1-otsec[tseci].i0)
			{
				needrecompile = 1;
				break;
			}

			if (memcmp(&text[tsec[tseci].i0],&otext[otsec[tseci].i0],tsec[tseci].i1-tsec[tseci].i0))
			{
				needrecompile = 1;
				break;
			}
		}
	}

	if (!needrecompile)
		return;

	memset(badlinebits, 0, (textsiz + 7) >> 3);

	if (usearbasm || usearbasmonly)
	{
		((PFNGLBINDPROGRAMARBPROC)glfp[glBindProgramARB])(GL_FRAGMENT_PROGRAM_ARB, 0);
		((PFNGLBINDPROGRAMARBPROC)glfp[glBindProgramARB])(GL_VERTEX_PROGRAM_ARB, 0);
		for (i = 0; i <= 2; i += 2)
		{
			for (; shadn[i] > 0; shadn[i]--)
				((PFNGLDELETEPROGRAMSARBPROC)glfp[glDeleteProgramsARB])(1, (const GLuint*)&shad[i][shadn[i] - 1]);
		}
	}
	else
	{
		//((PFNGLDETACHSHADERPROC)glfp[glDetachShader])(shadprog[i],shad[2][i]); //necessary?
		//((PFNGLDETACHSHADERPROC)glfp[glDetachShader])(shadprog[i],shad[0][i]);

		for (i = 0; i < 3; i++)
		{
			for (; shadn[i] > 0; shadn[i]--)
				((PFNGLDELETESHADERPROC)glfp[glDeleteShader])(shad[i][shadn[i] - 1]);
		}

		for(; shadprogn > 0; shadprogn--)
			((PFNGLDELETEPROGRAMPROC)glfp[glDeleteProgram])(shadprog[shadprogn - 1]);
	}

	for(tseci = 0; tseci < tsecn; tseci++)
	{
		if (!tsec[tseci].typ)
			continue;

		//Compare block
		//if ((tseci >= otsecn) || (tsec[tseci].typ != otsec[tseci].typ)) needrecompile = 1;
		//else if (tsec[tseci].i1-tsec[tseci].i0 != otsec[tseci].i1-otsec[tseci].i0) needrecompile = 1;
		//else if (memcmp(&text[tsec[tseci].i0],&otext[otsec[tseci].i0],tsec[tseci].i1-tsec[tseci].i0)) needrecompile = 1;
		//else needrecompile = 0;

		j = tsec[tseci].typ-1;

		if ((text[tsec[tseci].i0] == '!' && text[tsec[tseci].i0+1] == '!') || usearbasmonly)
		{
			if ((!usearbasm) && (!usearbasmonly))
				((PFNGLUSEPROGRAMPROC)glfp[glUseProgram])(0);

			usearbasm = 1;
			glGetError(); //flush errors (could be from script)

			if (tsec[tseci].nam[0])
				sprintf(tbuf, "compile %s_asm %s", shadnam[j], tsec[tseci].nam);
			else
				sprintf(tbuf, "compile %s_asm#%d", shadnam[j], tsec[tseci].cnt);

			if (tsec[tseci].typ&1)
				kputs(tbuf, 1);

			if (tsec[tseci].typ == 1)
				i = GL_VERTEX_PROGRAM_ARB;
			else if (tsec[tseci].typ == 3)
				i = GL_FRAGMENT_PROGRAM_ARB;
			else
				i = 0;

			if (i)
			{
				glEnable(i);
				((PFNGLGENPROGRAMSARBPROC)glfp[glGenProgramsARB])(1, (GLuint*)&j);
				((PFNGLBINDPROGRAMARBPROC)glfp[glBindProgramARB])(i, j);

				if (i == GL_VERTEX_PROGRAM_ARB)
					shad[0][shadn[0]] = j;
				else
					shad[2][shadn[2]] = j;

				((PFNGLPROGRAMSTRINGARBPROC)glfp[glProgramStringARB])(i, GL_PROGRAM_FORMAT_ASCII_ARB, tsec[tseci].i1 - tsec[tseci].i0, &text[tsec[tseci].i0]);

				if (glGetError() != GL_NO_ERROR)
				{
					erst = (const char*) glGetString(GL_PROGRAM_ERROR_STRING_ARB);
					kputs(erst,1);

					for(j = 0; erst[j]; j++)
					{
						if ((j) && (erst[j-1] != '\n'))
							continue;

						if (!_memicmp(&erst[j], "line ", 5))
							k = atol(&erst[j + 5]) - 1;
						else if (!_memicmp(&erst[j], "Error, line ", 12))
							k = atol(&erst[j + 12]) - 1;
						else
							continue;

						if ((unsigned)k < (unsigned)textsiz)
							badlinebits[(tsec[tseci].linofs + k) >> 3] |= (1 << ((tsec[tseci].linofs + k) & 7));
					}

					updatelines(1);
					return;
				}
			}
		}
		else if (j == 1 && !glfp[glProgramParameteri])
		{
			kputs("ERROR: Geometry shader not supported by this hardware :/",1);
		}
		else
		{
			usearbasm = 0;

			if (tsec[tseci].nam[0])
				sprintf(tbuf, "compile %s %s", shadnam[j], tsec[tseci].nam);
			else
				sprintf(tbuf, "compile %s#%d", shadnam[j], tsec[tseci].cnt);

			kputs(tbuf, 1);

			if (j == 1)
				geo2blocki[tsec[tseci].cnt] = tseci; //map shader to block for geometry (to access geo_in, geo_out, geo_nverts)

			i = ((PFNGLCREATESHADERPROC)(glfp[glCreateShader]))(shadconst[j]);
			shad[j][shadn[j]] = i;
			cptr = &text[tsec[tseci].i0];
			ch = text[tsec[tseci].i1];
			text[tsec[tseci].i1] = 0;
			((PFNGLSHADERSOURCEPROC)glfp[glShaderSource])(i, 1, (const GLchar**)&cptr, 0);
			text[tsec[tseci].i1] = ch;

			((PFNGLCOMPILESHADERPROC)glfp[glCompileShader])(i);
			((PFNGLGETSHADERIVPROC)glfp[glGetShaderiv])(i,GL_COMPILE_STATUS,&compiled);

			if (!compiled)
			{
				((PFNGLGETINFOLOGARBPROC)glfp[glGetInfoLogARB])(i,sizeof(tbuf),0,tbuf); kputs(tbuf,1);
				glsl_geterrorlines(tbuf,tsec[tseci].linofs);
				updatelines(1);
				return;
			}
		}

		shadn[tsec[tseci].typ-1]++;
	}

	gcurshader = 0;
	setshader_int(0,-1,0);
	updatelines(1);
}

///////////////////////////////////////////////////////////////////////////////
// This cover function protects SOME cases
static EXCEPTION_RECORD gexception_record;
static CONTEXT gexception_context;
static int myexception_getaddr(LPEXCEPTION_POINTERS pxi)
{
	memcpy(&gexception_record,pxi->ExceptionRecord,sizeof(EXCEPTION_RECORD));
	memcpy(&gexception_context, pxi->ContextRecord, sizeof(CONTEXT));
	return(EXCEPTION_EXECUTE_HANDLER);
}

///////////////////////////////////////////////////////////////////////////////
void safeevalfunc()
{
	__try
	{
		gevalfunc();
	}
	__except(myexception_getaddr(GetExceptionInformation()))
	{
		char tbuf[256];
		sprintf(tbuf, "\nShader Exception 0x%08x @ 0x%08x :/", gexception_record.ExceptionCode, gexception_record.ExceptionAddress);
		kputs(tbuf, 1);
		gshadercrashed = 1;
		MessageBeep(16); //evil
	}
}

///////////////////////////////////////////////////////////////////////////////
static void Draw(HWND hWnd, HWND hWndEdit)
{       
    static double kglcolorbufferbit   = GL_COLOR_BUFFER_BIT;
    static double kgldepthbufferbit   = GL_DEPTH_BUFFER_BIT;
    static double kglstencilbufferbit = GL_STENCIL_BUFFER_BIT;
	static double kgl_points     = 0.0, kgl_lines = 1.0, kgl_line_loop  = 2.0;
	static double kgl_line_strip = 3.0, kgl_tris  = 4.0, kgl_tri_strip  = 5.0;
	static double kgl_tri_fan    = 6.0, kgl_quads = 7.0, kgl_quad_strip = 8.0;
	static double kgl_polygon    = 9.0, kgl_texture0 = GL_TEXTURE0;
	static double kgl_lines_adjacency = 10.0, kgl_line_strip_adjacency = 11.0;
	static double kgl_triangles_adjacency = 12.0, kgl_triangle_strip_adjacency = 13.0;
	static double kgl_none = GL_NONE, kgl_front = GL_FRONT, kgl_back = GL_BACK, kgl_frontback = GL_FRONT_AND_BACK;
	static double kglzero             = GL_ZERO          , kglone                   = GL_ONE;
	static double kglsrccolor         = GL_SRC_COLOR     , kgloneminussrccolor      = GL_ONE_MINUS_SRC_COLOR;
	static double kgldstcolor         = GL_DST_COLOR     , kgloneminusdstcolor      = GL_ONE_MINUS_DST_COLOR;
	static double kglsrcalpha         = GL_SRC_ALPHA     , kgloneminussrcalpha      = GL_ONE_MINUS_SRC_ALPHA;
	static double kgldstalpha         = GL_DST_ALPHA     , kgloneminusdstalpha      = GL_ONE_MINUS_DST_ALPHA;
	static double kglconstantcolor    = GL_CONSTANT_COLOR, kgloneminusconstantcolor = GL_ONE_MINUS_CONSTANT_COLOR;
	static double kglconstantalpha    = GL_CONSTANT_ALPHA, kgloneminusconstantalpha = GL_ONE_MINUS_CONSTANT_ALPHA;
	static double kglsrcalphasaturate = GL_SRC_ALPHA_SATURATE;
	static double kgldepthtest = GL_DEPTH_TEST;
	static double kglbgra32 = KGL_BGRA32, kglchar = KGL_CHAR, kglshort = KGL_SHORT, kglint = KGL_INT, kglfloat = KGL_FLOAT, kglvec4 = KGL_VEC4;
	static double kgllinear = KGL_LINEAR, kglnearest = KGL_NEAREST;
	static double kglmipmap0 = KGL_MIPMAP0, kglmipmap1 = KGL_MIPMAP1, kglmipmap2 = KGL_MIPMAP2, kglmipmap3 = KGL_MIPMAP3;
	static double kglrepeat = KGL_REPEAT, kglclamp = KGL_CLAMP, kglclamptoedge = KGL_CLAMP_TO_EDGE;

	static double dxres, dyres, dmousx, dmousy;
	static evalextyp myext[] =
	{
		{"GL_COLOR_BUFFER_BIT"     ,&kglcolorbufferbit   },
		{"GL_DEPTH_BUFFER_BIT"     ,&kgldepthbufferbit   },
		{"GL_STENCIL_BUFFER_BIT"   ,&kglstencilbufferbit },
    
		{"GL_POINTS"          ,&kgl_points    },
		{"GL_LINES"           ,&kgl_lines     },
		{"GL_LINE_LOOP"       ,&kgl_line_loop },
		{"GL_LINE_STRIP"      ,&kgl_line_strip},
		{"GL_TRIANGLES"       ,&kgl_tris      },
		{"GL_TRIANGLE_STRIP"  ,&kgl_tri_strip },
		{"GL_TRIANGLE_FAN"    ,&kgl_tri_fan   },
		{"GL_QUADS"           ,&kgl_quads     }, //x
		{"GL_QUAD_STRIP"      ,&kgl_quad_strip}, //x
		{"GL_POLYGON"         ,&kgl_polygon   }, //x

		{"GL_LINES_ADJACENCY"         ,&kgl_lines_adjacency},
		{"GL_LINE_STRIP_ADJACENCY"    ,&kgl_line_strip_adjacency},
		{"GL_TRIANGLES_ADJACENCY"     ,&kgl_triangles_adjacency},
		{"GL_TRIANGLE_STRIP_ADJACENCY",&kgl_triangle_strip_adjacency},
		{"GL_LINES_ADJACENCY_EXT"         ,&kgl_lines_adjacency},
		{"GL_LINE_STRIP_ADJACENCY_EXT"    ,&kgl_line_strip_adjacency},
		{"GL_TRIANGLES_ADJACENCY_EXT"     ,&kgl_triangles_adjacency},
		{"GL_TRIANGLE_STRIP_ADJACENCY_EXT",&kgl_triangle_strip_adjacency},

        {"GLCLEAR()"          ,qglClear       }, //X?
	 	{"GLBEGIN()"          ,qglBegin       }, //X?
		{"GLEND()"            ,qglEnd         }, //X?

		{"GLVERTEX(,)"        ,qglVertex2d    }, //X?
		{"GLVERTEX(,,)"       ,qglVertex3d    }, //X?
		{"GLVERTEX(,,,)"      ,qglVertex4d    }, //X?
		{"GLTEXCOORD(,)"      ,qglTexCoord2d  }, //X?
		{"GLTEXCOORD(,,)"     ,qglTexCoord3d  }, //X?
		{"GLTEXCOORD(,,,)"    ,qglTexCoord4d  }, //X?
		{"GLCOLOR(,,)"        ,qglColor3d     }, //X?
		{"GLCOLOR(,,,)"       ,qglColor4d     }, //X?
		{"GLNORMAL(,,)"       ,qglNormal3d    }, //X?

		{"GLPROGRAMLOCALPARAM(,,,,)",kglProgramLocalParam}, //for arb asm
		{"GLPROGRAMENVPARAM(,,,,)",kglProgramEnvParam}, //for arb asm

		{"GLGETUNIFORMLOC($)",kglGetUniformLoc},
		{"GLUNIFORM1F(,)"     ,kglUniform1f   },
		{"GLUNIFORM2F(,,)"    ,kglUniform2f   },
		{"GLUNIFORM3F(,,,)"   ,kglUniform3f   },
		{"GLUNIFORM4F(,,,,)"  ,kglUniform4f   },
		{"GLUNIFORM1I(,)"     ,kglUniform1i   },
		{"GLUNIFORM2I(,,)"    ,kglUniform2i   },
		{"GLUNIFORM3I(,,,)"   ,kglUniform3i   },
		{"GLUNIFORM4I(,,,,)"  ,kglUniform4i   },
		{"GLUNIFORM1FV(,,&)"  ,kglUniform1fv  },
		{"GLUNIFORM2FV(,,&)"  ,kglUniform2fv  },
		{"GLUNIFORM3FV(,,&)"  ,kglUniform3fv  },
		{"GLUNIFORM4FV(,,&)"  ,kglUniform4fv  },
		{"GLUNIFORM1IV(,,&)"  ,kglUniform1iv  },
		{"GLUNIFORM2IV(,,&)"  ,kglUniform2iv  },
		{"GLUNIFORM3IV(,,&)"  ,kglUniform3iv  },
		{"GLUNIFORM4IV(,,&)"  ,kglUniform4iv  },

		{"GLGETATTRIBLOC($)"  ,kglGetAttribLoc},
		{"GLVERTEXATTRIB1F(,)",kglVertexAttrib1f},
		{"GLVERTEXATTRIB2F(,,)",kglVertexAttrib2f},
		{"GLVERTEXATTRIB3F(,,,)" ,kglVertexAttrib3f},
		{"GLVERTEXATTRIB4F(,,,,)",kglVertexAttrib4f},

		{"GLPUSHMATRIX()"     ,qglPushMatrix  }, //x
		{"GLPOPMATRIX()"      ,qglPopMatrix   }, //x
		{"GLMULTMATRIX(&)"    ,qglMultMatrixd }, //x
		{"GLTRANSLATE(,,)"    ,qglTranslated  }, //x
		{"GLROTATE(,,,)"      ,qglRotated     }, //x
		{"GLSCALE(,,)"        ,qglScaled      }, //x
		{"GLUPERSPECTIVE(,,,)",kgluPerspective},
		{"GLULOOKAT(,,,,,,,,)",qgluLookAt     }, //?
		{"SETFOV()"           ,ksetfov        },

		{"KGL_BGRA32"         ,&kglbgra32     },
		{"KGL_CHAR"           ,&kglchar       },
		{"KGL_SHORT"          ,&kglshort      },
		{"KGL_INT"            ,&kglint        },
		{"KGL_FLOAT"          ,&kglfloat      },
		{"KGL_VEC4"           ,&kglvec4       },
		{"KGL_LINEAR"         ,&kgllinear     },
		{"KGL_NEAREST"        ,&kglnearest    },
		{"KGL_MIPMAP"         ,&kglmipmap3    },
		{"KGL_MIPMAP0"        ,&kglmipmap0    },
		{"KGL_MIPMAP1"        ,&kglmipmap1    },
		{"KGL_MIPMAP2"        ,&kglmipmap2    },
		{"KGL_MIPMAP3"        ,&kglmipmap3    },
		{"KGL_REPEAT"         ,&kglrepeat     },
		{"KGL_CLAMP"          ,&kglclamp      },
		{"KGL_CLAMP_TO_EDGE"  ,&kglclamptoedge},
		{"GLCAPTURE()"        ,qglCapture     }, //?
		{"GLCAPTURE(,,,)"     ,kglCapture     }, //?
		{"GLCAPTUREEND()"     ,qglEndCapture  }, //?
		{"GLSETTEX(,$)"       ,kglsettex      }, //?
		{"GLSETTEX(,$,)"      ,kglsettex2     }, //?
		{"GLSETTEX(,&,,)"     ,kglsettexarray1}, //?
		{"GLSETTEX(,&,,,)"    ,kglsettexarray2}, //?
		{"GLSETTEX(,&,,,,)"   ,kglsettexarray3}, //?
		{"GLGETTEX(,&,,,)"    ,kglgettexarray2}, //?
		{"GLQUAD()"           ,qglQuad        }, //?
		{"GLBINDTEXTURE()"    ,qglBindTex     },
		{"GL_TEXTURE0"        ,&kgl_texture0  },
		{"GLACTIVETEXTURE()"  ,kglActiveTex   },
		{"GLTEXTDISABLE()"    ,qglEndTex      }, //Deprecated:it's a nop!
		{"GL_NONE"            ,&kgl_none      },
		{"GL_FRONT"           ,&kgl_front     },
		{"GL_BACK"            ,&kgl_back      },
		{"GL_FRONT_AND_BACK"  ,&kgl_frontback },
		{"GLCULLFACE()"       ,kglCullFace    },

		{"GL_ZERO"                    ,&kglzero                 }, //dst default
		{"GL_SRC_COLOR"               ,&kglsrccolor             },
		{"GL_DST_COLOR"               ,&kgldstcolor             },
		{"GL_SRC_ALPHA"               ,&kglsrcalpha             }, //src for transparency
		{"GL_DST_ALPHA"               ,&kgldstalpha             },
		{"GL_CONSTANT_COLOR"          ,&kglconstantcolor        },
		{"GL_CONSTANT_ALPHA"          ,&kglconstantalpha        },
		{"GL_SRC_ALPHA_SATURATE"      ,&kglsrcalphasaturate     },
		{"GL_ONE"                     ,&kglone                  }, //src default
		{"GL_ONE_MINUS_SRC_COLOR"     ,&kgloneminussrccolor     },
		{"GL_ONE_MINUS_DST_COLOR"     ,&kgloneminusdstcolor     },
		{"GL_ONE_MINUS_SRC_ALPHA"     ,&kgloneminussrcalpha     }, //dst for transparency
		{"GL_ONE_MINUS_DST_ALPHA"     ,&kgloneminusdstalpha     },
		{"GL_ONE_MINUS_CONSTANT_COLOR",&kgloneminusconstantcolor},
		{"GL_ONE_MINUS_CONSTANT_ALPHA",&kgloneminusconstantalpha},
		{"GLBLENDFUNC(,)"     ,kglBlendFunc   },
		{"GL_DEPTH_TEST"      ,&kgldepthtest  },
		{"GLENABLE()"         ,kglEnable      },
		{"GLDISABLE()"        ,kglDisable     },

		{"GLALPHAENABLE()"    ,qglAlphaEnable }, //Deprecated:use glBlendFunc/glEnable(GL_DEPTH_TEST) instead
		{"GLALPHADISABLE()"   ,qglAlphaDisable}, //Deprecated:use glBlendFunc/glDisable(GL_DEPTH_TEST) instead

		{"GLLINEWIDTH()"      ,qglLineWidth   },

		{"GLSETSHADER()"      ,qglsetshader   }, //vshad=  0 ,gshad= -1 ,fshad=[ 0] (old style)
		{"GLSETSHADER($,$)"   ,kglsetshader2  }, //vshad=[$0],gshad= -1 ,fshad=[$1]
		{"GLSETSHADER($,$,$)" ,kglsetshader3  }, //vshad=[$0],gshad=[$1],fshad=[$2]

		{"GLKLOCKSTART()"     ,glklockstart   },
		{"GLKLOCKELAPSED()"   ,glklockelapsed },
		{"KLOCK()"            ,myklock        },
		{"NUMFRAMES"          ,&dnumframes    },

		{"XRES"               ,&dxres         },
		{"YRES"               ,&dyres         },
		{"MOUSX"              ,&dmousx        },
		{"MOUSY"              ,&dmousy        },
		{"BSTATUS"            ,&dbstatus      },
		{"KEYSTATUS[256]"     ,dkeystatus     },

		{"RGB(,,)"            ,kmyrgb         },     //convert r,g,b to 24-bit col
		{"RGBA(,,,)"          ,kmyrgba        },     //convert r,g,b,a to 32-bit col
		{"NOISE()"            ,noise1d        },     //x     (Tom's noise function)
		{"NOISE(,)"           ,noise2d        },     //x,y   (Tom's noise function)
		{"NOISE(,,)"          ,noise3d        },     //x,y,z (Tom's noise function)
		{"PRINTF($,.)"        ,myprintf       },
		{"PRINTG(,,,$,.)"     ,myprintg       },
		{"SRAND()"            ,mysrand        },
		{"SLEEP()"            ,mysleep        },
		{"GLSWAPINTERVAL()"   ,glswapinterval },
		{"PLAYNOTE(,,)"       ,myplaynote     },
		{"MOUNTZIP($)"        ,mykzaddstack   },
	};

	POINT p0, p1;
	int i, needrecompile, tseci;
	char ch;

	// Find host block (use only last one if multiple found)
	for (tseci = 0; tsec[tseci].typ; tseci++)
	{
		if (tseci >= tsecn)
			return;
	}

	while (tsec[tseci].nxt >= 0)
		tseci = tsec[tseci].nxt;

	if ((tseci >= otsecn) || (otsec[tseci].typ) || (otsec[tseci].nxt >= 0))
		needrecompile = 1;
	else if (tsec[tseci].i1-tsec[tseci].i0 != otsec[tseci].i1-otsec[tseci].i0)
		needrecompile = 1;
	else if (memcmp(&text[tsec[tseci].i0],&otext[otsec[tseci].i0],tsec[tseci].i1-tsec[tseci].i0))
		needrecompile = 1;
	else
		needrecompile = 0;

	if (popts.compctrlent)
		needrecompile = 0;

	if (dorecompile&1)
	{
		dorecompile &= ~1;
		needrecompile = 1;
	}

	if (needrecompile)
	{
		gshadercrashed = 0;

		qglAlphaDisable(0.0);
		kglCullFace(GL_NONE);

		QueryPerformanceCounter((LARGE_INTEGER*)&qtim0);
		dnumframes = 0.0;

		kputs("Compile host program: ", 0);

		if (gevalfunc)
			kasm87free(gevalfunc);

		kasm87addext(myext, sizeof(myext) / sizeof(myext[0]));

		ch = text[tsec[tseci].i1];
		text[tsec[tseci].i1] = 0;
		gevalfunc = (double (__cdecl*)()) kasm87(&text[tsec[tseci].i0]);
		text[tsec[tseci].i1] = ch;
		gevalfuncleng = kasm87leng;

		//NOTE: use tsec[tseci].linofs as offset when adding support for line of error

		kasm87addext(nullptr, 0);

		songtime = 0;

		if (gthand)
		{
			for (i = 0; i < 3; i++)
				ResetEvent(ghevent[i]);
		}
	}

	dxres = (double)oglxres;
	dyres = (double)oglyres;
	p0.x = p0.y = 0;
	ClientToScreen(hWndDraw, &p0);
	GetCursorPos(&p1);
	dmousx = p1.x - p0.x;
	dmousy = p1.y - p0.y;

	if (gevalfunc != nullptr && !gshaderstuck && !gshadercrashed)
	{
		if (needrecompile)
			kputs("OK", 1);

		qglsetshader(0);

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
		safeevalfunc();
		SetEvent(ghevent[1]);

		if (WaitForSingleObject(ghevent[2], 1000) == WAIT_TIMEOUT)
		{
			gshaderstuck = 1; MessageBeep(16); //evil
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

				sprintf(buf, "%s \\\\.\\pipe\\txtbuf /scrolly=%d /setsel0=%d /setsel1=%d /savfil=%s", gexefullpath, scrolly, setsel0, setsel1, gsavfilnam);
				CreateProcess(0, buf, 0, 0, 1, CREATE_NEW_CONSOLE, 0, 0, &si, &pi);

				if (ConnectNamedPipe(hpipe, 0))
				{
					u = strlen(text);

					if (!WriteFile(hpipe, text, u, &u, 0))
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
		if (needrecompile)
			kputs(kasm87err, 1);
	}

	dnumframes++;
}

///////////////////////////////////////////////////////////////////////////////
extern void SaveFile(HWND);
static int passasksave()
{
	if (!SendMessage(hWndEdit, EM_GETMODIFY, 0, 0))
		return 1;

	switch (MessageBox(ghwnd, "Save changes?", prognam, MB_YESNOCANCEL))
	{
		case IDYES: SaveFile(ghwnd); return 1;
		case IDNO: return 1;
		case IDCANCEL: break;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void Load(char* filename, HWND hWndEdit)
{
	FILE* fil;
	int i, j, k, ind[4], leng, fileformat;
	char* buf = 0;

	if (!passasksave())
		return;

	fil = fopen(filename, "rb");

	if (fil)
	{
		if (!_memicmp(filename, "\\\\.\\pipe\\", 9)) //load from pipe instead of file
		{
			leng = 0; i = 0; // For pipes, file size is not known in advance

			while (!ferror(fil))
			{
				k = fgetc(fil);

				if (k == EOF)
					break;

				j = i;
				i = (k == '\r' || k == '\n');

				if (i < j)
				{
					text[leng] = 13;
					text[leng+1] = 10;
					leng += 2;
				}

				if (i)
					continue;

				text[leng] = k; leng++;
			}

			text[leng] = 0;
			fileformat = 3;
		}
		else
		{
			char buf5[5];

			strcpy(gsavfilnam, filename); gsavfilnamptr = 0;

			// Autodetect file format... (0:65536 byte file with tons of 0's, 1:4 null-terminated strings)
			fseek(fil, 0, SEEK_END);
			leng = ftell(fil);
			fseek(fil, 0, SEEK_SET);
			fseek(fil, leng - sizeof(buf5), SEEK_SET);
			fread(buf5, sizeof(buf5), 1, fil);
			fseek(fil, 0, SEEK_SET);

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

		switch(fileformat)
		{
			case 0:
				buf = (char*)malloc(65536);
				fread(buf, 1, 65536, fil); // vertex,fragment,eval
				sprintf(text, "%s\r\n\r\n@v: //================================\r\n\r\n%s\r\n\r\n@f: //================================\r\n\r\n%s", &buf[32768], &buf[0], &buf[16384]);

				for (i = 0; text[i]; i++)
				{
					if (text[i] > 32)
						break;
				}

				if (text[i] == '{') // Hack attempting to fix many scripts that lack () at beginning
				{
					memmove(&text[2], text, strlen(text) + 1);
					text[0] = '('; text[1] = ')';
				}

				free(buf);
				break;

			case 1:
				buf = (char*)malloc(leng);
				fread(buf, 1, leng, fil);
				i = 0;
				j = strlen(&buf[i]) + 1; ind[0] = i; i += j;
				j = strlen(&buf[i]) + 1; ind[1] = i; i += j;
				j = strlen(&buf[i]) + 1; ind[2] = i; i += j;
				j = strlen(&buf[i]) + 1; ind[3] = i; i += j;
				sprintf(text,"%s\r\n\r\n@v: //================================\r\n\r\n%s\r\n\r\n@f: //================================\r\n\r\n%s",&buf[ind[2]],&buf[ind[0]],&buf[ind[1]]);
				free(buf);
				break;

			case 2:
				fread(text, 1, leng, fil);
				text[leng] = 0;
				break;

			case 3: // same as format 2, but for pipe
				break;
		}

		fclose(fil);

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

		SetWindowText(hWndEdit, text);
	}

	//otext[0] = text[0]^1; otext[1] = 0; //force recompile and reset time

	kglActiveTex(0.0);
	qglBindTex(0.0);
	ksetfov(90.0);
	captexsiz = 512;
	dorecompile = 3;
	glLineWidth(1.0f);

	if (glfp[wglSwapIntervalEXT])
		((PFNWGLSWAPINTERVALEXTPROC)glfp[wglSwapIntervalEXT])(1);

	{
		char tbuf[512];
		sprintf(tbuf,"\nloaded %s",filename);
		kputs(tbuf, 1);
	}

	updatelines(1);
	SendMessage(hWndEdit, EM_SETMODIFY, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
static void updatelines(int force)
{
	static int firstvisline = 0;
	int i, j, k, m, n, lin, totlin;

	lin = SendMessage(hWndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

	if (lin == firstvisline && !force)
		return;

	firstvisline = lin;

	GetWindowText(hWndEdit, ttext, textsiz);

	j = 0;
	k = 1;
	totlin = 0;

	for (i = 0; true; i++)
	{
		if (ttext[i] == '@' && (!i || ttext[i-1] == '\r' || ttext[i-1] == '\n'))
			k = 0;

		if (ttext[i] == '\n' || !ttext[i])
		{
			char buf[32];

			if (lin > 0)
				lin--;
			else
			{
				if (k == 0)
				{
					for(m = 8; m > 0; m--)
						line[j++] = popts.sepchar;
				}
				else
				{
					for (m = k, n = 0; m > 0; n++, m /= 10)
						buf[n] = (m % 10) + '0';

					if (badlinebits[totlin >> 3] & (1 << (totlin&7)))
					{
						while (n < 4)
							buf[n++] = '*';

						buf[n] = '*'; n++;
					}

					for(n--; n >= 0; n--)
						line[j++] = buf[n];
				}

				line[j++] = '\r';
				line[j++] = '\n';
			}

			if (!ttext[i])
				break;

			k++;
			totlin++;
		}
	}

	line[j] = 0;
	SetWindowText(hWndLine, line);
}

///////////////////////////////////////////////////////////////////////////////
static void NewFile(int mode)
{
	if (!passasksave())
		return;

	if (!mode)
	{
		SetWindowText(hWndEdit, "");
	}
	else if (mode == 1)
	{
		SetWindowText(hWndEdit,
			"glquad(1);\r\n"
			"@v\r\n"
			"void main(){gl_Position=ftransform();}\r\n"
			"@f\r\n"
			"void main(){gl_FragColor=gl_FragCoord*.001;}");
	}
	else if (mode == 2)
	{
		SetWindowText(hWndEdit,
			"   //Host code (EVAL)\r\n"
			"if (numframes == 0)\r\n"
			"{\r\n"
			"   glsettex(0,\"earth.jpg\"); static env; env = glGetUniformLoc(\"env\");\r\n"
			"}\r\n"
			"\r\n"
			"t = klock();\r\n"
			"glbindtexture(0);\r\n"
			"glUniform4f(env,cos(t/2),sin(t/2),0,0);\r\n"
			"glUniform4f(env+1,noise(t,0.5)+1,noise(t,1.5)+1,noise(t,2.5)+1,1);\r\n"
			"glBegin(GL_QUADS);\r\n"
			"glTexCoord(0,0); glVertex(-2,-1,-6);\r\n"
			"glTexCoord(1,0); glVertex(+2,-1,-6);\r\n"
			"glTexCoord(1,1); glVertex(+2,-1,-2);\r\n"
			"glTexCoord(0,1); glVertex(-2,-1,-2);\r\n"
			"glEnd();\r\n"
			"printg(xres-64,0,0xffffff,\"%.2f fps\",numframes/t);\r\n"
			"\r\n"
			"@v:vertex_shader //================================\r\n"
			"varying vec4 p, v, c, t;\r\n"
			"varying vec3 n;\r\n"
			"void main ()\r\n"
			"{\r\n"
			"   gl_Position = ftransform();\r\n"
			"   p = gl_Position;\r\n"
			"   v = gl_Vertex;\r\n"
			"   n = gl_Normal;\r\n"
			"   c = gl_Color;\r\n"
			"   t = gl_MultiTexCoord0;\r\n"
			"}\r\n"
			"\r\n"
			"@f:fragment_shader //================================\r\n"
			"varying vec4 p, v, c, t;\r\n"
			"varying vec3 n;\r\n"
			"uniform sampler2D tex0;\r\n"
			"uniform vec4 env[2];\r\n"
			"void main ()\r\n"
			"{\r\n"
			"   gl_FragColor = texture2D(tex0,t.xy+env[0].xy)*env[1].rgba;\r\n"
			"}");
	}
	else if (mode == 3)
	{
		SetWindowText(hWndEdit,
			"   //Host code (EVAL)\r\n"
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

	kglActiveTex(0.0);
	qglBindTex(0.0);
	ksetfov(90.0);
	captexsiz = 512;
	dorecompile = 3;
	updatelines(1);
	gsavfilnam[0] = 0;
	gsavfilnamptr = 0;
	SendMessage(hWndEdit, EM_SETMODIFY, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
static void LoadFile(HWND lwnd)
{
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";

	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = lwnd;
	ofn.lpstrFilter = "Polydraw Shader Script (*.pss;*.bin)\0*.pss;*.bin\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "pss";
	if (GetOpenFileName(&ofn)) Load(ofn.lpstrFile,hWndEdit);
	shkeystatus = 0; memset(dkeystatus,0,sizeof(dkeystatus));
}

///////////////////////////////////////////////////////////////////////////////
static void Save(char* filename)
{
	FILE *fil = fopen(filename,"wb");

	if (fil)
	{
		strcpy(gsavfilnam, filename);
		gsavfilnamptr = 0;

		fwrite(text, 1, strlen(text), fil);
		fclose(fil);
		MessageBeep(48);
		SendMessage(hWndEdit, EM_SETMODIFY, 0, 0);
	}

	{
		char tbuf[512];
		sprintf(tbuf,"\nsaved %s",filename);
		kputs(tbuf,1);
	}
}

///////////////////////////////////////////////////////////////////////////////
static void SaveFile(HWND lwnd)
{
	OPENFILENAME ofn;
	char filnam[MAX_PATH] = "";

	strcpy(filnam, gsavfilnam);

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = lwnd;
	ofn.lpstrFilter = "Polydraw Shader Script (*.pss)\0*.pss\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filnam;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "pss";

	if (GetSaveFileName(&ofn))
		Save(ofn.lpstrFile);

	shkeystatus = 0; memset(dkeystatus, 0, sizeof(dkeystatus));
}

///////////////////////////////////////////////////////////////////////////////
static void resetwindows(int cmdshow);
static void updateshifts(LPARAM lParam, int mode)
{
	if (!mode)
	{
		switch (lParam&0x17f0000)
		{
			case 0x02a0000: shkeystatus &= ~(3<<16); break; //0x2a
			case 0x0360000: shkeystatus &= ~(3<<16); break; //0x36
			case 0x01d0000: shkeystatus &= ~(1<<18); break; //0x1d
			case 0x11d0000: shkeystatus &= ~(1<<19); break; //0x9d
			case 0x0380000: shkeystatus &= ~(1<<20); break; //0x38
			case 0x1380000: shkeystatus &= ~(1<<21); break; //0xb8
		}
	}
	else
	{
		switch (lParam&0x17f0000)
		{
			case 0x02a0000: shkeystatus |= (1<<16); break; //0x2a
			case 0x0360000: shkeystatus |= (1<<17); break; //0x36
			case 0x01d0000: shkeystatus |= (1<<18); break; //0x1d
			case 0x11d0000: shkeystatus |= (1<<19); break; //0x9d
			case 0x0380000: shkeystatus |= (1<<20); break; //0x38
			case 0x1380000: shkeystatus |= (1<<21); break; //0xb8
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
static void helpabout (void)
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
	MessageBox(ghwnd,tbuf,prognam,MB_OK);
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
static HWND gfind_wnd = 0;
static unsigned int gfind_msg = 0;
static FINDREPLACE gfind_fr; //Must be global/static
static char gfind_findst[256], gfind_replacest[256]; //Must be global/static and sizeof >= 80
static int gfind_inited = 0;
static int gfind_mymemcmp(char *basestart, int baseind, char *findst, int findleng, int frflags)
{
	int i;
	unsigned char ch;

	if (frflags&FR_MATCHCASE)
	{
		if (memcmp(&basestart[baseind], findst, findleng))
			return -1;
	}
	else
	{
		if (_memicmp(&basestart[baseind], findst, findleng))
			return -1;
	}

	if (!(frflags&FR_WHOLEWORD))
		return 0;

	ch = basestart[baseind+findleng];

	for(i = 2; i > 0; i--)
	{
		if (((ch >= '0') && (ch <= '9')) || (((ch-'A')&0xdf) <= 'Z'-'A'))
			return -1;

		if (!baseind)
			return 0;

		ch = basestart[baseind-1];
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static int findreplace_process(LPFINDREPLACE lpfr)
{
	HWND hwnd;
	int i, j, textleng, findleng, repleng, findsel0, findsel1;

	if (!lpfr)
		return 0;

	hwnd = lpfr->hwndOwner;

	if (lpfr->Flags&FR_DIALOGTERM)
	{
		SetFocus(hwnd);
		return 0;
	}

	if (lpfr->Flags&(FR_FINDNEXT|FR_REPLACE|FR_REPLACEALL))
	{
		GetWindowText(hwnd,ttext,textsiz); textleng = strlen(ttext);
		findleng = strlen(lpfr->lpstrFindWhat); repleng = strlen(lpfr->lpstrReplaceWith);

		SendMessage(hwnd,EM_GETSEL,(WPARAM)&findsel0,(LPARAM)&findsel1);

		if (lpfr->Flags&FR_REPLACEALL)
		{
			for (i = textleng-findleng; i >= 0; i--)
			{
				if (gfind_mymemcmp(ttext,i,lpfr->lpstrFindWhat,findleng,lpfr->Flags)) continue;
				SendMessage(hwnd,EM_SETSEL,i,i+findleng);
				SendMessage(hwnd,EM_REPLACESEL,0,(LPARAM)lpfr->lpstrReplaceWith);
				i += 1-findleng;
				if (findsel0 < i) findsel0 += repleng-findleng;
				if (findsel1 < i) findsel1 += repleng-findleng;
			}

			SendMessage(hwnd,EM_SETSEL,findsel0,findsel1);
			SendMessage(hwnd,EM_SCROLLCARET,0,0);
			SendMessage(gfind_wnd,WM_CLOSE,0,0);
			MessageBeep(0);
			return 0;
		}

		i = findsel0;

		if ((lpfr->Flags&FR_REPLACE) && (findsel1 - findsel0 == findleng) && (findsel0 <= textleng - findleng))
		{
			if (!gfind_mymemcmp(ttext, i, lpfr->lpstrFindWhat, findleng, lpfr->Flags))
			{
				SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)lpfr->lpstrReplaceWith);
				GetWindowText(hwnd, ttext, textleng); textleng = strlen(ttext);
			}
		}

		for (j = 0; j < textleng; j++)
		{
			if (lpfr->Flags&FR_DOWN)
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

			if ((i <= textleng-findleng) && (!gfind_mymemcmp(ttext,i,lpfr->lpstrFindWhat,findleng,lpfr->Flags)))
				break;
		}

		if (j >= textleng)
		{
			SendMessage(hwnd,EM_SETSEL,findsel0,findsel0);
			MessageBeep(0);
			return 0;
		}

		SendMessage(hwnd,EM_SETSEL,i,i+findleng);
		SendMessage(hwnd,EM_SCROLLCARET,0,0);

		{ // Move find/replace dialog out of way if covering highlighted text
			RECT rfind, redit;
			POINT p0, pchar;

			GetWindowRect(gfind_wnd,&rfind);

			p0.x = p0.y = 0; ClientToScreen(hwnd,&p0);
			SendMessage(hwnd,EM_GETRECT,0,(LPARAM)&redit);
			j = SendMessage(hwnd,EM_POSFROMCHAR,i,0); pchar.x = LOWORD(j); pchar.y = HIWORD(j);
			j = ((labs(popts.fontheight)*20)>>4); // Estimated character height

			if ((p0.y+pchar.y+j >= rfind.top) && (p0.y+pchar.y < rfind.bottom))
			{
				if ((p0.y+pchar.y + (j >> 1))*2 < rfind.top+rfind.bottom)
					  MoveWindow(gfind_wnd,rfind.left,p0.y+pchar.y+j,rfind.right-rfind.left,rfind.bottom-rfind.top,1);
				else
					MoveWindow(gfind_wnd,rfind.left,p0.y+pchar.y+rfind.top-rfind.bottom,rfind.right-rfind.left,rfind.bottom-rfind.top,1);
			}
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void findreplace(HWND hwnd, int isreplace)
{
	if (!gfind_msg)
	{
		gfind_msg = RegisterWindowMessage(FINDMSGSTRING);
		gfind_findst[0] = 0; gfind_replacest[0] = 0;

		gfind_fr.lStructSize = sizeof(gfind_fr);
		gfind_fr.hwndOwner = hwnd;
		gfind_fr.hInstance = ghinst;
		gfind_fr.Flags = FR_DOWN;
		gfind_fr.lpstrFindWhat    = gfind_findst;
		gfind_fr.lpstrReplaceWith = gfind_replacest;
		gfind_fr.wFindWhatLen     = sizeof(gfind_findst);
		gfind_fr.wReplaceWithLen  = sizeof(gfind_replacest);
		gfind_fr.lCustData = 0;
		gfind_fr.lpfnHook = 0;
		gfind_fr.lpTemplateName = 0;

	}

	//|FR_DOWN     |FR_NOUPDOWN   |FR_HIDEUPDOWN
	//|FR_WHOLEWORD|FR_NOWHOLEWORD|FR_HIDEWHOLEWORD
	//|FR_MATCHCASE|FR_NOMATCHCASE|FR_HIDEMATCHCASE
	//|FR_FINDNEXT|FR_REPLACE|FR_REPLACEALL
	//|FR_DIALOGTERM|FR_SHOWHELP|FR_ENABLEHOOK
	//|FR_ENABLETEMPLATE|FR_ENABLETEMPLATEHANDLE
	gfind_fr.Flags &= ~FR_DIALOGTERM; //Need this to prevent bad things on 2nd call

	if (!isreplace)
		gfind_wnd = FindText(&gfind_fr);
	else
		gfind_wnd = ReplaceText(&gfind_fr);
}

///////////////////////////////////////////////////////////////////////////////
static void findnext(int isnext)
{
	if ((!gfind_findst[0]) || (!gfind_msg))
		return;

	gfind_fr.Flags &= ~(FR_DIALOGTERM|FR_REPLACE|FR_REPLACEALL);
	gfind_fr.Flags |= FR_FINDNEXT;

	if (isnext)
		gfind_fr.Flags |= FR_DOWN;
	else
		gfind_fr.Flags &=~FR_DOWN;

	findreplace_process(&gfind_fr);
}

#if 0
///////////////////////////////////////////////////////////////////////////////
// Hacks to make text editor nicer :)
static LRESULT (CALLBACK *ohWndEdit)(HWND, UINT, WPARAM, LPARAM);
static const int g_TabWidth = 4;

static LRESULT CALLBACK nhWndEdit(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int isoverwrite = 0;
	static int malcaret = 0;
	int i, j, k;

	switch (msg)
	{
	case WM_KEYUP:
		updateshifts(lParam, 0);
		i = ((lParam >> 16) & 127) + ((lParam >> 17) & 128);

		if (dkeystatus[i] != 0.0)
			dkeystatus[i] = 0.0;

		break;

	case WM_KEYDOWN:
		updateshifts(lParam, 1);
		i = ((lParam >> 16) & 127) + ((lParam >> 17) & 128);

		if (i == 1 && gmehax)
			PostQuitMessage(0);

		if (dkeystatus[i] == 0.0)
			dkeystatus[i] = 1.0;

		if ((wParam & 255) == VK_F1)
		{
			helpabout();
			return 0;
		}

		if (popts.fullscreen)
			return 0;

		if ((wParam&255) == VK_F3)
		{
			findnext((shkeystatus & 0x30000) == 0);
			return 0;
		}

		if (((wParam&255) == VK_INSERT) && (!shkeystatus))
		{
			isoverwrite = !isoverwrite;

			if (malcaret)
			{
				HideCaret(hWndEdit);
				DestroyCaret();
			}
			else
				malcaret = 1;

			// Caret size is total hack guess!
			j = ((labs(popts.fontheight) * 20) >> 4);
			i = labs(popts.fontwidth);

			if (!i)
				i = ((j * 9) >> 4);

			CreateCaret(hWndEdit, 0, isoverwrite*i, j);
			ShowCaret(hWndEdit);
		}

		if ((wParam & 255) == 0xbb && (shkeystatus & 0xc0000)) //Ctrl+'='
		{
			int i0, i1;
			SendMessage(hWndEdit, EM_GETSEL, (unsigned)&i0, (unsigned)&i1);

			if (i0 < i1)
			{
				GetWindowText(hWndEdit, ttext, textsiz);

				if (eval_highlight(&ttext[i0], i1-i0))
					MessageBeep(64);
				else
					MessageBeep(16);
			}
		}

		break;

		case WM_SYSCHAR:
			if ((wParam & 255) == VK_RETURN)
			{
				popts.fullscreen = !popts.fullscreen;
				CheckMenuItem(gmenu, MENU_FULLSCREEN, popts.fullscreen*MF_CHECKED);
				resetwindows(SW_NORMAL);
				return(0);
			}

			break;

		case WM_CHAR:
			if ((wParam & 255) == 10) // Ctrl+Enter
			{
				dorecompile = 3;
				return(0);
			}

			if (shkeystatus & 0x3c0000)
			{
				if ((wParam & 255) == 0x0c) // Ctrl+L
				{
					LoadFile(ghwnd);
					return 0;
				}

				if ((wParam & 255) == 0x13) // Ctrl+S
				{
					if (gsavfilnam[0])
						Save(gsavfilnam);
					else
						SaveFile(ghwnd);
					return 0;
				}

				if ((wParam & 255) == 0x06) // Ctrl+F
				{
					findreplace(hWndEdit, 0);
					shkeystatus = 0;
					return 0;
				}

				if ((wParam & 255) == 0x12) // Ctrl+R
				{
					findreplace(hWndEdit, 1);
					shkeystatus = 0;
					return 0;
				}
			}

			if (popts.fullscreen)
				return 0;

			if ((wParam & 255) == '\t') // Tab and Shift+Tab
			{
				int i0, i1, l, l0, l1;
				SendMessage(hWndEdit, EM_GETSEL, (unsigned)&i0, (unsigned)&i1);

				if (i0 >= i1) // Tab with no highlight..
				{
				    if (!(shkeystatus & 0x30000)) // Tab..
                    {
						SendMessage(hWndEdit, EM_SETSEL, i0, i0);
						//SendMessage(hWndEdit, EM_REPLACESEL, 1, (LPARAM)"    ");
						SendMessage(hWndEdit, EM_REPLACESEL, 1, (LPARAM)"\t");
                    }
                    else // Shift+Tab..
                    {
						char buf[2048];
						*((long*)&buf[0]) = 2048;

						l = SendMessage(hWndEdit, EM_LINEFROMCHAR, i0, 0);
						k = SendMessage(hWndEdit, EM_GETLINE, l, (LPARAM)buf);
						j = SendMessage(hWndEdit, EM_LINEINDEX, l, 0);

						i = (i0 - j - 1);

						if (buf[i] == '\t')
							--i;
						else // spaces
						{
							for (; i >= 0 && i < k && (i0 - j - i) <= g_TabWidth; --i)
							{
								if (buf[i] != ' ')
									break;
							}
						}

						SendMessage(hWndEdit, EM_SETSEL, i + j + 1, i0);
						SendMessage(hWndEdit, EM_REPLACESEL, 1, (LPARAM)"");
                    }
				}
				else // Tab and Shift+Tab with highlight..
				{
					l0 = SendMessage(hWndEdit, EM_LINEFROMCHAR, i0, 0);
					l1 = SendMessage(hWndEdit, EM_LINEFROMCHAR, i1 - 1, 0);

					for(l = l0; l <= l1; l++) // for each line
					{
						j = SendMessage(hWndEdit, EM_LINEINDEX, l, 0);

						if (!(shkeystatus & 0x30000)) // Tab..
						{
							/*
							i1 += g_TabWidth;

							if (j < i0)
								i0 += g_TabWidth;
							*/
							++i1;

							SendMessage(hWndEdit, EM_SETSEL, j, j);
							//SendMessage(hWndEdit, EM_REPLACESEL, 1, (LPARAM)"    ");
							SendMessage(hWndEdit, EM_REPLACESEL, 1, (LPARAM)"\t");
						}
						else // Shift+Tab..
						{
							char buf[max(4, 3)];
							*((long*)&buf[0]) = g_TabWidth;
							k = SendMessage(hWndEdit, EM_GETLINE, l, (LPARAM)buf);
							i = 1;

							if (buf[0] == '\t')
								--i1;
							else // spaces
							{
								for (i = 0; i < k; i++)
								{
									if (buf[i] != ' ')
										break;
								}

								i1 -= i;

								if (j < i0)
									i0 -= i;
							}

							SendMessage(hWndEdit, EM_SETSEL, j, j + i);
							SendMessage(hWndEdit, EM_REPLACESEL, 1, (LPARAM)"");
						}
					}

					SendMessage(hWndEdit, EM_SETSEL, i0, i1);
				}

				return 0;
			}

			if (isoverwrite && (wParam & 255) >= 0x20) //See: http://www.jeffluther.net/unify/Tech-Newsletter/pdf/1999/0499-6.pdf
			{
				SendMessage(hWndEdit, EM_GETSEL, (unsigned)&i, 0); //i = caret index
				j = SendMessage(hWndEdit, EM_LINEINDEX, -1, 0);    //j = index to home of caret's line
				k = SendMessage(hWndEdit, EM_LINELENGTH, j, 0);    //j+k = index to end of caret's line

				if (i < (j + k))
					SendMessage(hWndEdit, EM_SETSEL, i, i + 1);
			}

			break;

		case WM_PAINT:
			updatelines(0);
			break;

	 //case WM_SETFOCUS: break; //Edit control has gained the input focus
		case WM_KILLFOCUS: //Edit control has lost the input focus
			if (malcaret)
			{
				malcaret = 0;
				HideCaret(hWndEdit);
				DestroyCaret();
				isoverwrite = 0;
			}

			break;

		default:
			if (gfind_msg && msg == gfind_msg)
				return(findreplace_process((LPFINDREPLACE)lParam)); // Needed for FindText/ReplaceText
	}

	return(CallWindowProc(ohWndEdit, hWnd, msg, wParam, lParam));
}

///////////////////////////////////////////////////////////////////////////////
static LRESULT (CALLBACK *ohWndCons)(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK nhWndCons(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i;
	switch (msg)
	{
		case WM_KEYUP:
			updateshifts(lParam,0);
			i = ((lParam >> 16) & 127) + ((lParam >> 17) & 128);

			if (dkeystatus[i] != 0.0)
				dkeystatus[i] = 0.0;

			break;

		case WM_KEYDOWN:
			updateshifts(lParam, 1);
			i = ((lParam >> 16) & 127) + ((lParam >> 17) & 128);
			if ((i == 1) && (gmehax)) PostQuitMessage(0);
			if (dkeystatus[i] == 0.0) dkeystatus[i] = 1.0;
			if ((wParam&255) == VK_F1) { helpabout(); return(0); }
			if ((wParam&255) == VK_F3) { findnext((shkeystatus&0x30000)==0); return(0); }
			break;

		case WM_SYSCHAR:
			if ((wParam&255) == VK_RETURN)
			{
				popts.fullscreen = !popts.fullscreen;
				CheckMenuItem(gmenu, MENU_FULLSCREEN, popts.fullscreen*MF_CHECKED);
				resetwindows(SW_NORMAL);
				return(0);
			}

			break;

		case WM_CHAR:
			if ((wParam&255) == 10) { dorecompile = 3; return(0); } //Ctrl+Enter
			if ((wParam&255) == 0x0c) { LoadFile(ghwnd); return(0); } //Ctrl+L
			if ((wParam&255) == 0x13) { if (gsavfilnam[0]) Save(gsavfilnam); else SaveFile(ghwnd); return(0); } //Ctrl+S
			if ((wParam&255) == 0x06) { findreplace(hWndEdit,0); shkeystatus = 0; return(0); } //Ctrl+F
			if ((wParam&255) == 0x12) { findreplace(hWndEdit,1); shkeystatus = 0; return(0); } //Ctrl+R
			break;
	}

	return(CallWindowProc(ohWndCons, hWnd, msg, wParam, lParam));
}

///////////////////////////////////////////////////////////////////////////////
static LRESULT (CALLBACK *ohWndLine)(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK nhWndLine(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT p0;
	int i;

	switch (msg)
	{
		case WM_SETFOCUS: // clicking line number results in focus changing to edit window
			SendMessage(hWnd, EM_GETSEL, (WPARAM)&i, 0);
			i = SendMessage(hWnd, EM_LINEFROMCHAR, i, 0) + SendMessage(hWndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

			// Must send button down here for highlight to catch correct line
			GetCursorPos(&p0); SendMessage(hWndEdit, WM_LBUTTONDOWN, MK_LBUTTON, (LPARAM)&p0);
			SetFocus(hWndEdit);

			// Place cursor at respective line of hWndEdit
			i = SendMessage(hWndEdit, EM_LINEINDEX, i, 0);
			SendMessage(hWndEdit, EM_SETSEL, i, i);

			return 0;
	}

	return(CallWindowProc(ohWndLine, hWnd, msg, wParam, lParam));
}
#endif

/// Scintilla Colors structure
struct SScintillaColors
{
	int	iItem;
	COLORREF rgb;
};

// A few basic colors
const COLORREF g_Black = RGB(0, 0, 0);
const COLORREF g_White = RGB(0xff, 0xff, 0xff);
const COLORREF g_Gray = RGB(0x1e, 0x1e, 0x1e);
const COLORREF g_Green = RGB(0, 0xff, 0);
const COLORREF g_Red = RGB(0xff, 0, 0);
const COLORREF g_Blue = RGB(0, 0, 0xff);
const COLORREF g_Yellow = RGB(0xff, 0xff, 0);
const COLORREF g_Magenta = RGB(0xff, 0, 0xff);
const COLORREF g_Cyan = RGB(0, 0xff, 0xff);

// C++ keywords
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
static SScintillaColors g_RGBSyntaxCpp[] =
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
void SetAStyle(HWND hWnd, int style, COLORREF fore, COLORREF back = RGB(0x1E, 0x1E, 0x1E), int size = -1, const char* face = nullptr)
{
	SendMessage(hWnd, SCI_STYLESETFORE, style, fore);
	SendMessage(hWnd, SCI_STYLESETBACK, style, back);

	if (size >= 1)
		SendMessage(hWnd, SCI_STYLESETSIZE, style, size);

	if (face)
		SendMessage(hWnd, SCI_STYLESETFONT, style, (LPARAM)face);
}

///////////////////////////////////////////////////////////////////////////////
static void resetwindows(int cmdshow)
{
	static int ooglxres = 0;
	static int ooglyres = 0;
	RECT r;
	int i, guiflags, x0[4], y0[4], x1[4], y1[4], linenumwid;

	i = ((labs(popts.fontheight) * 20) >> 4);
	linenumwid = labs(popts.fontwidth);

	if (!linenumwid)
		linenumwid = ((i * 9) >> 4);

	linenumwid *= 4;

	if (!ghwnd)
	{
		RECT rw;
		int x, y;

		SystemParametersInfo(SPI_GETWORKAREA, 0, &rw, 0);
		x = ((rw.right - rw.left - xres) >> 1) + rw.left;
		y = ((rw.bottom - rw.top - yres) >> 1) + rw.top;
		ghwnd = CreateWindow("PolyDraw", prognam, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX, x, y, xres, yres, 0, 0, ghinst, 0); //|WS_VISIBLE|WS_POPUPWINDOW|WS_CAPTION
		ShowWindow(ghwnd, cmdshow);
	}

	guiflags = WS_VISIBLE | WS_CHILD | WS_VSCROLL; //|WS_HSCROLL|WS_CAPTION|WS_SIZEBOX|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU;
	guiflags |= ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL;

	if (!popts.fullscreen)
	{
		oglxres = ((xres >> 1) & ~3);
		oglyres = ((oglxres * 3) >> 2);
		x1[0] = oglxres;
		y1[0] = oglyres;
		x1[1] = oglxres;
		y1[1] = yres - oglyres;
		x1[2] = linenumwid;
		y1[2] = yres;
		x1[3] = xres - oglxres - linenumwid;
		y1[3] = yres;

		if (!(popts.rendcorn & 1))
		{
			x0[0] = 0;
			x0[1] = 0;
			x0[2] = oglxres;
			x0[3] = oglxres + linenumwid;
		}
		else
		{
			x0[0] = xres - oglxres;
			x0[1] = xres - oglxres;
			x0[2] = 0;
			x0[3] = linenumwid;
		}

		if (!(popts.rendcorn & 2))
		{
			y0[0] = 0;
			y0[1] = oglyres;
			y0[2] = 0;
			y0[3] = 0;
		}
		else
		{
			y0[0] = yres - oglyres;
			y0[1] = 0;
			y0[2] = 0;
			y0[3] = 0;
		}
	}
	else
	{
		oglxres = xres;
		oglyres = yres;
		x0[0] = 0;
		y0[0] = 0;
		x1[0] = oglxres;
		y1[0] = oglyres;
		x0[1] = 0;
		y0[1] = oglyres;
		x1[1] = 0;
		y1[1] = 0;
		x0[2] = oglxres;
		y0[2] = 0;
		x1[2] = 0;
		y1[2] = 0;
		x0[3] = oglxres;
		y0[3] = 0;
		x1[3] = 0;
		y1[3] = 0;
	}

	if (hWndDraw)
		MoveWindow(hWndDraw, x0[0], y0[0], x1[0], y1[0], 1);
	else
		hWndDraw = CreateWindowEx(0, "PolyDraw", "Render", WS_VISIBLE | WS_CHILD, x0[0], y0[0], x1[0], y1[0], ghwnd, (HMENU)100, ghinst, 0);

	if (hWndCons)
		MoveWindow(hWndCons, x0[1], y0[1], x1[1], y1[1], 1);
	else
		hWndCons = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "Console", guiflags | ES_READONLY | WS_HSCROLL, x0[1], y0[1], x1[1], y1[1], ghwnd, (HMENU)101, ghinst, 0);

	if (hWndLine)
		MoveWindow(hWndLine, x0[2], y0[2], x1[2], y1[2], 1);
	else
		hWndLine = CreateWindowEx(WS_EX_WINDOWEDGE, "edit", "Lines", (guiflags | ES_READONLY | ES_RIGHT)&~WS_VSCROLL, x0[2], y0[2], x1[2], y1[2], ghwnd, (HMENU)101, ghinst, 0);

	if (hWndEdit)
		MoveWindow(hWndEdit, x0[3], y0[3], x1[3], y1[3], 1);
	else
	{
		hWndEdit = CreateWindowEx(WS_EX_WINDOWEDGE, "Scintilla", "Script", guiflags | ES_NOHIDESEL, x0[3], y0[3], x1[3], y1[3], ghwnd, (HMENU)102, ghinst, 0);

		// CPP lexer
		SendMessage(hWndEdit, SCI_SETLEXER, SCLEX_CPP, 0);

		// Set number of style bits to use
		SendMessage(hWndEdit, SCI_SETSTYLEBITS, 5, 0);

		// Set tab width
		SendMessage(hWndEdit, SCI_SETTABWIDTH, 4, 0);

		// Use CPP keywords
		SendMessage(hWndEdit, SCI_SETKEYWORDS, 0, (LPARAM)g_cppKeyWords);

		// Set up the global default style. These attributes are used wherever no explicit choices are made.
		SetAStyle(hWndEdit, STYLE_DEFAULT, g_White, g_Gray, 10, "Courier New");

		// Set caret foreground color
		SendMessage(hWndEdit, SCI_SETCARETFORE, RGB(255, 255, 255), 0);

		// Set all styles
		SendMessage(hWndEdit, SCI_STYLECLEARALL, 0, 0);

		// Set selection color
		SendMessage(hWndEdit, SCI_SETSELBACK, TRUE, RGB(0, 0, 255));

		// Set syntax colors
		for (long i = 0; g_RGBSyntaxCpp[i].iItem != -1; i++)
			SetAStyle(hWndEdit, g_RGBSyntaxCpp[i].iItem, g_RGBSyntaxCpp[i].rgb);

#if 0
		SendMessage(hWndEdit, EM_LIMITTEXT, textsiz - 1, 0);

		// See subclassing controls here: http://msdn.microsoft.com/en-us/library/bb773183.aspx
		// NOTE:replace these with SetWindowLongPtr if porting to 64-bit windows!

		ohWndCons = (LRESULT(__stdcall*)(HWND, UINT, WPARAM, LPARAM))SetWindowLong(hWndCons, GWL_WNDPROC, (long)/*(LONG_PTR)*/nhWndCons);
		ohWndEdit = (LRESULT(__stdcall*)(HWND, UINT, WPARAM, LPARAM))SetWindowLong(hWndEdit, GWL_WNDPROC, (long)/*(LONG_PTR)*/nhWndEdit);
		ohWndLine = (LRESULT(__stdcall*)(HWND, UINT, WPARAM, LPARAM))SetWindowLong(hWndLine, GWL_WNDPROC, (long)/*(LONG_PTR)*/nhWndLine);
#endif
	}

	// increase virtual size of line window
	SendMessage(hWndLine, EM_GETRECT, 0, (LPARAM)&r);
	r.left = -1000;
	r.right = linenumwid - 4;
	SendMessage(hWndLine, EM_SETRECTNP, 0, (LPARAM)&r);

	if (ooglxres != oglxres || ooglyres != oglyres)
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

	switch (msg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_LBUTTONDOWN:
			if (fmod(dbstatus, 2.0) < 1)
				dbstatus += 1;

			p0.x = p0.y = 0;
			ClientToScreen(hWndDraw, &p0);
			GetCursorPos(&p1);

			if (((unsigned)(p1.x-p0.x) < (unsigned)oglxres) && ((unsigned)(p1.y-p0.y) < (unsigned)oglyres))
				SetFocus(ghwnd);

			break;

		case WM_LBUTTONUP:   if (fmod(dbstatus,2.0) >= 1) dbstatus -= 1; break;
		case WM_RBUTTONDOWN: if (fmod(dbstatus,4.0) <  2) dbstatus += 2; break;
		case WM_RBUTTONUP:   if (fmod(dbstatus,4.0) >= 2) dbstatus -= 2; break;
		case WM_MBUTTONDOWN: if (fmod(dbstatus,8.0) <  4) dbstatus += 4; break;
		case WM_MBUTTONUP:   if (fmod(dbstatus,8.0) >= 4) dbstatus -= 4; break;
		case WM_KEYUP:
			updateshifts(lParam, 0);
			i = ((lParam >> 16) & 127) + ((lParam >> 17) & 128);

			if (dkeystatus[i] != 0.0)
				dkeystatus[i] = 0.0;

			break;

		case WM_KEYDOWN:
			updateshifts(lParam,1);
			i = ((lParam >> 16) & 127) + ((lParam >> 17) & 128);
			if ((i == 1) && (gmehax)) PostQuitMessage(0);
			if (dkeystatus[i] == 0.0) dkeystatus[i] = 1.0;
			if ((wParam&255) == VK_F1) { helpabout(); return(0); }
			if ((wParam&255) == VK_F3) { findnext((shkeystatus&0x30000)==0); return(0); }
			break;

		case WM_SYSCHAR:
			if ((wParam&255) == VK_RETURN)
			{
				popts.fullscreen = !popts.fullscreen;
				CheckMenuItem(gmenu,MENU_FULLSCREEN,popts.fullscreen*MF_CHECKED);
				resetwindows(SW_NORMAL);
				return 0;
			}

			break;

		case WM_CHAR:
			if ((wParam&255) == 10) { dorecompile = 3; return(0); } // Ctrl+Enter
			if ((wParam&255) == 0x0c) { LoadFile(ghwnd); return(0); } // Ctrl+L
			if ((wParam&255) == 0x13) { if (gsavfilnam[0]) Save(gsavfilnam); else SaveFile(ghwnd); return(0); } // Ctrl+S
			if ((wParam&255) == 0x06) { findreplace(hWndEdit,0); shkeystatus = 0; return(0); } // Ctrl+F
			if ((wParam&255) == 0x12) { findreplace(hWndEdit,1); shkeystatus = 0; return(0); } // Ctrl+R
			break;

		case WM_SIZE:
			if (hWnd != ghwnd) break;
			if ((wParam == SIZE_MAXHIDE) || (wParam == SIZE_MINIMIZED)) { ActiveApp = 0; break; }
			ActiveApp = 1;
			xres = LOWORD(lParam);
			yres = HIWORD(lParam);
			if ((oxres != xres) || (oyres != yres)) { oxres = xres; oyres = yres; resetwindows(SW_NORMAL); }
			break;

		case WM_ACTIVATEAPP: ActiveApp = (BOOL)wParam; shkeystatus = 0; break;

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
			switch(LOWORD(wParam)) // process menu
			{
			case MENU_FILENEW+0: case MENU_FILENEW+1: case MENU_FILENEW+2: case MENU_FILENEW+3:
				NewFile(LOWORD(wParam) - MENU_FILENEW);
				break;
			case MENU_FILEOPEN:    LoadFile(hWnd); break;
			case MENU_FILESAVE:    if (gsavfilnam[0]) { Save(gsavfilnam); break; } //no break intentional
			case MENU_FILESAVEAS:  SaveFile(hWnd); break;
			case MENU_FILEEXIT:    if (passasksave()) { PostQuitMessage(0); } break;
			case MENU_EDITFIND:    findreplace(hWndEdit,0); break;
			case MENU_EDITFINDNEXT: findnext(1); break;
			case MENU_EDITFINDPREV: findnext(0); break;
			case MENU_EDITREPLACE: findreplace(hWndEdit,1); break;
			case MENU_COMPCONTENT: popts.compctrlent ^= 1; if (!popts.compctrlent) dorecompile = 3; CheckMenuItem(gmenu,MENU_COMPCONTENT,popts.compctrlent*MF_CHECKED); break;
			case MENU_EVALHIGHLIGHT:
				{
					int i0, i1;
					SendMessage(hWndEdit, EM_GETSEL, (unsigned)&i0, (unsigned)&i1);

					if (i0 < i1)
					{
						GetWindowText(hWndEdit,ttext,textsiz);

						if (eval_highlight(&ttext[i0], i1 - i0))
							MessageBeep(64);
						else
							MessageBeep(16);
					}
				}

				break;

			case MENU_RENDPLC+0: case MENU_RENDPLC+1: case MENU_RENDPLC+2: case MENU_RENDPLC+3:
				popts.rendcorn = LOWORD(wParam) - MENU_RENDPLC;
				popts.fullscreen = 0;

				for (i = 0; i < 4; i++)
					CheckMenuItem(gmenu, MENU_RENDPLC + i, (LOWORD(wParam) == MENU_RENDPLC + i)*MF_CHECKED);

				CheckMenuItem(gmenu, MENU_FULLSCREEN, popts.fullscreen*MF_CHECKED);
				resetwindows(SW_NORMAL);
				break;

			case MENU_FULLSCREEN:
				popts.fullscreen = !popts.fullscreen;
				CheckMenuItem(gmenu, MENU_FULLSCREEN, popts.fullscreen*MF_CHECKED);
				resetwindows(SW_NORMAL);
				break;

			case MENU_CLEARBUFFER:
				popts.clearbuffer = !popts.clearbuffer;
				CheckMenuItem(gmenu, MENU_CLEARBUFFER, popts.clearbuffer*MF_CHECKED);
				resetwindows(SW_NORMAL);
				break;

			case MENU_FONT:
				{
					CHOOSEFONT cf;
					static LOGFONT lf;

					memset(&cf, 0, sizeof(cf));
					cf.lStructSize = sizeof(cf);
					cf.hwndOwner = hWnd;
					cf.lpLogFont = &lf;
					lf.lfHeight = popts.fontheight;
					lf.lfWidth = popts.fontwidth;
					strcpy(lf.lfFaceName,popts.fontname);
					cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_INITTOLOGFONTSTRUCT | CF_NOSTYLESEL;

					if (ChooseFont(&cf))
					{
						if (lf.lfFaceName[0])
						{
							if (hfont)
								DeleteObject(hfont);

							popts.fontheight = lf.lfHeight;
							popts.fontwidth = lf.lfWidth;
							strcpy(popts.fontname, lf.lfFaceName);

							popts.sepchar = '-'; //Many XP fonts do not have solid hyphen char :/
							//     if (!stricmp(popts.fontname,"Consolas"      )) popts.sepchar = 6; //also 151
							//else if (!stricmp(popts.fontname,"Courier"       )) popts.sepchar = 6;
							//else if (!stricmp(popts.fontname,"Courier New"   )) popts.sepchar = 151; //also 6
							//else if (!stricmp(popts.fontname,"Fixedsys"      )) popts.sepchar = 6;
							//else if (!stricmp(popts.fontname,"Lucida Console")) popts.sepchar = 6; //also 151
							//else if (!stricmp(popts.fontname,"Terminal"      )) popts.sepchar = 196;
							//else                                                popts.sepchar = '-';

							hfont = CreateFont(popts.fontheight, popts.fontwidth, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, popts.fontname);

							SendMessage(hWndCons, WM_SETFONT, (WPARAM)hfont, 0); ShowWindow(hWndCons, SW_HIDE); UpdateWindow(hWndCons); ShowWindow(hWndCons, SW_SHOWNORMAL);
							SendMessage(hWndEdit, WM_SETFONT, (WPARAM)hfont, 0); ShowWindow(hWndEdit, SW_HIDE); UpdateWindow(hWndEdit); ShowWindow(hWndEdit, SW_SHOWNORMAL);
							SendMessage(hWndLine, WM_SETFONT, (WPARAM)hfont, 0); ShowWindow(hWndLine, SW_HIDE); UpdateWindow(hWndLine); ShowWindow(hWndLine, SW_SHOWNORMAL);
							resetwindows(SW_NORMAL);
							updatelines(1);
						}
					}
				}

				break;

			case MENU_HELPTEXT:
				{
					char tbuf[MAX_PATH];
					sprintf(tbuf, "%spolydraw.txt", gexedironly);
					_spawnlp(_P_NOWAIT, "notepad.exe", "notepad.exe", tbuf, 0);
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
				case EN_UPDATE: //Edit control's contents have changed
					if ((HWND)lParam == hWndEdit)
						updatelines(1); //text changed in edit window (would be cheaper than calling strcmp :P)

					break;
			}

			break;
	}

	return DefWindowProc(hWnd,msg,wParam,lParam);
}

///////////////////////////////////////////////////////////////////////////////
static void EnableOpenGL(HWND hWnd, HDC *hDC, HGLRC *hRC)
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
static int checkext(char* extnam)
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
	return(argc);
}

///////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if (LoadLibrary("SciLexer.dll") == nullptr)
	{
		MessageBox(ghwnd, "Loading Scintilla DLL failed.  Make sure SciLexer.dll is available.", prognam, MB_OK);
		return 1;
	}

	WNDCLASS wc;
	MSG msg;
	HDC hDC;
	HGLRC hRC;
	RECT rw;

	__int64 q = 0I64, qlast = 0I64;
	int qnum = 0;
	int i, j, k, z, argc, argfilindex = -1, setsel0 = -1, setsel1 = -1, scrolly = -1;
	char buf[1024], *argv[MAX_PATH >> 1], *savfilnam = 0;

	//osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	//GetVersionEx(&osvi);

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rw, 0);
	xres = (((rw.right - rw.left) * 3) >> 2);
	yres = (((rw.bottom - rw.top) * 3) >> 2);
	nCmdShow = SW_MAXIMIZE;

	GetModuleFileName(0, gexefullpath, sizeof(gexefullpath));

	for (i = 0, j = -1; gexefullpath[i]; i++)
	{
		if (gexefullpath[i] == '\\' || gexefullpath[i] == '/')
			j = i;
	}

	strcpy(gexedironly, gexefullpath);
	gexedironly[j + 1] = 0;
	sprintf(ginifilnam, "%spolydraw.ini", gexedironly);
	loadini();

	kzaddstack(gexedironly);

	argc = cmdline2arg(lpCmdLine,argv);

	for (i = argc - 1; i > 0; i--)
	{
		if ((argv[i][0] != '/') && (argv[i][0] != '-'))
		{
			argfilindex = i;
			continue;
		}

		if (!_stricmp(&argv[i][1], "qme")) // for integration with MoonEdit
		{
			gmehax = 1;
			popts.fullscreen = 1;
			continue;
		}

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

		if ((argv[i][1] >= '0') && (argv[i][1] <= '9'))
		{
			k = 0; z = 0;

			for (j = 1; ; j++)
			{
				if ((argv[i][j] >= '0') && (argv[i][j] <= '9'))
				{
					k = (k*10+argv[i][j]-'0');
					continue;
				}

				switch (z)
				{
					case 0: xres = k; nCmdShow = SW_NORMAL; break;
					case 1: yres = k; break;
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

	ghinst = hInstance;

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
	textsiz = 65536;
	text = (char*)malloc(textsiz); if (!text) { MessageBox(ghwnd,"malloc failed",prognam,MB_OK); return 1; }
	otext = (char*)malloc(textsiz); if (!otext) { MessageBox(ghwnd, "malloc failed", prognam, MB_OK); return 1; }
	ttext = (char*)malloc(textsiz); if (!ttext) { MessageBox(ghwnd, "malloc failed", prognam, MB_OK); return 1; }
	line = (char*)malloc(textsiz); if (!line) { MessageBox(ghwnd, "malloc failed", prognam, MB_OK); return 1; }
	badlinebits = (char*)malloc((textsiz + 7) >> 3); if (!badlinebits) { MessageBox(ghwnd, "malloc failed", prognam, MB_OK); return 1; }

	memset(badlinebits,0,(textsiz+7)>>3);

	resetwindows(nCmdShow);

	hfont = CreateFont(popts.fontheight, popts.fontwidth, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, popts.fontname);
	SendMessage(hWndCons, WM_SETFONT, (WPARAM)hfont, 0);
	SendMessage(hWndEdit, WM_SETFONT, (WPARAM)hfont, 0);
	SendMessage(hWndLine, WM_SETFONT, (WPARAM)hfont, 0);

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
		sptr = menuadd(sptr, "Compile on Ctrl+Enter", popts.compctrlent*MF_CHECKED, MENU_COMPCONTENT);
		sptr = menuadd(sptr, "Evaluate highlighted text\tCtrl+'='", 0, MENU_EVALHIGHLIGHT);
		sptr = menuadd(sptr, "Select Render corner", MF_POPUP, 0);
		sptr = menuadd(sptr, "Top Left", (popts.rendcorn == 0)*MF_CHECKED, MENU_RENDPLC + 0);
		sptr = menuadd(sptr, "Top Right", (popts.rendcorn == 1)*MF_CHECKED, MENU_RENDPLC + 1);
		sptr = menuadd(sptr, "Bottom Left", (popts.rendcorn == 2)*MF_CHECKED, MENU_RENDPLC + 2);
		sptr = menuadd(sptr, "Bottom Right", (popts.rendcorn == 3)*MF_CHECKED | MF_END, MENU_RENDPLC + 3);
		sptr = menuadd(sptr, "Fullscreen Render\tAlt+Enter", (popts.fullscreen != 0)*MF_CHECKED, MENU_FULLSCREEN);
		sptr = menuadd(sptr, "Clear screen every frame", (popts.clearbuffer != 0)*MF_CHECKED, MENU_CLEARBUFFER);
		sptr = menuadd(sptr, "Select &Font..", MF_END, MENU_FONT);
		sptr = menuadd(sptr, "&Help", MF_POPUP | MF_END, 0);
		sptr = menuadd(sptr, "   PolyDraw.txt..", 0, MENU_HELPTEXT);
		sptr = menuadd(sptr, "   &About\tF1", MF_END, MENU_HELPABOUT);
		gmenu = LoadMenuIndirect(sbuf);
		SetMenu(ghwnd, gmenu);
	}

	EnableOpenGL(hWndDraw, &hDC, &hRC);

	kputs("GL_VENDOR:   ", 0);
	kputs((const char*)glGetString(GL_VENDOR), 1);
	kputs("GL_RENDERER: ", 0);
	kputs((const char*)glGetString(GL_RENDERER), 1);
	kputs("GL_VERSION:  ", 0);
	kputs((const char*)glGetString(GL_VERSION), 1);
	//if ((!checkext("GL_ARB_vertex_program")) || (!checkext("GL_ARB_fragment_program")))
	//if (checkext("GL_EXT_geometry_shader4")) ... ?

	if (!wglGetProcAddress("glCreateShaderObjectARB"))
		NewFile(3);
	else
	{
		NewFile(2);
		kputs("GLSL_VERSION:", 0);
		kputs((const char*)glGetString(0x8B8C /*GL_SHADING_LANGUAGE_VERSION*/), 1);

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
	}

	kputs("----------------------------------------", 1);

	//supporttimerquery = (checkext("GL_EXT_timer_query") || checkext("GL_ARB_timer_query"));

	for(i = 0; i < NUMGLFUNC; i++)
	{
		if (!useoldglfuncs)
		{
			glfp[i] = (glfp_t)wglGetProcAddress(glnames[i]);

			if (glfp[i])
				continue;

			useoldglfuncs = 1;
		}

		glfp[i] = (glfp_t)wglGetProcAddress(glnames_old[i]);

		if (glfp[i])
			continue;

		sprintf(buf, "%s() / %s() not supported. :/", glnames[i], glnames_old[i]);

		if (i < glCreateShader)
		{
			MessageBox(ghwnd, buf, prognam, MB_OK);
			ExitProcess(0);
		}

		if (i < glGenQueries)
		{
			kputs(buf, 1);
			kputs("NOTE: This machine is limited to ARB ASM :/", 1);
			supporttimerquery = 0;
			usearbasmonly = 1;
			break;
		}

		if (i < glGetQueryObjectui64vEXT)
		{
			supporttimerquery = 0;
			break;
		}
	}

	if (supporttimerquery)
		((PFNGLGENQUERIESPROC)glfp[glGenQueries])(1, (GLuint*)queries);

	if (glfp[wglSwapIntervalEXT])
		((PFNWGLSWAPINTERVALEXTPROC)glfp[wglSwapIntervalEXT])(1);

	noiseinit();

	text[0] = 0;
	otext[0] = 0;

	if (argfilindex >= 0)
		Load(argv[argfilindex], hWndEdit);

	SetFocus(hWndEdit);

	if (scrolly >= 0 || setsel0 >= 0 || setsel1 >= 0)
	{
		if (scrolly >= 0)
			SendMessage(hWndEdit, EM_LINESCROLL, 0, scrolly);

		if (setsel0 >= 0 && setsel1 >= 0)
			SendMessage(hWndEdit, EM_SETSEL, setsel0, setsel1);
	}

	if (savfilnam)
	{
		strcpy(gsavfilnam, savfilnam);
		gsavfilnamptr = 0;
		kputs("File name is: ", 0);
		kputs(gsavfilnam, 1);
	}

	QueryPerformanceFrequency((LARGE_INTEGER*)&qper);
	QueryPerformanceCounter((LARGE_INTEGER*)&qtim0);

	printg_init();
	//CreateEmptyTexture(0,32,32,1,KGL_BGRA32); //avoid harmless gl error at glUniform1i(..("tex0")..,0)

	while (1)
	{
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				goto quitit;

			if (gfind_wnd && IsWindow(gfind_wnd) && IsDialogMessage(gfind_wnd, &msg))
				continue; //Needed for FindText/ReplaceText (keyboard shortcuts)

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!ActiveApp)
		{
			Sleep(100);
			continue;
		}

		glClearColor(0.f, 0.f, 0.f, 0.f);

		if (popts.clearbuffer)
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glViewport(0,0,oglxres,oglyres);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(gfov, (float)oglxres / (float)oglyres, 0.1, 1000.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		GetWindowText(hWndEdit, text, textsiz);
		tsecn = txt2sec(text, tsec);

		setShaders(ghwnd, hWndEdit);

		if (shadn[2])
			Draw(ghwnd, hWndEdit);

		if (!shadn[2] || !gevalfunc)
		{
			Sleep(1);

			if (popts.rendcorn == 4 && gmehax)
			{
				CheckMenuItem(gmenu, MENU_FULLSCREEN, 0);
				popts.fullscreen = 0;
				resetwindows(SW_NORMAL);
			}
		}

		strcpy(otext, text);
		otsecn = tsecn;
		memcpy(otsec, tsec, tsecn*sizeof(tsec_t));

		SwapBuffers(hDC);

		QueryPerformanceCounter((LARGE_INTEGER *)&q);
		qnum++;

		if (q-qlast > qper || !gsavfilnamptr)
		{
			gsavfilnamptr = gsavfilnam;

			for (i = 0; gsavfilnam[i]; i++)
			{
				if (gsavfilnam[i] == '\\')
					gsavfilnamptr = &gsavfilnam[i + 1];
			}

			i = sprintf(buf, "%s", prognam);

			if (gsavfilnam[0])
				i += sprintf(&buf[i], " - %s", gsavfilnamptr);

			if (SendMessage(hWndEdit, EM_GETMODIFY, 0, 0))
				i += sprintf(&buf[i], " *");

			i += sprintf(&buf[i], " (%.1f fps)", ((double)qper)*((double)qnum) / ((double)(q - qlast)));

			qlast = q;
			qnum = 0;
			SetWindowText(ghwnd, buf);
		}
	}

quitit:
	//passasksave();

	if (supporttimerquery)
		((PFNGLDELETEQUERIESPROC)glfp[glDeleteQueries])(1, (GLuint*)queries);

	playnoteuninit();
	DisableOpenGL(hWndDraw, hDC, hRC);
	DestroyWindow(ghwnd);

	if (!gmehax)
		saveini();

	if (badlinebits)
	{
		free(badlinebits);
		badlinebits = 0;
	}

	if (line)
	{
		free(line);
		line = 0;
	}

	if (ttext)
	{
		free(ttext);
		ttext = 0;
	}

	if (otext)
	{
		free(otext);
		otext = 0;
	}

	if (text)
	{
		free(text);
		text = 0;
	}

	return 0;
}

