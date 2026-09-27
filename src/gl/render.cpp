#include "GLES.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "wrapped.h"
#include "globals.h"
#include "glhelper.h"
#include "maths.h"
#include "draw_state.h"

void WRAP(glBegin(GLenum mode))
{
    DLREC(WRAP(glBegin(mode)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(mode > 9) { SetError(GL_INVALID_ENUM); return; }
    globals->render.lastPrimitiveMode = mode;
    globals->render.begin = true;
    
    globals->render.vertices.clear();
    globals->render.colors.clear();
    globals->render.texcoords.clear();
    globals->render.normals.clear();
}

void WRAP(glEnd())
{
    DLREC(WRAP(glEnd()));
    if(!globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    globals->render.begin = false;
    if(globals->render.vertices.empty()) return;
    
    draw_state_t saved;
    globals->render.begin = false;
    
    TransformFixedVerts(); // also draws.
    glUseProgram(globals->gl.activeProgram); // restore program
    
    globals->render.vertices.clear();
    globals->render.colors.clear();
    globals->render.texcoords.clear();
    globals->render.normals.clear();
}

void WRAP(glColor3f(GLfloat r, GLfloat g, GLfloat b))                     { DLREC(WRAP(glColor3f(r,g,b)));      globals->render.color = {r, g, b, 1.0f}; }
void WRAP(glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a))          { DLREC(WRAP(glColor4f(r,g,b,a)));    globals->render.color = {r, g, b, a}; }
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

void WRAP(glVertex3f(GLfloat x, GLfloat y, GLfloat z))
{
    DLREC(WRAP(glVertex3f(x, y, z)));
    if(!globals->render.begin) return;
    globals->render.vertices.push_back({x, y, z, 1});
    globals->render.colors.push_back(globals->render.color);
    globals->render.texcoords.push_back(globals->render.texcoord);
    globals->render.normals.push_back(globals->render.normal);
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
    globals->render.texcoords.push_back(globals->render.texcoord);
    globals->render.normals.push_back(globals->render.normal);
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
    globals->render.texcoord = { s, t };
}
void WRAP(glTexCoord1f(GLfloat s))                   { WRAP(glTexCoord2f(s, 0.0f)); }
void WRAP(glTexCoord1d(GLdouble s))                  { WRAP(glTexCoord2f((float)s, 0.0f)); }
void WRAP(glTexCoord1i(GLint s))                     { WRAP(glTexCoord2f((float)s, 0.0f)); }
void WRAP(glTexCoord1s(GLshort s))                   { WRAP(glTexCoord2f((float)s, 0.0f)); }
void WRAP(glTexCoord2d(GLdouble s, GLdouble t))      { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord2i(GLint s, GLint t))            { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord2s(GLshort s, GLshort t))        { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord3f(GLfloat s, GLfloat t, GLfloat r))  { WRAP(glTexCoord2f(s,t)); /* r dropped */ }
void WRAP(glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)){ WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord3i(GLint s, GLint t, GLint r))         { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord3s(GLshort s, GLshort t, GLshort r))   { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)) { WRAP(glTexCoord2f(s,t)); }
void WRAP(glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)){ WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord4i(GLint s, GLint t, GLint r, GLint q))           { WRAP(glTexCoord2f((float)s,(float)t)); }
void WRAP(glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q))   { WRAP(glTexCoord2f((float)s,(float)t)); }
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
    if(array == GL_VERTEX_ARRAY)        globals->client.vertexArrayEnabled = true;
    else if(array == GL_COLOR_ARRAY)    globals->client.colorArrayEnabled = true;
    else if(array == GL_TEXTURE_COORD_ARRAY) globals->client.texCoord[globals->client.clientActiveTextureUnit].enabled = true;
    else if(array == GL_NORMAL_ARRAY)   globals->client.normalArrayEnabled = true;
}

