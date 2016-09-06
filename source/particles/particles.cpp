
#include "particles.h"

#include <cmath>
#include <iostream>
#include <string>
#include <random>
#include <chrono>

#include <immintrin.h>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#pragma warning(pop)

#include <glbinding/gl32ext/gl.h>
#include <glbinding/ContextInfo.h>

#include <cgutils/common.h>


using namespace gl32core;


namespace
{

    const auto gravity = glm::vec4(0.0f, -9.80665f, 0.0f, 0.0f); // m/s^2;
    const auto friction = 0.3333f;
    const auto velocityThreshold = 0.01f;

#ifdef SYSTEM_DARWIN
#define thread_local 
#endif
    thread_local std::random_device rd;
    thread_local auto mt = std::mt19937{ rd() };
    thread_local auto frand = std::uniform_real_distribution<float>{ -1.f, 1.f };


    int getComputeMaxInvocations()
    {
        auto maxInvocations = 0;
        glGetIntegerv(gl::GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);

        return maxInvocations;
    }


    glm::ivec3 getMaxComputeWorkGroupCounts()
    {
        auto counts = glm::ivec3{};
        glGetIntegeri_v(gl::GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &counts.x);
        glGetIntegeri_v(gl::GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &counts.y);
        glGetIntegeri_v(gl::GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &counts.z);

        return counts;
    }


}


Particles::Particles()
: m_processingMode(ProcessingMode::CPU_OMP_AVX2) // initialization is faulty when beginning with GPU
, m_drawMode(DrawingMode::Fluid)
, m_num(100000)
, m_radius(128.f)
, m_paused(false)
, m_time(std::chrono::high_resolution_clock::now())
, m_time0(std::chrono::high_resolution_clock::now())
, m_bufferStorageAvailable(false)
, m_bufferPointer(nullptr)
, m_computeShadersAvailable(false)
, m_measure(false)
, m_measureCount(0)
{
}

Particles::~Particles()
{
    glDeleteBuffers(static_cast<GLsizei>(m_vbos.size()), m_vbos.data());
    glDeleteVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    for (auto i = 0ull; i < m_programs.size(); ++i)
    {
        glDeleteProgram(m_programs[i]);
    }
    for (auto i = 0ull; i < m_vertexShaders.size(); ++i)
    {
        glDeleteShader(m_vertexShaders[i]);
    }
    for (auto i = 0ull; i < m_fragmentShaders.size(); ++i)
    {
        glDeleteShader(m_fragmentShaders[i]);
    }
    for (auto i = 0ull; i < m_geometryShaders.size(); ++i)
    {
        glDeleteShader(m_geometryShaders[i]);
    }
    for (auto i = 0ull; i < m_computeShaders.size(); ++i)
    {
        glDeleteShader(m_computeShaders[i]);
    }

    //glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
}

