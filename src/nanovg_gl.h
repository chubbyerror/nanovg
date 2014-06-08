//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#ifndef NANOVG_GL_H
#define NANOVG_GL_H

#ifdef __cplusplus
extern "C" {
#endif

#define NVG_ANTIALIAS 1

#if defined NANOVG_GL2_IMPLEMENTATION
#  define NANOVG_GL2 1
#  define NANOVG_GL_IMPLEMENTATION 1
#elif defined NANOVG_GL3_IMPLEMENTATION
#  define NANOVG_GL3 1
#  define NANOVG_GL_IMPLEMENTATION 1
#  define NANOVG_GL_USE_UNIFORMBUFFER 1
#elif defined NANOVG_GLES2_IMPLEMENTATION
#  define NANOVG_GLES2 1
#  define NANOVG_GL_IMPLEMENTATION 1
#elif defined NANOVG_GLES3_IMPLEMENTATION
#  define NANOVG_GLES3 1
#  define NANOVG_GL_IMPLEMENTATION 1
#endif


#if defined NANOVG_GL2

struct NVGcontext* nvgCreateGL2(int atlasw, int atlash, int edgeaa);
void nvgDeleteGL2(struct NVGcontext* ctx);

#elif defined NANOVG_GL3

struct NVGcontext* nvgCreateGL3(int atlasw, int atlash, int edgeaa);
void nvgDeleteGL3(struct NVGcontext* ctx);

#elif defined NANOVG_GLES2

struct NVGcontext* nvgCreateGLES2(int atlasw, int atlash, int edgeaa);
void nvgDeleteGLES2(struct NVGcontext* ctx);

#elif defined NANOVG_GLES3

struct NVGcontext* nvgCreateGLES3(int atlasw, int atlash, int edgeaa);
void nvgDeleteGLES3(struct NVGcontext* ctx);

#endif

enum NVGLtextureflags {
	NVGL_TEXTURE_FLIP_Y   = 0x01,
	NVGL_TEXTURE_NODELETE = 0x02,
};

int nvglCreateImageFromHandle(struct NVGcontext* ctx, GLuint textureId, int flags);
GLuint nvglImageHandle(struct NVGcontext* ctx, int image);
void nvglImageFlags(struct NVGcontext* ctx, int image, int flags);


#ifdef __cplusplus
}
#endif

#endif /* NANOVG_GL_H */

#ifdef NANOVG_GL_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nanovg.h"

enum GLNVGuniformLoc {
	GLNVG_LOC_VIEWSIZE,
	GLNVG_LOC_TEX,
#if NANOVG_GL_USE_UNIFORMBUFFER
	GLNVG_LOC_FRAG,
#else
	GLNVG_LOC_SCISSORMAT,
	GLNVG_LOC_SCISSOREXT,
	GLNVG_LOC_SCISSORSCALE,
	GLNVG_LOC_PAINTMAT,
	GLNVG_LOC_EXTENT,
	GLNVG_LOC_RADIUS,
	GLNVG_LOC_FEATHER,
	GLNVG_LOC_INNERCOL,
	GLNVG_LOC_OUTERCOL,
	GLNVG_LOC_STROKEMULT,
	GLNVG_LOC_TEXTYPE,
	GLNVG_LOC_TYPE,
#endif
	GLNVG_MAX_LOCS
};

enum GLNVGshaderType {
	NSVG_SHADER_FILLGRAD,
	NSVG_SHADER_FILLIMG,
	NSVG_SHADER_SIMPLE,
	NSVG_SHADER_IMG
};

#if NANOVG_GL_USE_UNIFORMBUFFER
enum GLNVGuniformBindings {
	GLNVG_FRAG_BINDING = 0,
};
#endif

struct GLNVGshader {
	GLuint prog;
	GLuint frag;
	GLuint vert;
	GLint loc[GLNVG_MAX_LOCS];
};

struct GLNVGtexture {
	int id;
	GLuint tex;
	int width, height;
	int type;
	int flags;
};

enum GLNVGcallType {
	GLNVG_NONE = 0,
	GLNVG_FILL,
	GLNVG_CONVEXFILL,
	GLNVG_STROKE,
	GLNVG_TRIANGLES,
};

struct GLNVGcall {
	int type;
	int image;
	int pathOffset;
	int pathCount;
	int triangleOffset;
	int triangleCount;
	int uniformOffset;
};

struct GLNVGpath {
	int fillOffset;
	int fillCount;
	int strokeOffset;
	int strokeCount;
};

struct GLNVGfragUniforms {
   float scissorMat[12]; // matrices are actually 3 vec4s
   float paintMat[12];
   struct NVGcolor innerCol;
   struct NVGcolor outerCol;
   float scissorExt[2];
   float scissorScale[2];
   float extent[2];
   float radius;
   float feather;
   float strokeMult;
   int texType;
   int type;
};

struct GLNVGcontext {
	struct GLNVGshader shader;
	struct GLNVGtexture* textures;
	float view[2];
	int ntextures;
	int ctextures;
	int textureId;
	GLuint vertBuf;
#if defined NANOVG_GL3
	GLuint vertArr;
#endif
#if NANOVG_GL_USE_UNIFORMBUFFER
	GLuint fragBuf;
#endif
	int fragSize;
	int edgeAntiAlias;

	// Per frame buffers
	struct GLNVGcall* calls;
	int ccalls;
	int ncalls;
	struct GLNVGpath* paths;
	int cpaths;
	int npaths;
	struct NVGvertex* verts;
	int cverts;
	int nverts;
	unsigned char* uniforms;
	int cuniforms;
	int nuniforms;
};

