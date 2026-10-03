#include "GLES.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "wrapped.h"
#include "globals.h"
#include "glhelper.h"
#include "maths.h"
#include "draw_state.h"
#include <algorithm>
#include <limits>
#include <cmath>

void WRAP(glBegin(GLenum mode))
{
    DLREC(WRAP(glBegin(mode)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(mode > 9) { SetError(GL_INVALID_ENUM); return; }
    globals->render.lastPrimitiveMode = mode;
    globals->render.begin = true;
    
    globals->render.vertices.clear();
    globals->render.colors.clear();
    for(auto& coords : globals->render.texcoords) coords.clear();
    globals->render.normals.clear();
    globals->render.secondaryColors.clear();
    globals->render.fogCoords.clear();
    globals->render.materialValues.clear();
}

void WRAP(glEnd())
{
    DLREC(WRAP(glEnd()));
    if(!globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    globals->render.begin = false;
    if(globals->render.vertices.empty()) return;
    
    draw_state_t saved;
    globals->render.begin = false;
    
    if(!globals->gl.conditionalDiscard) TransformFixedVerts();
    glUseProgram(globals->gl.activeProgram); // restore program
    
    globals->render.vertices.clear();
    globals->render.colors.clear();
    for(auto& coords : globals->render.texcoords) coords.clear();
    globals->render.normals.clear();
    globals->render.secondaryColors.clear();
    globals->render.fogCoords.clear();
    globals->render.materialValues.clear();
}

void WRAP(glColor3f(GLfloat r, GLfloat g, GLfloat b))                     { DLREC(WRAP(glColor3f(r,g,b)));      globals->render.color = {r, g, b, 1.0f}; UpdateColorMaterial(); }
void WRAP(glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a))          { DLREC(WRAP(glColor4f(r,g,b,a)));    globals->render.color = {r, g, b, a}; UpdateColorMaterial(); }
void WRAP(glColor3b(GLbyte r, GLbyte g, GLbyte b))                         { WRAP(glColor3f(b2f(r),b2f(g),b2f(b))); }
void WRAP(glColor3d(GLdouble r, GLdouble g, GLdouble b))                   { WRAP(glColor3f((float)r,(float)g,(float)b)); }
void WRAP(glColor3i(GLint r, GLint g, GLint b))                            { WRAP(glColor3f(i2f(r),i2f(g),i2f(b))); }
void WRAP(glColor3s(GLshort r, GLshort g, GLshort b))                      { WRAP(glColor3f(s2f(r),s2f(g),s2f(b))); }
void WRAP(glColor3ub(GLubyte r, GLubyte g, GLubyte b))                     { WRAP(glColor3f(ub2f(r),ub2f(g),ub2f(b))); }
void WRAP(glColor3ui(GLuint r, GLuint g, GLuint b))                        { WRAP(glColor3f(ui2f(r),ui2f(g),ui2f(b))); }
void WRAP(glColor3us(GLushort r, GLushort g, GLushort b))                  { WRAP(glColor3f(us2f(r),us2f(g),us2f(b))); }
void WRAP(glColor4b(GLbyte r, GLbyte g, GLbyte b, GLbyte a))               { WRAP(glColor4f(b2f(r),b2f(g),b2f(b),b2f(a))); }
void WRAP(glColor4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a))       { WRAP(glColor4f((float)r,(float)g,(float)b,(float)a)); }
void WRAP(glColor4i(GLint r, GLint g, GLint b, GLint a))                   { WRAP(glColor4f(i2f(r),i2f(g),i2f(b),i2f(a))); }
void WRAP(glColor4s(GLshort r, GLshort g, GLshort b, GLshort a))           { WRAP(glColor4f(s2f(r),s2f(g),s2f(b),s2f(a))); }
void WRAP(glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a))          { WRAP(glColor4f(ub2f(r),ub2f(g),ub2f(b),ub2f(a))); }
void WRAP(glColor4ui(GLuint r, GLuint g, GLuint b, GLuint a))              { WRAP(glColor4f(ui2f(r),ui2f(g),ui2f(b),ui2f(a))); }
void WRAP(glColor4us(GLushort r, GLushort g, GLushort b, GLushort a))      { WRAP(glColor4f(us2f(r),us2f(g),us2f(b),us2f(a))); }

void WRAP(glColor3bv(const GLbyte* v))    { WRAP(glColor3b(v[0],v[1],v[2])); }
void WRAP(glColor3dv(const GLdouble* v))  { WRAP(glColor3d(v[0],v[1],v[2])); }
void WRAP(glColor3fv(const GLfloat* v))   { WRAP(glColor3f(v[0],v[1],v[2])); }
void WRAP(glColor3iv(const GLint* v))     { WRAP(glColor3i(v[0],v[1],v[2])); }
void WRAP(glColor3sv(const GLshort* v))   { WRAP(glColor3s(v[0],v[1],v[2])); }
void WRAP(glColor3ubv(const GLubyte* v))  { WRAP(glColor3ub(v[0],v[1],v[2])); }
void WRAP(glColor3uiv(const GLuint* v))   { WRAP(glColor3ui(v[0],v[1],v[2])); }
void WRAP(glColor3usv(const GLushort* v)) { WRAP(glColor3us(v[0],v[1],v[2])); }
void WRAP(glColor4bv(const GLbyte* v))    { WRAP(glColor4b(v[0],v[1],v[2],v[3])); }
void WRAP(glColor4dv(const GLdouble* v))  { WRAP(glColor4d(v[0],v[1],v[2],v[3])); }
void WRAP(glColor4fv(const GLfloat* v))   { WRAP(glColor4f(v[0],v[1],v[2],v[3])); }
void WRAP(glColor4iv(const GLint* v))     { WRAP(glColor4i(v[0],v[1],v[2],v[3])); }
void WRAP(glColor4sv(const GLshort* v))   { WRAP(glColor4s(v[0],v[1],v[2],v[3])); }
void WRAP(glColor4ubv(const GLubyte* v))  { WRAP(glColor4ub(v[0],v[1],v[2],v[3])); }
void WRAP(glColor4uiv(const GLuint* v))   { WRAP(glColor4ui(v[0],v[1],v[2],v[3])); }
void WRAP(glColor4usv(const GLushort* v)) { WRAP(glColor4us(v[0],v[1],v[2],v[3])); }

void WRAP(glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz))
{
    DLREC(WRAP(glNormal3f(nx,ny,nz)));
    globals->render.normal = { nx, ny, nz };
}
void WRAP(glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz))       { WRAP(glNormal3f(b2f(nx),b2f(ny),b2f(nz))); }
void WRAP(glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)) { WRAP(glNormal3f((float)nx,(float)ny,(float)nz)); }
void WRAP(glNormal3i(GLint nx, GLint ny, GLint nz))          { WRAP(glNormal3f(i2f(nx),i2f(ny),i2f(nz))); }
void WRAP(glNormal3s(GLshort nx, GLshort ny, GLshort nz))    { WRAP(glNormal3f(s2f(nx),s2f(ny),s2f(nz))); }
void WRAP(glNormal3bv(const GLbyte* v))    { WRAP(glNormal3b(v[0],v[1],v[2])); }
void WRAP(glNormal3dv(const GLdouble* v))  { WRAP(glNormal3d(v[0],v[1],v[2])); }
void WRAP(glNormal3fv(const GLfloat* v))   { WRAP(glNormal3f(v[0],v[1],v[2])); }
void WRAP(glNormal3iv(const GLint* v))     { WRAP(glNormal3i(v[0],v[1],v[2])); }
void WRAP(glNormal3sv(const GLshort* v))   { WRAP(glNormal3s(v[0],v[1],v[2])); }

