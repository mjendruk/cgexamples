
#include <glbinding/gl32core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 3.2 Core symbols


// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class ScrAT
{
public:
    ScrAT();
    ~ScrAT();

    void initialize();
    bool loadShaders();
    bool loadTextures();

    void resize(int w, int h);
    void render();
    void execute();

protected:
    void loadUniformLocations();

protected:
    gl::GLuint m_vbo;
    gl::GLuint m_colors;
    gl::GLuint m_program;
    gl::GLuint m_vertexShader;
    gl::GLuint m_fragmentShader;
    gl::GLuint m_vao;
    gl::GLuint m_fbo;
    std::array<gl::GLuint, 1> m_textures;

    //gl::GLuint m_moepLocation;

    int m_width;
    int m_height;
};