static int glnvg__maxi(int a, int b) { return a > b ? a : b; }

static struct GLNVGtexture* glnvg__allocTexture(struct GLNVGcontext* gl)
{
	struct GLNVGtexture* tex = NULL;
	int i;

	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].id == 0) {
			tex = &gl->textures[i];
			break;
		}
	}
	if (tex == NULL) {
		if (gl->ntextures+1 > gl->ctextures) {
			struct GLNVGtexture* textures;
			int ctextures = glnvg__maxi(gl->ntextures+1, 4) +  gl->ctextures/2; // 1.5x Overallocate
			textures = (struct GLNVGtexture*)realloc(gl->textures, sizeof(struct GLNVGtexture)*ctextures);
			if (textures == NULL) return NULL;
			gl->textures = textures;
			gl->ctextures = ctextures;
		}
		tex = &gl->textures[gl->ntextures++];
	}
	
	memset(tex, 0, sizeof(*tex));
	tex->id = ++gl->textureId;
	
	return tex;
}

static struct GLNVGtexture* glnvg__findTexture(struct GLNVGcontext* gl, int id)
{
	int i;
	for (i = 0; i < gl->ntextures; i++)
		if (gl->textures[i].id == id)
			return &gl->textures[i];
	return NULL;
}

static int glnvg__deleteTexture(struct GLNVGcontext* gl, int id)
{
	int i;
	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].id == id) {
			if (gl->textures[i].tex != 0 && (gl->textures[i].flags & NVGL_TEXTURE_NODELETE) == 0)
				glDeleteTextures(1, &gl->textures[i].tex);
			memset(&gl->textures[i], 0, sizeof(gl->textures[i]));
			return 1;
		}
	}
	return 0;
}

static void glnvg__dumpShaderError(GLuint shader, const char* name, const char* type)
{
	char str[512+1];
	int len = 0;
	glGetShaderInfoLog(shader, 512, &len, str);
	if (len > 512) len = 512;
	str[len] = '\0';
	printf("Shader %s/%s error:\n%s\n", name, type, str);
}

static void glnvg__dumpProgramError(GLuint prog, const char* name)
{
	char str[512+1];
	int len = 0;
	glGetProgramInfoLog(prog, 512, &len, str);
	if (len > 512) len = 512;
	str[len] = '\0';
	printf("Program %s error:\n%s\n", name, str);
}

static int glnvg__checkError(const char* str)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("Error %08x after %s\n", err, str);
		return 1;
	}
	return 0;
}

static int glnvg__createShader(struct GLNVGshader* shader, const char* name, const char* header, const char* opts, const char* vshader, const char* fshader)
{
	GLint status;
	GLuint prog, vert, frag;
	const char* str[3];
	str[0] = header;
	str[1] = opts != NULL ? opts : "";

	memset(shader, 0, sizeof(*shader));

	prog = glCreateProgram();
	vert = glCreateShader(GL_VERTEX_SHADER);
	frag = glCreateShader(GL_FRAGMENT_SHADER);
	str[2] = vshader;
	glShaderSource(vert, 3, str, 0);
	str[2] = fshader;
	glShaderSource(frag, 3, str, 0);

	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpShaderError(vert, name, "vert");
		return 0;
	}

	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpShaderError(frag, name, "frag");
		return 0;
	}

	glAttachShader(prog, vert);
	glAttachShader(prog, frag);

	glBindAttribLocation(prog, 0, "vertex");
	glBindAttribLocation(prog, 1, "tcoord");

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpProgramError(prog, name);
		return 0;
	}

	shader->prog = prog;
	shader->vert = vert;
	shader->frag = frag;

	return 1;
}

static void glnvg__deleteShader(struct GLNVGshader* shader)
{
	if (shader->prog != 0)
		glDeleteProgram(shader->prog);
	if (shader->vert != 0)
		glDeleteShader(shader->vert);
	if (shader->frag != 0)
		glDeleteShader(shader->frag);
}

static void glnvg__getUniforms(struct GLNVGshader* shader)
{
	shader->loc[GLNVG_LOC_VIEWSIZE] = glGetUniformLocation(shader->prog, "viewSize");
	shader->loc[GLNVG_LOC_TEX] = glGetUniformLocation(shader->prog, "tex");

#if NANOVG_GL_USE_UNIFORMBUFFER
	shader->loc[GLNVG_LOC_FRAG] = glGetUniformBlockIndex(shader->prog, "frag");
#else
	shader->loc[GLNVG_LOC_SCISSORMAT] = glGetUniformLocation(shader->prog, "scissorMat");
	shader->loc[GLNVG_LOC_SCISSOREXT] = glGetUniformLocation(shader->prog, "scissorExt");
	shader->loc[GLNVG_LOC_SCISSORSCALE] = glGetUniformLocation(shader->prog, "scissorScale");
	shader->loc[GLNVG_LOC_PAINTMAT] = glGetUniformLocation(shader->prog, "paintMat");
	shader->loc[GLNVG_LOC_EXTENT] = glGetUniformLocation(shader->prog, "extent");
	shader->loc[GLNVG_LOC_RADIUS] = glGetUniformLocation(shader->prog, "radius");
	shader->loc[GLNVG_LOC_FEATHER] = glGetUniformLocation(shader->prog, "feather");
	shader->loc[GLNVG_LOC_INNERCOL] = glGetUniformLocation(shader->prog, "innerCol");
	shader->loc[GLNVG_LOC_OUTERCOL] = glGetUniformLocation(shader->prog, "outerCol");
	shader->loc[GLNVG_LOC_STROKEMULT] = glGetUniformLocation(shader->prog, "strokeMult");
	shader->loc[GLNVG_LOC_TEXTYPE] = glGetUniformLocation(shader->prog, "texType");
	shader->loc[GLNVG_LOC_TYPE] = glGetUniformLocation(shader->prog, "type");
#endif
}