void Particles::setupBuffer(const bool mapBuffer, const bool bufferStorageAvailable)
{
    assert(m_vaos[0]);

    if (m_vbos[0])
    {
        if (m_bufferPointer)
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            m_bufferPointer = nullptr;
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glDeleteBuffers(1, &m_vbos[0]);
    }
    glGenBuffers(1, &m_vbos[0]);

    glBindVertexArray(m_vaos[0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);

    if (mapBuffer && bufferStorageAvailable)
    {
        glBufferStorage(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, nullptr,
            gl32ext::GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | gl32ext::GL_MAP_PERSISTENT_BIT);
        m_bufferPointer = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * m_num,
            GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT | gl32ext::GL_MAP_PERSISTENT_BIT);
    }
    else if (bufferStorageAvailable)
    {
        glBufferStorage(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, nullptr,
            gl32ext::GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    }
    else
    {
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, nullptr, GL_STREAM_DRAW);
    }

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Particles::initialize()
{
    // can map and unmap buffer api be used
    //m_bufferStorageAvailable = glbinding::ContextInfo::supported({ GLextension::GL_ARB_buffer_storage });
    // can compute shader 
    m_computeShadersAvailable = glbinding::ContextInfo::supported({ GLextension::GL_ARB_compute_shader });

    // setup common state

    glClearColor(1.f, 1.f, 1.f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    // setup resources : VAO

    glGenBuffers(m_vbos.size(), m_vbos.data());
    glGenVertexArrays(m_vaos.size(), m_vaos.data());

    setupBuffer(m_processingMode != ProcessingMode::GPU_ComputeShaders, m_bufferStorageAvailable);

   
    glGenTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
    //glGenRenderbuffers(static_cast<GLsizei>(m_renderBuffers.size()), m_renderBuffers.data());
    glGenFramebuffers(static_cast<GLsizei>(m_fbo.size()), m_fbo.data());
    
    static const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };

    // setup render target for fluid pass
   
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    //glBindTexture(GL_TEXTURE_2D, 0);
  
    //glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffers[0]);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
    ////glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[0]);
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textures[0], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_textures[0], 0);
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_renderBuffers[0]);
    glDrawBuffers(1, drawBuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // setup render target for fluid pass (temp for sparated blur)

    glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    //glBindTexture(GL_TEXTURE_2D, 0);

    //glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffers[1]);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
    ////glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[1]);
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textures[1], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_textures[1], 0);
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_renderBuffers[1]);
    glDrawBuffers(1, drawBuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // setup screen aligned triangle 

    static const float verticesScrAT[] = { -1.f, -3.f, -1.f, 1.f, 3.f, 1.f };

    glBindVertexArray(m_vaos[1]);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(verticesScrAT), verticesScrAT, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    setupShaders();
    setupTextures();

    prepare();
}

void Particles::cleanup()
{
}

void Particles::setupTextures()
{
}

void Particles::setupShaders()
{
    // create handles

    for (auto i = 0ull; i < m_programs.size(); ++i)
    {
        m_programs[i] = glCreateProgram();
    }

    for (auto i = 0ull; i < m_vertexShaders.size(); ++i)
    {
        m_vertexShaders[i] = glCreateShader(GL_VERTEX_SHADER);
    }

    for (auto i = 0ull; i < m_geometryShaders.size(); ++i)
    {
        m_geometryShaders[i] = glCreateShader(GL_GEOMETRY_SHADER);
    }

    for (auto i = 0ull; i < m_fragmentShaders.size(); ++i)
    {
        m_fragmentShaders[i] = glCreateShader(GL_FRAGMENT_SHADER);
    }

    if (m_computeShadersAvailable)
    {
        for (auto i = 0ull; i < m_computeShaders.size(); ++i)
        {
            m_computeShaders[i] = glCreateShader(gl32ext::GL_COMPUTE_SHADER);
        }
    }

    glAttachShader(m_programs[0], m_vertexShaders[0]);
    glAttachShader(m_programs[0], m_fragmentShaders[0]);


    glAttachShader(m_programs[1], m_vertexShaders[0]);
    glAttachShader(m_programs[1], m_geometryShaders[0]);
    glAttachShader(m_programs[1], m_fragmentShaders[1]);

    glAttachShader(m_programs[2], m_vertexShaders[0]);
    glAttachShader(m_programs[2], m_geometryShaders[0]);
    glAttachShader(m_programs[2], m_fragmentShaders[2]);

    if (m_computeShadersAvailable)
    {
        glAttachShader(m_programs[3], m_computeShaders[0]);
    }

    glAttachShader(m_programs[4], m_vertexShaders[1]);
    glAttachShader(m_programs[4], m_geometryShaders[1]);
    glAttachShader(m_programs[4], m_fragmentShaders[3]);

    glAttachShader(m_programs[5], m_vertexShaders[2]);
    glAttachShader(m_programs[5], m_fragmentShaders[4]);

    glBindFragDataLocation(m_programs[0], 0, "out_color");
    glBindFragDataLocation(m_programs[1], 0, "out_color");
    glBindFragDataLocation(m_programs[2], 0, "out_color");
    glBindFragDataLocation(m_programs[4], 0, "out_color");
    glBindFragDataLocation(m_programs[5], 0, "out_color");

    loadShaders();
}