static void CaptureMaterial()
{
    if(!globals->ff.lightingEnabled || globals->gl.activeProgram) return;
    for(const auto& material : globals->ff.materials)
    {
        auto& values = globals->render.materialValues;
        values.push_back(*(const vector4_t*)material.ambient);
        values.push_back(*(const vector4_t*)material.diffuse);
        values.push_back(*(const vector4_t*)material.specular);
        values.push_back(*(const vector4_t*)material.emission);
        values.push_back({material.shininess,0,0,0});
    }
}

void WRAP(glVertex3f(GLfloat x, GLfloat y, GLfloat z))
{
    DLREC(WRAP(glVertex3f(x, y, z)));
    if(!globals->render.begin) return;
    globals->render.vertices.push_back({x, y, z, 1});
    globals->render.colors.push_back(globals->render.color);
    globals->render.texcoords[0].push_back(globals->render.texcoord);
    for(int i = 1; i < 8; ++i) globals->render.texcoords[i].push_back(globals->render.multiTexcoord[i-1]);
    globals->render.normals.push_back(globals->render.normal);
    globals->render.secondaryColors.push_back(globals->render.secondaryColor);
    globals->render.fogCoords.push_back(globals->render.fogCoord);
    CaptureMaterial();
}
void WRAP(glVertex2f(GLfloat x, GLfloat y))   { WRAP(glVertex3f(x, y, 0.0f)); }
void WRAP(glVertex2i(GLint x, GLint y))        { WRAP(glVertex3f((float)x,(float)y, 0.0f)); }
void WRAP(glVertex3i(GLint x, GLint y, GLint z)){ WRAP(glVertex3f((float)x,(float)y,(float)z)); }
void WRAP(glVertex2d(GLdouble x, GLdouble y))  { WRAP(glVertex3f((float)x,(float)y, 0.0f)); }
void WRAP(glVertex2s(GLshort x, GLshort y))    { WRAP(glVertex3f((float)x,(float)y, 0.0f)); }
void WRAP(glVertex3d(GLdouble x, GLdouble y, GLdouble z)) { WRAP(glVertex3f((float)x,(float)y,(float)z)); }
void WRAP(glVertex3s(GLshort x, GLshort y, GLshort z))    { WRAP(glVertex3f((float)x,(float)y,(float)z)); }
void WRAP(glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w))
{
    DLREC(WRAP(glVertex4f(x,y,z,w)));
    if(!globals->render.begin) return;
    globals->render.vertices.push_back({x,y,z,w});
    globals->render.colors.push_back(globals->render.color);
    globals->render.texcoords[0].push_back(globals->render.texcoord);
    for(int i = 1; i < 8; ++i) globals->render.texcoords[i].push_back(globals->render.multiTexcoord[i-1]);
    globals->render.normals.push_back(globals->render.normal);
    globals->render.secondaryColors.push_back(globals->render.secondaryColor);
    globals->render.fogCoords.push_back(globals->render.fogCoord);
    CaptureMaterial();
}
void WRAP(glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)) { WRAP(glVertex4f((float)x,(float)y,(float)z,(float)w)); }
void WRAP(glVertex4i(GLint x, GLint y, GLint z, GLint w))             { WRAP(glVertex4f((float)x,(float)y,(float)z,(float)w)); }
void WRAP(glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w))     { WRAP(glVertex4f((float)x,(float)y,(float)z,(float)w)); }
void WRAP(glVertex2fv(const GLfloat* v))  { WRAP(glVertex2f(v[0],v[1])); }
void WRAP(glVertex2dv(const GLdouble* v)) { WRAP(glVertex2d(v[0],v[1])); }
void WRAP(glVertex2iv(const GLint* v))    { WRAP(glVertex2i(v[0],v[1])); }
void WRAP(glVertex2sv(const GLshort* v))  { WRAP(glVertex2s(v[0],v[1])); }
void WRAP(glVertex3fv(const GLfloat* v))  { WRAP(glVertex3f(v[0],v[1],v[2])); }
void WRAP(glVertex3dv(const GLdouble* v)) { WRAP(glVertex3d(v[0],v[1],v[2])); }
void WRAP(glVertex3iv(const GLint* v))    { WRAP(glVertex3i(v[0],v[1],v[2])); }
void WRAP(glVertex3sv(const GLshort* v))  { WRAP(glVertex3s(v[0],v[1],v[2])); }
void WRAP(glVertex4fv(const GLfloat* v))  { WRAP(glVertex4f(v[0],v[1],v[2],v[3])); }
void WRAP(glVertex4dv(const GLdouble* v)) { WRAP(glVertex4d(v[0],v[1],v[2],v[3])); }
void WRAP(glVertex4iv(const GLint* v))    { WRAP(glVertex4i(v[0],v[1],v[2],v[3])); }
void WRAP(glVertex4sv(const GLshort* v))  { WRAP(glVertex4s(v[0],v[1],v[2],v[3])); }

void WRAP(glTexCoord2f(GLfloat s, GLfloat t))
{
    DLREC(WRAP(glTexCoord2f(s,t)));
    globals->render.texcoord = { s, t, 0, 1 };
}
void WRAP(glTexCoord1f(GLfloat s))                   { WRAP(glTexCoord2f(s, 0.0f)); }
void WRAP(glTexCoord1d(GLdouble s))                  { WRAP(glTexCoord2f((float)s, 0.0f)); }
void WRAP(glTexCoord1i(GLint s))                     { WRAP(glTexCoord2f((float)s, 0.0f)); }
void WRAP(glTexCoord1s(GLshort s))                   { WRAP(glTexCoord2f((float)s, 0.0f)); }
void WRAP(glTexCoord2d(GLdouble s, GLdouble t))      { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord2i(GLint s, GLint t))            { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord2s(GLshort s, GLshort t))        { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord3f(GLfloat s, GLfloat t, GLfloat r))  { WRAP(glTexCoord4f(s,t,r,1)); }
void WRAP(glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)){ WRAP(glTexCoord4f((float)s,(float)t,(float)r,1)); }
void WRAP(glTexCoord3i(GLint s, GLint t, GLint r))         { WRAP(glTexCoord4f((float)s,(float)t,(float)r,1)); }
void WRAP(glTexCoord3s(GLshort s, GLshort t, GLshort r))   { WRAP(glTexCoord4f((float)s,(float)t,(float)r,1)); }
void WRAP(glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q))
{
    DLREC(WRAP(glTexCoord4f(s,t,r,q)));
    globals->render.texcoord = {s,t,r,q};
}
void WRAP(glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)){ WRAP(glTexCoord4f((float)s,(float)t,(float)r,(float)q)); }
void WRAP(glTexCoord4i(GLint s, GLint t, GLint r, GLint q))           { WRAP(glTexCoord4f((float)s,(float)t,(float)r,(float)q)); }
void WRAP(glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q))   { WRAP(glTexCoord4f((float)s,(float)t,(float)r,(float)q)); }
void WRAP(glTexCoord1fv(const GLfloat* v))  { WRAP(glTexCoord1f(v[0])); }
void WRAP(glTexCoord2fv(const GLfloat* v))  { WRAP(glTexCoord2f(v[0],v[1])); }
void WRAP(glTexCoord2dv(const GLdouble* v)) { WRAP(glTexCoord2d(v[0],v[1])); }
void WRAP(glTexCoord2iv(const GLint* v))    { WRAP(glTexCoord2i(v[0],v[1])); }
void WRAP(glTexCoord2sv(const GLshort* v))  { WRAP(glTexCoord2s(v[0],v[1])); }
void WRAP(glTexCoord3fv(const GLfloat* v))  { WRAP(glTexCoord3f(v[0],v[1],v[2])); }
void WRAP(glTexCoord4fv(const GLfloat* v))  { WRAP(glTexCoord4f(v[0],v[1],v[2],v[3])); }