static int glnvg__renderCreate(void* uptr)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	int align = 4;

	// TODO: mediump float may not be enough for GLES2 in iOS.
	// see the following discussion: https://github.com/memononen/nanovg/issues/46
	static const char* shaderHeader =
#if defined NANOVG_GL2
		"#define NANOVG_GL2 1\n";
#elif defined NANOVG_GL3
		"#version 150 core\n"
#if NANOVG_GL_USE_UNIFORMBUFFER
		"#define USE_UNIFORMBUFFER 1\n"
#endif
		"#define NANOVG_GL3 1\n";
#elif defined NANOVG_GLES2
		"#version 100\n"
		"#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
		" precision highp float;\n"
		"#else\n"
		" precision mediump float;\n"
		"#endif\n"
		"#define NANOVG_GL2 1\n";
#elif defined NANOVG_GLES3
		"#version 300 es\n"
		"precision highp float;\n"
		"#define NANOVG_GL3 1\n";
#endif

	static const char* fillVertShader =
		"#ifdef NANOVG_GL3\n"
		"	uniform vec2 viewSize;\n"
		"	in vec2 vertex;\n"
		"	in vec2 tcoord;\n"
		"	out vec2 ftcoord;\n"
		"	out vec2 fpos;\n"
		"#else\n"
		"	uniform vec2 viewSize;\n"
		"	attribute vec2 vertex;\n"
		"	attribute vec2 tcoord;\n"
		"	varying vec2 ftcoord;\n"
		"	varying vec2 fpos;\n"
		"#endif\n"
		"void main(void) {\n"
		"	ftcoord = tcoord;\n"
		"	fpos = vertex;\n"
		"	gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);\n"
		"}\n";

	static const char* fillFragShader = 
		"#ifdef NANOVG_GL3\n"
		"#ifdef USE_UNIFORMBUFFER\n"
		"	layout(std140) uniform frag {\n"
		"		mat3 scissorMat;\n"
		"		mat3 paintMat;\n"
		"		vec4 innerCol;\n"
		"		vec4 outerCol;\n"
		"		vec2 scissorExt;\n"
		"		vec2 scissorScale;\n"
		"		vec2 extent;\n"
		"		float radius;\n"
		"		float feather;\n"
		"		float strokeMult;\n"
		"		int texType;\n"
		"		int type;\n"
		"	};\n"
		"#else\n"
		"	uniform mat3 scissorMat;\n"
		"	uniform mat3 paintMat;\n"
		"	uniform vec4 innerCol;\n"
		"	uniform vec4 outerCol;\n"
		"	uniform vec2 scissorExt;\n"
		"	uniform vec2 scissorScale;\n"
		"	uniform vec2 extent;\n"
		"	uniform float radius;\n"
		"	uniform float feather;\n"
		"	uniform float strokeMult;\n"
		"	uniform int texType;\n"
		"	uniform int type;\n"
		"#endif\n"
		"	uniform sampler2D tex;\n"
		"	in vec2 ftcoord;\n"
		"	in vec2 fpos;\n"
		"	out vec4 outColor;\n"
		"#else\n"
		"	uniform mat3 scissorMat;\n"
		"	uniform mat3 paintMat;\n"
		"	uniform vec4 innerCol;\n"
		"	uniform vec4 outerCol;\n"
		"	uniform vec2 scissorExt;\n"
		"	uniform vec2 scissorScale;\n"
		"	uniform vec2 extent;\n"
		"	uniform float radius;\n"
		"	uniform float feather;\n"
		"	uniform float strokeMult;\n"
		"	uniform int texType;\n"
		"	uniform int type;\n"
		"	uniform sampler2D tex;\n"
		"	varying vec2 ftcoord;\n"
		"	varying vec2 fpos;\n"
		"#endif\n"
		"\n"
		"float sdroundrect(vec2 pt, vec2 ext, float rad) {\n"
		"	vec2 ext2 = ext - vec2(rad,rad);\n"
		"	vec2 d = abs(pt) - ext2;\n"
		"	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;\n"
		"}\n"
		"\n"
		"// Scissoring\n"
		"float scissorMask(vec2 p) {\n"
		"	vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);\n"
		"	sc = vec2(0.5,0.5) - sc * scissorScale;\n"
		"	return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);\n"
		"}\n"
		"#ifdef EDGE_AA\n"
		"// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.\n"
		"float strokeMask() {\n"
		"	return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y);\n"
		"}\n"
		"#endif\n"
		"\n"
		"void main(void) {\n"
		"   vec4 result;\n"
		"	float scissor = scissorMask(fpos);\n"
		"#ifdef EDGE_AA\n"
		"	float strokeAlpha = strokeMask();\n"
		"#else\n"
		"	float strokeAlpha = 1.0;\n"
		"#endif\n"
		"	if (type == 0) {			// Gradient\n"
		"		// Calculate gradient color using box gradient\n"
		"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy;\n"
		"		float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
		"		vec4 color = mix(innerCol,outerCol,d);\n"
		"		// Combine alpha\n"
		"		color.w *= strokeAlpha * scissor;\n"
		"		result = color;\n"
		"	} else if (type == 1) {		// Image\n"
		"		// Calculate color fron texture\n"
		"		vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;\n"
		"#ifdef NANOVG_GL3\n"
		"		vec4 color = texture(tex, pt);\n"
		"#else\n"
		"		vec4 color = texture2D(tex, pt);\n"
		"#endif\n"
		"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
		"		// Combine alpha\n"
		"		color.w *= strokeAlpha * scissor;\n"
		"		result = color;\n"
		"	} else if (type == 2) {		// Stencil fill\n"
		"		result = vec4(1,1,1,1);\n"
		"	} else if (type == 3) {		// Textured tris\n"
		"#ifdef NANOVG_GL3\n"
		"		vec4 color = texture(tex, ftcoord);\n"
		"#else\n"
		"		vec4 color = texture2D(tex, ftcoord);\n"
		"#endif\n"
		"   	color = texType == 0 ? color : vec4(1,1,1,color.x);\n"
		"		color.w *= scissor;\n"
		"		result = color * innerCol;\n"
		"	}\n"
		"#ifdef NANOVG_GL3\n"
		"	outColor = result;\n"
		"#else\n"
		"	gl_FragColor = result;\n"
		"#endif\n"
		"}\n";

	glnvg__checkError("init");

	if (gl->edgeAntiAlias) {
		if (glnvg__createShader(&gl->shader, "shader", shaderHeader, "#define EDGE_AA 1\n", fillVertShader, fillFragShader) == 0)
			return 0;
	} else {
		if (glnvg__createShader(&gl->shader, "shader", shaderHeader, NULL, fillVertShader, fillFragShader) == 0)
			return 0;
	}

	glnvg__checkError("uniform locations");
	glnvg__getUniforms(&gl->shader);

	// Create dynamic vertex array