bool Particles::loadShader(GLuint & shader, const std::string & sourceFile) const
{
    const auto source = cgutils::textFromFile(sourceFile.c_str());
    const auto source_ptr = source.c_str();
    if (source_ptr)
        glShaderSource(shader, 1, &source_ptr, 0);

    glCompileShader(shader);
    return cgutils::checkForCompilationError(shader, sourceFile);
}

bool Particles::loadShaders()
{
    auto success = true;

    success &= loadShader(m_vertexShaders[0],   "data/particles/particles.vert");
    success &= loadShader(m_geometryShaders[0], "data/particles/particles.geom");
    success &= loadShader(m_fragmentShaders[0], "data/particles/particles.frag");
    success &= loadShader(m_fragmentShaders[1], "data/particles/particles-circle.frag");
    success &= loadShader(m_fragmentShaders[2], "data/particles/particles-sphere.frag");

    if (m_computeShadersAvailable)
    {
        success &= loadShader(m_computeShaders[0], "data/particles/particles.comp");
    }

    success &= loadShader(m_vertexShaders[1],   "data/particles/particles-fluid.vert");
    success &= loadShader(m_geometryShaders[1], "data/particles/particles-fluid.geom");
    success &= loadShader(m_fragmentShaders[3], "data/particles/particles-fluid.frag");

    success &= loadShader(m_vertexShaders[2], "data/particles/fluid-pass.vert");
    success &= loadShader(m_fragmentShaders[4], "data/particles/fluid-pass.frag");

    glLinkProgram(m_programs[0]);

    success &= cgutils::checkForLinkerError(m_programs[0], "particles program");

    glLinkProgram(m_programs[1]);
    success &= cgutils::checkForLinkerError(m_programs[1], "particles circle program");

    glLinkProgram(m_programs[2]);
    success &= cgutils::checkForLinkerError(m_programs[2], "particles sphere program");

    if (m_computeShadersAvailable)
    {
        glLinkProgram(m_programs[3]);
        success &= cgutils::checkForLinkerError(m_programs[3], "particles movement program");
    }

    glLinkProgram(m_programs[4]);
    success &= cgutils::checkForLinkerError(m_programs[4], "particles fluid program");

    glLinkProgram(m_programs[5]);
    success &= cgutils::checkForLinkerError(m_programs[5], "fluid pass program");

    if (!success)
        return false;

    loadUniformLocations();

    return true;

}

void Particles::loadUniformLocations()
{
    glUseProgram(m_programs[0]);
    m_uniformLocations[0]  = glGetUniformLocation(m_programs[0], "transform");
    m_uniformLocations[1]  = glGetUniformLocation(m_programs[0], "scale");

    glUseProgram(m_programs[1]);
    m_uniformLocations[2]  = glGetUniformLocation(m_programs[1], "transform");
    m_uniformLocations[3]  = glGetUniformLocation(m_programs[1], "scale");

    glUseProgram(m_programs[2]);
    m_uniformLocations[4]  = glGetUniformLocation(m_programs[2], "transform");
    m_uniformLocations[5]  = glGetUniformLocation(m_programs[2], "scale");

    glUseProgram(m_programs[4]); // fluid
    m_uniformLocations[6]  = glGetUniformLocation(m_programs[4], "view");
    m_uniformLocations[7]  = glGetUniformLocation(m_programs[4], "projection");
    m_uniformLocations[8]  = glGetUniformLocation(m_programs[4], "ndcInverse");
    m_uniformLocations[9]  = glGetUniformLocation(m_programs[4], "scale");
    m_uniformLocations[10] = glGetUniformLocation(m_programs[4], "normal");
    m_uniformLocations[11] = glGetUniformLocation(m_programs[4], "eye");

    glUseProgram(m_programs[5]); // fluid-pass
    m_uniformLocations[12] = glGetUniformLocation(m_programs[5], "source");
    m_uniformLocations[13] = glGetUniformLocation(m_programs[5], "advance");
    m_uniformLocations[14] = glGetUniformLocation(m_programs[5], "ndcInverse");

    glUseProgram(0);
}