void WRAP(glBindTexture(GLenum target, GLuint texture))
{
    DLREC(WRAP(glBindTexture(target, texture)));
    bool created = texture && !globals->textures[texture];
    if(created)
    {
        globals->textures[texture] = new texture_desc_t{};
        globals->textures[texture]->id = texture;
        globals->textures[texture]->target = target;
    }
    globals->gl.activeTexture = texture ? globals->textures[texture] : nullptr;
    switch(target)
    {
        case 0x0DE0: //GL_TEXTURE_1D:
        case 0x84F5: //GL_TEXTURE_RECTANGLE_ARB:
            glBindTexture(GL_TEXTURE_2D, texture);
            if(created && target == 0x84F5)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            break;

        default:
            glBindTexture(target, texture);
            break;
    }
}

void WRAP(glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, void *indices, GLint basevertex))
{
    if(end < start) { SetError(GL_INVALID_VALUE); return; }
    WRAP(glDrawElementsBaseVertex(mode, count, type, indices, basevertex));
}

void WRAP(glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        WRAP(glDrawArrays(mode, first[i], count[i]));
    }
}

void WRAP(glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        WRAP(glDrawElements(mode, count[i], type, indices[i]));
    }
}

void WRAP(glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount, const GLint* basevertex))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        WRAP(glDrawElementsBaseVertex(mode, count[i], type, indices[i], basevertex[i]));
    }
}

void WRAP(glClipPlane(GLenum plane, const GLdouble *equation))
{
    DLREC_DATA(equation, (4), WRAP(glClipPlane(plane, _values.data())));
    int index = plane - GL_CLIP_PLANE0;
    if (index >= 0 && index < 6)
    {
        globals->ff.clipPlanes[index][0] = (float)equation[0];
        globals->ff.clipPlanes[index][1] = (float)equation[1];
        globals->ff.clipPlanes[index][2] = (float)equation[2];
        globals->ff.clipPlanes[index][3] = (float)equation[3];
    }
}

void WRAP(glClipPlanef(GLenum plane, const GLfloat *equation))
{
    DLREC_DATA(equation, (4), WRAP(glClipPlanef(plane, _values.data())));
    int index = plane - GL_CLIP_PLANE0;
    if (index >= 0 && index < 6)
    {
        globals->ff.clipPlanes[index][0] = equation[0];
        globals->ff.clipPlanes[index][1] = equation[1];
        globals->ff.clipPlanes[index][2] = equation[2];
        globals->ff.clipPlanes[index][3] = equation[3];
    }
}

void WRAP(glClearDepth(double value))
{
    glClearDepthf((float)value);
}

void WRAP(glDepthRange(double n, double f))
{
    glDepthRangef((float)n, (float)f);
}

void WRAP(glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer))
{
    // TODO: glVertexAttribLPointer
    SetError(GL_INVALID_OPERATION);
}

void WRAP(glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z))
{
    glVertexAttrib3f(index, (float)x, (float)y, (float)z);
}

// TODO: a ton of attributes...

void WRAP(glShadeModel(GLenum model))
{
    DLREC(WRAP(glShadeModel(model)));
    globals->ff.shadeModel = model;
}

void WRAP(glPolygonMode(GLenum face, GLenum mode))
{
    DLREC(WRAP(glPolygonMode(face, mode)));
    if(face != GL_FRONT_AND_BACK) { SetError(GL_INVALID_ENUM); return; }
    if(mode < 0x1B00 || mode > 0x1B02) { SetError(GL_INVALID_ENUM); return; }
    switch(mode)
    {
        case 0x1B00: globals->gl.lastPolygonMode = 1; break; // GL_POINT 0x1B00
        case 0x1B01: globals->gl.lastPolygonMode = 2; break; // GL_LINE 0x1B01
        default:     globals->gl.lastPolygonMode = 0; break; // GL_FILL 0x1B02
    }
}

void WRAP(glEnableClientState(GLenum array))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(array == GL_VERTEX_ARRAY)        globals->client.vertexArrayEnabled = true;
    else if(array == GL_COLOR_ARRAY)    globals->client.colorArrayEnabled = true;
    else if(array == GL_TEXTURE_COORD_ARRAY) globals->client.texCoord[globals->client.clientActiveTextureUnit].enabled = true;
    else if(array == GL_NORMAL_ARRAY)   globals->client.normalArrayEnabled = true;
    else if(array == 0x8457) globals->client.fogCoordArrayEnabled = true;
    else if(array == 0x845E) globals->client.secondaryColorArrayEnabled = true;
    else SetError(GL_INVALID_ENUM);
}

void WRAP(glDisableClientState(GLenum array))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(array == 0x8457) { globals->client.fogCoordArrayEnabled = false; return; }
    if(array == 0x845E) { globals->client.secondaryColorArrayEnabled = false; return; }
    if(array == GL_VERTEX_ARRAY)        globals->client.vertexArrayEnabled = false;
    else if(array == GL_COLOR_ARRAY)    globals->client.colorArrayEnabled = false;
    else if(array == GL_TEXTURE_COORD_ARRAY) globals->client.texCoord[globals->client.clientActiveTextureUnit].enabled = false;
    else if(array == GL_NORMAL_ARRAY) globals->client.normalArrayEnabled = false;
    else SetError(GL_INVALID_ENUM);
}

void WRAP(glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *ptr))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(size < 2 || size > 4 || stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_HALF_FLOAT && type != 0x140A && !IsPackedVertex(type)) { SetError(GL_INVALID_ENUM); return; }
    if(IsPackedVertex(type) && size != 4) { SetError(GL_INVALID_OPERATION); return; }
    GLint buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    globals->client.vertexSize   = size;
    globals->client.vertexType   = type;
    globals->client.vertexStride = stride;
    globals->client.vertexPtr    = ptr;
    globals->client.vertexBuffer = buffer;
}

void WRAP(glColorPointer(GLint size, GLenum type, GLsizei stride, const void *ptr))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if((size != 3 && size != 4 && size != 0x80E1) || stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!GetGLTypeSize(type) && !IsPackedVertex(type)) { SetError(GL_INVALID_ENUM); return; }
    if((IsPackedVertex(type) && size == 3) || (size == 0x80E1 && type != GL_UNSIGNED_BYTE && !IsPackedVertex(type))) { SetError(GL_INVALID_OPERATION); return; }
    GLint buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    globals->client.colorSize   = size;
    globals->client.colorType   = type;
    globals->client.colorStride = stride;
    globals->client.colorPtr    = ptr;
    globals->client.colorBuffer = buffer;
}

void WRAP(glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *ptr))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(size < 1 || size > 4 || stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_HALF_FLOAT && type != 0x140A && !IsPackedVertex(type)) { SetError(GL_INVALID_ENUM); return; }
    if(IsPackedVertex(type) && size != 4) { SetError(GL_INVALID_OPERATION); return; }
    GLint buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    texcoord_state_t& state = globals->client.texCoord[globals->client.clientActiveTextureUnit];
    state.texCoordSize   = size;
    state.texCoordType   = type;
    state.texCoordStride = stride;
    state.texCoordPtr    = ptr;
    state.texCoordBuffer = buffer;
}

void WRAP(glNormalPointer(GLenum type, GLsizei stride, const void *ptr))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(type != GL_BYTE && type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_HALF_FLOAT && type != 0x140A && !IsPackedVertex(type)) { SetError(GL_INVALID_ENUM); return; }
    GLint buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    globals->client.normalType   = type;
    globals->client.normalStride = stride;
    globals->client.normalPtr    = ptr;
    globals->client.normalBuffer = buffer;
}

void WRAP(glLightf(GLenum light, GLenum pname, GLfloat param))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(pname < GL_SPOT_EXPONENT || pname > GL_QUADRATIC_ATTENUATION) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glLightfv(light, pname, &param));
}

