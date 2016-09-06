
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
        CPU_OMP_AVX2,
        GPU_ComputeShaders
    };

    enum class DrawingMode
    {
        None,
        BuiltInPoints,
        CustomQuads,
        ShadedQuads,
        Fluid
    };

public:
    Particles();
    ~Particles();

    void initialize();
    void cleanup();
    bool loadShaders();
    void benchmark();

    void resize(int w, int h);
    void render();
    void execute();

    void pause();

    void setProcessing(const ProcessingMode mode);
    void setDrawing(const DrawingMode mode);
    
    float scale();
    void setScale(float scale);

    float angle() const;
    void rotate(float angle);

protected:
    bool loadShader(gl::GLuint & shader, const std::string & sourceFile) const;
    void loadUniformLocations();
    void setupTextures();
    void setupShaders();
    void resizeTextures();

    void prepare();
    void spawn(std::uint32_t index);

    float elapsed();

    void process(float elapsed);
    void processOMP(float elapsed);
    void processSSE41(float elapsed);
    void processAVX2(float elapsed);
    void processComputeShaders(float elapsed);
    
    void setupBuffer(bool mapBuffer, bool bufferStorageAvailable);


protected:
    std::array<gl::GLuint, 3> m_vbos;

    std::array<gl::GLuint, 6> m_programs;
    std::array<gl::GLuint, 3> m_vertexShaders;
    std::array<gl::GLuint, 2> m_geometryShaders;
    std::array<gl::GLuint, 5> m_fragmentShaders;
    std::array<gl::GLuint, 1> m_computeShaders;

    std::array<gl::GLuint, 2> m_fbo;
    std::array<gl::GLuint, 2> m_textures;
    //std::array<gl::GLuint, 2> m_renderBuffers;

    std::array<gl::GLuint, 2> m_vaos;

    std::array<gl::GLuint, 15> m_uniformLocations;

    std::vector<glm::vec4, aligned_allocator<glm::vec4, SIMD_COUNT * sizeof(glm::vec4)>> m_positions;
    std::vector<glm::vec4, aligned_allocator<glm::vec4, SIMD_COUNT * sizeof(glm::vec4)>> m_velocities;


    ProcessingMode m_processingMode;
    DrawingMode m_drawMode;

    std::int32_t m_num;
    float m_radius;

    bool m_measure;
    size_t m_measureCount;
    std::chrono::high_resolution_clock::time_point m_measureTime0;

    bool m_paused;
    float m_angle;

    using secs = std::chrono::duration<float, std::chrono::seconds::period>;
    std::chrono::high_resolution_clock::time_point m_time;
    std::chrono::high_resolution_clock::time_point m_time0; // time at start of program to reduce float resolution error

    float m_elapsedSinceEpoch;

    int m_width;
    int m_height;

    bool m_bufferStorageAvailable;
    void * m_bufferPointer;

    bool m_computeShadersAvailable;
};
