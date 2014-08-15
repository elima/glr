#ifndef _GLR_SYMBOLS_H_
#define _GLR_SYMBOLS_H_

#include <GLES3/gl3.h>

void   glGenFramebuffers                (GLsizei n, GLuint* ids);
void   glDeleteFramebuffers             (GLsizei n, const GLuint *framebuffers);
void   glBindFramebuffer                (GLenum target, GLuint framebuffer);
void   glFramebufferTexture2D           (GLenum target,
                                         GLenum attachment,
                                         GLenum textarget,
                                         GLuint texture,
                                         GLint  level);
GLenum glCheckFramebufferStatus         (GLenum target);
void   glBlitFramebuffer                (GLint      srcX0,
                                         GLint      srcY0,
                                         GLint      srcX1,
                                         GLint      srcY1,
                                         GLint      dstX0,
                                         GLint      dstY0,
                                         GLint      dstX1,
                                         GLint      dstY1,
                                         GLbitfield mask,
                                         GLenum     filter);

GLuint glCreateProgram                  (void);
void   glDeleteProgram                  (GLuint program);
void   glLinkProgram                    (GLuint program);
void   glUseProgram                     (GLuint program);
GLint  glGetUniformLocation             (GLuint        program,
                                         const GLchar *name);

GLuint glCreateShader                   (GLenum shaderType);
void   glDeleteShader                   (GLuint shader);
void   glCompileShader                  (GLuint shader);
void   glAttachShader                   (GLuint program, GLuint shader);
void   glShaderSource                   (GLuint                shader,
                                         GLsizei               count,
                                         const GLchar * const *string,
                                         const GLint          *length);
void   glGetShaderiv                    (GLuint  shader,
                                         GLenum  pname,
                                         GLint  *params);
void   glGetShaderInfoLog               (GLuint   shader,
                                         GLsizei  maxLength,
                                         GLsizei *length,
                                         GLchar  *infoLog);

void   glUniform1ui                     (GLint location, GLuint v0);
void   glUniform1i                      (GLint location, GLint v0);

void   glBindAttribLocation             (GLuint        program,
                                         GLuint        index,
                                         const GLchar *name);
void   glVertexAttribPointer            (GLuint        index,
                                         GLint         size,
                                         GLenum        type,
                                         GLboolean     normalized,
                                         GLsizei       stride,
                                         const GLvoid *pointer);
void   glVertexAttribDivisor            (GLuint index, GLuint divisor);
void   glEnableVertexAttribArray        (GLuint index);
void   glDrawArraysInstanced            (GLenum  mode,
                                         GLint   first,
                                         GLsizei count,
                                         GLsizei primcount);

void   glTexImage2DMultisample          (GLenum    target,
                                         GLsizei   samples,
                                         GLint     internalformat,
                                         GLsizei   width,
                                         GLsizei   height,
                                         GLboolean fixedsamplelocations);

void   glGenRenderbuffers               (GLsizei  n,
                                         GLuint  *renderbuffers);
void   glRenderbufferStorageMultisample (GLenum  target,
                                         GLsizei samples,
                                         GLenum  internalformat,
                                         GLsizei width,
                                         GLsizei height);
void   glFramebufferRenderbuffer        (GLenum target,
                                         GLenum attachment,
                                         GLenum renderbuffertarget,
                                         GLuint renderbuffer);
void   glBindRenderbuffer               (GLenum target,
                                         GLuint renderbuffer);
void   glDeleteRenderbuffers            (GLsizei       n,
                                         const GLuint *renderbuffers);

#endif /* _GLR_SYMBOLS_H_ */