void WRAP(glLightfv(GLenum light, GLenum pname, const GLfloat *params))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(light < GL_LIGHT0 || light > GL_LIGHT7 || pname < GL_AMBIENT || pname > GL_QUADRATIC_ATTENUATION) { SetError(GL_INVALID_ENUM); return; }
    DLREC_DATA(params, (pname == GL_SPOT_DIRECTION ? 3 : pname >= GL_AMBIENT && pname <= GL_POSITION ? 4 : 1), WRAP(glLightfv(light, pname, _values.data())));
    int idx = light - GL_LIGHT0;
    if(idx < 0 || idx > 7 || pname < GL_AMBIENT || pname > GL_QUADRATIC_ATTENUATION) { SetError(GL_INVALID_ENUM); return; }
    if((pname == GL_SPOT_EXPONENT && (params[0] < 0 || params[0] > 128)) ||
       (pname == GL_SPOT_CUTOFF && (params[0] < 0 || (params[0] > 90 && params[0] != 180))) ||
       (pname >= GL_CONSTANT_ATTENUATION && params[0] < 0)) { SetError(GL_INVALID_VALUE); return; }

    if(pname == 0x1200)       // GL_AMBIENT
        memcpy(&globals->ff.lights[idx].ambient, params, 4 * sizeof(float));
    else if(pname == 0x1201)  // GL_DIFFUSE
        memcpy(&globals->ff.lights[idx].diffuse, params, 4 * sizeof(float));
    else if(pname == 0x1202)  // GL_SPECULAR
        memcpy(&globals->ff.lights[idx].spec, params, 4 * sizeof(float));
    else if(pname == 0x1203)  // GL_POSITION
        MultiplyMatrix2(globals->matrix.modelview.Current().m, params, &globals->ff.lights[idx].pos.x);
    else if(pname == 0x1204)  // GL_SPOT_DIRECTION
    {
        const float* matrix = globals->matrix.modelview.Current().m;
        auto& direction = globals->ff.lights[idx].dir;
        direction.x = matrix[0] * params[0] + matrix[4] * params[1] + matrix[8] * params[2];
        direction.y = matrix[1] * params[0] + matrix[5] * params[1] + matrix[9] * params[2];
        direction.z = matrix[2] * params[0] + matrix[6] * params[1] + matrix[10] * params[2];
    }
    else if(pname == 0x1205)  // GL_SPOT_EXPONENT
        globals->ff.lights[idx].spotExp = params[0];
    else if(pname == 0x1206)  // GL_SPOT_CUTOFF
        globals->ff.lights[idx].spotCutoff = params[0];
    else if(pname == 0x1207)  // GL_CONSTANT_ATTENUATION
        globals->ff.lights[idx].attenuationConst = params[0];
    else if(pname == 0x1208)  // GL_LINEAR_ATTENUATION
        globals->ff.lights[idx].attenuationLinear = params[0];
    else if(pname == 0x1209)  // GL_QUADRATIC_ATTENUATION
        globals->ff.lights[idx].attenuationQuad = params[0];
}

void WRAP(glLightModelf(GLenum pname, GLfloat param))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(pname == GL_LIGHT_MODEL_AMBIENT) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glLightModelfv(pname, &param));
}

void WRAP(glLightModelfv(GLenum pname, const GLfloat* params))
{
    DLREC_DATA(params, (pname == GL_LIGHT_MODEL_AMBIENT ? 4 : 1), WRAP(glLightModelfv(pname, _values.data())));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(pname == GL_LIGHT_MODEL_AMBIENT)
        memcpy(&globals->render.ambient, params, 4 * sizeof(float));
    else if(pname == GL_LIGHT_MODEL_TWO_SIDE)
        globals->ff.lightModelTwoSide = (params[0] != 0.0f);
    else if(pname == 0x0B51) // GL_LIGHT_MODEL_LOCAL_VIEWER
        globals->ff.lightModelLocalViewer = (params[0] != 0.0f);
    else if(pname == 0x81F8)
    {
        if(params[0] != 0x81F9 && params[0] != 0x81FA) { SetError(GL_INVALID_ENUM); return; }
        globals->ff.lightModelColorControl = (GLenum)params[0];
    }
    else SetError(GL_INVALID_ENUM);
}

void WRAP(glLightModeli(GLenum pname, GLint param))
{
    WRAP(glLightModelf(pname, (GLfloat)param));
}

void WRAP(glMaterialf(GLenum face, GLenum pname, GLfloat param))
{
    if(pname != GL_SHININESS) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glMaterialfv(face, pname, &param));
}

void WRAP(glMaterialfv(GLenum face, GLenum pname, const GLfloat *params))
{
    if(face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) { SetError(GL_INVALID_ENUM); return; }
    if(pname != GL_AMBIENT && pname != GL_DIFFUSE && pname != GL_SPECULAR && pname != GL_EMISSION && pname != GL_SHININESS && pname != GL_AMBIENT_AND_DIFFUSE && pname != 0x1603) { SetError(GL_INVALID_ENUM); return; }
    DLREC_DATA(params, (pname == GL_SHININESS ? 1 : pname == 0x1603 ? 3 : 4), WRAP(glMaterialfv(face, pname, _values.data())));
    if(pname == GL_SHININESS && (params[0] < 0 || params[0] > 128)) { SetError(GL_INVALID_VALUE); return; }
    for(int i = 0; i < 2; ++i)
    {
        if(face == (i ? GL_FRONT : GL_BACK)) continue;
        auto& material = globals->ff.materials[i];
        if(pname == GL_AMBIENT || pname == GL_AMBIENT_AND_DIFFUSE) memcpy(material.ambient, params, 4 * sizeof(float));
        if(pname == GL_DIFFUSE || pname == GL_AMBIENT_AND_DIFFUSE) memcpy(material.diffuse, params, 4 * sizeof(float));
        if(pname == GL_SPECULAR) memcpy(material.specular, params, 4 * sizeof(float));
        if(pname == GL_EMISSION) memcpy(material.emission, params, 4 * sizeof(float));
        if(pname == GL_SHININESS) material.shininess = params[0];
        if(pname == 0x1603) memcpy(material.indexes, params, 3 * sizeof(float));
    }
    UpdateColorMaterial();
}

void WRAP(glMateriali(GLenum face, GLenum pname, GLint param))
{
    float fp = (float)param;
    WRAP(glMaterialf(face, pname, fp));
}

void WRAP(glMaterialiv(GLenum face, GLenum pname, const GLint *params))
{
    if(face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) { SetError(GL_INVALID_ENUM); return; }
    if(pname != GL_AMBIENT && pname != GL_DIFFUSE && pname != GL_SPECULAR && pname != GL_EMISSION && pname != GL_SHININESS && pname != GL_AMBIENT_AND_DIFFUSE && pname != 0x1603) { SetError(GL_INVALID_ENUM); return; }
    float fp[4] = {};
    for(int i = 0; i < (pname == GL_SHININESS ? 1 : pname == 0x1603 ? 3 : 4); ++i) fp[i] = pname == GL_SHININESS || pname == 0x1603 ? (float)params[i] : i2f(params[i]);
    WRAP(glMaterialfv(face, pname, fp));
}

void WRAP(glFogf(GLenum pname, GLfloat param))
{
    DLREC(WRAP(glFogf(pname, param)));
    if(pname == GL_FOG_MODE || pname == 0x8450) { WRAP(glFogi(pname, (GLint)param)); return; }
    if(pname == GL_FOG_DENSITY && param < 0) { SetError(GL_INVALID_VALUE); return; }
    if(pname == 0x0B62)       globals->ff.fogDensity = param; // GL_FOG_DENSITY
    else if(pname == 0x0B63)  globals->ff.fogStart   = param; // GL_FOG_START
    else if(pname == 0x0B64)  globals->ff.fogEnd     = param; // GL_FOG_END
    else SetError(GL_INVALID_ENUM);
}

void WRAP(glFogfv(GLenum pname, const GLfloat *params))
{
    DLREC_DATA(params, (pname == GL_FOG_COLOR ? 4 : 1), WRAP(glFogfv(pname, _values.data())));
    if(pname == GL_FOG_COLOR) memcpy(&globals->ff.fogColor, params, 4 * sizeof(float));
    else WRAP(glFogf(pname, params[0]));
}