#if defined NANOVG_GL3
	glGenVertexArrays(1, &gl->vertArr);
#endif
	glGenBuffers(1, &gl->vertBuf);

#if NANOVG_GL_USE_UNIFORMBUFFER
	// Create UBOs
	glUniformBlockBinding(gl->shader.prog, gl->shader.loc[GLNVG_LOC_FRAG], GLNVG_FRAG_BINDING);
	glGenBuffers(1, &gl->fragBuf); 
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
#endif
	gl->fragSize = sizeof(struct GLNVGfragUniforms) + align - sizeof(struct GLNVGfragUniforms) % align;

	glnvg__checkError("create done");

	glFinish();

	return 1;
}

static int glnvg__renderCreateTexture(void* uptr, int type, int w, int h, const unsigned char* data)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGtexture* tex = glnvg__allocTexture(gl);

	if (tex == NULL) return 0;

	glGenTextures(1, &tex->tex);
	tex->width = w;
	tex->height = h;
	tex->type = type;
	glBindTexture(GL_TEXTURE_2D, tex->tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
#ifndef NANOVG_GLES2
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#endif

	if (type == NVG_TEXTURE_RGBA)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
#if defined(NANOVG_GLES2)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
#elif defined(NANOVG_GLES3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
#endif

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
#ifndef NANOVG_GLES2
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#endif

	if (glnvg__checkError("create tex"))
		return 0;

	glBindTexture(GL_TEXTURE_2D, 0);

	return tex->id;
}


static int glnvg__renderDeleteTexture(void* uptr, int image)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	return glnvg__deleteTexture(gl, image);
}

static int glnvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGtexture* tex = glnvg__findTexture(gl, image);

	if (tex == NULL) return 0;
	glBindTexture(GL_TEXTURE_2D, tex->tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

#ifndef NANOVG_GLES2
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, x);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, y);
#else
	// No support for all of skip, need to update a whole row at a time.
	if (tex->type == NVG_TEXTURE_RGBA)
		data += y*tex->width*4;
	else
		data += y*tex->width;
	x = 0;
	w = tex->width;
#endif

	if (tex->type == NVG_TEXTURE_RGBA)
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
#ifdef NANOVG_GLES2
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
#else
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_RED, GL_UNSIGNED_BYTE, data);
#endif

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
#ifndef NANOVG_GLES2
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#endif

	glBindTexture(GL_TEXTURE_2D, 0);

	return 1;
}

static int glnvg__renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
	if (tex == NULL) return 0;
	*w = tex->width;
	*h = tex->height;
	return 1;
}

static void glnvg__xformToMat3x4(float* m3, float* t)
{
	m3[0] = t[0];
	m3[1] = t[1];
	m3[2] = 0.0f;
	m3[3] = 0.0f;
	m3[4] = t[2];
	m3[5] = t[3];
	m3[6] = 0.0f;
	m3[7] = 0.0f;
	m3[8] = t[4];
	m3[9] = t[5];
	m3[10] = 1.0f;
	m3[11] = 0.0f;
}

static int glnvg__convertPaint(struct GLNVGcontext* gl, struct GLNVGfragUniforms* frag, struct NVGpaint* paint,
							   struct NVGscissor* scissor, float width, float fringe)
{
	struct GLNVGtexture* tex = NULL;
	float invxform[6];

	memset(frag, 0, sizeof(*frag));

	frag->innerCol = paint->innerColor;
	frag->outerCol = paint->outerColor;

