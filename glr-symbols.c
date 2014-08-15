#include "glr-symbols.h"

#include <EGL/egl.h>

#define STRINGIFY(x) (const char *) #x
#define glGetProcAddress(a) eglGetProcAddress (STRINGIFY (a))

typedef void   (GL_APIENTRY *PFNGLGENFRAMEBUFFERS)        (GLsizei  n,
                                                           GLuint  *ids);
typedef void   (GL_APIENTRY *PFNGLDELETEFRAMEBUFFERS)     (GLsizei       n,
                                                           const GLuint *framebuffers);
typedef void   (GL_APIENTRY *PFNGLBINDFRAMEBUFFER)        (GLenum target,
                                                           GLuint framebuffer);
typedef void   (GL_APIENTRY *PFNGLFRAMEBUFFERTEXTURE2D)   (GLenum target,
                                                           GLenum attachment,
                                                           GLenum textarget,
                                                           GLuint texture,
                                                           GLint  level);
typedef GLenum (GL_APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUS) (GLenum target);
typedef void   (GL_APIENTRY *PFNGLBLITFRAMEBUFFER)        (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                                           GLbitfield mask, GLenum filter);

typedef GLuint (GL_APIENTRY *PFNGLCREATEPROGRAM)          (void);
typedef void   (GL_APIENTRY *PFNGLDELETEPROGRAM)          (GLuint program);
typedef void   (GL_APIENTRY *PFNGLLINKPROGRAM)            (GLuint program);
typedef void   (GL_APIENTRY *PFNGLUSEPROGRAM)             (GLuint program);
typedef GLint  (GL_APIENTRY *PFNGLGETUNIFORMLOCATION)     (GLuint        program,
                                                           const GLchar *name);

typedef GLuint (GL_APIENTRY *PFNGLCREATESHADER)           (GLenum shaderType);
typedef void   (GL_APIENTRY *PFNGLDELETESHADER)           (GLuint shader);
typedef void   (GL_APIENTRY *PFNGLCOMPILESHADER)          (GLuint shader);
typedef void   (GL_APIENTRY *PFNGLATTACHSHADER)           (GLuint program,
                                                           GLuint shader);
typedef void   (GL_APIENTRY *PFNGLSHADERSOURCE)           (GLuint                shader,
                                                           GLsizei               count,
                                                           const GLchar * const *string,
                                                           const GLint          *length);
typedef void   (GL_APIENTRY *PFNGLGETSHADERIV)            (GLuint  shader,
                                                           GLenum  pname,
                                                           GLint  *params);
typedef void   (GL_APIENTRY *PFNGLGETSHADERINFOLOG)       (GLuint   shader,
                                                           GLsizei  maxLength,
                                                           GLsizei *length,
                                                           GLchar  *infoLog);

typedef void   (GL_APIENTRY *PFNGLUNIFORM1UI)             (GLint  location,
                                                           GLuint v0);
typedef void   (GL_APIENTRY *PFNGLUNIFORM1I)              (GLint  location,
                                                           GLint v0);

typedef void   (GL_APIENTRY *PFNGLBINDATTRIBLOCATION)     (GLuint        program,
                                                           GLuint        index,
                                                           const GLchar *name);
typedef void   (GL_APIENTRY *PFNGLVERTEXATTRIBPOINTER)    (GLuint        index,
                                                           GLint         size,
                                                           GLenum        type,
                                                           GLboolean     normalized,
                                                           GLsizei       stride,
                                                           const GLvoid *pointer);
typedef void   (GL_APIENTRY *PFNGLVERTEXATTRIBDIVISOR)    (GLuint index,
                                                           GLuint divisor);
typedef void   (GL_APIENTRY *PFNGLENABLEVERTEXATTRIBARRAY) (GLuint index);
typedef void   (GL_APIENTRY *PFNGLDRAWARRAYSINSTANCED)     (GLenum  mode,
                                                            GLint   first,
                                                            GLsizei count,
                                                            GLsizei primcount);

typedef void   (GL_APIENTRY *PFNGLTEXIMAGE2DMULTISAMPLE)  (GLenum    target,
                                                           GLsizei   samples,
                                                           GLint     internalformat,
                                                           GLsizei   width,
                                                           GLsizei   height,
                                                           GLboolean fixedsamplelocations);

typedef void  (GL_APIENTRY *PFNGLGENRENDERBUFFERS)              (GLsizei  n,
                                                                 GLuint  *renderbuffers);
