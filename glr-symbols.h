#ifndef _GLR_SYMBOLS_H_
#define _GLR_SYMBOLS_H_

#include <GL/gl.h>

inline void   glGenFramebuffers         (GLsizei n, GLuint* ids);
inline void   glDeleteFramebuffers      (GLsizei n, GLuint  *framebuffers);
inline void   glBindFramebuffer         (GLenum target, GLuint framebuffer);
inline void   glFramebufferTexture2D    (GLenum target,
                                         GLenum attachment,
                                         GLenum textarget,
                                         GLuint texture,
                                         GLint  level);
inline GLenum glCheckFramebufferStatus  (GLenum target);
inline void   glBlitFramebuffer         (GLint      srcX0,
                                         GLint      srcY0,
                                         GLint      srcX1,
                                         GLint      srcY1,
                                         GLint      dstX0,
                                         GLint      dstY0,
                                         GLint      dstX1,
                                         GLint      dstY1,
                                         GLbitfield mask,
                                         GLenum     filter);

inline GLuint glCreateProgram           (void);
inline void   glDeleteProgram           (GLuint program);
inline void   glLinkProgram             (GLuint program);
inline void   glUseProgram              (GLuint program);
inline GLint  glGetUniformLocation      (GLuint        program,
                                         const GLchar *name);

inline GLuint glCreateShader            (GLenum shaderType);
inline void   glDeleteShader            (GLuint shader);
inline void   glCompileShader           (GLuint shader);
inline void   glAttachShader            (GLuint program, GLuint shader);
inline void   glShaderSource            (GLuint                shader,
                                         GLsizei               count,
                                         const GLchar * const *string,
                                         const GLint          *length);
inline void   glGetShaderiv             (GLuint  shader,
                                         GLenum  pname,
                                         GLint  *params);
inline void   glGetShaderInfoLog        (GLuint   shader,
                                         GLsizei  maxLength,
                                         GLsizei *length,
                                         GLchar  *infoLog);

inline void   glUniform1ui              (GLint location, GLuint v0);
inline void   glUniform1i               (GLint location, GLint v0);

inline void   glBindAttribLocation      (GLuint        program,
                                         GLuint        index,
                                         const GLchar *name);
inline void   glVertexAttribPointer     (GLuint        index,
                                         GLint         size,
                                         GLenum        type,
                                         GLboolean     normalized,
                                         GLsizei       stride,
                                         const GLvoid *pointer);
inline void   glVertexAttribDivisor     (GLuint index, GLuint divisor);
inline void   glEnableVertexAttribArray (GLuint index);
inline void   glDrawArraysInstanced     (GLenum  mode,
                                         GLint   first,
                                         GLsizei count,
                                         GLsizei primcount);

inline void   glTexImage2DMultisample   (GLenum    target,
                                         GLsizei   samples,
                                         GLint     internalformat,
                                         GLsizei   width,
                                         GLsizei   height,
                                         GLboolean fixedsamplelocations);

#endif /* _GLR_SYMBOLS_H_ */