	if (scissor->extent[0] < 0.5f || scissor->extent[1] < 0.5f) {
		memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
		frag->scissorExt[0] = 1.0f;
		frag->scissorExt[1] = 1.0f;
		frag->scissorScale[0] = 1.0f;
		frag->scissorScale[1] = 1.0f;
	} else {
		nvgTransformInverse(invxform, scissor->xform);
		glnvg__xformToMat3x4(frag->scissorMat, invxform);
		frag->scissorExt[0] = scissor->extent[0];
		frag->scissorExt[1] = scissor->extent[1];
		frag->scissorScale[0] = sqrtf(scissor->xform[0]*scissor->xform[0] + scissor->xform[2]*scissor->xform[2]) / fringe;
		frag->scissorScale[1] = sqrtf(scissor->xform[1]*scissor->xform[1] + scissor->xform[3]*scissor->xform[3]) / fringe;
	}

	memcpy(frag->extent, paint->extent, sizeof(frag->extent));
	frag->strokeMult = (width*0.5f + fringe*0.5f) / fringe;

	if (paint->image != 0) {
		tex = glnvg__findTexture(gl, paint->image);
		if (tex == NULL) return 0;
		if ((tex->flags & NVGL_TEXTURE_FLIP_Y) != 0) {
			float flipped[6];
			nvgTransformScale(flipped, 1.0f, -1.0f);
			nvgTransformMultiply(flipped, paint->xform);
			nvgTransformInverse(invxform, flipped);
		} else {
			nvgTransformInverse(invxform, paint->xform);
		}
		frag->type = NSVG_SHADER_FILLIMG;
		frag->texType = tex->type == NVG_TEXTURE_RGBA ? 0 : 1;
	} else {
		frag->type = NSVG_SHADER_FILLGRAD;
		frag->radius = paint->radius;
		frag->feather = paint->feather;
		nvgTransformInverse(invxform, paint->xform);
	}

	glnvg__xformToMat3x4(frag->paintMat, invxform);

	return 1;
}

static struct GLNVGfragUniforms* nvg__fragUniformPtr(struct GLNVGcontext* gl, int i);

#if !NANOVG_GL_USE_UNIFORMBUFFER
static void glnvg__mat3(float* dst, float* src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];

	dst[3] = src[4];
	dst[4] = src[5];
	dst[5] = src[6];

	dst[6] = src[8];
	dst[7] = src[9];
	dst[8] = src[10];
}
#endif

static void glnvg__setUniforms(struct GLNVGcontext* gl, int uniformOffset, int image)
{
#if NANOVG_GL_USE_UNIFORMBUFFER
	glBindBufferRange(GL_UNIFORM_BUFFER, GLNVG_FRAG_BINDING, gl->fragBuf, uniformOffset, sizeof(struct GLNVGfragUniforms));
#else
	struct GLNVGfragUniforms* frag = nvg__fragUniformPtr(gl, uniformOffset);
	float tmp[9]; // Maybe there's a way to get rid of this...
	glnvg__mat3(tmp, frag->scissorMat);
	glUniformMatrix3fv(gl->shader.loc[GLNVG_LOC_SCISSORMAT], 1, GL_FALSE, tmp);
	glnvg__mat3(tmp, frag->paintMat);
	glUniformMatrix3fv(gl->shader.loc[GLNVG_LOC_PAINTMAT], 1, GL_FALSE, tmp);
	glUniform4fv(gl->shader.loc[GLNVG_LOC_INNERCOL], 1, frag->innerCol.rgba);
	glUniform4fv(gl->shader.loc[GLNVG_LOC_OUTERCOL], 1, frag->outerCol.rgba);
	glUniform2fv(gl->shader.loc[GLNVG_LOC_SCISSOREXT], 1, frag->scissorExt);
	glUniform2fv(gl->shader.loc[GLNVG_LOC_SCISSORSCALE], 1, frag->scissorScale);
	glUniform2fv(gl->shader.loc[GLNVG_LOC_EXTENT], 1, frag->extent);
	glUniform1f(gl->shader.loc[GLNVG_LOC_RADIUS], frag->radius);
	glUniform1f(gl->shader.loc[GLNVG_LOC_FEATHER], frag->feather);
	glUniform1f(gl->shader.loc[GLNVG_LOC_STROKEMULT], frag->strokeMult);
	glUniform1i(gl->shader.loc[GLNVG_LOC_TEXTYPE], frag->texType);
	glUniform1i(gl->shader.loc[GLNVG_LOC_TYPE], frag->type);
#endif

	if (image != 0) {
		struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
		glBindTexture(GL_TEXTURE_2D, tex != NULL ? tex->tex : 0);
		glnvg__checkError("tex paint tex");
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

static void glnvg__renderViewport(void* uptr, int width, int height, int alphaBlend)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	NVG_NOTUSED(alphaBlend);
	gl->view[0] = (float)width;
	gl->view[1] = (float)height;
}

static void glnvg__fill(struct GLNVGcontext* gl, struct GLNVGcall* call)
{
	struct GLNVGpath* paths = &gl->paths[call->pathOffset];
	int i, npaths = call->pathCount;

	// Draw shapes
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xff);
	glStencilFunc(GL_ALWAYS, 0, ~0L);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// set bindpoint for solid loc
	glnvg__setUniforms(gl, call->uniformOffset, 0);
	glnvg__checkError("fill simple");

	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
	glDisable(GL_CULL_FACE);
	for (i = 0; i < npaths; i++)
		glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
	glEnable(GL_CULL_FACE);

	// Draw aliased off-pixels
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glnvg__setUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
	glnvg__checkError("fill fill");

	if (gl->edgeAntiAlias) {
		glStencilFunc(GL_EQUAL, 0x00, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		// Draw fringes
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
	}

	// Draw fill
	glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
	glDrawArrays(GL_TRIANGLES, call->triangleOffset, call->triangleCount);

	glDisable(GL_STENCIL_TEST);
}

static void glnvg__convexFill(struct GLNVGcontext* gl, struct GLNVGcall* call)
{
	struct GLNVGpath* paths = &gl->paths[call->pathOffset];
	int i, npaths = call->pathCount;

	glnvg__setUniforms(gl, call->uniformOffset, call->image);
	glnvg__checkError("convex fill");

	for (i = 0; i < npaths; i++)
		glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
	if (gl->edgeAntiAlias) {
		// Draw fringes
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
	}
}

static void glnvg__stroke(struct GLNVGcontext* gl, struct GLNVGcall* call)
{
	struct GLNVGpath* paths = &gl->paths[call->pathOffset];
	int npaths = call->pathCount, i;

	glnvg__setUniforms(gl, call->uniformOffset, call->image);
	glnvg__checkError("stroke fill");

	// Draw Strokes
	for (i = 0; i < npaths; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
}

static void glnvg__triangles(struct GLNVGcontext* gl, struct GLNVGcall* call)
{
	glnvg__setUniforms(gl, call->uniformOffset, call->image);
	glnvg__checkError("triangles fill");

	glDrawArrays(GL_TRIANGLES, call->triangleOffset, call->triangleCount);
}

static void glnvg__renderFlush(void* uptr, int alphaBlend)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	int i;

	if (gl->ncalls > 0) {

		// Setup require GL state.
		glUseProgram(gl->shader.prog);

		if (alphaBlend == NVG_PREMULTIPLIED_ALPHA)
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_SCISSOR_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glStencilMask(0xffffffff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
		glActiveTexture(GL_TEXTURE0);

#if NANOVG_GL_USE_UNIFORMBUFFER
		// Upload ubo for frag shaders
		glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);
		glBufferData(GL_UNIFORM_BUFFER, gl->nuniforms * gl->fragSize, gl->uniforms, GL_STREAM_DRAW);
#endif

		// Upload vertex data
#if defined NANOVG_GL3
		glBindVertexArray(gl->vertArr);
#endif
		glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);
		glBufferData(GL_ARRAY_BUFFER, gl->nverts * sizeof(struct NVGvertex), gl->verts, GL_STREAM_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(size_t)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct NVGvertex), (const GLvoid*)(0 + 2*sizeof(float)));

		// Set view and texture just once per frame.		
		glUniform1i(gl->shader.loc[GLNVG_LOC_TEX], 0);
		glUniform2fv(gl->shader.loc[GLNVG_LOC_VIEWSIZE], 1, gl->view);

#if NANOVG_GL_USE_UNIFORMBUFFER
		glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);
