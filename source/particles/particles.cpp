
#include "particles.h"

#include <cmath>
#include <iostream>
#include <string>
#include <random>

#include <immintrin.h>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#pragma warning(pop)

#include <glbinding/gl32ext/gl.h>

#include <cgutils/common.h>


using namespace gl32core;


namespace
{


static const auto gravity = glm::vec4(0.0f, -9.80665f, 0.0f, 0.0f); // m/s^2;
static const auto friction = 0.3333f;
static const auto velocityThreshold = 0.01f;

#ifdef SYSTEM_DARWIN
#define thread_local 
#endif
thread_local std::random_device rd;
thread_local auto mt = std::mt19937{ rd() };
thread_local auto frand = std::uniform_real_distribution<float>{ -1.f, 1.f };


}


Particles::Particles()
: m_num(100000)
, m_scale(32.f)
, m_paused(false)
, m_time(std::chrono::high_resolution_clock::now())
{
}

Particles::~Particles()
{
    glDeleteBuffers(static_cast<GLsizei>(m_vbos.size()), m_vbos.data());
    glDeleteVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    for(auto i = 0ull; i < m_programs.size(); ++i)
        glDeleteProgram(m_programs[i]);
    for (auto i = 0ull; i < m_vertexShaders.size(); ++i)
        glDeleteShader(m_vertexShaders[i]);
    for (auto i = 0ull; i < m_fragmentShaders.size(); ++i)
        glDeleteShader(m_fragmentShaders[i]);
    for (auto i = 0ull; i < m_geometryShaders.size(); ++i)
        glDeleteShader(m_geometryShaders[i]);
    //glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
}

void Particles::initialize()
{
    //glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
    glClearColor(1.f, 1.f, 1.f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glGenBuffers(static_cast<GLsizei>(m_vbos.size()), m_vbos.data());

//    static const float vertices[] = { -1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f };

    glGenVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    glBindVertexArray(m_vaos[0]);

    //glEnableVertexAttribArray(0);
    //glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, m_positions.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);

    /*glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, m_velocities.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);*/

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    for (auto i = 0ull; i < m_programs.size(); ++i)
    {
        m_programs[i] = glCreateProgram();

        m_vertexShaders[i] = glCreateShader(GL_VERTEX_SHADER);
        m_geometryShaders[i] = glCreateShader(GL_GEOMETRY_SHADER);
        m_fragmentShaders[i] = glCreateShader(GL_FRAGMENT_SHADER);

        glAttachShader(m_programs[i], m_vertexShaders[i]);
        glAttachShader(m_programs[i], m_geometryShaders[i]);
        glAttachShader(m_programs[i], m_fragmentShaders[i]);

        glBindFragDataLocation(m_programs[i], 0, "out_color");
    }

    loadShaders();

    //glGenTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());


    prepare();
}

void Particles::cleanup()
{
}

bool Particles::loadShaders()
{
    static const auto sourceFiles = std::array<std::string, 3>{{
        "data/particles/particles.vert",
        "data/particles/particles.geom",
        "data/particles/particles.frag" }};

    {   static const auto i = 0;

        const auto vertexShaderSource = cgutils::textFromFile(sourceFiles[0].c_str());
        const auto vertexShaderSource_ptr = vertexShaderSource.c_str();
        if (vertexShaderSource_ptr)
            glShaderSource(m_vertexShaders[i], 1, &vertexShaderSource_ptr, 0);

        glCompileShader(m_vertexShaders[i]);
        bool success = cgutils::checkForCompilationError(m_vertexShaders[i], sourceFiles[0]);


        const auto geometryShaderSource = cgutils::textFromFile(sourceFiles[1].c_str());
        const auto geometryShaderSource_ptr = geometryShaderSource.c_str();
        if (geometryShaderSource_ptr)
            glShaderSource(m_geometryShaders[i], 1, &geometryShaderSource_ptr, 0);

        glCompileShader(m_geometryShaders[i]);
        success &= cgutils::checkForCompilationError(m_geometryShaders[i], sourceFiles[1]);


        const auto fragmentShaderSource = cgutils::textFromFile(sourceFiles[2].c_str());
        const auto fragmentShaderSource_ptr = fragmentShaderSource.c_str();
        if (fragmentShaderSource_ptr)
            glShaderSource(m_fragmentShaders[i], 1, &fragmentShaderSource_ptr, 0);

        glCompileShader(m_fragmentShaders[i]);
        success &= cgutils::checkForCompilationError(m_fragmentShaders[i], sourceFiles[2]);

        if (!success)
            return false;

        gl::glLinkProgram(m_programs[i]);

        success &= cgutils::checkForLinkerError(m_programs[i], "particles program");
        if (!success)
            return false;
    }

    loadUniformLocations();

    return true;
}

void Particles::loadUniformLocations()
{
    glUseProgram(m_programs[0]);

    m_uniformLocations[0] = glGetUniformLocation(m_programs[0], "transform");
    glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(m_transform));

    m_uniformLocations[1] = glGetUniformLocation(m_programs[0], "scale");
    glUniform2f(m_uniformLocations[1], m_scale / m_width, m_scale / m_height);

    glUseProgram(0);
}