void WRAP(glFogi(GLenum pname, GLint param))
{
    DLREC(WRAP(glFogi(pname, param)));
    if(pname == 0x0B65)
    {
        if(param != GL_LINEAR && param != GL_EXP && param != GL_EXP2) { SetError(GL_INVALID_ENUM); return; }
        globals->ff.fogMode = param;
    }
    else if(pname == 0x8450)
    {
        if(param != 0x8451 && param != 0x8452) { SetError(GL_INVALID_ENUM); return; }
        globals->ff.fogSource = param;
    }
    else WRAP(glFogf(pname, (GLfloat)param));
}

void WRAP(glLogicOp(GLenum opcode))
{
    DLREC(WRAP(glLogicOp(opcode)));
    globals->ff.logicOpMode = opcode;
}

void WRAP(glClientActiveTexture(GLenum texture))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + 8) { SetError(GL_INVALID_ENUM); return; }
    globals->client.clientActiveTextureUnit = texture - GL_TEXTURE0;
}

void WRAP(glTexEnvi(GLenum target, GLenum pname, GLint param))
{
    DLREC(WRAP(glTexEnvi(target, pname, param)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
    if(target == 0x8861)
    {
        if(pname != 0x8862 || (param != GL_FALSE && param != GL_TRUE)) { SetError(GL_INVALID_ENUM); return; }
        globals->ff.pointCoordReplace[globals->ff.activeTextureUnit] = param != GL_FALSE;
        return;
    }
    if(target != GL_TEXTURE_ENV) { SetError(GL_INVALID_ENUM); return; }
    auto& state = globals->ff.texEnv[globals->ff.activeTextureUnit];
    if(pname == GL_TEXTURE_ENV_MODE)
    {
        if(param != GL_REPLACE && param != GL_MODULATE && param != GL_ADD && param != GL_BLEND && param != GL_DECAL && param != 0x8570) { SetError(GL_INVALID_ENUM); return; }
        state.mode = param;
    }
    else if(pname == 0x8571 || pname == 0x8572)
    {
        if(param != GL_REPLACE && param != GL_MODULATE && param != GL_ADD && param != 0x8574 && param != 0x8575 && param != GL_SUBTRACT && !(pname == 0x8571 && (param == 0x86AE || param == 0x86AF))) { SetError(GL_INVALID_ENUM); return; }
        (pname == 0x8571 ? state.combineRGB : state.combineAlpha) = param;
    }
    else if(pname == 0x8573 || pname == 0x0D1C)
    {
        if(param != 1 && param != 2 && param != 4) { SetError(GL_INVALID_VALUE); return; }
        (pname == 0x8573 ? state.scaleRGB : state.scaleAlpha) = param;
    }
    else if((pname >= 0x8580 && pname <= 0x8582) || (pname >= 0x8588 && pname <= 0x858A))
    {
        if(param != GL_TEXTURE && param != 0x8576 && param != 0x8577 && param != 0x8578 && !(param >= GL_TEXTURE0 && param < GL_TEXTURE0 + 8)) { SetError(GL_INVALID_ENUM); return; }
        (pname >= 0x8588 ? state.sourceAlpha[pname-0x8588] : state.sourceRGB[pname-0x8580]) = param;
    }
    else if((pname >= 0x8590 && pname <= 0x8592) || (pname >= 0x8598 && pname <= 0x859A))
    {
        if(param != GL_SRC_ALPHA && param != GL_ONE_MINUS_SRC_ALPHA && !(pname <= 0x8592 && (param == GL_SRC_COLOR || param == GL_ONE_MINUS_SRC_COLOR))) { SetError(GL_INVALID_ENUM); return; }
        (pname >= 0x8598 ? state.operandAlpha[pname-0x8598] : state.operandRGB[pname-0x8590]) = param;
    }
    else SetError(GL_INVALID_ENUM);
}

void WRAP(glTexEnvf(GLenum target, GLenum pname, GLfloat param))
{
    if(target == 0x8861 && pname == 0x8862 && param != GL_FALSE && param != GL_TRUE) { SetError(GL_INVALID_ENUM); return; }
    if((pname == 0x8573 || pname == 0x0D1C) && param != 1 && param != 2 && param != 4) { SetError(GL_INVALID_VALUE); return; }
    WRAP(glTexEnvi(target, pname, (GLint)param));
}

void WRAP(glTexEnvfv(GLenum target, GLenum pname, const GLfloat *p))
{
    DLREC_DATA(p, (pname == GL_TEXTURE_ENV_COLOR ? 4 : 1), WRAP(glTexEnvfv(target, pname, _values.data())));
    if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
    auto& state = globals->ff.texEnv[globals->ff.activeTextureUnit];
    if(target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_COLOR)
        state.color = { p[0], p[1], p[2], p[3] };
    else WRAP(glTexEnvf(target, pname, p[0]));
}

void WRAP(glTexEnviv(GLenum target, GLenum pname, const GLint *params))
{
    if(pname != GL_TEXTURE_ENV_COLOR)
    {
        WRAP(glTexEnvi(target, pname, params[0]));
    }
    else
    {
        float fp[4] = { i2f(params[0]), i2f(params[1]), i2f(params[2]), i2f(params[3]) };
        WRAP(glTexEnvfv(target, pname, fp));
    }
}

void WRAP(glColorMaterial(GLenum face, GLenum mode))
{
    DLREC(WRAP(glColorMaterial(face, mode)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) { SetError(GL_INVALID_ENUM); return; }
    if(mode != GL_AMBIENT && mode != GL_DIFFUSE && mode != GL_AMBIENT_AND_DIFFUSE && mode != GL_SPECULAR && mode != GL_EMISSION) { SetError(GL_INVALID_ENUM); return; }
    globals->ff.colorMaterialFace = face;
    globals->ff.colorMaterialMode = mode;
    UpdateColorMaterial();
}

void UpdateColorMaterial()
{
    if(!globals->render.colorMaterial) return;
    float color[4] = {globals->render.color.x, globals->render.color.y, globals->render.color.z, globals->render.color.w};
    for(float& value : color) value = std::max(0.0f, std::min(1.0f, value));
    GLenum mode = globals->ff.colorMaterialMode;
    for(int i = 0; i < 2; ++i)
    {
        if(globals->ff.colorMaterialFace == (i ? GL_FRONT : GL_BACK)) continue;
        auto& material = globals->ff.materials[i];
        if(mode == GL_AMBIENT || mode == GL_AMBIENT_AND_DIFFUSE) memcpy(material.ambient, color, sizeof(color));
        if(mode == GL_DIFFUSE || mode == GL_AMBIENT_AND_DIFFUSE) memcpy(material.diffuse, color, sizeof(color));
        if(mode == GL_SPECULAR) memcpy(material.specular, color, sizeof(color));
        if(mode == GL_EMISSION) memcpy(material.emission, color, sizeof(color));
    }
}

static bool ReadLegacyElement(GLint index, GLint size, GLenum type, GLsizei stride, GLuint buffer, const void* pointer, bool normalized, GLfloat* values)
{
    bool bgra = size == 0x80E1;
    size_t scalar = GetGLTypeSize(type), bytes = GetArrayElementSize(size, type), step = stride ? stride : bytes;
    if(!bytes || (!buffer && !pointer)) { SetError(GL_INVALID_OPERATION); return false; }
    if(bgra) size = 4;
    size_t base = (size_t)pointer, limit = buffer ? (size_t)std::numeric_limits<GLintptr>::max() : SIZE_MAX;
    if(base > limit || bytes > limit - base || (size_t)index > (limit - base - bytes) / step) { SetError(GL_INVALID_OPERATION); return false; }
    size_t offset = (size_t)index * step;
    GLint previous = 0;
    const char* data = (const char*)pointer;
    if(buffer)
    {
        glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &previous);
        glBindBuffer(GL_COPY_READ_BUFFER, buffer);
        data = (const char*)glMapBufferRange(GL_COPY_READ_BUFFER, (GLintptr)(base + offset), bytes, GL_MAP_READ_BIT);
        if(!data) { glBindBuffer(GL_COPY_READ_BUFFER, previous); return false; }
    }
    else data += offset;
    auto read = [&](auto zero, int component) { decltype(zero) value; memcpy(&value, data + component * scalar, scalar); return value; };
    if(IsPackedVertex(type))
    {
        GLuint value;
        memcpy(&value, data, sizeof(value));
        UnpackVertex(type, value, normalized, values);
    }
    else for(int c = 0; c < size; ++c)
    {
        switch(type)
        {
            case GL_BYTE: values[c] = normalized ? b2f(read(GLbyte{}, c)) : read(GLbyte{}, c); break;
            case GL_UNSIGNED_BYTE: values[c] = normalized ? ub2f(read(GLubyte{}, c)) : read(GLubyte{}, c); break;
            case GL_SHORT: values[c] = normalized ? s2f(read(GLshort{}, c)) : read(GLshort{}, c); break;
            case GL_UNSIGNED_SHORT: values[c] = normalized ? us2f(read(GLushort{}, c)) : read(GLushort{}, c); break;
            case GL_INT: values[c] = normalized ? i2f(read(GLint{}, c)) : (GLfloat)read(GLint{}, c); break;
            case GL_UNSIGNED_INT: values[c] = normalized ? ui2f(read(GLuint{}, c)) : (GLfloat)read(GLuint{}, c); break;
            case GL_FLOAT: values[c] = read(GLfloat{}, c); break;
            case GL_HALF_FLOAT:
            {
                GLushort value = read(GLushort{}, c);
                int exponent = (value >> 10) & 31, mantissa = value & 1023;
                float result = exponent == 31 ? (mantissa ? std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity()) : std::ldexp((float)(exponent ? mantissa + 1024 : mantissa), exponent ? exponent - 25 : -24);
                values[c] = value & 0x8000 ? -result : result;
                break;
            }
            case 0x140A: values[c] = (GLfloat)read(GLdouble{}, c); break;
            default: values[c] = 0; break;
        }
    }
    if(bgra) std::swap(values[0], values[2]);
    if(buffer) { glUnmapBuffer(GL_COPY_READ_BUFFER); glBindBuffer(GL_COPY_READ_BUFFER, previous); }
    return true;
}

void WRAP(glArrayElement(GLint i))
{
    if(i < 0) { SetError(GL_INVALID_VALUE); return; }
    const auto& client = globals->client;
    GLfloat values[4];
    if(client.secondaryColorArrayEnabled)
    {
        if(!ReadLegacyElement(i, client.secondaryColorSize, client.secondaryColorType, client.secondaryColorStride, client.secondaryColorBuffer, client.secondaryColorPtr, true, values)) return;
        WRAP(glSecondaryColor3f(values[0], values[1], values[2]));
    }
    if(client.fogCoordArrayEnabled)
    {
        if(!ReadLegacyElement(i, 1, client.fogCoordType, client.fogCoordStride, client.fogCoordBuffer, client.fogCoordPtr, false, values)) return;
        WRAP(glFogCoordf(values[0]));
    }

    if(client.normalArrayEnabled)
    {
        if(!ReadLegacyElement(i, 3, client.normalType, client.normalStride, client.normalBuffer, client.normalPtr, true, values)) return;
        WRAP(glNormal3f(values[0], values[1], values[2]));
    }
    if(client.colorArrayEnabled)
    {
        values[3] = 1;
        if(!ReadLegacyElement(i, client.colorSize, client.colorType, client.colorStride, client.colorBuffer, client.colorPtr, true, values)) return;
        WRAP(glColor4f(values[0], values[1], values[2], values[3]));
    }
    for(int unit = 0; unit < 8; ++unit)
    {
        const auto& state = client.texCoord[unit];
        if(!state.enabled) continue;
        values[1] = values[2] = 0;
        values[3] = 1;
        if(!ReadLegacyElement(i, state.texCoordSize, state.texCoordType, state.texCoordStride, state.texCoordBuffer, state.texCoordPtr, false, values)) return;
        WRAP(glMultiTexCoord4f(GL_TEXTURE0 + unit, values[0], values[1], values[2], values[3]));
    }
    if(client.vertexArrayEnabled)
    {
        values[2] = 0;
        values[3] = 1;
        if(!ReadLegacyElement(i, client.vertexSize, client.vertexType, client.vertexStride, client.vertexBuffer, client.vertexPtr, false, values)) return;
        WRAP(glVertex4f(values[0], values[1], values[2], values[3]));
    }
}
void WRAP(glInterleavedArrays(GLenum format, GLsizei stride, const void* pointer))
{
    WRAP(glDisableClientState(0x8457));
    WRAP(glDisableClientState(0x845E));
    // Disable all first
    WRAP(glDisableClientState(GL_VERTEX_ARRAY));
    WRAP(glDisableClientState(GL_NORMAL_ARRAY));
    WRAP(glDisableClientState(GL_COLOR_ARRAY));
    WRAP(glDisableClientState(GL_TEXTURE_COORD_ARRAY));

    const char* p = (const char*)pointer;
    int et = sizeof(GLfloat);      // float size
    int ec = sizeof(GLubyte);      // byte size

    // Format layout table: {hasTexCoord, texSize, hasColor, colorType, colorSize, hasNormal, vertSize, totalFloats}
    struct Layout { bool tc; int ts; bool col; GLenum ct; int cs; bool nm; int vs; int totalBytes; };
    Layout L = {};
    switch(format)
    {
        case 0x2A20: // GL_V2F
            L = {false,0,false,0,0,false,2, 2*et}; break;
        case 0x2A21: // GL_V3F
            L = {false,0,false,0,0,false,3, 3*et}; break;
        case 0x2A22: // GL_C4UB_V2F
            L = {false,0,true,GL_UNSIGNED_BYTE,4,false,2, 4*ec+2*et}; break;
        case 0x2A23: // GL_C4UB_V3F
            L = {false,0,true,GL_UNSIGNED_BYTE,4,false,3, 4*ec+3*et}; break;
        case 0x2A24: // GL_C3F_V3F
            L = {false,0,true,GL_FLOAT,3,false,3, 6*et}; break;
        case 0x2A25: // GL_N3F_V3F
            L = {false,0,false,0,0,true,3, 6*et}; break;
        case 0x2A26: // GL_C4F_N3F_V3F
            L = {false,0,true,GL_FLOAT,4,true,3, 10*et}; break;
        case 0x2A27: // GL_T2F_V3F
            L = {true,2,false,0,0,false,3, 5*et}; break;
        case 0x2A28: // GL_T4F_V4F
            L = {true,4,false,0,0,false,4, 8*et}; break;
        case 0x2A29: // GL_T2F_C4UB_V3F
            L = {true,2,true,GL_UNSIGNED_BYTE,4,false,3, 2*et+4*ec+3*et}; break;
        case 0x2A2A: // GL_T2F_C3F_V3F
            L = {true,2,true,GL_FLOAT,3,false,3, 8*et}; break;
        case 0x2A2B: // GL_T2F_N3F_V3F
            L = {true,2,false,0,0,true,3, 8*et}; break;
        case 0x2A2C: // GL_T2F_C4F_N3F_V3F
            L = {true,2,true,GL_FLOAT,4,true,3, 12*et}; break;
        case 0x2A2D: // GL_T4F_C4F_N3F_V4F
            L = {true,4,true,GL_FLOAT,4,true,4, 15*et}; break;
        default:
            ERR("glInterleavedArrays: unknown format 0x%X", format);
            return;
    }

    if(stride == 0) stride = L.totalBytes;

    // Walk through layout: TexCoord | Color | Normal | Vertex
    int offset = 0;
    if(L.tc)
    {
        WRAP(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
        WRAP(glTexCoordPointer(L.ts, GL_FLOAT, stride, p + offset));
        offset += L.ts * et;
    }
    if(L.col)
    {
        WRAP(glEnableClientState(GL_COLOR_ARRAY));
        int csz = (L.ct == GL_UNSIGNED_BYTE) ? L.cs * ec : L.cs * et;
        WRAP(glColorPointer(L.cs, L.ct, stride, p + offset));
        offset += csz;
    }
    if(L.nm)
    {
        WRAP(glEnableClientState(GL_NORMAL_ARRAY));
        WRAP(glNormalPointer(GL_FLOAT, stride, p + offset));
        offset += 3 * et;
    }
    WRAP(glEnableClientState(GL_VERTEX_ARRAY));
    WRAP(glVertexPointer(L.vs, GL_FLOAT, stride, p + offset));
}

static inline void SetMultiTexCoord(GLenum unit, float s, float t, float r = 0, float q = 1)
{
    int u = (int)(unit - GL_TEXTURE0);
    if(u < 0 || u >= 8) { SetError(GL_INVALID_ENUM); return; }
    if(u == 0) globals->render.texcoord = { s, t, r, q };
    else globals->render.multiTexcoord[u-1] = { s, t, r, q };
}

void WRAP(glMultiTexCoord1f(GLenum t, GLfloat s))                              { DLREC(WRAP(glMultiTexCoord1f(t,s)));         SetMultiTexCoord(t,s,0); }
void WRAP(glMultiTexCoord2f(GLenum t, GLfloat s, GLfloat r))                   { DLREC(WRAP(glMultiTexCoord2f(t,s,r)));       SetMultiTexCoord(t,s,r); }
void WRAP(glMultiTexCoord3f(GLenum t, GLfloat s, GLfloat r, GLfloat q))        { DLREC(WRAP(glMultiTexCoord3f(t,s,r,q)));     SetMultiTexCoord(t,s,r,q); }
void WRAP(glMultiTexCoord4f(GLenum t, GLfloat s, GLfloat r, GLfloat q, GLfloat w)){ DLREC(WRAP(glMultiTexCoord4f(t,s,r,q,w))); SetMultiTexCoord(t,s,r,q,w); }
void WRAP(glMultiTexCoord1d(GLenum t, GLdouble s))                             { WRAP(glMultiTexCoord1f(t,(float)s)); }
void WRAP(glMultiTexCoord2d(GLenum t, GLdouble s, GLdouble r))                 { WRAP(glMultiTexCoord2f(t,(float)s,(float)r)); }
void WRAP(glMultiTexCoord3d(GLenum t, GLdouble s, GLdouble r, GLdouble q))     { WRAP(glMultiTexCoord3f(t,(float)s,(float)r,(float)q)); }
void WRAP(glMultiTexCoord4d(GLenum t, GLdouble s, GLdouble r, GLdouble q, GLdouble w)){ WRAP(glMultiTexCoord4f(t,(float)s,(float)r,(float)q,(float)w)); }
void WRAP(glMultiTexCoord1i(GLenum t, GLint s))                                { WRAP(glMultiTexCoord1f(t,(float)s)); }
void WRAP(glMultiTexCoord2i(GLenum t, GLint s, GLint r))                       { WRAP(glMultiTexCoord2f(t,(float)s,(float)r)); }
void WRAP(glMultiTexCoord2s(GLenum t, GLshort s, GLshort r))                   { WRAP(glMultiTexCoord2f(t,(float)s,(float)r)); }
void WRAP(glMultiTexCoord1fv(GLenum t, const GLfloat* v))                      { WRAP(glMultiTexCoord1f(t,v[0])); }
void WRAP(glMultiTexCoord2fv(GLenum t, const GLfloat* v))                      { WRAP(glMultiTexCoord2f(t,v[0],v[1])); }
void WRAP(glMultiTexCoord3fv(GLenum t, const GLfloat* v))                      { WRAP(glMultiTexCoord3f(t,v[0],v[1],v[2])); }
void WRAP(glMultiTexCoord4fv(GLenum t, const GLfloat* v))                      { WRAP(glMultiTexCoord4f(t,v[0],v[1],v[2],v[3])); }
void WRAP(glMultiTexCoord2dv(GLenum t, const GLdouble* v))                     { WRAP(glMultiTexCoord2d(t,v[0],v[1])); }
void WRAP(glMultiTexCoord2iv(GLenum t, const GLint* v))                        { WRAP(glMultiTexCoord2i(t,v[0],v[1])); }
void WRAP(glMultiTexCoord2sv(GLenum t, const GLshort* v))                      { WRAP(glMultiTexCoord2s(t,v[0],v[1])); }

void WRAP(glTexGenf(GLenum coord, GLenum pname, GLfloat param))
{
    DLREC(WRAP(glTexGenf(coord,pname,param)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(coord < GL_S || coord > (GL_S + 3) || pname != GL_TEXTURE_GEN_MODE) { SetError(GL_INVALID_ENUM); return; }
    if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
    if(param != 0x2401 && param != 0x2400 && param != 0x2402 && param != 0x8511 && param != 0x8512) { SetError(GL_INVALID_ENUM); return; }
    if((param == 0x2402 && coord > (GL_S + 1)) || ((param == 0x8511 || param == 0x8512) && coord == (GL_S + 3))) { SetError(GL_INVALID_ENUM); return; }
    globals->ff.texGen[globals->ff.activeTextureUnit][coord-GL_S].mode = (GLenum)param;
}
void WRAP(glTexGend(GLenum coord, GLenum pname, GLdouble param)) { WRAP(glTexGenf(coord,pname,(float)param)); }
void WRAP(glTexGeni(GLenum coord, GLenum pname, GLint param)) { WRAP(glTexGenf(coord,pname,(float)param)); }
void WRAP(glTexGenfv(GLenum coord, GLenum pname, const GLfloat* p))
{
    DLREC_DATA(p, (pname == GL_OBJECT_PLANE || pname == GL_EYE_PLANE ? 4 : 1), WRAP(glTexGenfv(coord,pname,_values.data())));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(coord < GL_S || coord > (GL_S + 3)) { SetError(GL_INVALID_ENUM); return; }
    if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
    auto& state = globals->ff.texGen[globals->ff.activeTextureUnit][coord-GL_S];
    if(pname == GL_TEXTURE_GEN_MODE) WRAP(glTexGenf(coord,pname,p[0]));
    else if(pname == GL_OBJECT_PLANE) memcpy(state.objectPlane, p, 4*sizeof(float));
    else if(pname == GL_EYE_PLANE)
    {
        float inverse[16];
        InverseMatrix(globals->matrix.modelview.Current().m, inverse);
        for(int c = 0; c < 4; ++c)
        {
            state.eyePlane[c] = 0;
            for(int j = 0; j < 4; ++j) state.eyePlane[c] += inverse[c*4+j] * p[j];
        }
    }
    else SetError(GL_INVALID_ENUM);
}
void WRAP(glTexGendv(GLenum coord, GLenum pname, const GLdouble* p))
{
    float values[4] = {};
    int count = pname == GL_OBJECT_PLANE || pname == GL_EYE_PLANE ? 4 : 1;
    for(int i = 0; i < count; ++i) values[i] = (float)p[i];
    WRAP(glTexGenfv(coord,pname,values));
}
void WRAP(glTexGeniv(GLenum coord, GLenum pname, const GLint* p))
{
    float values[4] = {};
    int count = pname == GL_OBJECT_PLANE || pname == GL_EYE_PLANE ? 4 : 1;
    for(int i = 0; i < count; ++i) values[i] = (float)p[i];
    WRAP(glTexGenfv(coord,pname,values));
}
void WRAP(glPointParameterf(GLenum pname, GLfloat param))
{
    DLREC(WRAP(glPointParameterf(pname,param)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    GLIN_InitExtensions();
    if(pname >= 0x8126 && pname <= 0x8128 && param < 0) { SetError(GL_INVALID_VALUE); return; }
    switch(pname)
    {
        case 0x8126: globals->ff.pointMin = param; break;
        case 0x8127: globals->ff.pointMax = param; break;
        case 0x8128: // GL_POINT_FADE_THRESHOLD_SIZE
            globals->ff.pointFadeThreshold = param; break;
        case 0x8CA0: // GL_POINT_SPRITE_COORD_ORIGIN
            if(param != 0x8CA1 && param != 0x8CA2) { SetError(GL_INVALID_ENUM); return; }
            globals->ff.pointOrigin = (GLenum)param;
            break;
        default: SetError(GL_INVALID_ENUM); break;
    }
}
void WRAP(glPointParameteri(GLenum pname, GLint param))  { WRAP(glPointParameterf(pname,(float)param)); }
void WRAP(glPointParameterfv(GLenum pname, const GLfloat* p))
{
    DLREC_DATA(p, (pname == 0x8129 ? 3 : 1), WRAP(glPointParameterfv(pname,_values.data())));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(pname == 0x8129) // GL_POINT_DISTANCE_ATTENUATION
        memcpy(globals->ff.pointAttenuation, p, 3*sizeof(float));
    else
        WRAP(glPointParameterf(pname, p[0]));
}
void WRAP(glPointParameteriv(GLenum pname, const GLint* p))
{
    float fp[3] = {(float)p[0],0,0};
    if(pname == 0x8129) { fp[1] = (float)p[1]; fp[2] = (float)p[2]; }
    WRAP(glPointParameterfv(pname,fp));
}

void WRAP(glSecondaryColorPointer(GLint size, GLenum type, GLsizei stride, const void* ptr))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if((size != 3 && size != 0x80E1) || stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!GetGLTypeSize(type) && !IsPackedVertex(type)) { SetError(GL_INVALID_ENUM); return; }
    if((IsPackedVertex(type) && size == 3) || (size == 0x80E1 && type != GL_UNSIGNED_BYTE && !IsPackedVertex(type))) { SetError(GL_INVALID_OPERATION); return; }
    globals->client.secondaryColorSize   = size;
    globals->client.secondaryColorType   = type;
    globals->client.secondaryColorStride = stride;
    globals->client.secondaryColorPtr    = ptr;
    GLint buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    globals->client.secondaryColorBuffer = buffer;
}

void WRAP(glFogCoordPointer(GLenum type, GLsizei stride, const void* ptr))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(type != GL_FLOAT && type != GL_HALF_FLOAT && type != 0x140A) { SetError(GL_INVALID_ENUM); return; }
    if(stride < 0) { SetError(GL_INVALID_VALUE); return; }
    GLint buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    globals->client.fogCoordType = type;
    globals->client.fogCoordStride = stride;
    globals->client.fogCoordPtr = ptr;
    globals->client.fogCoordBuffer = buffer;
}

void WRAP(glEdgeFlag(GLboolean flag))                    { SetError(GL_INVALID_OPERATION); }
void WRAP(glEdgeFlagv(const GLboolean* flag))            { SetError(GL_INVALID_OPERATION); }
void WRAP(glEdgeFlagPointer(GLsizei stride, const void* ptr)) { SetError(GL_INVALID_OPERATION); }

void WRAP(glPolygonStipple(const GLubyte* mask))
{
    DLREC(WRAP(glPolygonStipple(mask)));
    memcpy(globals->ff.polygonStipple, mask, 128);
}
void WRAP(glGetPolygonStipple(GLubyte* mask))
{
    memcpy(mask, globals->ff.polygonStipple, 128);
}
void WRAP(glLineStipple(GLint factor, GLushort pattern))
{
    DLREC(WRAP(glLineStipple(factor,pattern)));
    globals->ff.lineStippleFactor  = factor;
    globals->ff.lineStipplePattern = pattern;
}

void WRAP(glPixelZoom(GLfloat xfactor, GLfloat yfactor))
{
    globals->ff.pixelZoomX = xfactor;
    globals->ff.pixelZoomY = yfactor;
}
void WRAP(glPixelTransferf(GLenum pname, GLfloat param))  { SetError(GL_INVALID_OPERATION); }
void WRAP(glPixelTransferi(GLenum pname, GLint param))    { SetError(GL_INVALID_OPERATION); }
void WRAP(glPixelMapfv(GLenum map, GLsizei size, const GLfloat* v))   { SetError(GL_INVALID_OPERATION); }
void WRAP(glPixelMapuiv(GLenum map, GLsizei size, const GLuint* v))   { SetError(GL_INVALID_OPERATION); }
void WRAP(glPixelMapusv(GLenum map, GLsizei size, const GLushort* v)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetPixelMapfv(GLenum map, GLfloat* v))   { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetPixelMapuiv(GLenum map, GLuint* v))   { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetPixelMapusv(GLenum map, GLushort* v)) { SetError(GL_INVALID_OPERATION); }

void WRAP(glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels))
{
    if(globals->gl.conditionalDiscard) return;
    if(!pixels || width <= 0 || height <= 0) return;
    if(!globals->render.rasterPosValid) return;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Remap unsupported formats same as glTexImage2D wrapper
    if(format == 0x80E0) format = GL_RGB;       // GL_BGR
    else if(format == 0x80E1) format = GL_RGBA; // GL_BGRA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, type, pixels);

    // Draw textured quad at raster position in NDC
    float rx = globals->render.rasterPos.x;
    float ry = globals->render.rasterPos.y;
    float rz = globals->render.rasterPos.z;
    // Raster pos is already in clip space — convert to NDC width/height
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
    float pw = 2.0f * width  / (float)viewport[2];
    float ph = 2.0f * height / (float)viewport[3];

    GLuint prevProg = globals->gl.activeProgram;
    // Use FFP textured no-lighting path
    bool prevTex = globals->render.texture;
    globals->render.texture = true;
    WRAP(glBegin(GL_TRIANGLE_FAN));
        WRAP(glTexCoord2f(0,0)); WRAP(glVertex3f(rx,    ry,    rz));
        WRAP(glTexCoord2f(1,0)); WRAP(glVertex3f(rx+pw, ry,    rz));
        WRAP(glTexCoord2f(1,1)); WRAP(glVertex3f(rx+pw, ry+ph, rz));
        WRAP(glTexCoord2f(0,1)); WRAP(glVertex3f(rx,    ry+ph, rz));
    WRAP(glEnd());
    globals->render.texture = prevTex;

    glDeleteTextures(1, &tex);
    glUseProgram(prevProg);

    // Advance raster pos
    globals->render.rasterPos.x += pw;
}

void WRAP(glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type))
{
    if(globals->gl.conditionalDiscard) return;
    // Read from read framebuffer, draw at raster pos
    if(!globals->render.rasterPosValid) return;
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, width, height, 0);
    // Reuse DrawPixels logic by treating it as a pre-uploaded texture
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
    float rx = globals->render.rasterPos.x;
    float ry = globals->render.rasterPos.y;
    float rz = globals->render.rasterPos.z;
    float pw = 2.0f * width  / (float)viewport[2];
    float ph = 2.0f * height / (float)viewport[3];
    bool prevTex = globals->render.texture;
    globals->render.texture = true;
    WRAP(glBegin(GL_TRIANGLE_FAN));
        WRAP(glTexCoord2f(0,0)); WRAP(glVertex3f(rx,    ry,    rz));
        WRAP(glTexCoord2f(1,0)); WRAP(glVertex3f(rx+pw, ry,    rz));
        WRAP(glTexCoord2f(1,1)); WRAP(glVertex3f(rx+pw, ry+ph, rz));
        WRAP(glTexCoord2f(0,1)); WRAP(glVertex3f(rx,    ry+ph, rz));
    WRAP(glEnd());
    globals->render.texture = prevTex;
    glDeleteTextures(1, &tex);
    glUseProgram(globals->gl.activeProgram);
}