#endif

		for (i = 0; i < gl->ncalls; i++) {
			struct GLNVGcall* call = &gl->calls[i];
			if (call->type == GLNVG_FILL)
				glnvg__fill(gl, call);
			else if (call->type == GLNVG_CONVEXFILL)
				glnvg__convexFill(gl, call);
			else if (call->type == GLNVG_STROKE)
				glnvg__stroke(gl, call);
			else if (call->type == GLNVG_TRIANGLES)
				glnvg__triangles(gl, call);
		}

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
#if defined NANOVG_GL3
		glBindVertexArray(0);
#endif
		glUseProgram(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Reset calls
	gl->nverts = 0;
	gl->npaths = 0;
	gl->ncalls = 0;
	gl->nuniforms = 0;
}

static int glnvg__maxVertCount(const struct NVGpath* paths, int npaths)
{
	int i, count = 0;
	for (i = 0; i < npaths; i++) {
		count += paths[i].nfill;
		count += paths[i].nstroke;
	}
	return count;
}

static struct GLNVGcall* glnvg__allocCall(struct GLNVGcontext* gl)
{
	struct GLNVGcall* ret = NULL;
	if (gl->ncalls+1 > gl->ccalls) {
		struct GLNVGcall* calls;
		int ccalls = glnvg__maxi(gl->ncalls+1, 128) + gl->ccalls; // 1.5x Overallocate
		calls = (struct GLNVGcall*)realloc(gl->calls, sizeof(struct GLNVGcall) * ccalls);
		if (calls == NULL) return NULL;
		gl->calls = calls;
		gl->ccalls = ccalls;
	}
	ret = &gl->calls[gl->ncalls++];
	memset(ret, 0, sizeof(struct GLNVGcall));
	return ret;
}

static int glnvg__allocPaths(struct GLNVGcontext* gl, int n)
{
	int ret = 0;
	if (gl->npaths+n > gl->cpaths) {
		struct GLNVGpath* paths;
		int cpaths = glnvg__maxi(gl->npaths + n, 128) + gl->cpaths; // 1.5x Overallocate
		paths = (struct GLNVGpath*)realloc(gl->paths, sizeof(struct GLNVGpath) * cpaths);
		if (paths == NULL) return -1;
		gl->paths = paths;
		gl->cpaths = cpaths;
	}
	ret = gl->npaths;
	gl->npaths += n;
	return ret;
}

static int glnvg__allocVerts(struct GLNVGcontext* gl, int n)
{
	int ret = 0;
	if (gl->nverts+n > gl->cverts) {
		struct NVGvertex* verts;
		int cverts = glnvg__maxi(gl->nverts + n, 4096) + gl->cverts/2; // 1.5x Overallocate
		verts = (struct NVGvertex*)realloc(gl->verts, sizeof(struct NVGvertex) * cverts);
		if (verts == NULL) return -1;
		gl->verts = verts;
		gl->cverts = cverts;
	}
	ret = gl->nverts;
	gl->nverts += n;
	return ret;
}

