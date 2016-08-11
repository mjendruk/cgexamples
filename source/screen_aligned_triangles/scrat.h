
#include <glbinding/gl32core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 3.2 Core symbols

#include <chrono>


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
    void render();
    void execute();

    void resetAC();
    void switchVAO();

    void incrementReplaySpeed();
    void decrementReplaySpeed();

protected:
    void loadUniformLocations();

    std::uint64_t record(bool benchmark);
    void replay();

    void updateThreshold();

protected:
    std::array<gl::GLuint, 2> m_vbos;

    std::array<gl::GLuint, 3> m_programs;
    std::array<gl::GLuint, 3> m_vertexShaders;
    std::array<gl::GLuint, 2> m_geometryShaders;
    std::array<gl::GLuint, 2> m_fragmentShaders;

    std::array<gl::GLuint, 3> m_vaos;
    gl::GLuint m_fbo;
    std::array<gl::GLuint, 1> m_textures;
    std::array<gl::GLuint, 4> m_uniformLocations;

    gl::GLuint m_query;
    gl::GLuint m_acbuffer;   

    bool m_recorded;
    std::array<float, 3> m_threshold; // { last, current, max }
    int m_vaoMode;

    using msecs = std::chrono::duration<float, std::chrono::milliseconds::period>;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_time;
    std::uint32_t m_timeDurationMagnitude;

    int m_width;
    int m_height;
};
