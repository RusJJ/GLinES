#include "gl_shader_conv.h"
#include "globals.h"

static char pszShaderSource[16384]; // 16kb?
static char pszNewShaderSource[16384] =
    "// Converted VERTEX shader, GLinES\n"
    "#version 320 es\n"
	"layout(location = 0) in vec4 vPosition;  \n"
    "void main() {                            \n"
    "   gl_Position = vPosition;              \n"
    "}                                        \n";

static char pszNewShaderSource2[16384] =
    "// Converted FRAGMENT shader, GLinES\n"
    "#version 320 es                                   \n"
    "precision mediump float;                          \n"
    "out vec4 fragColor;                               \n"
    "void main() {                                     \n"
    "    fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );      \n"
    "}                                                 \n";


static char pszShaderLog[256];
void GLIN_Wrap_glCompileShader(GLuint shader)
{
    static GLsizei len;

    //glGetShaderSource(shader, sizeof(pszShaderSource), &len, pszShaderSource);

    glShaderSource(shader, 1, globals->shaders[shader]->isVertexShader ? (const GLchar**)&pszNewShaderSource : (const GLchar**)&pszNewShaderSource2, NULL);

    glCompileShader(shader);

    static GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE)
    {
        static GLint length; // Useless variable. Do not initialize it every time.
        glGetShaderInfoLog(shader, sizeof(pszShaderLog), &length, pszShaderLog);
        ERR("%s shader #%d compilation error:\n%s", globals->shaders[shader]->isVertexShader ? "Vertex" : "Fragment", shader, pszShaderLog);
    }
}

GLuint GLIN_Wrap_glCreateShader(GLenum type)
{
    GLuint nShader = glCreateShader(type);

    auto pShader = globals->shaders[nShader];
    if(pShader == NULL)
    {
        pShader = new shader_desc_t;
        globals->shaders[nShader] = pShader;
    }
    pShader->shader = nShader;
    pShader->isVertexShader = (type == GL_VERTEX_SHADER);

    DBG("glCreateShader = %d, isVertex = %d", nShader, (type == GL_VERTEX_SHADER));

    return nShader;
}