static int glnvg__allocFragUniforms(struct GLNVGcontext* gl, int n)
{
	int ret = 0, structSize = gl->fragSize;
	if (gl->nuniforms+n > gl->cuniforms) {
		unsigned char* uniforms;
		int cuniforms = glnvg__maxi(gl->nuniforms+n, 128) + gl->cuniforms/2; // 1.5x Overallocate
		uniforms = (unsigned char*)realloc(gl->uniforms, structSize * cuniforms);
		if (uniforms == NULL) return -1;
		gl->uniforms = uniforms;
		gl->cuniforms = cuniforms;
	}
	ret = gl->nuniforms * structSize;
	gl->nuniforms += n;
	return ret;
}

static struct GLNVGfragUniforms* nvg__fragUniformPtr(struct GLNVGcontext* gl, int i)
{
	return (struct GLNVGfragUniforms*)&gl->uniforms[i];
}

static void glnvg__vset(struct NVGvertex* vtx, float x, float y, float u, float v)
{
	vtx->x = x;
	vtx->y = y;
	vtx->u = u;
	vtx->v = v;
}

static void glnvg__renderFill(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
							  const float* bounds, const struct NVGpath* paths, int npaths)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGcall* call = glnvg__allocCall(gl);
	struct NVGvertex* quad;
	struct GLNVGfragUniforms* frag;
	int i, maxverts, offset;

	if (call == NULL) return;

	call->type = GLNVG_FILL;
	call->pathOffset = glnvg__allocPaths(gl, npaths);
	if (call->pathOffset == -1) goto error;
	call->pathCount = npaths;
	call->image = paint->image;

	if (npaths == 1 && paths[0].convex)
		call->type = GLNVG_CONVEXFILL;

	// Allocate vertices for all the paths.
	maxverts = glnvg__maxVertCount(paths, npaths) + 6;
	offset = glnvg__allocVerts(gl, maxverts);
	if (offset == -1) goto error;

	for (i = 0; i < npaths; i++) {
		struct GLNVGpath* copy = &gl->paths[call->pathOffset + i];
		const struct NVGpath* path = &paths[i];
		memset(copy, 0, sizeof(struct GLNVGpath));
		if (path->nfill > 0) {
			copy->fillOffset = offset;
			copy->fillCount = path->nfill;
			memcpy(&gl->verts[offset], path->fill, sizeof(struct NVGvertex) * path->nfill);
			offset += path->nfill;
		}
		if (path->nstroke > 0) {
			copy->strokeOffset = offset;
			copy->strokeCount = path->nstroke;
			memcpy(&gl->verts[offset], path->stroke, sizeof(struct NVGvertex) * path->nstroke);
			offset += path->nstroke;
		}
	}

	// Quad
	call->triangleOffset = offset;
	call->triangleCount = 6;
	quad = &gl->verts[call->triangleOffset];
	glnvg__vset(&quad[0], bounds[0], bounds[3], 0.5f, 1.0f);
	glnvg__vset(&quad[1], bounds[2], bounds[3], 0.5f, 1.0f);
	glnvg__vset(&quad[2], bounds[2], bounds[1], 0.5f, 1.0f);

	glnvg__vset(&quad[3], bounds[0], bounds[3], 0.5f, 1.0f);
	glnvg__vset(&quad[4], bounds[2], bounds[1], 0.5f, 1.0f);
	glnvg__vset(&quad[5], bounds[0], bounds[1], 0.5f, 1.0f);

	// Setup uniforms for draw calls
	if (call->type == GLNVG_FILL) {
		call->uniformOffset = glnvg__allocFragUniforms(gl, 2);
		if (call->uniformOffset == -1) goto error;
		// Simple shader for stencil
		frag = nvg__fragUniformPtr(gl, call->uniformOffset);
		memset(frag, 0, sizeof(*frag));
		frag->type = NSVG_SHADER_SIMPLE;
		// Fill shader
		glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, fringe, fringe);
	} else {
		call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
		if (call->uniformOffset == -1) goto error;
		// Fill shader
		glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint, scissor, fringe, fringe);
	}

	return;

error:
	// We get here if call alloc was ok, but something else is not.
	// Roll back the last call to prevent drawing it.
	if (gl->ncalls > 0) gl->ncalls--;
}

static void glnvg__renderStroke(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
								float strokeWidth, const struct NVGpath* paths, int npaths)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGcall* call = glnvg__allocCall(gl);
	int i, maxverts, offset;

	if (call == NULL) return;

	call->type = GLNVG_STROKE;
	call->pathOffset = glnvg__allocPaths(gl, npaths);
	if (call->pathOffset == -1) goto error;
	call->pathCount = npaths;
	call->image = paint->image;

	// Allocate vertices for all the paths.
	maxverts = glnvg__maxVertCount(paths, npaths);
	offset = glnvg__allocVerts(gl, maxverts);
	if (offset == -1) goto error;

	for (i = 0; i < npaths; i++) {
		struct GLNVGpath* copy = &gl->paths[call->pathOffset + i];
		const struct NVGpath* path = &paths[i];
		memset(copy, 0, sizeof(struct GLNVGpath));
		if (path->nstroke) {
			copy->strokeOffset = offset;
			copy->strokeCount = path->nstroke;
			memcpy(&gl->verts[offset], path->stroke, sizeof(struct NVGvertex) * path->nstroke);
			offset += path->nstroke;
		}
	}

	// Fill shader
	call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
	if (call->uniformOffset == -1) goto error;
	glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint, scissor, strokeWidth, fringe);

	return;

