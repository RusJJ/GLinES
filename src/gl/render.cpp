#include "GLES.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "globals.h"
#include "glhelper.h"

void WRAP(glBegin(GLenum mode))
{
    globals->lastPrimitiveMode = mode;
    globals->render.begin = true;
    
    globals->render.vertices.clear();
    globals->render.colors.clear();
    globals->render.texcoords.clear();
    globals->render.normals.clear();
}

void WRAP(glEnd())
{
    if(!globals->render.begin) return;
    if(globals->render.vertices.empty()) return;
    
    UseFixedProgram();
    globals->render.begin = false;
    
    TransformFixedVerts();
    
    globals->render.vertices.clear();
    globals->render.colors.clear();
    globals->render.texcoords.clear();
    globals->render.normals.clear();
}

void WRAP(glColor3f(GLfloat red, GLfloat green, GLfloat blue))
{
    globals->render.color = { red, green, blue, 1.0f };
}

void WRAP(glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))
{
    globals->render.color = { red, green, blue, alpha };
}

const double __1DIV32767 = ( 1.0 / 32767.0 );
void WRAP(glColor4sv(const GLshort* v))
{
    vector4_t& clr = globals->render.color;
    clr.x = __1DIV32767 * v[0];
    clr.y = __1DIV32767 * v[1];
    clr.z = __1DIV32767 * v[2];
    clr.w = __1DIV32767 * v[3];
}

void WRAP(glTexCoord2f(GLfloat s, GLfloat t))
{
    globals->render.texcoord = { s, t };
}

void WRAP(glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz))
{
    globals->render.normal = { nx, ny, nz };
}

void WRAP(glColorMaskIndexed(GLuint framebuffer, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))
{
    glColorMaski(framebuffer, red, green, blue, alpha);
}

void WRAP(glVertex3f(GLfloat x, GLfloat y, GLfloat z))
{
    if(!globals->render.begin) return;
    globals->render.vertices.push_back({x, y, z});
    globals->render.colors.push_back(globals->render.color);
    globals->render.texcoords.push_back(globals->render.texcoord);
    globals->render.normals.push_back(globals->render.normal);
}

void WRAP(glVertex2f(GLfloat x, GLfloat y))
{
    WRAP(glVertex3f(x, y, 0.0f));
}

void WRAP(glVertex2i(GLint x, GLint y))
{
    WRAP(glVertex2f(x, y));
}

void WRAP(glBindTexture(GLenum target, GLuint texture))
{
    globals->gl.activeTexture = globals->textures[texture];
    switch(target)
    {
        case 0x0DE0: //GL_TEXTURE_1D:
        case 0x84F5: //GL_TEXTURE_RECTANGLE_ARB:
            glBindTexture(GL_TEXTURE_2D, texture);
            break;

        default:
            glBindTexture(target, texture);
            break;
    }
}

void WRAP(glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, void *indices, GLint basevertex))
{
    if(!basevertex) return glDrawRangeElements(mode, start, end, count, type, indices);
    glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
}

void WRAP(glClipPlane(GLenum plane, const GLdouble *equation))
{
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
    int index = plane - GL_CLIP_PLANE0;
    if (index >= 0 && index < 6)
    {
        globals->ff.clipPlanes[index][0] = (float)equation[0];
        globals->ff.clipPlanes[index][1] = (float)equation[1];
        globals->ff.clipPlanes[index][2] = (float)equation[2];
        globals->ff.clipPlanes[index][3] = (float)equation[3];
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
    MSG("glVertexAttribLPointer(...)");
}

void WRAP(glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z))
{
    glVertexAttrib3f(index, x, y, z);
}

// TODO: a ton of attributes...

void WRAP(glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        glDrawArrays(mode, first[i], count[i]);
    }
}

void WRAP(glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        glDrawElements(mode, count[i], type, indices[i]);
    }
}

void WRAP(glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount, const GLint* basevertex))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        glDrawElementsBaseVertex(mode, count[i], type, indices[i], basevertex[i]);
    }
}

void WRAP(glShadeModel(GLenum model))
{
    globals->ff.shadeModel = model;
}

void WRAP(glPolygonMode(GLenum face, GLenum mode))
{
    switch(mode)
    {
        case 0x1B00: globals->gl.lastPolygonMode = 1; break; // GL_POINT 0x1B00
        case 0x1B01: globals->gl.lastPolygonMode = 2; break; // GL_LINE 0x1B01
        default: globals->gl.lastPolygonMode = 0; break; // GL_FILL 0x1B02
    }
}

void WRAP(glEnableClientState(GLenum array))
{
    if(array == GL_VERTEX_ARRAY) globals->client.vertexArrayEnabled = true;
    else if(array == GL_COLOR_ARRAY) globals->client.colorArrayEnabled = true;
    else if(array == GL_TEXTURE_COORD_ARRAY) globals->client.texCoordArrayEnabled = true;
    else if(array == GL_NORMAL_ARRAY) globals->client.normalArrayEnabled = true;
}

