#include "glr-symbols.h"

#include <GL/glx.h>

#define STRINGIFY(x) (const GLubyte *) #x
#define glGetProcAddress(a) glXGetProcAddress (STRINGIFY (a))

typedef void   (GLAPIENTRY *PFNGLGENFRAMEBUFFERS)        (GLsizei  n,
                                                          GLuint  *ids);
typedef void   (GLAPIENTRY *PFNGLDELETEFRAMEBUFFERS)     (GLsizei       n,
                                                          const GLuint *framebuffers);
typedef void   (GLAPIENTRY *PFNGLBINDFRAMEBUFFER)        (GLenum target,
                                                          GLuint framebuffer);
typedef void   (GLAPIENTRY *PFNGLFRAMEBUFFERTEXTURE2D)   (GLenum target,
                                                          GLenum attachment,
                                                          GLenum textarget,
                                                          GLuint texture,
                                                          GLint  level);
typedef GLenum (GLAPIENTRY *PFNGLCHECKFRAMEBUFFERSTATUS) (GLenum target);
typedef void   (GLAPIENTRY *PFNGLBLITFRAMEBUFFER)        (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                                          GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                                          GLbitfield mask, GLenum filter);

typedef GLuint (GLAPIENTRY *PFNGLCREATEPROGRAM)          (void);
typedef void   (GLAPIENTRY *PFNGLDELETEPROGRAM)          (GLuint program);
typedef void   (GLAPIENTRY *PFNGLLINKPROGRAM)            (GLuint program);
typedef void   (GLAPIENTRY *PFNGLUSEPROGRAM)             (GLuint program);
typedef GLint  (GLAPIENTRY *PFNGLGETUNIFORMLOCATION)     (GLuint        program,
                                                          const GLchar *name);

typedef GLuint (GLAPIENTRY *PFNGLCREATESHADER)           (GLenum shaderType);
typedef void   (GLAPIENTRY *PFNGLDELETESHADER)           (GLuint shader);
typedef void   (GLAPIENTRY *PFNGLCOMPILESHADER)          (GLuint shader);
typedef void   (GLAPIENTRY *PFNGLATTACHSHADER)           (GLuint program,
                                                          GLuint shader);
typedef void   (GLAPIENTRY *PFNGLSHADERSOURCE)           (GLuint                shader,
                                                          GLsizei               count,
                                                          const GLchar * const *string,
                                                          const GLint          *length);
typedef void   (GLAPIENTRY *PFNGLGETSHADERIV)            (GLuint  shader,
                                                          GLenum  pname,
                                                          GLint  *params);
typedef void   (GLAPIENTRY *PFNGLGETSHADERINFOLOG)       (GLuint   shader,
                                                          GLsizei  maxLength,
                                                          GLsizei *length,
                                                          GLchar  *infoLog);

typedef void   (GLAPIENTRY *PFNGLUNIFORM1UI)             (GLint  location,
                                                          GLuint v0);
typedef void   (GLAPIENTRY *PFNGLUNIFORM1I)              (GLint  location,
                                                          GLint v0);

typedef void   (GLAPIENTRY *PFNGLBINDATTRIBLOCATION)     (GLuint        program,
                                                          GLuint        index,
                                                          const GLchar *name);
typedef void   (GLAPIENTRY *PFNGLVERTEXATTRIBPOINTER)    (GLuint        index,
                                                          GLint         size,
                                                          GLenum        type,
                                                          GLboolean     normalized,
                                                          GLsizei       stride,
                                                          const GLvoid *pointer);
typedef void   (GLAPIENTRY *PFNGLVERTEXATTRIBDIVISOR)    (GLuint index,
                                                          GLuint divisor);
typedef void   (GLAPIENTRY *PFNGLENABLEVERTEXATTRIBARRAY) (GLuint index);
typedef void   (GLAPIENTRY *PFNGLDRAWARRAYSINSTANCED)     (GLenum  mode,
                                                           GLint   first,
                                                           GLsizei count,
                                                           GLsizei primcount);

typedef void   (GLAPIENTRY *PFNGLTEXIMAGE2DMULTISAMPLE)  (GLenum    target,
                                                          GLsizei   samples,
                                                          GLint     internalformat,
                                                          GLsizei   width,
                                                          GLsizei   height,
                                                          GLboolean fixedsamplelocations);

typedef void  (GLAPIENTRY *PFNGLGENRENDERBUFFERS)              (GLsizei  n,
                                                                GLuint  *renderbuffers);
typedef void (GLAPIENTRY *PFNGLRENDERBUFFERSTORAGEMULTISAMPLE) (GLenum  target,
                                                                GLsizei samples,
                                                                GLenum  internalformat,
                                                                GLsizei width,
                                                                GLsizei height);
typedef void (GLAPIENTRY *PFNGLFRAMEBUFFERRENDERBUFFER)        (GLenum target,
                                                                GLenum attachment,
                                                                GLenum renderbuffertarget,
                                                                GLuint renderbuffer);
typedef void (GLAPIENTRY *PFNGLBINDRENDERBUFFER)               (GLenum target,
                                                                GLuint renderbuffer);
