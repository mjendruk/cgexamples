
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

#ifdef BUILD_WITH_AVX2
#define SIMD_COUNT 2
#else
#define SIMD_COUNT 1
#endif


// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class Particles
{
public:
    enum class ProcessingMode
    {
        CPU,
        CPU_OMP,
        CPU_OMP_SSE41,
        CPU_OMP_AVX2 
    };

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

    void setProcessing(const ProcessingMode mode);
    
    float scale();
    void setScale(float scale);

    float angle() const;
    void rotate(float angle);

protected:
    void loadUniformLocations();

    void prepare();
    void spawn(std::uint32_t index);

    float elapsed();

    void process();
    void processOMP();
    void processSSE41();
    void processAVX2();
    

protected:
    std::array<gl::GLuint, 2> m_vbos;

    std::array<gl::GLuint, 1> m_programs;
    std::array<gl::GLuint, 1> m_vertexShaders;
    std::array<gl::GLuint, 1> m_fragmentShaders;
    std::array<gl::GLuint, 1> m_geometryShaders;

    std::array<gl::GLuint, 1> m_vaos;

    //std::array<gl::GLuint, 1> m_textures;
    std::array<gl::GLuint, 3> m_uniformLocations;

    std::vector<glm::vec4, aligned_allocator<glm::vec4, SIMD_COUNT * sizeof(glm::vec4)>> m_positions;
    std::vector<glm::vec4, aligned_allocator<glm::vec4, SIMD_COUNT * sizeof(glm::vec4)>> m_velocities;


    ProcessingMode m_mode;

    std::int32_t m_num;
    float m_scale;

    bool m_paused;
    float m_angle;

    glm::mat4 m_transform;

    using secs = std::chrono::duration<float, std::chrono::seconds::period>;
    std::chrono::high_resolution_clock::time_point m_time;
    float m_elapsedSinceEpoch;

    int m_width;
    int m_height;

    bool m_bufferStorageAvaiable;
    void * m_bufferPointer;
};