void WRAP(glDisableClientState(GLenum array))
{
    if(array == GL_VERTEX_ARRAY) globals->client.vertexArrayEnabled = false;
    else if(array == GL_COLOR_ARRAY) globals->client.colorArrayEnabled = false;
    else if(array == GL_TEXTURE_COORD_ARRAY) globals->client.texCoordArrayEnabled = false;
    else if(array == GL_NORMAL_ARRAY) globals->client.normalArrayEnabled = false;
}

void WRAP(glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *ptr))
{
    globals->client.vertexSize = size;
    globals->client.vertexType = type;
    globals->client.vertexStride = stride;
    globals->client.vertexPtr = ptr;
}

void WRAP(glColorPointer(GLint size, GLenum type, GLsizei stride, const void *ptr))
{
    globals->client.colorSize = size;
    globals->client.colorType = type;
    globals->client.colorStride = stride;
    globals->client.colorPtr = ptr;
}

void WRAP(glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *ptr))
{
    globals->client.texCoordSize = size;
    globals->client.texCoordType = type;
    globals->client.texCoordStride = stride;
    globals->client.texCoordPtr = ptr;
}

void WRAP(glNormalPointer(GLenum type, GLsizei stride, const void *ptr))
{
    globals->client.normalType = type;
    globals->client.normalStride = stride;
    globals->client.normalPtr = ptr;
}

void WRAP(glLightf(GLenum light, GLenum pname, GLfloat param))
{
    WRAP(glLightfv(light, pname, &param));
}

void WRAP(glLightfv(GLenum light, GLenum pname, const GLfloat *params))
{
    int idx = light - GL_LIGHT0;
    if(idx < 0 || idx > 7) return;

    if(pname == 0x1200) // GL_AMBIENT
    {
        memcpy(globals->ff.lightAmbient[idx], params, 4 * sizeof(float));
    }
    else if (pname == 0x1201) // GL_DIFFUSE
    {
        memcpy(globals->ff.lightDiffuse[idx], params, 4 * sizeof(float));
    }
    else if (pname == 0x1202) // GL_SPECULAR
    {
        memcpy(globals->ff.lightSpecular[idx], params, 4 * sizeof(float));
    }
    else if (pname == 0x1203) // GL_POSITION
    {
        memcpy(globals->ff.lightPos[idx], params, 4 * sizeof(float));
    }
    else if (pname == 0x1204) // GL_SPOT_DIRECTION
    {
        memcpy(globals->ff.lightSpotDir[idx], params, 4 * sizeof(float));
    }
    else if (pname == 0x1205) // GL_SPOT_EXPONENT
    {
        globals->ff.lightSpotExponent[idx] = params[0];
    }
    else if (pname == 0x1206) // GL_SPOT_CUTOFF
    {
        globals->ff.lightSpotCutoff[idx] = params[0];
    }
    else if (pname == 0x1207) // GL_CONSTANT_ATTENUATION
    {
        globals->ff.lightAttenuation[idx][0] = params[0];
    }
    else if (pname == 0x1208) // GL_LINEAR_ATTENUATION
    {
        globals->ff.lightAttenuation[idx][1] = params[0];
    }
    else if (pname == 0x1209) // GL_QUADRATIC_ATTENUATION
    {
        globals->ff.lightAttenuation[idx][2] = params[0];
    }
}

void WRAP(glLightModelf(GLenum pname, GLfloat param))
{
    WRAP(glLightModelfv(pname, &param));
}

void WRAP(glLightModelfv(GLenum pname, const GLfloat* params))
{
    
}

void WRAP(glMaterialf(GLenum face, GLenum pname, GLfloat param))
{
    if (pname == 0x1601) // GL_SHININESS
    {
        globals->ff.matShininess = param;
    }
}

void WRAP(glMaterialfv(GLenum face, GLenum pname, const GLfloat *params))
{
    if (pname == 0x1200) // GL_AMBIENT
    {
        memcpy(globals->ff.matAmbient, params, 4 * sizeof(float));
    }
    else if (pname == 0x1201) // GL_DIFFUSE
    {
        memcpy(globals->ff.matDiffuse, params, 4 * sizeof(float));
    }
    else if (pname == 0x1202) // GL_SPECULAR
    {
        memcpy(globals->ff.matSpecular, params, 4 * sizeof(float));
    }
    else if (pname == 0x1600) // GL_EMISSION
    {
        memcpy(globals->ff.matEmission, params, 4 * sizeof(float));
    }
}

void WRAP(glFogf(GLenum pname, GLfloat param))
{
    if (pname == 0x0B62) // GL_FOG_DENSITY
    {
        globals->ff.fogDensity = param;
    }
    else if (pname == 0x0B63) // GL_FOG_START
    {
        globals->ff.fogStart = param;
    }
    else if (pname == 0x0B64) // GL_FOG_END
    {
        globals->ff.fogEnd = param;
    }
}

void WRAP(glFogfv(GLenum pname, const GLfloat *params))
{
    if (pname == 0x0B66) // GL_FOG_COLOR
    {
        memcpy(globals->ff.fogColor, params, 4 * sizeof(float));
    }
}

void WRAP(glFogi(GLenum pname, GLint param))
{
    if (pname == 0x0B65) // GL_FOG_MODE
    {
        globals->ff.fogMode = param;
    }
}