typedef void (GLAPIENTRY *PFNGLDELETERENDERBUFFERS)            (GLsizei       n,
                                                                const GLuint *renderbuffers);

inline void
glGenFramebuffers (GLsizei n, GLuint* ids)
{
  ((PFNGLGENFRAMEBUFFERS) glGetProcAddress (glGenFramebuffers)) (n, ids);
}

inline void
glBindFramebuffer (GLenum target, GLuint framebuffer)
{
  ((PFNGLBINDFRAMEBUFFER) glGetProcAddress (glBindFramebuffer)) (target,
                                                                 framebuffer);
}

inline void
glGetShaderiv (GLuint  shader,
               GLenum  pname,
               GLint  *params)
{
  ((PFNGLGETSHADERIV) glGetProcAddress (glGetShaderiv)) (shader, pname, params);
}

inline void
glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
  ((PFNGLDELETEFRAMEBUFFERS)
   glGetProcAddress (glDeleteFramebuffers)) (n, framebuffers);
}

inline void
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

inline GLenum
glCheckFramebufferStatus (GLenum target)
{
  static PFNGLCHECKFRAMEBUFFERSTATUS pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLCHECKFRAMEBUFFERSTATUS)
      glGetProcAddress (glCheckFramebufferStatus);

  return pfn (target);
}

inline void
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

inline GLuint
glCreateProgram (void)
{
  static PFNGLCREATEPROGRAM pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLCREATEPROGRAM) glGetProcAddress (glCreateProgram);

  return pfn ();
}

inline void
glDeleteProgram (GLuint program)
{
  static PFNGLDELETEPROGRAM pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLDELETEPROGRAM)
     glGetProcAddress (glDeleteProgram);

  pfn (program);
}

inline void
glLinkProgram (GLuint program)
{
  static PFNGLLINKPROGRAM pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLLINKPROGRAM) glGetProcAddress (glLinkProgram);

  pfn (program);
}

inline void
glUseProgram (GLuint program)
{
  static PFNGLUSEPROGRAM pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLUSEPROGRAM) glGetProcAddress (glUseProgram);

  pfn (program);
}

inline GLint
glGetUniformLocation (GLuint program, const GLchar *name)
{
  static PFNGLGETUNIFORMLOCATION pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLGETUNIFORMLOCATION)
     glGetProcAddress (glGetUniformLocation);

  return pfn (program, name);
}

inline GLuint
glCreateShader (GLenum shaderType)
{
  static PFNGLCREATESHADER pfn = NULL;

  if (pfn == NULL)
    pfn = (PFNGLCREATESHADER) glGetProcAddress (glCreateShader);

  return pfn (shaderType);
}

inline void
glDeleteShader (GLuint shader)
{
  static PFNGLDELETESHADER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLDELETESHADER)
     glGetProcAddress (glDeleteShader);

  pfn (shader);
}

inline void
glCompileShader (GLuint shader)
{
  static PFNGLCOMPILESHADER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLCOMPILESHADER)
     glGetProcAddress (glCompileShader);

  pfn (shader);
}

inline void
glAttachShader (GLuint program, GLuint shader)
{
  static PFNGLATTACHSHADER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLATTACHSHADER)
     glGetProcAddress (glAttachShader);

  pfn (program, shader);
}

inline void
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

inline void
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

inline void
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

inline void
glUniform1ui (GLint location, GLuint v0)
{
  static PFNGLUNIFORM1UI pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLUNIFORM1UI)
     glGetProcAddress (glUniform1ui);

  pfn (location, v0);
}

inline void
glUniform1i (GLint location, GLint v0)
{
  static PFNGLUNIFORM1I pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLUNIFORM1I)
     glGetProcAddress (glUniform1i);

  pfn (location, v0);
}

inline void
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

inline void
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

inline void
glVertexAttribDivisor (GLuint index,
                       GLuint divisor)
{
  static PFNGLVERTEXATTRIBDIVISOR pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLVERTEXATTRIBDIVISOR)
     glGetProcAddress (glVertexAttribDivisor);

  pfn (index, divisor);
}

inline void
glEnableVertexAttribArray (GLuint index)
{
  static PFNGLENABLEVERTEXATTRIBARRAY pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLENABLEVERTEXATTRIBARRAY)
     glGetProcAddress (glEnableVertexAttribArray);

  pfn (index);
}

inline void
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

inline void
glGenRenderbuffers (GLsizei n, GLuint *renderbuffers)
{
  static PFNGLGENRENDERBUFFERS pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLGENRENDERBUFFERS)
     glGetProcAddress (glGenRenderbuffersOES);

  pfn (n, renderbuffers);
}

inline void
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

inline void
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

inline void
glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
  static PFNGLBINDRENDERBUFFER pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLBINDRENDERBUFFER)
     glGetProcAddress (glBindRenderbuffer);

  pfn (target, renderbuffer);
}

inline void
glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers)
{
  static PFNGLDELETERENDERBUFFERS pfn = NULL;

  if (pfn == NULL)
   pfn = (PFNGLDELETERENDERBUFFERS)
     glGetProcAddress (glDeleteRenderbuffers);

  pfn (n, renderbuffers);
}
