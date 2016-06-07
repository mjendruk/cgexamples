
#include <glbinding/gl32core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 3.2 Core symbols


// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class ScrAT
{
public:
    ScrAT();
    ~ScrAT();

    void initialize();
    void cleanup();
    bool loadShaders();
    bool loadTextures();

    void resize(int w, int h);
    void record();
    void replay();
    void render();
    void execute();

    void resetAC();
    void switchVAO();

protected:
    void loadUniformLocations();

protected:
    std::array<gl::GLuint, 2> m_vbos;
    gl::GLuint m_colors;
    
    std::array<gl::GLuint, 2> m_programs;
    std::array<gl::GLuint, 2> m_vertexShaders;
    std::array<gl::GLuint, 2> m_fragmentShaders;

    std::array<gl::GLuint, 2> m_vaos;
    gl::GLuint m_fbo;
    std::array<gl::GLuint, 1> m_textures;
    std::array<gl::GLuint, 2> m_uniformLocations;

    gl::GLuint m_acbuffer;   

    bool m_recorded;
    float m_threshold;
    int m_vaoMode;

    int m_width;
    int m_height;
};