void Particles::resize(int w, int h)
{
    m_width = w;
    m_height = h;
    resizeTextures();
}

void Particles::resizeTextures()
{
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffers[0]);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
    //glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffers[1]);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
    //glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Particles::pause()
{
    m_paused = !m_paused;
    elapsed();
}

void Particles::benchmark()
{
    m_measure = true;

    m_measureCount = 0;
    m_measureTime0 = m_time;
}

void Particles::setProcessing(const ProcessingMode mode)
{
    // switch from GPU to CPU -> copy back position and velocity information
    if (m_processingMode == ProcessingMode::GPU_ComputeShaders
        && mode != ProcessingMode::GPU_ComputeShaders)
    {
        glBindBuffer(GL_COPY_READ_BUFFER, m_vbos[0]);
        glGetBufferSubData(GL_COPY_READ_BUFFER, 0, sizeof(glm::vec4) * m_num, m_positions.data());

        glBindBuffer(GL_COPY_READ_BUFFER, m_vbos[1]);
        glGetBufferSubData(GL_COPY_READ_BUFFER, 0, sizeof(glm::vec4) * m_num, m_velocities.data());

        glBindBuffer(GL_COPY_READ_BUFFER, 0);

        if (m_bufferStorageAvailable)
        {
            setupBuffer(true, m_bufferStorageAvailable);
        }
    }

    // switch from CPU to GPU -> copy velocity information
    // (positions of the last frame are expected to be on gpu anyway)
    if (m_processingMode != ProcessingMode::GPU_ComputeShaders
        && mode == ProcessingMode::GPU_ComputeShaders)
    {
        glBindBuffer(GL_COPY_WRITE_BUFFER, m_vbos[1]);
        glBufferData(GL_COPY_WRITE_BUFFER, sizeof(glm::vec4) * m_num, m_velocities.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

        if (m_bufferStorageAvailable)
        {
            setupBuffer(false, m_bufferStorageAvailable);

            glBindBuffer(GL_COPY_WRITE_BUFFER, m_vbos[0]);
            glBufferSubData(GL_COPY_WRITE_BUFFER, 0, sizeof(glm::vec4) * m_num, m_positions.data());
            glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        }
    }

    m_processingMode = mode;
}

void Particles::setDrawing(const DrawingMode mode)
{
    m_drawMode = mode;
}

float Particles::scale()
{
    return m_radius;
}

void Particles::setScale(const float scale)
{
    m_radius = glm::clamp(scale, 1.f, 1024.f);
}

float Particles::angle() const
{
    return m_angle;
}

void Particles::rotate(const float angle)
{
    m_angle = angle;
}

void Particles::spawn(const std::uint32_t index)
{
    const auto r = glm::normalize(glm::vec4(frand(rd), frand(rd), frand(rd), 0.0f));

    const auto e = m_elapsedSinceEpoch * 10.f;

    m_velocities[index] = r * (frand(rd) * 0.5f + 0.5f) + glm::vec4(
        4.f * sin(0.121031f * e), 4.f + (frand(rd)) * sin(e * 0.618709f), 4.f * sin(e * 0.545545f), 0.0f);

    m_positions[index] = r * 0.1f + glm::vec4(0.0f, 0.2f, 0.0f, 1.0f);
}

void Particles::prepare()
{
    m_positions.resize(m_num);
    m_velocities.resize(m_num);

#pragma omp parallel for
    for (auto i = 0; i < m_num; ++i)
        spawn(i);

    elapsed();
}

float Particles::elapsed()
{
    const auto t0 = m_time;
    m_time = std::chrono::high_resolution_clock::now();

    m_elapsedSinceEpoch = secs(m_time - m_time0).count();
    const auto elapsedSinceLast = secs(m_time - t0).count();

    return elapsedSinceLast;
}

void Particles::process(float elapsed)
{
    const auto elapsed2 = elapsed * elapsed;

    for (auto i = 0; i < static_cast<std::int32_t>(m_num); ++i)
    {
        auto & p = m_positions[i];
        auto & v = m_velocities[i];

        const auto f = gravity - v * friction;

        p += (v * elapsed) + (0.5f * f * elapsed2);
        v += (f * elapsed);

        if (p.y < 0.f)
        {
            p.y *= -1.f;
            v.y *= -1.f;

            v *= 1.0 - friction;
        }

        p.w = glm::dot(glm::vec3(v), glm::vec3(v));
    }

    for (auto i = 0; i < m_num; ++i)
    {
        if (m_positions[i].w < velocityThreshold)
            spawn(i);
    }
}

void Particles::processOMP(float elapsed)
{
    const auto elapsed2 = elapsed * elapsed;

#pragma omp parallel for
    for (auto i = 0; i < static_cast<std::int32_t>(m_num); ++i)
    {
        auto & p = m_positions[i];
        auto & v = m_velocities[i];

        const auto f = gravity - v * friction;

        p += (v * elapsed) + (0.5f * f * elapsed2);
        v += (f * elapsed);

        if (p.y < 0.f)
        {
            p.y *= -1.f;
            v.y *= -1.f;

            v *= 1.0 - friction;
        }

        p.w = glm::dot(glm::vec3(v), glm::vec3(v));
    }

#pragma omp parallel for
    for (auto i = 0; i < m_num; ++i)
    {
        if (m_positions[i].w < velocityThreshold)
            spawn(i);
    }
}

void Particles::processSSE41(float elapsed)
{
    static const auto sse_gravity = _mm_load_ps(glm::value_ptr(gravity));
    static const auto sse_friction = _mm_set_ps(0.0f, friction, friction, friction);
    static const auto sse_one_minus_friction = _mm_set_ps(0.0f, 1.0f - friction, 1.0f - friction, 1.0f - friction);
    static const auto sse_05 = _mm_set_ps(0.0f, 0.5f, 0.5f, 0.5f);
    static const auto sse_1 = _mm_set_ps(0.0f, 1.0f, 1.0f, 1.0f);
    static const auto sse_0 = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);
    static const auto sse_yminus1 = _mm_set_ps(0.0f, 1.0f, -1.0f, 1.0f);
    static const auto sse_one_minus_friction_yminus1 = _mm_mul_ps(sse_one_minus_friction, sse_yminus1);

    const auto sse_elapsed = _mm_set_ps(0.0f, elapsed, elapsed, elapsed);
    const auto sse_elapsed2 = _mm_mul_ps(sse_elapsed, sse_elapsed);
    const auto sse_elapsed2_5 = _mm_mul_ps(sse_05, sse_elapsed2);

#pragma omp parallel for
    for (auto i = 0; i < static_cast<std::int32_t>(m_num); ++i)
    {
        auto sse_position = _mm_load_ps(glm::value_ptr(m_positions[i]));
        auto sse_velocity = _mm_load_ps(glm::value_ptr(m_velocities[i]));

        const auto sse_f = _mm_sub_ps(sse_gravity, _mm_mul_ps(sse_velocity, sse_friction));

        sse_position = _mm_add_ps(sse_position, _mm_add_ps(_mm_mul_ps(sse_velocity, sse_elapsed), _mm_mul_ps(sse_f, sse_elapsed2_5)));
        sse_velocity = _mm_add_ps(_mm_mul_ps(sse_f, sse_elapsed), sse_velocity);

        const auto sse_compare = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_mm_cmplt_ps(sse_position, sse_0)), _MM_SHUFFLE(1, 1, 1, 1)));

        sse_position = _mm_mul_ps(sse_position, _mm_blendv_ps(sse_1, sse_yminus1, sse_compare));
        sse_velocity = _mm_mul_ps(sse_velocity, _mm_blendv_ps(sse_1, sse_one_minus_friction_yminus1, sse_compare));

        sse_position = _mm_blend_ps(sse_position, _mm_dp_ps(sse_velocity, sse_velocity, 0x78), 0x8);

        _mm_store_ps(glm::value_ptr(m_positions[i]), sse_position);
        _mm_store_ps(glm::value_ptr(m_velocities[i]), sse_velocity);
    }

