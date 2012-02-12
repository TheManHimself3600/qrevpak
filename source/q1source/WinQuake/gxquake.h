/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/******************* These are supposed to be in <gl/gl.h> and <gl/glu.h> and MUST BE DELETED ASAP: *********************/
typedef unsigned int GLenum;

typedef float GLfloat;

typedef float GLclampf;

typedef unsigned int GLuint;

typedef unsigned char GLboolean;

typedef int GLsizei;

typedef int GLint;

typedef unsigned char GLubyte;

typedef void GLvoid;

typedef unsigned int GLbitfield;

typedef double GLclampd;

typedef double GLdouble;

typedef signed char GLbyte;

typedef short GLshort;

typedef unsigned short GLushort;

#define GL_LINEAR_MIPMAP_NEAREST 1

#define GL_LINEAR 2

#define GL_TEXTURE_2D 3

#define GL_NEAREST 4

#define GL_NEAREST_MIPMAP_NEAREST 5

#define GL_NEAREST_MIPMAP_LINEAR 6

#define GL_LINEAR_MIPMAP_LINEAR 7

#define GL_TEXTURE_MIN_FILTER 8

#define GL_TEXTURE_MAG_FILTER 9

#define GL_QUADS 10

#define GL_ALPHA_TEST 11

#define GL_BLEND 12

#define GL_RGBA 13

#define GL_UNSIGNED_BYTE 14

#define GL_FRONT 15

#define GL_BACK 16

#define GL_DEPTH_TEST 19

#define GL_COLOR_INDEX 21

#define GL_TRIANGLE_FAN 22

#define GL_SMOOTH 23

#define GL_ONE 24

#define GL_SRC_ALPHA 25

#define GL_ONE_MINUS_SRC_ALPHA 26

#define GL_TRIANGLE_STRIP 27

#define GL_TEXTURE_ENV 28

#define GL_TEXTURE_ENV_MODE 29

#define GL_MODULATE 30

#define GL_PERSPECTIVE_CORRECTION_HINT 31

#define GL_FASTEST 32

#define GL_REPLACE 33

#define GL_FLAT 34

#define GL_NICEST 35

#define GL_COLOR_BUFFER_BIT 1024

#define GL_DEPTH_BUFFER_BIT 2048

#define GL_LEQUAL 37

#define GL_GEQUAL 38

#define GL_ALPHA 39

#define GL_LUMINANCE 40

#define GL_INTENSITY 41

#define GL_POLYGON 42

#define GL_ZERO 43

#define GL_ONE_MINUS_SRC_COLOR 44

#define GL_RGB 45

#define GL_TRIANGLES 46

#define GL_VENDOR 47

#define GL_RENDERER 48

#define GL_VERSION 49

#define GL_EXTENSIONS 50

#define GL_GREATER 51

#define GL_FRONT_AND_BACK 52

#define GL_FILL 53

#define GL_TEXTURE_WRAP_S 54

#define GL_REPEAT 55

#define GL_TEXTURE_WRAP_T 56

#define GL_RGBA8 58

#define GL_RGB5_A1 59

#define GL_RGBA4 60

#define GL_RGBA2 61

#define GL_RGB8 62

#define GL_RGB5 63

#define GL_RGB4 64

#define GL_R3_G3_B2 65

#define GL_VERTEX_ARRAY 66

#define GL_FLOAT 67

#define GL_COLOR_ARRAY 68

#define GL_FALSE 0

#define GL_POINTS 69

#define GL_TRUE 1

#define GL_SCISSOR_TEST 70

#define GL_BACK_LEFT 71

#define GL_LINES 72

#define GL_NO_ERROR 0

#define GL_POINT_SMOOTH 73

#define GL_LINE_STRIP 74

#define GL_SRC_COLOR 75

#define GL_INTENSITY8 76

#define GL_LUMINANCE8 77

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);

void glTexParameterf(GLenum target, GLenum pname, GLfloat param);

void glEnable(GLenum cap);

void glDisable(GLenum cap);

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

void glBegin(GLenum mode);

void glVertex2f(GLfloat x, GLfloat y);

void glEnd(void);

void glColor3f(GLfloat red, GLfloat green, GLfloat blue);

void glBindTexture(GLenum target, GLuint texture);

void glTexCoord2f(GLfloat s, GLfloat t);

void glDrawBuffer(GLenum mode);

void glClear(GLbitfield mask);

void glDepthFunc(GLenum func);

void glDepthRange(GLclampd nearVal, GLclampd farVal);

void glGetFloatv(GLenum pname, GLfloat* params);

void glColor4fv(const GLfloat* v);

void glVertex3f(GLfloat x, GLfloat y, GLfloat z);

void glVertex3fv(const GLfloat* v);

void glTexEnvf(GLenum target, GLenum pname, GLfloat param);

void glShadeModel(GLenum mode);

void glHint(GLenum target, GLenum mode);

void glFinish(void);

void glReadBuffer(GLenum mode);

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data);

void glDepthMask(GLboolean flag);

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);

void glBlendFunc(GLenum sfactor, GLenum dfactor);

void glFlush(void);

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

void glAlphaFunc(GLenum func, GLclampf ref);

void glPolygonMode(GLenum face, GLenum mode);

void glColor3ubv(const GLubyte* v);

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

/************************************************************************************************************************/

void GX_BeginRendering (int *x, int *y, int *width, int *height);
void GX_EndRendering (void);