void WRAP(glDisableClientState(GLenum array))
{
    if(array == GL_VERTEX_ARRAY)        globals->client.vertexArrayEnabled = false;
    else if(array == GL_COLOR_ARRAY)    globals->client.colorArrayEnabled = false;
    else if(array == GL_TEXTURE_COORD_ARRAY) globals->client.texCoord[globals->client.clientActiveTextureUnit].enabled = false;
    else if(array == GL_NORMAL_ARRAY)
    {
        globals->client.normalArrayEnabled = false;
        glVertexAttrib3f(2, 0.0f, 0.0f, 1.0f); 
    }
}

void WRAP(glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *ptr))
{
    if(size < 2 || size > 4 || stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!GetGLTypeSize(type)) { SetError(GL_INVALID_ENUM); return; }
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
    if(size < 3 || size > 4 || stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!GetGLTypeSize(type)) { SetError(GL_INVALID_ENUM); return; }
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
    if(size < 1 || size > 4 || stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!GetGLTypeSize(type)) { SetError(GL_INVALID_ENUM); return; }
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
    if(false || stride < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!GetGLTypeSize(type)) { SetError(GL_INVALID_ENUM); return; }
    GLint buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    globals->client.normalType   = type;
    globals->client.normalStride = stride;
    globals->client.normalPtr    = ptr;
    globals->client.normalBuffer = buffer;
}

void WRAP(glLightf(GLenum light, GLenum pname, GLfloat param))
{
    WRAP(glLightfv(light, pname, &param));
}

void WRAP(glLightfv(GLenum light, GLenum pname, const GLfloat *params))
{
    DLREC_DATA(params, (pname == GL_SPOT_DIRECTION ? 3 : pname >= GL_AMBIENT && pname <= GL_POSITION ? 4 : 1), WRAP(glLightfv(light, pname, _values.data())));
    int idx = light - GL_LIGHT0;
    if(idx < 0 || idx > 7) return;

    if(pname == 0x1200)       // GL_AMBIENT
        memcpy(&globals->ff.lights[idx].ambient, params, 4 * sizeof(float));
    else if(pname == 0x1201)  // GL_DIFFUSE
        memcpy(&globals->ff.lights[idx].diffuse, params, 4 * sizeof(float));
    else if(pname == 0x1202)  // GL_SPECULAR
        memcpy(&globals->ff.lights[idx].spec, params, 4 * sizeof(float));
    else if(pname == 0x1203)  // GL_POSITION
        MultiplyMatrix2(globals->matrix.modelview.Current().m, params, &globals->ff.lights[idx].pos.x);
    else if(pname == 0x1204)  // GL_SPOT_DIRECTION
        memcpy(&globals->ff.lights[idx].dir, params, 3 * sizeof(float));
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
    WRAP(glLightModelfv(pname, &param));
}

void WRAP(glLightModelfv(GLenum pname, const GLfloat* params))
{
    if(pname == GL_LIGHT_MODEL_AMBIENT)
        memcpy(&globals->render.ambient, params, 4 * sizeof(float));
    else if(pname == GL_LIGHT_MODEL_TWO_SIDE)
        globals->ff.lightModelTwoSide = (params[0] != 0.0f);
    else if(pname == 0x0B51) // GL_LIGHT_MODEL_LOCAL_VIEWER
        globals->ff.lightModelLocalViewer = (params[0] != 0.0f);
}

void WRAP(glLightModeli(GLenum pname, GLint param))
{
    if(pname == GL_LIGHT_MODEL_TWO_SIDE)
        globals->ff.lightModelTwoSide = (param != 0);
    else if(pname == 0x0B51) // GL_LIGHT_MODEL_LOCAL_VIEWER
        globals->ff.lightModelLocalViewer = (param != 0);
}

void WRAP(glMaterialf(GLenum face, GLenum pname, GLfloat param))
{
    DLREC(WRAP(glMaterialf(face, pname, param)));
    if(pname == 0x1601) globals->ff.matShininess = param; // GL_SHININESS
}