#pragma omp parallel for
    for (auto i = 0; i < m_num; ++i)
    {
        if (m_positions[i].w < velocityThreshold)
            spawn(i);
    }
}

void Particles::processAVX2(float elapsed)
{
#ifdef BUILD_WITH_AVX2
    static const auto avx_gravity = _mm256_set_ps(gravity.w, gravity.z, gravity.y, gravity.x, gravity.w, gravity.z, gravity.y, gravity.x);
    static const auto avx_friction = _mm256_set_ps(0.0f, friction, friction, friction, 0.0f, friction, friction, friction);
    static const auto avx_one_minus_friction = _mm256_set_ps(0.0f, 1.0f - friction, 1.0f - friction, 1.0f - friction, 0.0f, 1.0f - friction, 1.0f - friction, 1.0f - friction);
    static const auto avx_05 = _mm256_set_ps(0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f);
    static const auto avx_1 = _mm256_set_ps(0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    static const auto avx_0 = _mm256_set_ps(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    static const auto avx_yminus1 = _mm256_set_ps(0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
    static const auto avx_one_minus_friction_yminus1 = _mm256_mul_ps(avx_one_minus_friction, avx_yminus1);

    const auto avx_elapsed = _mm256_set_ps(0.0f, elapsed, elapsed, elapsed, 0.0f, elapsed, elapsed, elapsed);
    const auto avx_elapsed2 = _mm256_mul_ps(avx_elapsed, avx_elapsed);
    const auto avx_elapsed2_5 = _mm256_mul_ps(avx_05, avx_elapsed2);

#pragma omp parallel for
    for (auto i = 0; i < static_cast<std::int32_t>(m_num) / 2; ++i)
    {
        auto avx_position = _mm256_load_ps(glm::value_ptr(m_positions[2 * i]));
        auto avx_velocity = _mm256_load_ps(glm::value_ptr(m_velocities[2 * i]));

        //const auto avx_f = _mm256_sub_ps(avx_gravity, _mm256_mul_ps(avx_velocity, avx_friction));
        const auto avx_f = _mm256_fnmadd_ps(avx_velocity, avx_friction, avx_gravity); // FMA4

                                                                                      //avx_position = _mm256_add_ps(avx_position, _mm256_add_ps(_mm256_mul_ps(avx_velocity, avx_elapsed), _mm256_mul_ps(avx_f, avx_elapsed2_5)));
        avx_position = _mm256_add_ps(_mm256_mul_ps(avx_velocity, avx_elapsed), _mm256_fmadd_ps(avx_f, avx_elapsed2_5, avx_position)); // FMA4
                                                                                                                                      //avx_velocity = _mm256_add_ps(_mm256_mul_ps(avx_f, avx_elapsed), avx_velocity);
        avx_velocity = _mm256_fmadd_ps(avx_f, avx_elapsed, avx_velocity); // FMA4

        const auto avx_compare = _mm256_permute_ps(_mm256_cmp_ps(avx_position, avx_0, 1), _MM_SHUFFLE(1, 1, 1, 1));

        avx_position = _mm256_mul_ps(avx_position, _mm256_blendv_ps(avx_1, avx_yminus1, avx_compare));
        avx_velocity = _mm256_mul_ps(avx_velocity, _mm256_blendv_ps(avx_1, avx_one_minus_friction_yminus1, avx_compare));

        avx_position = _mm256_blend_ps(avx_position, _mm256_dp_ps(avx_velocity, avx_velocity, 0x78), 0x88);

        _mm256_store_ps(glm::value_ptr(m_positions[2 * i]), avx_position);
        _mm256_store_ps(glm::value_ptr(m_velocities[2 * i]), avx_velocity);
    }

#pragma omp parallel for
    for (auto i = 0; i < m_num; ++i)
    {
        if (m_positions[i].w < velocityThreshold)
            spawn(i);
    }
#endif
}

void Particles::processComputeShaders(float elapsed)
{
    static const int max_invocations = getComputeMaxInvocations();
    static const glm::ivec3 max_count = getMaxComputeWorkGroupCounts();

    const auto elapsed2 = elapsed * elapsed;

    const auto groups = static_cast<int>(ceil(static_cast<float>(m_num) / static_cast<float>(64)));

    const auto workGroupSize = glm::ivec3(groups, 1, 1);

    glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, m_vbos[0]);
    glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, m_vbos[1]);

    glUseProgram(m_programs[3]);

    glUniform1f(0, elapsed);
    glUniform1f(1, elapsed2);
    glUniform1f(2, m_elapsedSinceEpoch);
    //glUniform1f(3, friction);
    //glUniform4fv(4, 1, glm::value_ptr(gravity));
    //glUniform1f(5, velocityThreshold);
    gl32ext::glDispatchCompute(workGroupSize.x, workGroupSize.y, workGroupSize.z);
    glUseProgram(0);

    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 1, 0);
}