typedef void (GL_APIENTRY *PFNGLRENDERBUFFERSTORAGEMULTISAMPLE) (GLenum  target,
                                                                 GLsizei samples,
                                                                 GLenum  internalformat,
                                                                 GLsizei width,
                                                                 GLsizei height);
typedef void (GL_APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFER)        (GLenum target,
                                                                 GLenum attachment,
                                                                 GLenum renderbuffertarget,
                                                                 GLuint renderbuffer);
typedef void (GL_APIENTRY *PFNGLBINDRENDERBUFFER)               (GLenum target,
                                                                 GLuint renderbuffer);
typedef void (GL_APIENTRY *PFNGLDELETERENDERBUFFERS)            (GLsizei       n,
                                                                 const GLuint *renderbuffers);

void
glGenFramebuffers (GLsizei n, GLuint* ids)
{
  ((PFNGLGENFRAMEBUFFERS) glGetProcAddress (glGenFramebuffers)) (n, ids);
}

void
glBindFramebuffer (GLenum target, GLuint framebuffer)
{
  ((PFNGLBINDFRAMEBUFFER) glGetProcAddress (glBindFramebuffer)) (target,
                                                                 framebuffer);
}

void
glGetShaderiv (GLuint  shader,
               GLenum  pname,
               GLint  *params)
{
  ((PFNGLGETSHADERIV) glGetProcAddress (glGetShaderiv)) (shader, pname, params);
}

void
glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
  ((PFNGLDELETEFRAMEBUFFERS)
   glGetProcAddress (glDeleteFramebuffers)) (n, framebuffers);
}

void
glFramebufferTexture2D (GLenum target,
                        GLenum attachment,
                        GLenum textarget,
                        GLuint texture,
                        GLint  level)
{
  ((PFNGLFRAMEBUFFERTEXTURE2D)
   glGetProcAddress (glFramebufferTexture2D)) (target,
                                               attachment,
                                               textarget,
                                               texture,
                                               level);
}

GLenum
glCheckFramebufferStatus (GLenum target)
{
  static PFNGLCHECKFRAMEBUFFERSTATUS pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLCHECKFRAMEBUFFERSTATUS)
      glGetProcAddress (glCheckFramebufferStatus);

  return pfn (target);
}

void
glBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                   GLbitfield mask, GLenum filter)
{
  static PFNGLBLITFRAMEBUFFER pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLBLITFRAMEBUFFER) glGetProcAddress (glBlitFramebuffer);

  pfn (srcX0, srcY0, srcX1, srcY1,
       dstX0, dstY0, dstX1, dstY1,
       mask,
       filter);
}

GLuint
glCreateProgram (void)
{
  static PFNGLCREATEPROGRAM pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLCREATEPROGRAM) glGetProcAddress (glCreateProgram);

  return pfn ();
}

void
glDeleteProgram (GLuint program)
{
  static PFNGLDELETEPROGRAM pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLDELETEPROGRAM)
     glGetProcAddress (glDeleteProgram);

  pfn (program);
}

void
glLinkProgram (GLuint program)
{
  static PFNGLLINKPROGRAM pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLLINKPROGRAM) glGetProcAddress (glLinkProgram);

  pfn (program);
}

void
glUseProgram (GLuint program)
{
  static PFNGLUSEPROGRAM pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLUSEPROGRAM) glGetProcAddress (glUseProgram);

  pfn (program);
}

GLint
glGetUniformLocation (GLuint program, const GLchar *name)
{
  static PFNGLGETUNIFORMLOCATION pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLGETUNIFORMLOCATION)
     glGetProcAddress (glGetUniformLocation);

  return pfn (program, name);
}

GLuint
glCreateShader (GLenum shaderType)
{
  static PFNGLCREATESHADER pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLCREATESHADER) glGetProcAddress (glCreateShader);

  return pfn (shaderType);
}

void
glDeleteShader (GLuint shader)
{
  static PFNGLDELETESHADER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLDELETESHADER)
     glGetProcAddress (glDeleteShader);

  pfn (shader);
}

void
glCompileShader (GLuint shader)
{
  static PFNGLCOMPILESHADER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLCOMPILESHADER)
     glGetProcAddress (glCompileShader);

  pfn (shader);
}

void
glAttachShader (GLuint program, GLuint shader)
{
  static PFNGLATTACHSHADER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLATTACHSHADER)
     glGetProcAddress (glAttachShader);

  pfn (program, shader);
}

void
glShaderSource (GLuint                shader,
                GLsizei               count,
                const GLchar * const *string,
                const GLint          *length)
{
  static PFNGLSHADERSOURCE pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLSHADERSOURCE)
     glGetProcAddress (glShaderSource);

  pfn (shader, count, string, length);
}