void WRAP(glMaterialfv(GLenum face, GLenum pname, const GLfloat *params))
{
    DLREC_DATA(params, (pname == GL_SHININESS ? 1 : 4), WRAP(glMaterialfv(face, pname, _values.data())));
    if(pname == 0x1200)       memcpy(globals->ff.matAmbient,  params, 4 * sizeof(float)); // GL_AMBIENT
    else if(pname == 0x1201)  memcpy(globals->ff.matDiffuse,  params, 4 * sizeof(float)); // GL_DIFFUSE
    else if(pname == 0x1202)  memcpy(globals->ff.matSpecular, params, 4 * sizeof(float)); // GL_SPECULAR
    else if(pname == 0x1600)  memcpy(globals->ff.matEmission, params, 4 * sizeof(float)); // GL_EMISSION
    else if(pname == 0x1601)  globals->ff.matShininess = params[0];                        // GL_SHININESS
    else if(pname == 0x1602)  // GL_AMBIENT_AND_DIFFUSE
    {
        memcpy(globals->ff.matAmbient, params, 4 * sizeof(float));
        memcpy(globals->ff.matDiffuse, params, 4 * sizeof(float));
    }
}

void WRAP(glMateriali(GLenum face, GLenum pname, GLint param))
{
    float fp = (float)param;
    WRAP(glMaterialf(face, pname, fp));
}

void WRAP(glMaterialiv(GLenum face, GLenum pname, const GLint *params))
{
    float fp[4] = {};
    for(int i = 0; i < (pname == GL_SHININESS ? 1 : 4); ++i) fp[i] = pname == GL_SHININESS ? (float)params[i] : i2f(params[i]);
    WRAP(glMaterialfv(face, pname, fp));
}

void WRAP(glFogf(GLenum pname, GLfloat param))
{
    DLREC(WRAP(glFogf(pname, param)));
    if(pname == GL_FOG_MODE) { WRAP(glFogi(pname, (GLint)param)); return; }
    if(pname == GL_FOG_DENSITY && param < 0) { SetError(GL_INVALID_VALUE); return; }
    if(pname == 0x0B62)       globals->ff.fogDensity = param; // GL_FOG_DENSITY
    else if(pname == 0x0B63)  globals->ff.fogStart   = param; // GL_FOG_START
    else if(pname == 0x0B64)  globals->ff.fogEnd     = param; // GL_FOG_END
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
    if(pname == 0x0B65) // GL_FOG_MODE
        globals->ff.fogMode = param;
}

void WRAP(glLogicOp(GLenum opcode))
{
    DLREC(WRAP(glLogicOp(opcode)));
    globals->ff.logicOpMode = opcode;
}

void WRAP(glClientActiveTexture(GLenum texture))
{
    if(texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + 8) { SetError(GL_INVALID_ENUM); return; }
    globals->client.clientActiveTextureUnit = texture - GL_TEXTURE0;
}

void WRAP(glTexEnvi(GLenum target, GLenum pname, GLint param))
{
    DLREC(WRAP(glTexEnvi(target, pname, param)));
    if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
    texcoord_state_t& state = globals->client.texCoord[globals->ff.activeTextureUnit];
    if(target == GL_TEXTURE_ENV)
    {
        if(pname == GL_TEXTURE_ENV_MODE)
        {
            if(param == GL_REPLACE)       state.texCoordBlendLogic = 0;
            else if(param == GL_ADD)      state.texCoordBlendLogic = 2;
            else if(param == GL_BLEND)    state.texCoordBlendLogic = 3;
            else if(param == GL_DECAL)    state.texCoordBlendLogic = 4;
            else                          state.texCoordBlendLogic = 1; // GL_MODULATE
        }
    }
}

