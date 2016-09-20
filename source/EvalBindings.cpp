
#include <windows.h>
#include <process.h>

#include <GL/glew.h>
#include <GL/GL.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <malloc.h>

#include <string>

#include "eval.hpp"
#include "PolyDraw.hpp"

///////////////////////////////////////////////////////////////////////////////
// Shadow copies of variables for binding.  Use SetEvalVars() to set them for
// the script (this keeps script from modifying real globals).
///////////////////////////////////////////////////////////////////////////////
static double s_dxres;
static double s_dyres;
static double s_dmousx;
static double s_dmousy;
static double s_dbstatus;
static double s_dkeystatus[256];

///////////////////////////////////////////////////////////////////////////////
static char* s_bmp = 0;
static int s_bmpmal = 0;

///////////////////////////////////////////////////////////////////////////////
// glu32.lib replacements..
void mygluPerspective(double fovy, double xy, double z0, double z1)
{
	fovy = tan(fovy*(PI / 360.0))*z0;
	xy *= fovy;
	glFrustum(-xy, xy, -fovy, fovy, z0, z1);
}

///////////////////////////////////////////////////////////////////////////////
void mygluLookAt(double px, double py, double pz, double fx, double fy, double fz, double ux, double uy, double uz)
{
	double t, r[3], d[3], f[3], mat[16];

	f[0] = px-fx; f[1] = py-fy; f[2] = pz-fz;
	t = 1.0/sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
	f[0] *= t; f[1] *= t; f[2] *= t;

	r[0] = f[2]*uy - f[1]*uz;
	r[1] = f[0]*uz - f[2]*ux;
	r[2] = f[1]*ux - f[0]*uy;
	t = 1.0/sqrt(r[0]*r[0] + r[1]*r[1] + r[2]*r[2]);
	r[0] *= t; r[1] *= t; r[2] *= t;

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
int mygluBuild2DMipmaps(GLenum target, GLint components, GLint xs, GLint ys, GLenum format, GLenum type, const void *data)
{
	unsigned char* wptr;
	unsigned char* rptr;
	unsigned char* rptr2;
	int i, x, y;
	int nxs, nys, xs4, nxs4;

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

		glTexImage2D(target, i, 4, nxs, nys, 0, format, type, data); // loading 1st time
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

double __cdecl qglClear       (double mask)                            { if (mask) glClear((GLbitfield)mask);
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
double __cdecl qglLineWidth   (double size)                            { glLineWidth((GLfloat)size); return(0.0); }

double __cdecl kmyrgb         (double r, double g, double b)           { return((double)((min(max((int)r,0),255)<<16) + (min(max((int)g,0),255)<<8) + min(max((int)b,0),255))); }
double __cdecl kmyrgba        (double r, double g, double b, double a) { return((double)((min(max((int)a,0),255)<<24) + (min(max((int)r,0),255)<<16) + (min(max((int)g,0),255)<<8) + min(max((int)b,0),255))); }

///////////////////////////////////////////////////////////////////////////////
// Tom Dobrowolski's noise algo
///////////////////////////////////////////////////////////////////////////////
static inline float fgrad(int h, float x, float y, float z)
{
	switch (h & 15)
	{
	case  0: return (x + y);
	case  1: return (-x + y);
	case  2: return (x - y);
	case  3: return (-x - y);
	case  4: return (x + z);
	case  5: return (-x + z);
	case  6: return (x - z);
	case  7: return (-x - z);
	case  8: return (y + z);
	case  9: return (-y + z);
	case 10: return (y - z);
	case 11: return (-y - z);
	case 12: return (x + y);
	case 13: return (-x + y);
	case 14: return (y - z);
	case 15: return (-y - z);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static int s_Noisep[512], s_LUT3m2[1024];
void noiseinit()
{
	for (int i = 256 - 1; i >= 0; i--)
		s_Noisep[i] = i;

	for (int i = 256 - 1; i > 0; i--)
	{
		int j = ((rand()*(i + 1)) >> 15);
		int k = s_Noisep[i];
		s_Noisep[i] = s_Noisep[j];
		s_Noisep[j] = k;
	}

	for (int i = 256 - 1; i >= 0; i--)
		s_Noisep[i + 256] = s_Noisep[i];

	for (int i = 1024 - 1; i >= 0; i--)
	{
		float f = ((float)i) / 1024.0f;
		s_LUT3m2[i] = (int)(((3.0f - 2.0f*f)*f*f)*1024.0f);
	}
}

///////////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER)
static inline void dtol(double f, int *a)
{
	_asm
	{
		mov eax, a
		fld f
		fistp dword ptr [eax]
	}
}
#else
static inline dtol(double f, int *a)
{
	a = (int)f;
}
#endif

///////////////////////////////////////////////////////////////////////////////
static double noise1d(double fx)
{
	int l[1];
	float p[1], t[1], f[2];

	dtol(fx - 0.5, &l[0]);
	p[0] = (float)fx - ((float)l[0]);
	l[0] &= 255;
	t[0] = (3.0f - 2.0f*p[0])*p[0] * p[0];
	f[0] = fgrad(s_Noisep[s_Noisep[s_Noisep[l[0]]]], p[0], 0, 0);
	f[1] = fgrad(s_Noisep[s_Noisep[s_Noisep[l[0] + 1]]], p[0] - 1, 0, 0);

	return ((f[1] - f[0])*t[0] + f[0]);
}

///////////////////////////////////////////////////////////////////////////////
static double noise2d(double fx, double fy)
{
	int i, l[2], a[4];
	float p[2], t[2], f[4];
	dtol(fx - 0.5, &l[0]); p[0] = (float)fx - ((float)l[0]); l[0] &= 255; t[0] = ((3.0f - 2.0f*p[0])*p[0] * p[0]);
	dtol(fy - 0.5, &l[1]); p[1] = (float)fy - ((float)l[1]); l[1] &= 255; t[1] = ((3.0f - 2.0f*p[1])*p[1] * p[1]);
	i = s_Noisep[l[0]]; a[0] = s_Noisep[i + l[1]]; a[2] = s_Noisep[i + l[1] + 1];
	i = s_Noisep[l[0] + 1]; a[1] = s_Noisep[i + l[1]]; a[3] = s_Noisep[i + l[1] + 1];
	f[0] = fgrad(s_Noisep[a[0]], p[0], p[1], 0);
	f[1] = fgrad(s_Noisep[a[1]], p[0] - 1, p[1], 0); p[1]--;
	f[2] = fgrad(s_Noisep[a[2]], p[0], p[1], 0);
	f[3] = fgrad(s_Noisep[a[3]], p[0] - 1, p[1], 0);
	f[0] = (f[1] - f[0])*t[0] + f[0];
	f[1] = (f[3] - f[2])*t[0] + f[2];
	return ((f[1] - f[0])*t[1] + f[0]);
}

///////////////////////////////////////////////////////////////////////////////
static double noise3d(double fx, double fy, double fz)
{
	int i, l[3], a[4];
	float p[3], t[3], f[8];

	dtol(fx - .5, &l[0]); p[0] = (float)fx - ((float)l[0]); l[0] &= 255; t[0] = (3.0f - 2.0f*p[0])*p[0] * p[0];
	dtol(fy - .5, &l[1]); p[1] = (float)fy - ((float)l[1]); l[1] &= 255; t[1] = (3.0f - 2.0f*p[1])*p[1] * p[1];
	dtol(fz - .5, &l[2]); p[2] = (float)fz - ((float)l[2]); l[2] &= 255; t[2] = (3.0f - 2.0f*p[2])*p[2] * p[2];
	i = s_Noisep[l[0]]; a[0] = s_Noisep[i + l[1]]; a[2] = s_Noisep[i + l[1] + 1];
	i = s_Noisep[l[0] + 1]; a[1] = s_Noisep[i + l[1]]; a[3] = s_Noisep[i + l[1] + 1];
	f[0] = fgrad(s_Noisep[a[0] + l[2]], p[0], p[1], p[2]);
	f[1] = fgrad(s_Noisep[a[1] + l[2]], p[0] - 1, p[1], p[2]);
	f[2] = fgrad(s_Noisep[a[2] + l[2]], p[0], p[1] - 1, p[2]);
	f[3] = fgrad(s_Noisep[a[3] + l[2]], p[0] - 1, p[1] - 1, p[2]); p[2]--;
	f[4] = fgrad(s_Noisep[a[0] + l[2] + 1], p[0], p[1], p[2]);
	f[5] = fgrad(s_Noisep[a[1] + l[2] + 1], p[0] - 1, p[1], p[2]);
	f[6] = fgrad(s_Noisep[a[2] + l[2] + 1], p[0], p[1] - 1, p[2]);
	f[7] = fgrad(s_Noisep[a[3] + l[2] + 1], p[0] - 1, p[1] - 1, p[2]);
	f[0] = (f[1] - f[0])*t[0] + f[0];
	f[1] = (f[3] - f[2])*t[0] + f[2];
	f[2] = (f[5] - f[4])*t[0] + f[4];
	f[3] = (f[7] - f[6])*t[0] + f[6];
	f[0] = (f[1] - f[0])*t[1] + f[0];
	f[1] = (f[3] - f[2])*t[1] + f[2];
	return ((f[1] - f[0])*t[2] + f[0]);
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
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, s_dxres, s_dyres, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

	glDisable(GL_DEPTH_TEST);

	if (alpha == 0.0) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
	if (alpha == 1.0) { glDisable(GL_BLEND); }

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1); glVertex2f(0, 0);
	glTexCoord2f(1, 1); glVertex2f(s_dxres, 0);
	glTexCoord2f(1, 0); glVertex2f(s_dxres, s_dyres);
	glTexCoord2f(0, 0); glVertex2f(0, s_dxres);
	glEnd();

	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW); glPopMatrix();
	glPopAttrib();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglsetshader(double d)
{
	return (setshader_int(0, -1, (int)d));
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsetshader3(char* st0, char* st1, char* st2)
{
	int j, shi[3] = { -1, -1, -1 };

	for (decltype(g_TextSections.size()) i = 0; i < g_TextSections.size(); i++)
	{
		j = ((int)g_TextSections[i].type) - 1;

		if (j < 0)
			continue;

		if ((j == 0 && !_stricmp(g_TextSections[i].name, st0))
		  || (j == 1 && !_stricmp(g_TextSections[i].name, st1))
		  || (j == 2 && !_stricmp(g_TextSections[i].name, st2)))
			shi[j] = g_TextSections[i].count;
	}

	return setshader_int(shi[0], shi[1], shi[2]);
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsetshader2(char* st0, char* st1)
{
	return (kglsetshader3(st0, "", st1));
}

///////////////////////////////////////////////////////////////////////////////
static const int g_CubeMapConst[6] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

///////////////////////////////////////////////////////////////////////////////
static const int s_CubeMapIndex[6] = { 1, 3, 4, 5, 0, 2 };

void CreateEmptyTexture(int itex, int xs, int ys, int zs, int icoltype)
{
	int i, internalFormat, format, type;

	g_Textures[itex].target = GL_TEXTURE_3D;

	if (zs == 1)
	{
		g_Textures[itex].target = GL_TEXTURE_2D;

		if (xs*6 == ys)
		{
			g_Textures[itex].target = GL_TEXTURE_CUBE_MAP;
			icoltype = (icoltype&~0xf00) | KGL_CLAMP_TO_EDGE;

			if ((icoltype&0xf0) >= KGL_MIPMAP)
				icoltype = (icoltype&~0xf0) | KGL_LINEAR;
		}
		else if (ys == 1)
			g_Textures[itex].target = GL_TEXTURE_1D;
	}

	g_Textures[itex].sizeX = xs;
	g_Textures[itex].sizeY = ys;
	g_Textures[itex].sizeZ = zs;
	g_Textures[itex].colorType = icoltype;

	glBindTexture(g_Textures[itex].target, itex);

	switch (icoltype & 0xf0)
	{
		case KGL_LINEAR: default:
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;

		case KGL_NEAREST:
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;

		case KGL_MIPMAP0: case KGL_MIPMAP1: case KGL_MIPMAP2: case KGL_MIPMAP3:
			switch(icoltype&0xf0)
			{
			case KGL_MIPMAP0: glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); break;
			case KGL_MIPMAP1: glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR); break;
			case KGL_MIPMAP2: glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); break;
			case KGL_MIPMAP3: glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); break;
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

			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
	}

	switch (icoltype & 0xf00)
	{
		case KGL_REPEAT: default:
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_R, GL_REPEAT);
			break;

		case KGL_MIRRORED_REPEAT:
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
			break;

		case KGL_CLAMP:
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_R, GL_CLAMP);
			break;

		case KGL_CLAMP_TO_EDGE:
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(g_Textures[itex].target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			break;
	}

	switch(icoltype & 15)
	{
		case KGL_BGRA32: internalFormat =                   4; format =  GL_BGRA_EXT; type = GL_UNSIGNED_BYTE; break;
		case KGL_CHAR:   internalFormat =       GL_LUMINANCE8; format = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
		case KGL_SHORT:  internalFormat =      GL_LUMINANCE16; format = GL_LUMINANCE; type =GL_UNSIGNED_SHORT; break;
		case KGL_INT:    internalFormat = GL_LUMINANCE32I_EXT; format = GL_LUMINANCE; type =  GL_UNSIGNED_INT; break;
		case KGL_FLOAT:  internalFormat = GL_LUMINANCE32F_ARB; format = GL_LUMINANCE; type =         GL_FLOAT; break;
		case KGL_VEC4:   internalFormat =      GL_RGBA32F_ARB; format =      GL_RGBA; type =         GL_FLOAT; break;
	}

	switch(g_Textures[itex].target)
	{
	case GL_TEXTURE_1D: glTexImage1D(g_Textures[itex].target, 0, internalFormat, g_Textures[itex].sizeX, 0, format, type, 0); break;
	case GL_TEXTURE_2D: glTexImage2D(g_Textures[itex].target, 0, internalFormat, g_Textures[itex].sizeX, g_Textures[itex].sizeY, 0, format, type, 0); break;
	case GL_TEXTURE_3D:
		glTexImage3D(g_Textures[itex].target, 0, internalFormat, g_Textures[itex].sizeX, g_Textures[itex].sizeY, g_Textures[itex].sizeZ, 0, format, type, 0);
		break;

	case GL_TEXTURE_CUBE_MAP:
		for (i = 0; i < 6; i++)
			glTexImage2D(g_CubeMapConst[i], 0, internalFormat, g_Textures[itex].sizeX, g_Textures[itex].sizeX, 0, format, type, 0);

		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
static bool g_LastCapture = false;

double __cdecl qglCapture(double dcaptexsiz)
{
	int i;
	i = (int)dcaptexsiz;

	if (i > 0 && i <= 8192)
		g_CapTextSize = i;

	i = (int)min(s_dxres, s_dyres);

	if (i < g_CapTextSize) // FIXME:ugly hack; use FBO to support full requested size?
	{
		for (g_CapTextSize = 1; (g_CapTextSize << 1) <= i; g_CapTextSize <<= 1)
			;
	}

	glViewport(0, 0, g_CapTextSize, g_CapTextSize);

	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); mygluPerspective(45, 1, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

	glScalef((GLfloat)(s_dyres / s_dxres), 1, 1);
	g_LastCapture = false;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Nice article about GPGPU in shaders: http://www.mathematik.tu-dortmund.de/~goeddeke/gpgpu/tutorial.html

static GLuint g_FrameBuffer = 0;

double __cdecl kglCapture(double dtex, double xsiz, double ysiz, double dcoltype)
{
	int itex, xs, ys, icoltype;

	itex = (int)dtex;
	icoltype = (int)dcoltype;

	if ((icoltype & 15) >= KGL_NUM)
		icoltype &= ~15;

	if (xsiz < 1.0 || ysiz < 1.0 || (xsiz*ysiz) > 67108864.0)
		return -1.0;

	xs = (int)xsiz;
	ys = (int)ysiz;

	//if ((!g_glfp[glGenFramebuffersEXT]) || (!g_glfp[glBindFramebufferEXT]) || (!g_glfp[glFramebufferTexture2DEXT]))
	//{
	//	kputs("Sorry, this HW doesn't support glcapture(,,,,) :/",1);
	//	gshadercrashed = 1;
	//	return(-1.0);
	//}

	if (!g_FrameBuffer)
		glGenFramebuffersEXT(1, &g_FrameBuffer); // create FBO/offscreen framebuf

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_FrameBuffer);

	if (g_Textures[itex].sizeX != g_CapTextSize
		|| g_Textures[itex].sizeY != g_CapTextSize
		|| g_Textures[itex].sizeZ != 1
		|| g_Textures[itex].colorType != icoltype)
		CreateEmptyTexture(itex, xs, ys, 1, icoltype);

	//tex[itex].tar = GL_TEXTURE_RECTANGLE_ARB;
	//glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE); //necessary?

	//glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT/*0..3*/, g_Textures[itex].target, itex, 0);

	glViewport(0, 0, xs, ys);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	mygluPerspective(45, 1, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glScalef(ys / (float)xs, 1, 1);

	g_LastCapture = true;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qglEndCapture(double dtex)
{
	int itex;
	itex = (int)dtex;

	if ((unsigned)itex >= (unsigned)MAX_USER_TEXURES)
		return -1.0;

	g_Textures[itex].name[0] = '\0';

	if (g_LastCapture)
	{
		glViewport(0, 0, s_dxres, s_dyres);
		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // restore
		return 0.0;
	}

	if (g_Textures[itex].sizeX != g_CapTextSize || g_Textures[itex].sizeY != g_CapTextSize || g_Textures[itex].sizeZ != 1 || g_Textures[itex].colorType != KGL_BGRA32)
		CreateEmptyTexture(itex, g_CapTextSize, g_CapTextSize, 1, KGL_BGRA32);

	glBindTexture(g_Textures[itex].target,(int)itex);
	glCopyTexImage2D(g_Textures[itex].target, 0, GL_RGBA, 0, 0, g_Textures[itex].sizeX, g_Textures[itex].sizeY, 0);

	g_Textures[itex].name[0] = '\0';

	glViewport(0, 0, s_dxres, s_dyres);
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW); glPopMatrix();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsettex2(double dtex, char* st, double dcoltype)
{
	int i, x, y, gotpic, itex, icoltype, leng, xsiz, ysiz;
	char* buf = NULL;

	itex = (int)dtex;

	if ((unsigned)itex >= (unsigned)MAX_USER_TEXURES)
		return -1.0;

	icoltype = ((int)dcoltype) & ~15;

	if (strlen(st) > MAX_PATH-1)
		return -2.0;

	if (!_stricmp(g_Textures[itex].name, st) && g_Textures[itex].colorType == icoltype)
		return 0.0;

	strcpy(g_Textures[itex].name, st);

	gotpic = 0;
	xsiz = 32;
	ysiz = 32;

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

	if (g_Textures[itex].sizeX != xsiz || g_Textures[itex].sizeY != ysiz || g_Textures[itex].sizeZ != 1 || g_Textures[itex].colorType != icoltype)
		CreateEmptyTexture(itex, xsiz, ysiz, 1, icoltype);

	i = xsiz*ysiz * 4;

	if (i > s_bmpmal)
	{
		s_bmpmal = i;
		s_bmp = (char *)realloc(s_bmp, s_bmpmal);
	}

	if (!gotpic)
	{
		static const int imagenotfoundbmp[32] = // generate placeholder image
		{
			0x7ce39138, 0x05145b10, 0x04145510, 0x3dd45110, 0x0517d110, 0x05145110, 0x7de45138, 0x00000000, //"IMAGE"
			0x01f39100, 0x00445300, 0x00445500, 0x00445900, 0x00445100, 0x00445100, 0x00439100, 0x00000000, //" NOT "
			0x3d144e7c, 0x45345104, 0x45545104, 0x4594513c, 0x45145104, 0x45145104, 0x3d138e04, 0x00000000, //"FOUND"
			0x00400000, 0x00200000, 0x00200600, 0x0027c600, 0x00200000, 0x00200600, 0x00400600, 0x00000000, //" :-( "
		};

		buf = s_bmp;

		for (y = 0; y < ysiz; y++)
		{
			for (x = 0; x < xsiz; x++, buf += 4)
			{
				if (imagenotfoundbmp[y] & (1 << x))
				{
					*((int*)buf) = 0xf0102030;
					continue;
				}

				*((int*)s_bmp) = (((rand() << 15) + rand()) & 0x1f1f1f) + 0xff506070;
			}
		}
	}
	else
	{
		kprender(buf, leng, (int*)s_bmp, g_Textures[itex].sizeX * 4, min(xsiz, g_Textures[itex].sizeX), min(ysiz, g_Textures[itex].sizeY), 0, 0);
	}

	glBindTexture(g_Textures[itex].target, itex);

	switch(g_Textures[itex].target)
	{
	case GL_TEXTURE_2D:
		glTexSubImage2D(g_Textures[itex].target, 0, 0, 0, g_Textures[itex].sizeX, g_Textures[itex].sizeY, GL_BGRA_EXT, GL_UNSIGNED_BYTE, s_bmp);

		if ((icoltype & 0xf0) >= KGL_MIPMAP)
			mygluBuild2DMipmaps(g_Textures[itex].target, 4, g_Textures[itex].sizeX, g_Textures[itex].sizeY, GL_BGRA_EXT, GL_UNSIGNED_BYTE, s_bmp);

		break;

	case GL_TEXTURE_CUBE_MAP:
		for (i = 0; i < 6; i++)
			glTexSubImage2D(g_CubeMapConst[i], 0, 0, 0, g_Textures[itex].sizeX, g_Textures[itex].sizeX, GL_BGRA_EXT, GL_UNSIGNED_BYTE, s_bmp + g_Textures[itex].sizeX*g_Textures[itex].sizeX * 4 * s_CubeMapIndex[i]);

		break;
	}

	if (gotpic)
		free(buf);

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsettex(double dtex, char* st)
{
	return kglsettex2(dtex, st, (double)(KGL_MIPMAP + KGL_REPEAT));
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglgettexarray2(double dtex, double* p, double dxsiz, double dysiz, double coltype)
{
	int i, xs, ys, itex, evalvalperpix, glbyteperpix, internalFormat, format, type;

	itex = (int)dtex;

	if ((unsigned)itex >= (unsigned)MAX_USER_TEXURES)
		return -1.0;

	if (dxsiz < 1.0 || dysiz < 1.0 || (dxsiz*dysiz) > 67108864.0)
		return -1.0;

	xs = (int)dxsiz;
	ys = (int)dysiz;

	if ((xs*ys) > (g_Textures[itex].sizeX*g_Textures[itex].sizeY))
		return -1.0;

	switch (g_Textures[itex].colorType & 15)
	{
	case KGL_BGRA32: evalvalperpix = 1; glbyteperpix = 4; break;
	case KGL_CHAR:   evalvalperpix = 1; glbyteperpix = 1; break;
	case KGL_SHORT:  evalvalperpix = 1; glbyteperpix = 2; break;
	case KGL_INT:    evalvalperpix = 1; glbyteperpix = 4; break;
	case KGL_FLOAT:  evalvalperpix = 1; glbyteperpix = 4; break;
	case KGL_VEC4:   evalvalperpix = 4; glbyteperpix = 16; break;
	}

// FIXME: gevalfunc is now not globally available
//	if (((int)p) < ((int)gevalfunc) || (((int)p) + ((xs*ys*evalvalperpix) << 3) > ((int)gevalfunc) + gevalfuncleng))
//		return -1.0;

	i = xs*ys*glbyteperpix;

	if (i > s_bmpmal)
	{
		s_bmpmal = i;
		s_bmp = (char*)realloc(s_bmp, s_bmpmal);
	}

	glBindTexture(g_Textures[itex].target, itex);

	switch (g_Textures[itex].colorType & 15)
	{
	case KGL_BGRA32: internalFormat = 4; format = GL_BGRA_EXT; type = GL_UNSIGNED_BYTE; break;
	case KGL_CHAR:   internalFormat = GL_LUMINANCE8; format = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
	case KGL_SHORT:  internalFormat = GL_LUMINANCE16; format = GL_LUMINANCE; type = GL_UNSIGNED_SHORT; break;
	case KGL_INT:    internalFormat = GL_LUMINANCE32I_EXT; format = GL_LUMINANCE; type = GL_UNSIGNED_INT; break;
	case KGL_FLOAT:  internalFormat = GL_LUMINANCE32F_ARB; format = GL_LUMINANCE; type = GL_FLOAT; break;
	case KGL_VEC4:   internalFormat = GL_RGBA32F_ARB; format = GL_RGBA; type = GL_FLOAT; break;
	}

	glGetTexImage(g_Textures[itex].target, 0, format, type, s_bmp);

	//preferred method .. doesn't work :/
	//glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	//glReadPixels(0,0,tex[itex].sizx,tex[itex].sizy,GL_BGRA_EXT,GL_UNSIGNED_BYTE,gbmp);

	switch (g_Textures[itex].colorType & 15)
	{
	case KGL_BGRA32: for (i = xs*ys - 1; i >= 0; i--) p[i] = (double)*(unsigned int*)((i << 2) + s_bmp); break;
	case KGL_CHAR:   for (i = xs*ys - 1; i >= 0; i--) p[i] = (double)*(unsigned char*)(i + s_bmp); break;
	case KGL_SHORT:  for (i = xs*ys - 1; i >= 0; i--) p[i] = (double)*(unsigned short*)((i << 1) + s_bmp); break;
	case KGL_INT:    for (i = xs*ys - 1; i >= 0; i--) p[i] = (double)*(unsigned int*)((i << 2) + s_bmp); break;
	case KGL_FLOAT:  for (i = xs*ys - 1; i >= 0; i--) p[i] = (double)*(float*)((i << 2) + s_bmp); break;
	case KGL_VEC4:   for (i = xs*ys * 4 - 1; i >= 0; i--) p[i] = (double)*(float*)((i << 2) + s_bmp); break;
	}

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglsettexarray3(double dtex, double *p, double dxsiz, double dysiz, double dzsiz, double dcoltype)
{
	int i, xs, ys, zs, itex, icoltype, evalvalperpix, glbyteperpix, internalFormat, format, type;

	itex = (int)dtex;

	if ((unsigned)itex >= (unsigned)MAX_USER_TEXURES)
		return(-1.0);

	icoltype = (int)dcoltype;
	g_Textures[itex].name[0] = 0;

	if (dxsiz < 1.0 || dysiz < 1.0 || dzsiz < 1.0 || (dxsiz*dysiz*dzsiz) > 67108864.0)
		return -1.0;

	xs = (int)dxsiz;
	ys = (int)dysiz;
	zs = (int)dzsiz;

	if (g_Textures[itex].sizeX != xs || g_Textures[itex].sizeY != ys || g_Textures[itex].sizeZ != zs || g_Textures[itex].colorType != icoltype)
		CreateEmptyTexture(itex, xs, ys, zs, icoltype);

	switch (icoltype & 15)
	{
	case KGL_BGRA32: evalvalperpix = 1; glbyteperpix = 4; break;
	case KGL_CHAR:   evalvalperpix = 1; glbyteperpix = 1; break;
	case KGL_SHORT:  evalvalperpix = 1; glbyteperpix = 2; break;
	case KGL_INT:    evalvalperpix = 1; glbyteperpix = 4; break;
	case KGL_FLOAT:  evalvalperpix = 1; glbyteperpix = 4; break;
	case KGL_VEC4:   evalvalperpix = 4; glbyteperpix =16; break;
	}

	i = xs*ys*zs*glbyteperpix;

	if (i > s_bmpmal)
	{
		s_bmpmal = i;
		s_bmp = (char *)realloc(s_bmp, s_bmpmal);
	}

// FIXME: gevalfunc is now not globally available
//	if ((((int)p) < ((int)gevalfunc)) || (((int)p) + ((xs*ys*zs*evalvalperpix) << 3) > ((int)gevalfunc) + gevalfuncleng))
//		return(-1.0);

	switch (icoltype & 15)
	{
	case KGL_BGRA32: for (i = xs*ys*zs - 1; i >= 0; i--) *(unsigned int *)((i << 2) + s_bmp) = (unsigned int)(p[i]); break;
	case KGL_CHAR:   for (i = xs*ys*zs - 1; i >= 0; i--) *(unsigned char *)(i + s_bmp) = (unsigned char)(p[i]); break;
	case KGL_SHORT:  for (i = xs*ys*zs - 1; i >= 0; i--) *(unsigned short *)((i << 1) + s_bmp) = (unsigned short)(p[i]); break;
	case KGL_INT:    for (i = xs*ys*zs - 1; i >= 0; i--) *(unsigned int *)((i << 2) + s_bmp) = (unsigned int)(p[i]); break;
	case KGL_FLOAT:  for (i = xs*ys*zs - 1; i >= 0; i--) *(float *)((i << 2) + s_bmp) = (float)(p[i]); break;
	case KGL_VEC4:   for (i = xs*ys*zs * 4 - 1; i >= 0; i--) *(float *)((i << 2) + s_bmp) = (float)(p[i]); break;
	}

	glBindTexture(g_Textures[itex].target, itex);

	switch (icoltype & 15)
	{
	case KGL_BGRA32: internalFormat = 4; format = GL_BGRA_EXT; type = GL_UNSIGNED_BYTE; break;
	case KGL_CHAR:   internalFormat = GL_LUMINANCE8; format = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
	case KGL_SHORT:  internalFormat = GL_LUMINANCE16; format = GL_LUMINANCE; type = GL_UNSIGNED_SHORT; break;
	case KGL_INT:    internalFormat = GL_LUMINANCE32I_EXT; format = GL_LUMINANCE; type = GL_UNSIGNED_INT; break;
	case KGL_FLOAT:  internalFormat = GL_LUMINANCE32F_ARB; format = GL_LUMINANCE; type = GL_FLOAT; break;
	case KGL_VEC4:   internalFormat = GL_RGBA32F_ARB; format = GL_RGBA; type = GL_FLOAT; break;
	}

	switch (g_Textures[itex].target)
	{
	case GL_TEXTURE_1D:
		glTexSubImage1D(g_Textures[itex].target, 0, 0, g_Textures[itex].sizeX, format, type, s_bmp);
		break;

	case GL_TEXTURE_2D:
		glTexSubImage2D(g_Textures[itex].target, 0, 0, 0, g_Textures[itex].sizeX, g_Textures[itex].sizeY, format, type, s_bmp);

		if ((icoltype & 0xf0) >= KGL_MIPMAP && (icoltype & 15) == KGL_BGRA32)
			mygluBuild2DMipmaps(g_Textures[itex].target, 4, g_Textures[itex].sizeX, g_Textures[itex].sizeY, GL_BGRA_EXT, GL_UNSIGNED_BYTE, s_bmp);

		break;

	case GL_TEXTURE_3D:
		glTexSubImage3D(g_Textures[itex].target, 0, 0, 0, 0, g_Textures[itex].sizeX, g_Textures[itex].sizeY, g_Textures[itex].sizeZ, format, type, s_bmp);
		break;

	case GL_TEXTURE_CUBE_MAP:
		for (i = 0; i < 6; i++)
			glTexSubImage2D(g_CubeMapConst[i], 0, 0, 0, g_Textures[itex].sizeX, g_Textures[itex].sizeX, format, type, s_bmp + g_Textures[itex].sizeX*g_Textures[itex].sizeX*glbyteperpix*s_CubeMapIndex[i]);

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
	int itex = (int)dtex;

	if ((unsigned)itex >= (unsigned)MAX_USER_TEXURES)
		return -1.0;

	glBindTexture(g_Textures[itex].target, itex);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kglActiveTex(double texunit)
{
	int itexunit = ((int)texunit)&3;

	//if (g_glfp[glActiveTexture])
	//	((PFNGLACTIVETEXTUREPROC)g_glfp[glActiveTexture])(itexunit+GL_TEXTURE0);
	glActiveTexture(itexunit + GL_TEXTURE0);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl kgluPerspective(double fovy, double xy, double z0, double z1)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	mygluPerspective(fovy, xy, z0, z1);

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double __cdecl qgluLookAt(double x, double y, double z, double px, double py, double pz, double ux, double uy, double uz)
{
	mygluLookAt(x, y, z, px, py, pz, ux, uy, uz);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double ksetfov(double fov)
{
	return SetFOV(fov, s_dxres, s_dyres);
}

///////////////////////////////////////////////////////////////////////////////
double kglProgramLocalParam(double ind, double a, double b, double c, double d)
{
	glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (unsigned)ind, a, b, c, d);
	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (unsigned)ind, a, b, c, d);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglProgramEnvParam(double ind, double a, double b, double c, double d)
{
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, (unsigned)ind, a, b, c, d);
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (unsigned)ind, a, b, c, d);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglGetUniformLoc(char *shadvarnam)
{
	int i = glGetUniformLocation(g_ShaderPrograms[g_CurrentShader], shadvarnam);
	return (double)i;
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform1f(double sh, double v0)                                  { glUniform1f((int)sh, (float)v0); return 0.0; }
double kglUniform2f(double sh, double v0, double v1)                       { glUniform2f((int)sh, (float)v0, (float)v1); return 0.0; }
double kglUniform3f(double sh, double v0, double v1, double v2)            { glUniform3f((int)sh, (float)v0, (float)v1, (float)v2); return 0.0; }
double kglUniform4f(double sh, double v0, double v1, double v2, double v3) { glUniform4f((int)sh, (float)v0, (float)v1, (float)v2, (float)v3); return 0.0; }
double kglUniform1i(double sh, double v0)                                  { glUniform1i((int)sh, (int)v0); return 0.0; }
double kglUniform2i(double sh, double v0, double v1)                       { glUniform2i((int)sh, (int)v0, (int)v1); return 0.0; }
double kglUniform3i(double sh, double v0, double v1, double v2)            { glUniform3i((int)sh, (int)v0, (int)v1, (int)v2); return 0.0; }
double kglUniform4i(double sh, double v0, double v1, double v2, double v3) { glUniform4i((int)sh, (int)v0, (int)v1, (int)v2, (int)v3); return 0.0; }

///////////////////////////////////////////////////////////////////////////////
#define MAXUNIFVALNUM 4096
double kglUniform1fv(double sh, double num, double *vals)
{
	int inum = min((int)num,MAXUNIFVALNUM);

	if (inum < 0)
		return 0.0;

	float* fvals = (float*)_alloca(inum*sizeof(fvals[0]));

	if (!fvals)
		return 0.0;
	
	for (int i = 0; i < inum; i++)
		fvals[i] = (float)vals[i];

	glUniform1fv((int)sh, inum, fvals);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform2fv(double sh, double num, double *vals)
{
	int i, inum;
	float* fvals;
	inum = min((int)num,MAXUNIFVALNUM); if (inum < 0) return(0.0);
	fvals = (float *)_alloca(inum*sizeof(fvals[0])); if (!fvals) return(0.0);
	for (i = 0; i < inum; i++) fvals[i] = (float)vals[i];
	glUniform2fv((int)sh, inum, fvals);
	return 0.0;
}
///////////////////////////////////////////////////////////////////////////////
double kglUniform3fv(double sh, double num, double *vals)
{
	int i, inum;
	float* fvals;
	inum = min((int)num, MAXUNIFVALNUM); if (inum < 0) return(0.0);
	fvals = (float *)_alloca(inum*sizeof(fvals[0])); if (!fvals) return(0.0);
	for (i = 0; i < inum; i++) fvals[i] = (float)vals[i];
	glUniform3fv((int)sh, inum, fvals);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform4fv(double sh, double num, double *vals)
{
	int i, inum;
	float* fvals;
	inum = min((int)num,MAXUNIFVALNUM); if (inum < 0) return(0.0);
	fvals = (float *)_alloca(inum*sizeof(fvals[0])); if (!fvals) return(0.0);
	for (i = 0; i < inum; i++) fvals[i] = (float)vals[i];
	glUniform4fv((int)sh, inum, fvals);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglUniform1iv(double sh, double num, double* vals)
{
	int i, inum;
	int* ivals;
	inum = min((int)num,MAXUNIFVALNUM); if (inum < 0) return(0.0);
	ivals = (int *)_alloca(inum*sizeof(ivals[0])); if (!ivals) return(0.0);
	for(i=0;i<inum;i++) ivals[i] = (int)vals[i];
	glUniform1iv((int)sh, inum, ivals);
	return 0.0;
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

	glUniform2iv((int)sh, inum, ivals);
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

	glUniform3iv((int)sh, inum, ivals);
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

	glUniform4iv((int)sh, inum, ivals);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglGetAttribLoc(char *shadvarnam)
{
	int i = glGetAttribLocation(g_ShaderPrograms[g_CurrentShader], shadvarnam);
	return (double)i;
}

///////////////////////////////////////////////////////////////////////////////
double kglVertexAttrib1f(double sh, double v0)                                  { glVertexAttrib1f((int)sh, (float)v0); return 0.0; }
double kglVertexAttrib2f(double sh, double v0, double v1)                       { glVertexAttrib2f((int)sh, (float)v0, (float)v1); return 0.0; }
double kglVertexAttrib3f(double sh, double v0, double v1, double v2)            { glVertexAttrib3f((int)sh, (float)v0, (float)v1, (float)v2); return 0.0; }
double kglVertexAttrib4f(double sh, double v0, double v1, double v2, double v3) { glVertexAttrib4f((int)sh, (float)v0, (float)v1, (float)v2, (float)v3); return 0.0; }

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
	glBlendFunc((GLenum)sfactor, (GLenum)dfactor);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double kglEnable(double d) { glEnable((GLenum)d); return 0; }
double kglDisable(double d) { glDisable((GLenum)d); return 0; }

///////////////////////////////////////////////////////////////////////////////
double mysleep(double ms)
{
	int i = ((int)ms);
	i = min(max(i, 0), 1000);
	Sleep(i);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
//double glswapinterval(double val)
//{
//	((PFNWGLSWAPINTERVALEXTPROC)g_glfp[wglSwapIntervalEXT])((int)val);
//	return 0.0;
//}

///////////////////////////////////////////////////////////////////////////////
static int ginstartklock = 0;
double glklockstart(double _)
{
	//if (supporttimerquery)
	{
		if (ginstartklock)
			glEndQuery(GL_TIME_ELAPSED_EXT);
		else
			ginstartklock = 1;

		glBeginQuery(GL_TIME_ELAPSED_EXT, g_Queries[0]);
	}

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
double glklockelapsed(double _)
{
	GLuint dtim;
	GLint got;

	//if (!supporttimerquery)
	//	return -1.0;

	if (!ginstartklock)
		return -2.0;

	ginstartklock = 0;
	glEndQuery(GL_TIME_ELAPSED_EXT);

	do
	{
		glGetQueryObjectiv(g_Queries[0], GL_QUERY_RESULT_AVAILABLE, &got);
	}
	while (!got);

	/*
	if (g_glfp[glGetQueryObjectui64vEXT])
	{
		((PFNGLGETQUERYOBJECTUI64VEXTPROC)g_glfp[glGetQueryObjectui64vEXT])(queries[0], GL_QUERY_RESULT, &qdtim);
		return(((double)qdtim)*1e-9);
	}
	*/

	glGetQueryObjectuiv(g_Queries[0], GL_QUERY_RESULT, &dtim);
	return (((double)dtim)*1e-9);
}

///////////////////////////////////////////////////////////////////////////////
double myklock(double d)
{
	__int64 q;
	int i = (int)d;
	if (!i)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&q);
		return(((double)(q - g_qtim0)) / ((double)g_qper));
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
double mysrand(double val)
{
	extern void ksrand(long);
	ksrand((int)val);
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
void SetEvalVars(double dxres, double dyres, double dmousx, double dmousy, double dbstatus, double dkeystatus[256])
{
	s_dxres = dxres;
	s_dyres = dyres;
	s_dmousx = dmousx;
	s_dmousy = dmousy;
	s_dbstatus = dbstatus;
	memcpy(s_dkeystatus, dkeystatus, sizeof(s_dkeystatus));
}

///////////////////////////////////////////////////////////////////////////////
EVALFUNC CompileEVALFunctionWithExt(std::string text)
{
	static double kglcolorbufferbit = GL_COLOR_BUFFER_BIT;
	static double kgldepthbufferbit = GL_DEPTH_BUFFER_BIT;
	static double kglstencilbufferbit = GL_STENCIL_BUFFER_BIT;
	static double kgl_points = 0.0, kgl_lines = 1.0, kgl_line_loop = 2.0;
	static double kgl_line_strip = 3.0, kgl_tris = 4.0, kgl_tri_strip = 5.0;
	static double kgl_tri_fan = 6.0, kgl_quads = 7.0, kgl_quad_strip = 8.0;
	static double kgl_polygon = 9.0, kgl_texture0 = GL_TEXTURE0;
	static double kgl_lines_adjacency = 10.0, kgl_line_strip_adjacency = 11.0;
	static double kgl_triangles_adjacency = 12.0, kgl_triangle_strip_adjacency = 13.0;
	static double kgl_none = GL_NONE, kgl_front = GL_FRONT, kgl_back = GL_BACK, kgl_frontback = GL_FRONT_AND_BACK;
	static double kglzero = GL_ZERO, kglone = GL_ONE;
	static double kglsrccolor = GL_SRC_COLOR, kgloneminussrccolor = GL_ONE_MINUS_SRC_COLOR;
	static double kgldstcolor = GL_DST_COLOR, kgloneminusdstcolor = GL_ONE_MINUS_DST_COLOR;
	static double kglsrcalpha = GL_SRC_ALPHA, kgloneminussrcalpha = GL_ONE_MINUS_SRC_ALPHA;
	static double kgldstalpha = GL_DST_ALPHA, kgloneminusdstalpha = GL_ONE_MINUS_DST_ALPHA;
	static double kglconstantcolor = GL_CONSTANT_COLOR, kgloneminusconstantcolor = GL_ONE_MINUS_CONSTANT_COLOR;
	static double kglconstantalpha = GL_CONSTANT_ALPHA, kgloneminusconstantalpha = GL_ONE_MINUS_CONSTANT_ALPHA;
	static double kglsrcalphasaturate = GL_SRC_ALPHA_SATURATE;
	static double kgldepthtest = GL_DEPTH_TEST;
	static double kglbgra32 = KGL_BGRA32, kglchar = KGL_CHAR, kglshort = KGL_SHORT, kglint = KGL_INT, kglfloat = KGL_FLOAT, kglvec4 = KGL_VEC4;
	static double kgllinear = KGL_LINEAR, kglnearest = KGL_NEAREST;
	static double kglmipmap0 = KGL_MIPMAP0, kglmipmap1 = KGL_MIPMAP1, kglmipmap2 = KGL_MIPMAP2, kglmipmap3 = KGL_MIPMAP3;
	static double kglrepeat = KGL_REPEAT, kglclamp = KGL_CLAMP, kglclamptoedge = KGL_CLAMP_TO_EDGE;

	//static double dxres, dyres, dmousx, dmousy;

	static evalextyp extensionTable[] =
	{
		{ "GL_COLOR_BUFFER_BIT", &kglcolorbufferbit },
		{ "GL_DEPTH_BUFFER_BIT", &kgldepthbufferbit },
		{ "GL_STENCIL_BUFFER_BIT", &kglstencilbufferbit },

		{ "GL_POINTS", &kgl_points },
		{ "GL_LINES", &kgl_lines },
		{ "GL_LINE_LOOP", &kgl_line_loop },
		{ "GL_LINE_STRIP", &kgl_line_strip },
		{ "GL_TRIANGLES", &kgl_tris },
		{ "GL_TRIANGLE_STRIP", &kgl_tri_strip },
		{ "GL_TRIANGLE_FAN", &kgl_tri_fan },
		{ "GL_QUADS", &kgl_quads }, //x
		{ "GL_QUAD_STRIP", &kgl_quad_strip }, //x
		{ "GL_POLYGON", &kgl_polygon }, //x

		{ "GL_LINES_ADJACENCY", &kgl_lines_adjacency },
		{ "GL_LINE_STRIP_ADJACENCY", &kgl_line_strip_adjacency },
		{ "GL_TRIANGLES_ADJACENCY", &kgl_triangles_adjacency },
		{ "GL_TRIANGLE_STRIP_ADJACENCY", &kgl_triangle_strip_adjacency },
		{ "GL_LINES_ADJACENCY_EXT", &kgl_lines_adjacency },
		{ "GL_LINE_STRIP_ADJACENCY_EXT", &kgl_line_strip_adjacency },
		{ "GL_TRIANGLES_ADJACENCY_EXT", &kgl_triangles_adjacency },
		{ "GL_TRIANGLE_STRIP_ADJACENCY_EXT", &kgl_triangle_strip_adjacency },

		{ "GLCLEAR()", qglClear }, //X?
		{ "GLBEGIN()", qglBegin }, //X?
		{ "GLEND()", qglEnd }, //X?

		{ "GLVERTEX(,)", qglVertex2d }, //X?
		{ "GLVERTEX(,,)", qglVertex3d }, //X?
		{ "GLVERTEX(,,,)", qglVertex4d }, //X?
		{ "GLTEXCOORD(,)", qglTexCoord2d }, //X?
		{ "GLTEXCOORD(,,)", qglTexCoord3d }, //X?
		{ "GLTEXCOORD(,,,)", qglTexCoord4d }, //X?
		{ "GLCOLOR(,,)", qglColor3d }, //X?
		{ "GLCOLOR(,,,)", qglColor4d }, //X?
		{ "GLNORMAL(,,)", qglNormal3d }, //X?

		{ "GLPROGRAMLOCALPARAM(,,,,)", kglProgramLocalParam }, //for arb asm
		{ "GLPROGRAMENVPARAM(,,,,)", kglProgramEnvParam }, //for arb asm

		{ "GLGETUNIFORMLOC($)", kglGetUniformLoc },
		{ "GLUNIFORM1F(,)", kglUniform1f },
		{ "GLUNIFORM2F(,,)", kglUniform2f },
		{ "GLUNIFORM3F(,,,)", kglUniform3f },
		{ "GLUNIFORM4F(,,,,)", kglUniform4f },
		{ "GLUNIFORM1I(,)", kglUniform1i },
		{ "GLUNIFORM2I(,,)", kglUniform2i },
		{ "GLUNIFORM3I(,,,)", kglUniform3i },
		{ "GLUNIFORM4I(,,,,)", kglUniform4i },
		{ "GLUNIFORM1FV(,,&)", kglUniform1fv },
		{ "GLUNIFORM2FV(,,&)", kglUniform2fv },
		{ "GLUNIFORM3FV(,,&)", kglUniform3fv },
		{ "GLUNIFORM4FV(,,&)", kglUniform4fv },
		{ "GLUNIFORM1IV(,,&)", kglUniform1iv },
		{ "GLUNIFORM2IV(,,&)", kglUniform2iv },
		{ "GLUNIFORM3IV(,,&)", kglUniform3iv },
		{ "GLUNIFORM4IV(,,&)", kglUniform4iv },

		{ "GLGETATTRIBLOC($)", kglGetAttribLoc },
		{ "GLVERTEXATTRIB1F(,)", kglVertexAttrib1f },
		{ "GLVERTEXATTRIB2F(,,)", kglVertexAttrib2f },
		{ "GLVERTEXATTRIB3F(,,,)", kglVertexAttrib3f },
		{ "GLVERTEXATTRIB4F(,,,,)", kglVertexAttrib4f },

		{ "GLPUSHMATRIX()", qglPushMatrix }, //x
		{ "GLPOPMATRIX()", qglPopMatrix }, //x
		{ "GLMULTMATRIX(&)", qglMultMatrixd }, //x
		{ "GLTRANSLATE(,,)", qglTranslated }, //x
		{ "GLROTATE(,,,)", qglRotated }, //x
		{ "GLSCALE(,,)", qglScaled }, //x
		{ "GLUPERSPECTIVE(,,,)", kgluPerspective },
		{ "GLULOOKAT(,,,,,,,,)", qgluLookAt }, //?
		{ "SETFOV()", ksetfov },

		{ "KGL_BGRA32", &kglbgra32 },
		{ "KGL_CHAR", &kglchar },
		{ "KGL_SHORT", &kglshort },
		{ "KGL_INT", &kglint },
		{ "KGL_FLOAT", &kglfloat },
		{ "KGL_VEC4", &kglvec4 },
		{ "KGL_LINEAR", &kgllinear },
		{ "KGL_NEAREST", &kglnearest },
		{ "KGL_MIPMAP", &kglmipmap3 },
		{ "KGL_MIPMAP0", &kglmipmap0 },
		{ "KGL_MIPMAP1", &kglmipmap1 },
		{ "KGL_MIPMAP2", &kglmipmap2 },
		{ "KGL_MIPMAP3", &kglmipmap3 },
		{ "KGL_REPEAT", &kglrepeat },
		{ "KGL_CLAMP", &kglclamp },
		{ "KGL_CLAMP_TO_EDGE", &kglclamptoedge },
		{ "GLCAPTURE()", qglCapture }, //?
		{ "GLCAPTURE(,,,)", kglCapture }, //?
		{ "GLCAPTUREEND()", qglEndCapture }, //?
		{ "GLSETTEX(,$)", kglsettex }, //?
		{ "GLSETTEX(,$,)", kglsettex2 }, //?
		{ "GLSETTEX(,&,,)", kglsettexarray1 }, //?
		{ "GLSETTEX(,&,,,)", kglsettexarray2 }, //?
		{ "GLSETTEX(,&,,,,)", kglsettexarray3 }, //?
		{ "GLGETTEX(,&,,,)", kglgettexarray2 }, //?
		{ "GLQUAD()", qglQuad }, //?
		{ "GLBINDTEXTURE()", qglBindTex },
		{ "GL_TEXTURE0", &kgl_texture0 },
		{ "GLACTIVETEXTURE()", kglActiveTex },
		{ "GLTEXTDISABLE()", qglEndTex }, // Deprecated:it's a nop!
		{ "GL_NONE", &kgl_none },
		{ "GL_FRONT", &kgl_front },
		{ "GL_BACK", &kgl_back },
		{ "GL_FRONT_AND_BACK", &kgl_frontback },
		{ "GLCULLFACE()", kglCullFace },

		{ "GL_ZERO", &kglzero }, //dst default
		{ "GL_SRC_COLOR", &kglsrccolor },
		{ "GL_DST_COLOR", &kgldstcolor },
		{ "GL_SRC_ALPHA", &kglsrcalpha }, //src for transparency
		{ "GL_DST_ALPHA", &kgldstalpha },
		{ "GL_CONSTANT_COLOR", &kglconstantcolor },
		{ "GL_CONSTANT_ALPHA", &kglconstantalpha },
		{ "GL_SRC_ALPHA_SATURATE", &kglsrcalphasaturate },
		{ "GL_ONE", &kglone }, //src default
		{ "GL_ONE_MINUS_SRC_COLOR", &kgloneminussrccolor },
		{ "GL_ONE_MINUS_DST_COLOR", &kgloneminusdstcolor },
		{ "GL_ONE_MINUS_SRC_ALPHA", &kgloneminussrcalpha }, //dst for transparency
		{ "GL_ONE_MINUS_DST_ALPHA", &kgloneminusdstalpha },
		{ "GL_ONE_MINUS_CONSTANT_COLOR", &kgloneminusconstantcolor },
		{ "GL_ONE_MINUS_CONSTANT_ALPHA", &kgloneminusconstantalpha },
		{ "GLBLENDFUNC(,)", kglBlendFunc },
		{ "GL_DEPTH_TEST", &kgldepthtest },
		{ "GLENABLE()", kglEnable },
		{ "GLDISABLE()", kglDisable },

		{ "GLALPHAENABLE()", qglAlphaEnable }, //Deprecated:use glBlendFunc/glEnable(GL_DEPTH_TEST) instead
		{ "GLALPHADISABLE()", qglAlphaDisable }, //Deprecated:use glBlendFunc/glDisable(GL_DEPTH_TEST) instead

		{ "GLLINEWIDTH()", qglLineWidth },

		{ "GLSETSHADER()", qglsetshader }, //vshad=  0 ,gshad= -1 ,fshad=[ 0] (old style)
		{ "GLSETSHADER($,$)", kglsetshader2 }, //vshad=[$0],gshad= -1 ,fshad=[$1]
		{ "GLSETSHADER($,$,$)", kglsetshader3 }, //vshad=[$0],gshad=[$1],fshad=[$2]

		{ "GLKLOCKSTART()", glklockstart },
		{ "GLKLOCKELAPSED()", glklockelapsed },
		{ "KLOCK()", myklock },
		{ "NUMFRAMES", &g_DNumFrames },

		// resolution and mouse position
		{ "XRES", &s_dxres },
		{ "YRES", &s_dyres },
		{ "MOUSX", &s_dmousx },
		{ "MOUSY", &s_dmousy },
		{ "BSTATUS", &s_dbstatus },
		{ "KEYSTATUS[256]", &s_dkeystatus },

		{ "RGB(,,)", kmyrgb },     //convert r,g,b to 24-bit col
		{ "RGBA(,,,)", kmyrgba },     //convert r,g,b,a to 32-bit col
		{ "NOISE()", noise1d },     //x     (Tom's noise function)
		{ "NOISE(,)", noise2d },     //x,y   (Tom's noise function)
		{ "NOISE(,,)", noise3d },     //x,y,z (Tom's noise function)
		{ "PRINTF($,.)", myprintf },
		{ "PRINTG(,,,$,.)", myprintg },
		{ "SRAND()", mysrand },
		{ "SLEEP()", mysleep },
		//{"GLSWAPINTERVAL()"   ,glswapinterval },
		{ "PLAYNOTE(,,)", MIDIPlayNote },
		{ "MOUNTZIP($)", mykzaddstack },
	};

	kasm87addext(extensionTable, (sizeof(extensionTable) / sizeof(extensionTable[0])));

	auto func = (EVALFUNC) kasm87(text.c_str());
	//gevalfuncleng = kasm87leng;

	kasm87addext(nullptr, 0);

	return func;
}