/******************* These might exist in GX hardware. Investigate:
// Function prototypes for the Texture Object Extension routines
typedef GLboolean (APIENTRY *ARETEXRESFUNCPTR)(GLsizei, const GLuint *,
                    const GLboolean *);
typedef void (APIENTRY *BINDTEXFUNCPTR)(GLenum, GLuint);
typedef void (APIENTRY *DELTEXFUNCPTR)(GLsizei, const GLuint *);
typedef void (APIENTRY *GENTEXFUNCPTR)(GLsizei, GLuint *);
typedef GLboolean (APIENTRY *ISTEXFUNCPTR)(GLuint);
typedef void (APIENTRY *PRIORTEXFUNCPTR)(GLsizei, const GLuint *,
                    const GLclampf *);
typedef void (APIENTRY *TEXSUBIMAGEPTR)(int, int, int, int, int, int, int, int, void *);

extern	BINDTEXFUNCPTR bindTexFunc;
extern	DELTEXFUNCPTR delTexFunc;
extern	TEXSUBIMAGEPTR TexSubImage2DFunc;
*******************/

extern	int texture_extension_number;
extern	int		texture_mode;

extern	float	gxdepthmin, gxdepthmax;

void GX_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean alpha);
void GX_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean alpha);
int GX_LoadTexture (char *identifier, int width, int height, byte *data, qboolean mipmap, qboolean alpha);
int GX_FindTexture (char *identifier);

typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} gxvert_t;

extern gxvert_t gxv;

extern	int gxx, gxy, gxwidth, gxheight;

/******************* These might exist in GX hardware. Investigate:
extern	PROC glArrayElementEXT;
extern	PROC glColorPointerEXT;
extern	PROC glTexturePointerEXT;
extern	PROC glVertexPointerEXT;
*******************/

// r_local.h -- private refresh defs

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle
#define	MAX_LBM_HEIGHT		480

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

#define BACKFACE_EPSILON	0.01


void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
texture_t *R_TextureAnimation (texture_t *base);

typedef struct surfcache_s
{
	struct surfcache_s	*next;
	struct surfcache_s 	**owner;		// NULL is an empty chunk of memory
	int					lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int					dlight;
	int					size;		// including header
	unsigned			width;
	unsigned			height;		// DEBUG only needed for debug
	float				mipscale;
	struct texture_s	*texture;	// checked for animating textures
	byte				data[4];	// width*height elements
} surfcache_t;


typedef struct
{
	pixel_t		*surfdat;	// destination for generated surface
	int			rowbytes;	// destination logical width in bytes
	msurface_t	*surf;		// description for surface to generate
	fixed8_t	lightadj[MAXLIGHTMAPS];
							// adjust for lightmap levels for dynamic lighting
	texture_t	*texture;	// corrected for animating textures
	int			surfmip;	// mipmapped ratio of surface texels / world pixels
	int			surfwidth;	// in mipmapped texels
	int			surfheight;	// in mipmapped texels
} drawsurf_t;


typedef enum {
	pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2, pt_blob, pt_blob2
} ptype_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
} particle_t;


//====================================================


extern	entity_t	r_worldentity;
extern	qboolean	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	entity_t	*currententity;
extern	int			r_visframecount;	// ??? what difs?
extern	int			r_framecount;
extern	mplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys;


//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_refdef;
extern	mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern	texture_t	*r_notexture_mip;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean	envmap;
extern	int	currenttexture;
extern	int	cnttextures[2];
extern	int	particletexture;
extern	int	playertextures;

extern	int	skytexturenum;		// index in cl.loadmodel, not gx texture object

extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_speeds;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_lightmap;
extern	cvar_t	r_shadows;
extern	cvar_t	r_mirroralpha;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_dynamic;
extern	cvar_t	r_novis;

extern	cvar_t	gx_clear;
extern	cvar_t	gx_cull;
extern	cvar_t	gx_poly;
extern	cvar_t	gx_texsort;
extern	cvar_t	gx_smoothmodels;
extern	cvar_t	gx_affinemodels;
extern	cvar_t	gx_polyblend;
extern	cvar_t	gx_keeptjunctions;
extern	cvar_t	gx_reporttjunctions;
extern	cvar_t	gx_flashblend;
extern	cvar_t	gx_nocolors;
extern	cvar_t	gx_doubleeyes;

extern	int		gx_lightmap_format;
extern	int		gx_solid_format;
extern	int		gx_alpha_format;

extern	cvar_t	gx_max_size;
extern	cvar_t	gx_playermip;

extern	int			mirrortexturenum;	// quake texturenum, not gxtexturenum
extern	qboolean	mirror;
extern	mplane_t	*mirror_plane;

extern	float	r_world_matrix[16];

void R_TranslatePlayerSkin (int playernum);
void GX_Bind (int texnum);

// Multitexture
#define    TEXTURE0_SGIS				0x835E
#define    TEXTURE1_SGIS				0x835F

typedef void (*lpMTexFUNC) (GLenum, GLfloat, GLfloat);
typedef void (*lpSelTexFUNC) (GLenum);
extern lpMTexFUNC qgxMTexCoord2fSGIS;
extern lpSelTexFUNC qgxSelectTextureSGIS;

extern qboolean gx_mtexable;

void GX_DisableMultitexture(void);
void GX_EnableMultitexture(void);