void WRAP(glTexEnvf(GLenum target, GLenum pname, GLfloat param))
{
    WRAP(glTexEnvi(target, pname, (GLint)param));
}

void WRAP(glTexEnvfv(GLenum target, GLenum pname, const GLfloat *p))
{
    DLREC_DATA(p, (pname == GL_TEXTURE_ENV_COLOR ? 4 : 1), WRAP(glTexEnvfv(target, pname, _values.data())));
    if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
    texcoord_state_t& state = globals->client.texCoord[globals->ff.activeTextureUnit];
    if(target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_COLOR)
        state.texCoordColor = { p[0], p[1], p[2], p[3] };
}

void WRAP(glTexEnviv(GLenum target, GLenum pname, const GLint *params))
{
    if(pname == GL_TEXTURE_ENV_MODE)
    {
        WRAP(glTexEnvi(target, pname, params[0]));
    }
    else
    {
        float fp[4] = { params[0]/255.f, params[1]/255.f, params[2]/255.f, params[3]/255.f };
        WRAP(glTexEnvfv(target, pname, fp));
    }
}

void WRAP(glColorMaterial(GLenum face, GLenum mode))
{
    DLREC(WRAP(glColorMaterial(face, mode)));
    globals->ff.colorMaterialFace = face;
    globals->ff.colorMaterialMode = mode;
}

void WRAP(glArrayElement(GLint i))
{
    if(i < 0) { SetError(GL_INVALID_VALUE); return; }

    // Normal
    if(globals->client.normalArrayEnabled && globals->client.normalPtr)
    {
        const char* base = (const char*)globals->client.normalPtr;
        GLsizei stride = globals->client.normalStride
            ? globals->client.normalStride
            : (3 * (GLsizei)GetGLTypeSize(globals->client.normalType));
        const char* p = base + i * stride;
        switch(globals->client.normalType)
        {
            case GL_FLOAT:  { const GLfloat*  v=(const GLfloat*)p;  WRAP(glNormal3f(v[0],v[1],v[2])); break; }
            case 0x140A: { const GLdouble* v=(const GLdouble*)p; WRAP(glNormal3f((float)v[0],(float)v[1],(float)v[2])); break; }
            case GL_BYTE:   { const GLbyte*   v=(const GLbyte*)p;   WRAP(glNormal3f(b2f(v[0]),b2f(v[1]),b2f(v[2]))); break; }
            case GL_SHORT:  { const GLshort*  v=(const GLshort*)p;  WRAP(glNormal3f(s2f(v[0]),s2f(v[1]),s2f(v[2]))); break; }
            case GL_INT:    { const GLint*    v=(const GLint*)p;    WRAP(glNormal3f(i2f(v[0]),i2f(v[1]),i2f(v[2]))); break; }
            default: break;
        }
    }
    // Color
    if(globals->client.colorArrayEnabled && globals->client.colorPtr)
    {
        const char* base = (const char*)globals->client.colorPtr;
        GLsizei stride = globals->client.colorStride
            ? globals->client.colorStride
            : (globals->client.colorSize * (GLsizei)GetGLTypeSize(globals->client.colorType));
        const char* p = base + i * stride;
        int sz = globals->client.colorSize;
        switch(globals->client.colorType)
        {
            case GL_FLOAT:         { const GLfloat*  v=(const GLfloat*)p;  WRAP(glColor4f(v[0],v[1],v[2],sz>3?v[3]:1.f)); break; }
            case GL_UNSIGNED_BYTE: { const GLubyte*  v=(const GLubyte*)p;  WRAP(glColor4f(ub2f(v[0]),ub2f(v[1]),ub2f(v[2]),sz>3?ub2f(v[3]):1.f)); break; }
            case GL_BYTE:          { const GLbyte*   v=(const GLbyte*)p;   WRAP(glColor4f(b2f(v[0]),b2f(v[1]),b2f(v[2]),sz>3?b2f(v[3]):1.f)); break; }
            case GL_SHORT:         { const GLshort*  v=(const GLshort*)p;  WRAP(glColor4f(s2f(v[0]),s2f(v[1]),s2f(v[2]),sz>3?s2f(v[3]):1.f)); break; }
            case GL_INT:           { const GLint*    v=(const GLint*)p;    WRAP(glColor4f(i2f(v[0]),i2f(v[1]),i2f(v[2]),sz>3?i2f(v[3]):1.f)); break; }
            default: break;
        }
    }
    // TexCoord (unit 0)
    {
        texcoord_state_t& ts = globals->client.texCoord[0];
        if(ts.enabled && ts.texCoordPtr)
        {
            const char* base = (const char*)ts.texCoordPtr;
            GLsizei stride = ts.texCoordStride
                ? ts.texCoordStride
                : (ts.texCoordSize * (GLsizei)GetGLTypeSize(ts.texCoordType));
            const char* p = base + i * stride;
            if(ts.texCoordType == GL_FLOAT)
            {
                const GLfloat* v = (const GLfloat*)p;
                WRAP(glTexCoord2f(v[0], ts.texCoordSize > 1 ? v[1] : 0.f));
            }
        }
    }
    // Vertex
    if(globals->client.vertexArrayEnabled && globals->client.vertexPtr)
    {
        const char* base = (const char*)globals->client.vertexPtr;
        GLsizei stride = globals->client.vertexStride
            ? globals->client.vertexStride
            : (globals->client.vertexSize * (GLsizei)GetGLTypeSize(globals->client.vertexType));
        const char* p = base + i * stride;
        int sz = globals->client.vertexSize;
        switch(globals->client.vertexType)
        {
            case GL_FLOAT:  { const GLfloat*  v=(const GLfloat*)p;  WRAP(glVertex3f(v[0],sz>1?v[1]:0.f,sz>2?v[2]:0.f)); break; }
            case 0x140A: { const GLdouble* v=(const GLdouble*)p; WRAP(glVertex3f((float)v[0],sz>1?(float)v[1]:0.f,sz>2?(float)v[2]:0.f)); break; }
            case GL_SHORT:  { const GLshort*  v=(const GLshort*)p;  WRAP(glVertex3f(v[0],sz>1?(float)v[1]:0.f,sz>2?(float)v[2]:0.f)); break; }
            case GL_INT:    { const GLint*    v=(const GLint*)p;    WRAP(glVertex3f((float)v[0],sz>1?(float)v[1]:0.f,sz>2?(float)v[2]:0.f)); break; }
            default: break;
        }
    }
}