void Particles::render()
{
    // measurement

    static const auto measureTargetCount = 1000;
    if (m_measure)
    {
        ++m_measureCount;
    }

    if (m_measure && m_measureCount == measureTargetCount)
    {
        const auto measureEnd = m_time;
        const auto time = (std::chrono::duration_cast<std::chrono::milliseconds>(measureEnd - m_measureTime0).count() * 1e-3f);

        std::cout << static_cast<float>(measureTargetCount) / time << " frames per second (" << time << "ms per frame)" << std::endl;
        m_measure = false;
    }

    // render

    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto eye = glm::vec3(glm::vec4(0.f, 1.f, 3.f, 0.f) * glm::rotate(glm::mat4(1.f), m_angle, glm::vec3(0.f, 1.f, 0.f)));
    const auto center = glm::vec3(0.f, 0.5f, 0.f);

    const auto view = glm::lookAt(eye, center, glm::vec3(0.f, 1.f, 0.f));
    const auto projection = glm::perspective(glm::radians(30.f), static_cast<float>(m_width) / m_height, 0.1f, 8.f);


    switch (m_drawMode)
    {
    case Particles::DrawingMode::None:
        break;
    case Particles::DrawingMode::Fluid:
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[0]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(m_programs[4]);

            glUniformMatrix4fv(m_uniformLocations[6], 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(m_uniformLocations[7], 1, GL_FALSE, glm::value_ptr(projection));
            const auto ndcInverse = glm::inverse(projection * view);
            glUniformMatrix4fv(m_uniformLocations[8], 1, GL_FALSE, glm::value_ptr(ndcInverse));
            const auto normal = glm::mat3(glm::inverse(view));
            glUniformMatrix3fv(m_uniformLocations[10], 1, GL_FALSE, glm::value_ptr(normal));
            const auto eye2 = glm::normalize(center - eye);
            glUniform3fv(m_uniformLocations[11], 1, glm::value_ptr(eye2));
            glUniform4f(m_uniformLocations[9], 1.f / m_width, 1.f / m_height, m_radius * 0.0007f, static_cast<float>(m_width) / m_height);

            glBindVertexArray(m_vaos[0]);
            glDrawArrays(GL_POINTS, 0, m_num);
            glBindVertexArray(0);


            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(m_vaos[1]);

            glActiveTexture(GL_TEXTURE0);
            glUseProgram(m_programs[5]);


            glUniformMatrix4fv(m_uniformLocations[14], 1, GL_FALSE, glm::value_ptr(ndcInverse));

            //glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[1]);
            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //glBindTexture(GL_TEXTURE_2D, m_textures[0]);

            //glUniform1i(m_uniformLocations[12], 0);
            //glUniform2f(m_uniformLocations[13], 1.f, 0.f);

            //glDrawArrays(GL_TRIANGLES, 0, 3);


            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindTexture(GL_TEXTURE_2D, m_textures[0]);

            glUniform1i(m_uniformLocations[12], 0);
            glUniform2f(m_uniformLocations[13], 0.f, 1.f);

            glDrawArrays(GL_TRIANGLES, 0, 3);


            glBindVertexArray(0);
            glUseProgram(0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindVertexArray(0);
        }
        break;

    default:
        {
            const auto programIndex = static_cast<size_t>(m_drawMode) - 1;
            const auto uniformLocationOffset = programIndex * 2;

            glUseProgram(m_programs[programIndex]);

            // setup view

            auto eye = glm::vec3(glm::vec4(0.f, 1.f, 3.f, 0.f) * glm::rotate(glm::mat4(1.f), m_angle, glm::vec3(0.f, 1.f, 0.f)));
            //auto eye = glm::vec3(0.f, 1.f, 2.f);

            const auto view = glm::lookAt(eye, glm::vec3(0.f, 0.5f, 0.f), glm::vec3(0.f, 1.f, 0.f));
            const auto projection = glm::perspective(glm::radians(30.f), static_cast<float>(m_width) / m_height, 0.1f, 8.f);

            const auto transform = projection * view;

            glUniformMatrix4fv(m_uniformLocations[uniformLocationOffset + 0], 1, GL_FALSE, glm::value_ptr(transform));
            glUniform3f(m_uniformLocations[uniformLocationOffset + 1], 1.f / m_width, 1.f / m_height, m_radius);

            glBindVertexArray(m_vaos[0]);

            if (m_drawMode == DrawingMode::BuiltInPoints)
                glPointSize(m_radius * 0.5f * glm::sqrt(glm::pi<float>()));

            glDrawArrays(GL_POINTS, 0, m_num);

            glBindVertexArray(0);

            glUseProgram(0);
        }
        break;
    }

    if (m_paused)
        return;

    const auto e = elapsed();

    auto maxElapsed = 0.016f;
    const auto numIterations = m_measure ? 1 : glm::max(1, glm::min(8, static_cast<int>(e / maxElapsed)));

    const auto e2 = e / numIterations;

    for (auto i = 0; i < numIterations; ++i)
    {
        switch (m_processingMode)
        {
        case Particles::ProcessingMode::CPU:
            process(e2);
            break;
        case Particles::ProcessingMode::CPU_OMP:
            processOMP(e2);
            break;
        case Particles::ProcessingMode::CPU_OMP_SSE41:
            processSSE41(e2);
            break;
        case Particles::ProcessingMode::CPU_OMP_AVX2:
            processAVX2(e2);
            break;
        case Particles::ProcessingMode::GPU_ComputeShaders:
            processComputeShaders(e2);
            break;

        default:
            break;
        }
    }

    if (m_processingMode == ProcessingMode::GPU_ComputeShaders)
        return;

    if (m_bufferStorageAvailable)
    {
        assert(nullptr != m_bufferPointer);

        std::memcpy(m_bufferPointer, m_positions.data(), sizeof(glm::vec4) * m_num);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
        glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * m_num);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, m_positions.data(), GL_STREAM_DRAW);
        //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * m_num, m_positions.data()); // sub data is slower
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}


void Particles::execute()
{
    render();
}