void Particles::resize(int w, int h)
{
    m_width = w;
    m_height = h;
}

void Particles::pause()
{
    m_paused = !m_paused;
    elapsed();
}

void Particles::setProcessing(const ProcessingMode mode)
{
    m_mode = mode;
}

float Particles::scale()
{
    return m_scale;
}

void Particles::setScale(const float scale)
{
    m_scale = glm::clamp(scale, 1.f, 1024.f);
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

    const auto e = 0.01f * m_elapsedSinceEpoch;

    m_velocities[index] = r * (frand(rd) * 0.5f + 0.5f) + glm::vec4(
        2.f * sin(0.121031f * e), 4.f + 2.f * sin(e * 0.618709f), 2.f * sin(e * 0.545545f), 0.0f);

    m_positions[index] = r * 0.1f + glm::vec4(0.0f, 0.2f, 0.0f, 1.f);
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

    m_elapsedSinceEpoch = std::chrono::time_point_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now()).time_since_epoch().count();

    return static_cast<float>(secs(m_time - t0).count());
}

void Particles::process()
{
    if (m_paused)
        return;

    const auto elapsed = this->elapsed();
    const auto elapsed2 = elapsed * elapsed;

    for (auto i = 0; i < static_cast<std::int32_t>(m_num); ++i)
    {
        const auto f = gravity - m_velocities[i] * friction;

        auto & p = m_positions[i];
        auto & v = m_velocities[i];

        p = p + (v * elapsed) + (0.5f * f * elapsed2);
        v = v + (f * elapsed);

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

void Particles::processOMP()
{
    if (m_paused)
        return;

    const auto elapsed = this->elapsed();
    const auto elapsed2 = elapsed * elapsed;

    #pragma omp parallel for
    for (auto i = 0; i < static_cast<std::int32_t>(m_num); ++i)
    {
        const auto f = gravity - m_velocities[i] * friction;

        auto & p = m_positions[i];
        auto & v = m_velocities[i];

        p = p + (v * elapsed) + (0.5f * f * elapsed2);
        v = v + (f * elapsed);

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

void Particles::processSSE41()
{
    static const auto sse_gravity = _mm_load_ps(glm::value_ptr(gravity));
    static const auto sse_friction = _mm_set_ps(0.0f, friction, friction, friction);
    static const auto sse_one_minus_friction = _mm_set_ps(0.0f, 1.0f - friction, 1.0f - friction, 1.0f - friction);
    static const auto sse_05 = _mm_set_ps(0.0f, 0.5f, 0.5f, 0.5f);
    static const auto sse_1 = _mm_set_ps(0.0f, 1.0f, 1.0f, 1.0f);
    static const auto sse_0 = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);
    static const auto sse_yminus1 = _mm_set_ps(0.0f, 1.0f, -1.0f, 1.0f);
    static const auto sse_one_minus_friction_yminus1 = _mm_mul_ps(sse_one_minus_friction, sse_yminus1);

    if (m_paused)
        return;

    const auto elapsed = this->elapsed();
    const auto sse_elapsed = _mm_set_ps(0.0f, elapsed, elapsed, elapsed);
    const auto sse_elapsed2 = _mm_mul_ps(sse_elapsed, sse_elapsed);

    #pragma omp parallel for
    for (auto i = 0; i < static_cast<std::int32_t>(m_num); ++i)
    {
        auto sse_position = _mm_load_ps(glm::value_ptr(m_positions[i]));
        auto sse_velocity = _mm_load_ps(glm::value_ptr(m_velocities[i]));

        const auto sse_f = _mm_sub_ps(sse_gravity, _mm_mul_ps(sse_velocity, sse_friction));

        sse_position = _mm_add_ps(sse_position, _mm_add_ps(_mm_mul_ps(sse_velocity, sse_elapsed), _mm_mul_ps(sse_05, _mm_mul_ps(sse_f, sse_elapsed2))));
        sse_velocity = _mm_add_ps(_mm_mul_ps(sse_f, sse_elapsed), sse_velocity);

        const auto sse_compare = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_mm_cmplt_ps(sse_position, sse_0)), _MM_SHUFFLE(1, 1, 1, 1)));

        sse_position = _mm_mul_ps(sse_position, _mm_blendv_ps(sse_1, sse_yminus1, sse_compare));
        sse_velocity = _mm_mul_ps(sse_velocity, _mm_blendv_ps(sse_1, sse_one_minus_friction_yminus1, sse_compare));

        const auto sse_velocity_length = _mm_dp_ps(sse_velocity, sse_velocity, 0x78);
        sse_position = _mm_blend_ps(sse_position, sse_velocity_length, 0x8);

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

void Particles::processAVX2()
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

    if (m_paused)
        return;

    const auto elapsed = this->elapsed();
    const auto avx_elapsed = _mm256_set_ps(0.0f, elapsed, elapsed, elapsed, 0.0f, elapsed, elapsed, elapsed);
    const auto avx_elapsed2 = _mm256_mul_ps(avx_elapsed, avx_elapsed);

    #pragma omp parallel for
    for (auto i = 0; i < static_cast<std::int32_t>(m_num)/2; ++i)
    {
        auto avx_position = _mm256_load_ps(glm::value_ptr(m_positions[2*i]));
        auto avx_velocity = _mm256_load_ps(glm::value_ptr(m_velocities[2*i]));

        const auto avx_f = _mm256_sub_ps(avx_gravity, _mm256_mul_ps(avx_velocity, avx_friction));

        avx_position = _mm256_add_ps(avx_position, _mm256_add_ps(_mm256_mul_ps(avx_velocity, avx_elapsed), _mm256_mul_ps(avx_05, _mm256_mul_ps(avx_f, avx_elapsed2))));
        avx_velocity = _mm256_add_ps(_mm256_mul_ps(avx_f, avx_elapsed), avx_velocity);

        const auto avx_compare = _mm256_permute_ps(_mm256_cmp_ps(avx_position, avx_0, 1), _MM_SHUFFLE(1, 1, 1, 1));

        avx_position = _mm256_mul_ps(avx_position, _mm256_blendv_ps(avx_1, avx_yminus1, avx_compare));
        avx_velocity = _mm256_mul_ps(avx_velocity, _mm256_blendv_ps(avx_1, avx_one_minus_friction_yminus1, avx_compare));

        const auto avx_velocity_length = _mm256_dp_ps(avx_velocity, avx_velocity, 0x78);
        avx_position = _mm256_blend_ps(avx_position, avx_velocity_length, 0x88);

        _mm256_store_ps(glm::value_ptr(m_positions[2*i]), avx_position);
        _mm256_store_ps(glm::value_ptr(m_velocities[2*i]), avx_velocity);
    }

    #pragma omp parallel for
    for (auto i = 0; i < m_num; ++i)
    {
        if (m_positions[i].w < velocityThreshold)
            spawn(i);
    }
#endif
}

void Particles::render()
{
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, m_textures[0]);

    glUseProgram(m_programs[0]);
//    glUniform1f(m_uniformLocations[0], 0);

    // setup view

    auto eye = glm::vec3(glm::vec4(0.f, 1.f, 3.f, 0.f) * glm::rotate(glm::mat4(1.f), m_angle, glm::vec3(0.f, 1.f, 0.f)));
    //auto eye = glm::vec3(0.f, 1.f, 2.f);

    const auto view = glm::lookAt(eye, glm::vec3(0.f, 0.5f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    const auto projection = glm::perspective(glm::radians(30.f), static_cast<float>(m_width) / m_height, 0.1f, 8.f);

    m_transform = projection * view;

    glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(m_transform));
    glUniform2f(m_uniformLocations[1], m_scale / m_width, m_scale / m_height);



    // draw v0

    //glBindVertexArray(m_vaos[0]);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //glBindVertexArray(0);

    //glUseProgram(0);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, 0);


    // draw v1

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glBindVertexArray(m_vaos[0]);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //glBindVertexArray(0);

    //glUseProgram(0);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, 0);
    //glDisable(GL_BLEND);


    // draw v2

    //process();

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glBindVertexArray(m_vaos[0]);

    //for(const auto & p : m_positions)
    //{
    //    auto T = glm::translate(m_transform, p);
    //    T = glm::scale(T, glm::vec3(0.01f));

    //    glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(T));
    //    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //}
    //glBindVertexArray(0);

    //glUseProgram(0);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, 0);
    //glDisable(GL_BLEND);


    // draw v3

    //glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);

    ////glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, m_positions.data(), GL_STATIC_DRAW);
    ////glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * m_num, m_positions.data());
    ////glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);

    ////glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, m_positions.data(), GL_STATIC_DRAW);
    //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * m_num, m_positions3.data());
    ////glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    ////glBindBuffer(GL_ARRAY_BUFFER, m_vbos[1]);
    ////glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_num, m_velocities.data(), GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);


    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vaos[0]);

    //auto T = glm::scale(m_transform, glm::vec3(0.01f));
    //glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(m_transform));

    glDrawArrays(GL_POINTS, 0, m_num);

    glBindVertexArray(0);

    glUseProgram(0);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, 0);
    //glDisable(GL_BLEND);
    //glDisable(GL_DEPTH_TEST);

    switch (m_mode)
    {
    case Particles::ProcessingMode::CPU:
        process();
        break;
    case Particles::ProcessingMode::CPU_OMP:
        processOMP();
        break;
    case Particles::ProcessingMode::CPU_OMP_SSE41:
        processSSE41();
        break;
    case Particles::ProcessingMode::CPU_OMP_AVX2:
        processAVX2();
        break;

    default:
        break;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * m_num, m_positions.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void Particles::execute()
{
    render();
}