void WRAP(glInterleavedArrays(GLenum format, GLsizei stride, const void* pointer))
{
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

static inline void SetMultiTexCoord(GLenum unit, float s, float t)
{
    int u = (int)(unit - GL_TEXTURE0);
    if(u < 0 || u >= 8) return;
    if(u == 0) globals->render.texcoord = { s, t };
    // For units > 0 we store into the per-unit slot so multi-tex FFP works
    globals->client.texCoord[u].texCoordPtr = nullptr; // mark as immediate
    // We piggyback on texCoordColor.x/y for immediate-mode coords
    globals->client.texCoord[u].texCoordColor.x = s;
    globals->client.texCoord[u].texCoordColor.y = t;
}

void WRAP(glMultiTexCoord1f(GLenum t, GLfloat s))                              { DLREC(WRAP(glMultiTexCoord1f(t,s)));         SetMultiTexCoord(t,s,0); }
void WRAP(glMultiTexCoord2f(GLenum t, GLfloat s, GLfloat r))                   { DLREC(WRAP(glMultiTexCoord2f(t,s,r)));       SetMultiTexCoord(t,s,r); }
void WRAP(glMultiTexCoord3f(GLenum t, GLfloat s, GLfloat r, GLfloat q))        { DLREC(WRAP(glMultiTexCoord3f(t,s,r,q)));     SetMultiTexCoord(t,s,r); }
void WRAP(glMultiTexCoord4f(GLenum t, GLfloat s, GLfloat r, GLfloat q, GLfloat w)){ DLREC(WRAP(glMultiTexCoord4f(t,s,r,q,w))); SetMultiTexCoord(t,s,r); }
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
    int c = coord - GL_S; if(c < 0 || c > 3) return;
    if(pname == GL_TEXTURE_GEN_MODE)
        globals->ff.texGen[c].mode = (GLenum)(int)param;
}
void WRAP(glTexGend(GLenum coord, GLenum pname, GLdouble param))   { WRAP(glTexGenf(coord,pname,(float)param)); }
void WRAP(glTexGeni(GLenum coord, GLenum pname, GLint param))      { DLREC(WRAP(glTexGeni(coord,pname,param))); int c=coord-GL_S; if(c<0||c>3)return; if(pname==GL_TEXTURE_GEN_MODE) globals->ff.texGen[c].mode=(GLenum)param; }
void WRAP(glTexGenfv(GLenum coord, GLenum pname, const GLfloat* p))
{
    DLREC(WRAP(glTexGenfv(coord,pname,p)));
    int c = coord - GL_S; if(c < 0 || c > 3) return;
    if(pname == GL_TEXTURE_GEN_MODE)      globals->ff.texGen[c].mode = (GLenum)(int)p[0];
    else if(pname == GL_OBJECT_PLANE)     memcpy(globals->ff.texGen[c].objectPlane, p, 4*sizeof(float));
    else if(pname == GL_EYE_PLANE)        memcpy(globals->ff.texGen[c].eyePlane,    p, 4*sizeof(float));
}
void WRAP(glTexGendv(GLenum coord, GLenum pname, const GLdouble* p))
{
    float fp[4] = {(float)p[0],(float)p[1],(float)p[2],(float)p[3]};
    WRAP(glTexGenfv(coord,pname,fp));
}
void WRAP(glTexGeniv(GLenum coord, GLenum pname, const GLint* p))
{
    float fp[4] = {(float)p[0],(float)p[1],(float)p[2],(float)p[3]};
    WRAP(glTexGenfv(coord,pname,fp));
}