error:
	// We get here if call alloc was ok, but something else is not.
	// Roll back the last call to prevent drawing it.
	if (gl->ncalls > 0) gl->ncalls--;
}

static void glnvg__renderTriangles(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor,
								   const struct NVGvertex* verts, int nverts)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	struct GLNVGcall* call = glnvg__allocCall(gl);
	struct GLNVGfragUniforms* frag;

	if (call == NULL) return;

	call->type = GLNVG_TRIANGLES;
	call->image = paint->image;

	// Allocate vertices for all the paths.
	call->triangleOffset = glnvg__allocVerts(gl, nverts);
	if (call->triangleOffset == -1) goto error;
	call->triangleCount = nverts;

	memcpy(&gl->verts[call->triangleOffset], verts, sizeof(struct NVGvertex) * nverts);

	// Fill shader
	call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
	if (call->uniformOffset == -1) goto error;
	frag = nvg__fragUniformPtr(gl, call->uniformOffset);
	glnvg__convertPaint(gl, frag, paint, scissor, 1.0f, 1.0f);
	frag->type = NSVG_SHADER_IMG;

	return;

error:
	// We get here if call alloc was ok, but something else is not.
	// Roll back the last call to prevent drawing it.
	if (gl->ncalls > 0) gl->ncalls--;
}

static void glnvg__renderDelete(void* uptr)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)uptr;
	int i;
	if (gl == NULL) return;

	glnvg__deleteShader(&gl->shader);

#if NANOVG_GL3
#if NANOVG_GL_USE_UNIFORMBUFFER
	if (gl->fragBuf != 0)
		glDeleteBuffers(1, &gl->fragBuf);
#endif
	if (gl->vertArr != 0)
		glDeleteVertexArrays(1, &gl->vertArr);
#endif
	if (gl->vertBuf != 0)
		glDeleteBuffers(1, &gl->vertBuf);

	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].tex != 0 && (gl->textures[i].flags & NVGL_TEXTURE_NODELETE) == 0)
			glDeleteTextures(1, &gl->textures[i].tex);
	}
	free(gl->textures);

	free(gl);
}


#if defined NANOVG_GL2
struct NVGcontext* nvgCreateGL2(int atlasw, int atlash, int edgeaa)
#elif defined NANOVG_GL3
struct NVGcontext* nvgCreateGL3(int atlasw, int atlash, int edgeaa)
#elif defined NANOVG_GLES2
struct NVGcontext* nvgCreateGLES2(int atlasw, int atlash, int edgeaa)
#elif defined NANOVG_GLES3
struct NVGcontext* nvgCreateGLES3(int atlasw, int atlash, int edgeaa)
#endif
{
	struct NVGparams params;
	struct NVGcontext* ctx = NULL;
	struct GLNVGcontext* gl = (struct GLNVGcontext*)malloc(sizeof(struct GLNVGcontext));
	if (gl == NULL) goto error;
	memset(gl, 0, sizeof(struct GLNVGcontext));

	memset(&params, 0, sizeof(params));
	params.renderCreate = glnvg__renderCreate;
	params.renderCreateTexture = glnvg__renderCreateTexture;
	params.renderDeleteTexture = glnvg__renderDeleteTexture;
	params.renderUpdateTexture = glnvg__renderUpdateTexture;
	params.renderGetTextureSize = glnvg__renderGetTextureSize;
	params.renderViewport = glnvg__renderViewport;
	params.renderFlush = glnvg__renderFlush;
	params.renderFill = glnvg__renderFill;
	params.renderStroke = glnvg__renderStroke;
	params.renderTriangles = glnvg__renderTriangles;
	params.renderDelete = glnvg__renderDelete;
	params.userPtr = gl;
	params.atlasWidth = atlasw;
	params.atlasHeight = atlash;
	params.edgeAntiAlias = edgeaa;

	gl->edgeAntiAlias = edgeaa;

	ctx = nvgCreateInternal(&params);
	if (ctx == NULL) goto error;

	return ctx;

error:
	// 'gl' is freed by nvgDeleteInternal.
	if (ctx != NULL) nvgDeleteInternal(ctx);
	return NULL;
}

#if NANOVG_GL2
void nvgDeleteGL2(struct NVGcontext* ctx)
#elif NANOVG_GL3
void nvgDeleteGL3(struct NVGcontext* ctx)
#elif NANOVG_GLES2
void nvgDeleteGLES2(struct NVGcontext* ctx)
#elif NANOVG_GLES3
void nvgDeleteGLES3(struct NVGcontext* ctx)
#endif
{
	nvgDeleteInternal(ctx);
}

int nvglCreateImageFromHandle(struct NVGcontext* ctx, GLuint textureId, int flags)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)nvgInternalParams(ctx)->userPtr;
	struct GLNVGtexture* tex = glnvg__allocTexture(gl);

	if (tex == NULL) return 0;

	tex->tex = textureId;
	tex->type = NVG_TEXTURE_RGBA;
    tex->flags = 0;
	glBindTexture(GL_TEXTURE_2D, tex->tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex->width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex->height);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex->id;
}

GLuint nvglImageHandle(struct NVGcontext* ctx, int image)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)nvgInternalParams(ctx)->userPtr;
	struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
	return tex->tex;
}

void nvglImageFlags(struct NVGcontext* ctx, int image, int flags)
{
	struct GLNVGcontext* gl = (struct GLNVGcontext*)nvgInternalParams(ctx)->userPtr;
	struct GLNVGtexture* tex = glnvg__findTexture(gl, image);
	tex->flags = flags;
}

#endif /* NANOVG_GL_IMPLEMENTATION */
