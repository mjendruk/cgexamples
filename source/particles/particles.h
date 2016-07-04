
#include <glbinding/gl32core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 3.2 Core symbols

#include <chrono>
#include <array>
#include <vector>

#include "allocator.h"

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#pragma warning(pop)


// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class Particles
{
public:
    Particles();
    ~Particles();

    void initialize();
    void cleanup();
    bool loadShaders();

    void resize(int w, int h);
    void render();
    void execute();

    void pause();
    
    float angle() const;
    void rotate(float angle);

protected:
    void loadUniformLocations();

    void prepare();
    void process();
    void spawn(std::uint32_t index);

protected:
    std::array<gl::GLuint, 2> m_vbos;

    std::array<gl::GLuint, 1> m_programs;
    std::array<gl::GLuint, 1> m_vertexShaders;
    std::array<gl::GLuint, 1> m_fragmentShaders;
    std::array<gl::GLuint, 1> m_geometryShaders;

    std::array<gl::GLuint, 1> m_vaos;

    //std::array<gl::GLuint, 1> m_textures;
    std::array<gl::GLuint, 3> m_uniformLocations;

    std::vector<glm::vec4, aligned_allocator<glm::vec4, 16>> m_positions;
    std::vector<glm::vec4, aligned_allocator<glm::vec4, 16>> m_velocities;

    std::int32_t m_num;
    bool m_paused;
    float m_angle;

    glm::mat4 m_transform;

    using secs = std::chrono::duration<double, std::chrono::seconds::period>;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_time;

    int m_width;
    int m_height;
};