void WRAP(glPointParameterf(GLenum pname, GLfloat param))
{
    DLREC(WRAP(glPointParameterf(pname,param)));
    switch(pname)
    {
        case 0x8128: // GL_POINT_FADE_THRESHOLD_SIZE
            globals->ff.pointFadeThreshold = param; break;
        case 0x8CA0: // GL_POINT_SPRITE_COORD_ORIGIN
            break;
        default: break;
    }
}
void WRAP(glPointParameteri(GLenum pname, GLint param))  { WRAP(glPointParameterf(pname,(float)param)); }
void WRAP(glPointParameterfv(GLenum pname, const GLfloat* p))
{
    DLREC(WRAP(glPointParameterfv(pname,p)));
    if(pname == 0x8129) // GL_POINT_DISTANCE_ATTENUATION
        memcpy(globals->ff.pointAttenuation, p, 3*sizeof(float));
    else
        WRAP(glPointParameterf(pname, p[0]));
}
void WRAP(glPointParameteriv(GLenum pname, const GLint* p))
{
    float fp[3] = {(float)p[0],(float)p[1],(float)p[2]};
    WRAP(glPointParameterfv(pname,fp));
}

void WRAP(glSecondaryColorPointer(GLint size, GLenum type, GLsizei stride, const void* ptr))
{
    // TODO: hook into fixed pipeline as secondary colour contribution
    globals->client.secondaryColorSize   = size;
    globals->client.secondaryColorType   = type;
    globals->client.secondaryColorStride = stride;
    globals->client.secondaryColorPtr    = ptr;
    globals->client.secondaryColorBuffer = globals->client.boundArrayBuffer;
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
