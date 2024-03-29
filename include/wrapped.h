void WRAP(glGetCompressedTexImage(GLenum target, GLint lod, GLvoid *img));
void WRAP(glTexCoord2f(GLfloat s, GLfloat t));
void WRAP(glPolygonMode(GLenum face, GLenum mode));
void WRAP(glPopAttrib());
void WRAP(glPushAttrib(GLbitfield mask));
void WRAP(glEnableClientState(GLenum array));
void WRAP(glDisableClientState(GLenum array));
void WRAP(glClientActiveTexture(GLenum texture));
void WRAP(glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer));
void WRAP(glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer));
void WRAP(glProgramEnvParameters4fv(GLenum target, GLuint index, GLsizei count, const GLfloat *params));
void WRAP(glEnableIndexed(GLenum target, GLuint index));
void WRAP(glDisableIndexed(GLenum target, GLuint index));
void WRAP(glGetBooleanIndexedv(GLenum target, GLuint index, GLboolean *data));
void WRAP(glPopClientAttrib());
void WRAP(glPushClientAttrib(GLbitfield mask));


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