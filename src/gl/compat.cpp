#include "gl_compat.h"
#include "gl_render.h"
#include "wrapped.h"

void WRAP(glFogiv(GLenum pname, const GLint* params))
{
    GLfloat values[4] = {};
    for(int i = 0; i < (pname == GL_FOG_COLOR ? 4 : 1); ++i) values[i] = pname == GL_FOG_COLOR ? i2f(params[i]) : (GLfloat)params[i];
    WRAP(glFogfv(pname, values));
}

void WRAP(glLighti(GLenum light, GLenum pname, GLint param)) { WRAP(glLightf(light, pname, (GLfloat)param)); }

void WRAP(glLightiv(GLenum light, GLenum pname, const GLint* params))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(light < GL_LIGHT0 || light > GL_LIGHT7 || pname < GL_AMBIENT || pname > GL_QUADRATIC_ATTENUATION) { SetError(GL_INVALID_ENUM); return; }
    GLfloat values[4] = {};
    int count = pname == GL_SPOT_DIRECTION ? 3 : pname >= GL_AMBIENT && pname <= GL_POSITION ? 4 : 1;
    for(int i = 0; i < count; ++i) values[i] = pname >= GL_AMBIENT && pname <= GL_SPECULAR ? i2f(params[i]) : (GLfloat)params[i];
    WRAP(glLightfv(light, pname, values));
}

void WRAP(glLightModeliv(GLenum pname, const GLint* params))
{
    GLfloat values[4] = {};
    for(int i = 0; i < (pname == GL_LIGHT_MODEL_AMBIENT ? 4 : 1); ++i) values[i] = pname == GL_LIGHT_MODEL_AMBIENT ? i2f(params[i]) : (GLfloat)params[i];
    WRAP(glLightModelfv(pname, values));
}

