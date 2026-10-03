#include "gl_compat.h"
#include "gl_render.h"
#include "wrapped.h"
#include "packed_vertex.h"

void WRAP(glVertexP2ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glVertex2fv(values));
}
void WRAP(glVertexP2uiv(GLenum type, const GLuint* value)) { WRAP(glVertexP2ui(type, *value)); }
void WRAP(glVertexP3ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glVertex3fv(values));
}
void WRAP(glVertexP3uiv(GLenum type, const GLuint* value)) { WRAP(glVertexP3ui(type, *value)); }
void WRAP(glVertexP4ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glVertex4fv(values));
}
void WRAP(glVertexP4uiv(GLenum type, const GLuint* value)) { WRAP(glVertexP4ui(type, *value)); }
void WRAP(glTexCoordP1ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glTexCoord1fv(values));
}
void WRAP(glTexCoordP1uiv(GLenum type, const GLuint* value)) { WRAP(glTexCoordP1ui(type, *value)); }
void WRAP(glTexCoordP2ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glTexCoord2fv(values));
}
void WRAP(glTexCoordP2uiv(GLenum type, const GLuint* value)) { WRAP(glTexCoordP2ui(type, *value)); }
void WRAP(glTexCoordP3ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glTexCoord3fv(values));
}
void WRAP(glTexCoordP3uiv(GLenum type, const GLuint* value)) { WRAP(glTexCoordP3ui(type, *value)); }
void WRAP(glTexCoordP4ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glTexCoord4fv(values));
}
void WRAP(glTexCoordP4uiv(GLenum type, const GLuint* value)) { WRAP(glTexCoordP4ui(type, *value)); }
void WRAP(glMultiTexCoordP1ui(GLenum texture, GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glMultiTexCoord1fv(texture, values));
}
void WRAP(glMultiTexCoordP1uiv(GLenum texture, GLenum type, const GLuint* value)) { WRAP(glMultiTexCoordP1ui(texture, type, *value)); }
void WRAP(glMultiTexCoordP2ui(GLenum texture, GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glMultiTexCoord2fv(texture, values));
}
void WRAP(glMultiTexCoordP2uiv(GLenum texture, GLenum type, const GLuint* value)) { WRAP(glMultiTexCoordP2ui(texture, type, *value)); }
void WRAP(glMultiTexCoordP3ui(GLenum texture, GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glMultiTexCoord3fv(texture, values));
}
void WRAP(glMultiTexCoordP3uiv(GLenum texture, GLenum type, const GLuint* value)) { WRAP(glMultiTexCoordP3ui(texture, type, *value)); }
void WRAP(glMultiTexCoordP4ui(GLenum texture, GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, false, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glMultiTexCoord4fv(texture, values));
}
void WRAP(glMultiTexCoordP4uiv(GLenum texture, GLenum type, const GLuint* value)) { WRAP(glMultiTexCoordP4ui(texture, type, *value)); }
void WRAP(glNormalP3ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, true, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glNormal3fv(values));
}
void WRAP(glNormalP3uiv(GLenum type, const GLuint* value)) { WRAP(glNormalP3ui(type, *value)); }
void WRAP(glColorP3ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, true, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glColor3fv(values));
}
void WRAP(glColorP3uiv(GLenum type, const GLuint* value)) { WRAP(glColorP3ui(type, *value)); }
void WRAP(glColorP4ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, true, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glColor4fv(values));
}
void WRAP(glColorP4uiv(GLenum type, const GLuint* value)) { WRAP(glColorP4ui(type, *value)); }
void WRAP(glSecondaryColorP3ui(GLenum type, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, true, values)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glSecondaryColor3fv(values));
}
void WRAP(glSecondaryColorP3uiv(GLenum type, const GLuint* value)) { WRAP(glSecondaryColorP3ui(type, *value)); }
void WRAP(glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, normalized, values)) { SetError(GL_INVALID_ENUM); return; }
    glVertexAttrib1fv(index, values);
}
void WRAP(glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint* value)) { WRAP(glVertexAttribP1ui(index, type, normalized, *value)); }
void WRAP(glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, normalized, values)) { SetError(GL_INVALID_ENUM); return; }
    glVertexAttrib2fv(index, values);
}
void WRAP(glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint* value)) { WRAP(glVertexAttribP2ui(index, type, normalized, *value)); }
void WRAP(glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, normalized, values)) { SetError(GL_INVALID_ENUM); return; }
    glVertexAttrib3fv(index, values);
}
void WRAP(glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint* value)) { WRAP(glVertexAttribP3ui(index, type, normalized, *value)); }
void WRAP(glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value))
{
    GLfloat values[4];
    if(!UnpackVertex(type, value, normalized, values)) { SetError(GL_INVALID_ENUM); return; }
    glVertexAttrib4fv(index, values);
}
void WRAP(glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint* value)) { WRAP(glVertexAttribP4ui(index, type, normalized, *value)); }
