
#include <glbinding/gl32core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 3.2 Core symbols

#include <GLFW/glfw3.h>

#include <chrono>


// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class SkyTriangle
{
public:
    SkyTriangle();
    ~SkyTriangle();

    void initialize();
    void cleanup();
    bool loadShaders();
    bool loadTextures();

    void resize(int w, int h);
    void resize(GLFWwindow * window);
    void render();
    void execute();

protected:
    void loadUniformLocations();

protected:
    std::array<gl::GLuint, 1> m_vbos;

    std::array<gl::GLuint, 1> m_programs;
    std::array<gl::GLuint, 1> m_vertexShaders;
    std::array<gl::GLuint, 1> m_fragmentShaders;

    std::array<gl::GLuint, 1> m_vaos;

    std::array<gl::GLuint, 1> m_textures;
    std::array<gl::GLuint, 3> m_uniformLocations;

    using msecs = std::chrono::duration<float, std::chrono::milliseconds::period>;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_time;

    int m_width;
    int m_height;
    float m_angle;
};