void WRAP(glVertexAttrib1d(GLuint index, GLdouble x)) { glVertexAttrib1f(index, (GLfloat)x); }
void WRAP(glVertexAttrib1dv(GLuint index, const GLdouble* v)) { glVertexAttrib1f(index, (GLfloat)v[0]); }
void WRAP(glVertexAttrib1s(GLuint index, GLshort x)) { glVertexAttrib1f(index, (GLfloat)x); }
void WRAP(glVertexAttrib1sv(GLuint index, const GLshort* v)) { glVertexAttrib1f(index, (GLfloat)v[0]); }
void WRAP(glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y)) { glVertexAttrib2f(index, (GLfloat)x, (GLfloat)y); }
void WRAP(glVertexAttrib2dv(GLuint index, const GLdouble* v)) { glVertexAttrib2f(index, (GLfloat)v[0], (GLfloat)v[1]); }
void WRAP(glVertexAttrib2s(GLuint index, GLshort x, GLshort y)) { glVertexAttrib2f(index, (GLfloat)x, (GLfloat)y); }
void WRAP(glVertexAttrib2sv(GLuint index, const GLshort* v)) { glVertexAttrib2f(index, (GLfloat)v[0], (GLfloat)v[1]); }
void WRAP(glVertexAttrib3dv(GLuint index, const GLdouble* v)) { glVertexAttrib3f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]); }
void WRAP(glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z)) { glVertexAttrib3f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z); }
void WRAP(glVertexAttrib3sv(GLuint index, const GLshort* v)) { glVertexAttrib3f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]); }
void WRAP(glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)) { glVertexAttrib4f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w); }
void WRAP(glVertexAttrib4dv(GLuint index, const GLdouble* v)) { glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]); }
void WRAP(glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)) { glVertexAttrib4f(index, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w); }
void WRAP(glVertexAttrib4sv(GLuint index, const GLshort* v)) { glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]); }
void WRAP(glVertexAttrib4bv(GLuint index, const GLbyte* v)) { glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]); }
void WRAP(glVertexAttrib4ubv(GLuint index, const GLubyte* v)) { glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]); }
void WRAP(glVertexAttrib4iv(GLuint index, const GLint* v)) { glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]); }
void WRAP(glVertexAttrib4uiv(GLuint index, const GLuint* v)) { glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]); }
void WRAP(glVertexAttrib4usv(GLuint index, const GLushort* v)) { glVertexAttrib4f(index, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]); }
void WRAP(glVertexAttrib4Nbv(GLuint index, const GLbyte* v)) { glVertexAttrib4f(index, b2f(v[0]), b2f(v[1]), b2f(v[2]), b2f(v[3])); }
void WRAP(glVertexAttrib4Nubv(GLuint index, const GLubyte* v)) { glVertexAttrib4f(index, ub2f(v[0]), ub2f(v[1]), ub2f(v[2]), ub2f(v[3])); }
void WRAP(glVertexAttrib4Nsv(GLuint index, const GLshort* v)) { glVertexAttrib4f(index, s2f(v[0]), s2f(v[1]), s2f(v[2]), s2f(v[3])); }
void WRAP(glVertexAttrib4Nusv(GLuint index, const GLushort* v)) { glVertexAttrib4f(index, us2f(v[0]), us2f(v[1]), us2f(v[2]), us2f(v[3])); }
void WRAP(glVertexAttrib4Niv(GLuint index, const GLint* v)) { glVertexAttrib4f(index, i2f(v[0]), i2f(v[1]), i2f(v[2]), i2f(v[3])); }
void WRAP(glVertexAttrib4Nuiv(GLuint index, const GLuint* v)) { glVertexAttrib4f(index, ui2f(v[0]), ui2f(v[1]), ui2f(v[2]), ui2f(v[3])); }
void WRAP(glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)) { glVertexAttrib4f(index, ub2f(x), ub2f(y), ub2f(z), ub2f(w)); }
void WRAP(glVertexAttribI1i(GLuint index, GLint x)) { glVertexAttribI4i(index, x, 0, 0, 1); }
void WRAP(glVertexAttribI1iv(GLuint index, const GLint* v)) { glVertexAttribI4i(index, v[0], 0, 0, 1); }
void WRAP(glVertexAttribI1ui(GLuint index, GLuint x)) { glVertexAttribI4ui(index, x, 0, 0, 1); }
void WRAP(glVertexAttribI1uiv(GLuint index, const GLuint* v)) { glVertexAttribI4ui(index, v[0], 0, 0, 1); }
void WRAP(glVertexAttribI2i(GLuint index, GLint x, GLint y)) { glVertexAttribI4i(index, x, y, 0, 1); }
void WRAP(glVertexAttribI2iv(GLuint index, const GLint* v)) { glVertexAttribI4i(index, v[0], v[1], 0, 1); }
void WRAP(glVertexAttribI2ui(GLuint index, GLuint x, GLuint y)) { glVertexAttribI4ui(index, x, y, 0, 1); }
void WRAP(glVertexAttribI2uiv(GLuint index, const GLuint* v)) { glVertexAttribI4ui(index, v[0], v[1], 0, 1); }
void WRAP(glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z)) { glVertexAttribI4i(index, x, y, z, 1); }
void WRAP(glVertexAttribI3iv(GLuint index, const GLint* v)) { glVertexAttribI4i(index, v[0], v[1], v[2], 1); }
void WRAP(glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z)) { glVertexAttribI4ui(index, x, y, z, 1); }
void WRAP(glVertexAttribI3uiv(GLuint index, const GLuint* v)) { glVertexAttribI4ui(index, v[0], v[1], v[2], 1); }
void WRAP(glVertexAttribI4bv(GLuint index, const GLbyte* v)) { glVertexAttribI4i(index, v[0], v[1], v[2], v[3]); }
void WRAP(glVertexAttribI4sv(GLuint index, const GLshort* v)) { glVertexAttribI4i(index, v[0], v[1], v[2], v[3]); }
void WRAP(glVertexAttribI4ubv(GLuint index, const GLubyte* v)) { glVertexAttribI4ui(index, v[0], v[1], v[2], v[3]); }
void WRAP(glVertexAttribI4usv(GLuint index, const GLushort* v)) { glVertexAttribI4ui(index, v[0], v[1], v[2], v[3]); }
void WRAP(glTexCoord1dv(const GLdouble* v)) { WRAP(glTexCoord1d(v[0])); }
void WRAP(glTexCoord1iv(const GLint* v)) { WRAP(glTexCoord1i(v[0])); }
void WRAP(glTexCoord1sv(const GLshort* v)) { WRAP(glTexCoord1s(v[0])); }
void WRAP(glTexCoord3dv(const GLdouble* v)) { WRAP(glTexCoord3d(v[0], v[1], v[2])); }
void WRAP(glTexCoord3iv(const GLint* v)) { WRAP(glTexCoord3i(v[0], v[1], v[2])); }
void WRAP(glTexCoord3sv(const GLshort* v)) { WRAP(glTexCoord3s(v[0], v[1], v[2])); }
void WRAP(glTexCoord4dv(const GLdouble* v)) { WRAP(glTexCoord4d(v[0], v[1], v[2], v[3])); }
void WRAP(glTexCoord4iv(const GLint* v)) { WRAP(glTexCoord4i(v[0], v[1], v[2], v[3])); }
void WRAP(glTexCoord4sv(const GLshort* v)) { WRAP(glTexCoord4s(v[0], v[1], v[2], v[3])); }
void WRAP(glRasterPos2sv(const GLshort* v)) { WRAP(glRasterPos2s(v[0], v[1])); }
void WRAP(glRasterPos3iv(const GLint* v)) { WRAP(glRasterPos3i(v[0], v[1], v[2])); }
void WRAP(glRasterPos3sv(const GLshort* v)) { WRAP(glRasterPos3s(v[0], v[1], v[2])); }
void WRAP(glRasterPos4iv(const GLint* v)) { WRAP(glRasterPos4i(v[0], v[1], v[2], v[3])); }
void WRAP(glRasterPos4sv(const GLshort* v)) { WRAP(glRasterPos4s(v[0], v[1], v[2], v[3])); }
void WRAP(glWindowPos2dv(const GLdouble* v)) { WRAP(glWindowPos2d(v[0], v[1])); }
void WRAP(glWindowPos2s(GLshort x, GLshort y)) { WRAP(glWindowPos2f((GLfloat)x, (GLfloat)y)); }
void WRAP(glWindowPos2sv(const GLshort* v)) { WRAP(glWindowPos2s(v[0], v[1])); }
void WRAP(glWindowPos3dv(const GLdouble* v)) { WRAP(glWindowPos3d(v[0], v[1], v[2])); }
void WRAP(glWindowPos3i(GLint x, GLint y, GLint z)) { WRAP(glWindowPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z)); }
void WRAP(glWindowPos3iv(const GLint* v)) { WRAP(glWindowPos3i(v[0], v[1], v[2])); }
void WRAP(glWindowPos3s(GLshort x, GLshort y, GLshort z)) { WRAP(glWindowPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z)); }
void WRAP(glWindowPos3sv(const GLshort* v)) { WRAP(glWindowPos3s(v[0], v[1], v[2])); }
void WRAP(glEvalCoord1dv(const GLdouble* v)) { WRAP(glEvalCoord1d(v[0])); }
void WRAP(glEvalCoord1fv(const GLfloat* v)) { WRAP(glEvalCoord1f(v[0])); }
void WRAP(glEvalCoord2dv(const GLdouble* v)) { WRAP(glEvalCoord2d(v[0], v[1])); }
void WRAP(glEvalCoord2fv(const GLfloat* v)) { WRAP(glEvalCoord2f(v[0], v[1])); }
void WRAP(glMultiTexCoord1dv(GLenum texture, const GLdouble* v)) { WRAP(glMultiTexCoord1d(texture, v[0])); }
void WRAP(glMultiTexCoord1iv(GLenum texture, const GLint* v)) { WRAP(glMultiTexCoord1i(texture, v[0])); }
void WRAP(glMultiTexCoord1s(GLenum texture, GLshort s)) { WRAP(glMultiTexCoord1f(texture, (GLfloat)s)); }
void WRAP(glMultiTexCoord1sv(GLenum texture, const GLshort* v)) { WRAP(glMultiTexCoord1s(texture, v[0])); }
void WRAP(glMultiTexCoord3dv(GLenum texture, const GLdouble* v)) { WRAP(glMultiTexCoord3d(texture, v[0], v[1], v[2])); }
void WRAP(glMultiTexCoord3i(GLenum texture, GLint s, GLint t, GLint r)) { WRAP(glMultiTexCoord3f(texture, (GLfloat)s, (GLfloat)t, (GLfloat)r)); }
void WRAP(glMultiTexCoord3iv(GLenum texture, const GLint* v)) { WRAP(glMultiTexCoord3i(texture, v[0], v[1], v[2])); }
void WRAP(glMultiTexCoord3s(GLenum texture, GLshort s, GLshort t, GLshort r)) { WRAP(glMultiTexCoord3f(texture, (GLfloat)s, (GLfloat)t, (GLfloat)r)); }
void WRAP(glMultiTexCoord3sv(GLenum texture, const GLshort* v)) { WRAP(glMultiTexCoord3s(texture, v[0], v[1], v[2])); }
void WRAP(glMultiTexCoord4dv(GLenum texture, const GLdouble* v)) { WRAP(glMultiTexCoord4d(texture, v[0], v[1], v[2], v[3])); }
void WRAP(glMultiTexCoord4i(GLenum texture, GLint s, GLint t, GLint r, GLint q)) { WRAP(glMultiTexCoord4f(texture, (GLfloat)s, (GLfloat)t, (GLfloat)r, (GLfloat)q)); }
void WRAP(glMultiTexCoord4iv(GLenum texture, const GLint* v)) { WRAP(glMultiTexCoord4i(texture, v[0], v[1], v[2], v[3])); }
void WRAP(glMultiTexCoord4s(GLenum texture, GLshort s, GLshort t, GLshort r, GLshort q)) { WRAP(glMultiTexCoord4f(texture, (GLfloat)s, (GLfloat)t, (GLfloat)r, (GLfloat)q)); }
void WRAP(glMultiTexCoord4sv(GLenum texture, const GLshort* v)) { WRAP(glMultiTexCoord4s(texture, v[0], v[1], v[2], v[3])); }
void WRAP(glSecondaryColor3b(GLbyte r, GLbyte g, GLbyte b)) { WRAP(glSecondaryColor3f(b2f(r), b2f(g), b2f(b))); }
void WRAP(glSecondaryColor3bv(const GLbyte* v)) { WRAP(glSecondaryColor3b(v[0], v[1], v[2])); }
void WRAP(glSecondaryColor3i(GLint r, GLint g, GLint b)) { WRAP(glSecondaryColor3f(i2f(r), i2f(g), i2f(b))); }
void WRAP(glSecondaryColor3iv(const GLint* v)) { WRAP(glSecondaryColor3i(v[0], v[1], v[2])); }
void WRAP(glSecondaryColor3s(GLshort r, GLshort g, GLshort b)) { WRAP(glSecondaryColor3f(s2f(r), s2f(g), s2f(b))); }
void WRAP(glSecondaryColor3sv(const GLshort* v)) { WRAP(glSecondaryColor3s(v[0], v[1], v[2])); }
void WRAP(glSecondaryColor3ubv(const GLubyte* v)) { WRAP(glSecondaryColor3ub(v[0], v[1], v[2])); }
void WRAP(glSecondaryColor3ui(GLuint r, GLuint g, GLuint b)) { WRAP(glSecondaryColor3f(ui2f(r), ui2f(g), ui2f(b))); }
void WRAP(glSecondaryColor3uiv(const GLuint* v)) { WRAP(glSecondaryColor3ui(v[0], v[1], v[2])); }
void WRAP(glSecondaryColor3us(GLushort r, GLushort g, GLushort b)) { WRAP(glSecondaryColor3f(us2f(r), us2f(g), us2f(b))); }
void WRAP(glSecondaryColor3usv(const GLushort* v)) { WRAP(glSecondaryColor3us(v[0], v[1], v[2])); }
void WRAP(glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble* params))
{
    GLfloat values[4] = {};
    glGetVertexAttribfv(index, pname, values);
    for(int i = 0; i < (pname == GL_CURRENT_VERTEX_ATTRIB ? 4 : 1); ++i) params[i] = values[i];
}
