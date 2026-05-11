void WRAP(glGetCompressedTexImage(GLenum target, GLint lod, GLvoid *img));
void WRAP(glPopAttrib());
void WRAP(glPushAttrib(GLbitfield mask));
void WRAP(glClientActiveTexture(GLenum texture));
void WRAP(glProgramEnvParameters4fv(GLenum target, GLuint index, GLsizei count, const GLfloat *params));
void WRAP(glEnableIndexed(GLenum target, GLuint index));
void WRAP(glDisableIndexed(GLenum target, GLuint index));
void WRAP(glGetBooleanIndexedv(GLenum target, GLuint index, GLboolean *data));
void WRAP(glPopClientAttrib());
void WRAP(glPushClientAttrib(GLbitfield mask));
void WRAP(glLogicOp(GLenum opcode));

void WRAP(glGetProgramiv( GLuint program,GLenum pname,GLint *params));
void WRAP(glGetIntegerv( GLenum pname, GLint * data));
void WRAP(glEnable(GLenum cap));
void WRAP(glDisable(GLenum cap));
GLboolean WRAP(glIsEnabled(GLenum cap));

void WRAP(glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter));
void WRAP(glDrawBuffers(GLsizei n, const GLenum *bufs));
void WRAP(glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, void *indices, GLint basevertex));
void WRAP(glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a));
void WRAP(glClear(GLbitfield mask));
void WRAP(glGetFloatv(GLenum pname, GLfloat* data));
void WRAP(glGetDoublev(GLenum pname, GLdouble* data));
void WRAP(glPixelStoref(GLenum pname, GLfloat param));
void WRAP(glDrawArrays(GLenum mode, GLint first, GLsizei count));
void WRAP(glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices));
void WRAP(glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices));