void
glGetShaderInfoLog (GLuint   shader,
                    GLsizei  maxLength,
                    GLsizei *length,
                    GLchar  *infoLog)
{
  static PFNGLGETSHADERINFOLOG pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLGETSHADERINFOLOG)
     glGetProcAddress (glGetShaderInfoLog);

  pfn (shader, maxLength, length, infoLog);
}

void
glTexImage2DMultisample (GLenum    target,
                         GLsizei   samples,
                         GLint     internalformat,
                         GLsizei   width,
                         GLsizei   height,
                         GLboolean fixedsamplelocations)
{
  static PFNGLTEXIMAGE2DMULTISAMPLE pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLTEXIMAGE2DMULTISAMPLE)
     glGetProcAddress (glTexImage2DMultisample);

  pfn (target, samples, internalformat, width, height, fixedsamplelocations);
}

void
glUniform1ui (GLint location, GLuint v0)
{
  static PFNGLUNIFORM1UI pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLUNIFORM1UI)
     glGetProcAddress (glUniform1ui);

  pfn (location, v0);
}

void
glUniform1i (GLint location, GLint v0)
{
  static PFNGLUNIFORM1I pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLUNIFORM1I)
     glGetProcAddress (glUniform1i);

  pfn (location, v0);
}

void
glBindAttribLocation (GLuint        program,
                      GLuint        index,
                      const GLchar *name)
{
  static PFNGLBINDATTRIBLOCATION pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLBINDATTRIBLOCATION)
     glGetProcAddress (glBindAttribLocation);

  pfn (program, index, name);
}

void
glVertexAttribPointer (GLuint        index,
                       GLint         size,
                       GLenum        type,
                       GLboolean     normalized,
                       GLsizei       stride,
                       const GLvoid *pointer)
{
  static PFNGLVERTEXATTRIBPOINTER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLVERTEXATTRIBPOINTER)
     glGetProcAddress (glVertexAttribPointer);

  pfn (index, size, type, normalized, stride, pointer);
}

void
glVertexAttribDivisor (GLuint index,
                       GLuint divisor)
{
  static PFNGLVERTEXATTRIBDIVISOR pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLVERTEXATTRIBDIVISOR)
     glGetProcAddress (glVertexAttribDivisor);

  pfn (index, divisor);
}

void
glEnableVertexAttribArray (GLuint index)
{
  static PFNGLENABLEVERTEXATTRIBARRAY pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLENABLEVERTEXATTRIBARRAY)
     glGetProcAddress (glEnableVertexAttribArray);

  pfn (index);
}

void
glDrawArraysInstanced (GLenum  mode,
                       GLint   first,
                       GLsizei count,
                       GLsizei primcount)
{
  static PFNGLDRAWARRAYSINSTANCED pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLDRAWARRAYSINSTANCED)
     glGetProcAddress (glDrawArraysInstanced);

  pfn (mode, first, count, primcount);
}

void
glGenRenderbuffers (GLsizei n, GLuint *renderbuffers)
{
  static PFNGLGENRENDERBUFFERS pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLGENRENDERBUFFERS)
     glGetProcAddress (glGenRenderbuffersOES);

  pfn (n, renderbuffers);
}

void
glRenderbufferStorageMultisample (GLenum  target,
                                  GLsizei samples,
                                  GLenum  internalformat,
                                  GLsizei width,
                                  GLsizei height)
{
  static PFNGLRENDERBUFFERSTORAGEMULTISAMPLE pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLE)
     glGetProcAddress (glRenderbufferStorageMultisample);

  pfn (target, samples, internalformat, width, height);
}

void
glFramebufferRenderbuffer (GLenum target,
                           GLenum attachment,
                           GLenum renderbuffertarget,
                           GLuint renderbuffer)
{
  static PFNGLFRAMEBUFFERRENDERBUFFER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLFRAMEBUFFERRENDERBUFFER)
     glGetProcAddress (glFramebufferRenderbuffer);

  pfn (target, attachment, renderbuffertarget, renderbuffer);
}

void
glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
  static PFNGLBINDRENDERBUFFER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLBINDRENDERBUFFER)
     glGetProcAddress (glBindRenderbuffer);

  pfn (target, renderbuffer);
}

void
glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers)
{
  static PFNGLDELETERENDERBUFFERS pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLDELETERENDERBUFFERS)
     glGetProcAddress (glDeleteRenderbuffers);

  pfn (n, renderbuffers);
}
