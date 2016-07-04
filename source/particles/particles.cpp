
#include "particles.h"

#include <cmath>
#include <iostream>
#include <string>

#include <xmmintrin.h>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#pragma warning(pop)

#include <glbinding/gl32ext/gl.h>

#include <cgutils/common.h>


using namespace gl32core;


Particles::Particles()
: m_time(std::chrono::high_resolution_clock::now())
, m_num(1e+6)
, m_paused(false)
{
}

Particles::~Particles()
{
    glDeleteBuffers(static_cast<GLsizei>(m_vbos.size()), m_vbos.data());
    glDeleteVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    for(auto i = 0; i < m_programs.size(); ++i)
        glDeleteProgram(m_programs[i]);
    for (auto i = 0; i < m_vertexShaders.size(); ++i)
        glDeleteShader(m_vertexShaders[i]);
    for (auto i = 0; i < m_fragmentShaders.size(); ++i)
        glDeleteShader(m_fragmentShaders[i]);
    for (auto i = 0; i < m_geometryShaders.size(); ++i)
        glDeleteShader(m_geometryShaders[i]);
    //glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
}

void Particles::initialize()
{
    //glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
    glClearColor(0.f, 0.f, 0.f, 1.0f);

    glGenBuffers(2, m_vbos.data());

//    static const float vertices[] = { -1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f };

    glGenVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    glBindVertexArray(m_vaos[0]);
    
    //glEnableVertexAttribArray(0);
    //glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_num, m_positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_num, m_velocities.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    for (auto i = 0; i < m_programs.size(); ++i)
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
    static const auto sourceFiles = std::array<std::string, 3>{
        "data/particles/particles.vert",
        "data/particles/particles.geom",
        "data/particles/particles.frag", };

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

    m_uniformLocations[1] = glGetUniformLocation(m_programs[0], "aspect");
    glUniform1f(m_uniformLocations[1], static_cast<float>(m_width) / m_height);

    glUseProgram(0);
}

void Particles::resize(int w, int h)
{
    m_width = w;
    m_height = h;

    glUseProgram(m_programs[0]);
    glUniform1f(m_uniformLocations[1], static_cast<float>(m_width) / m_height);
    glUseProgram(0);
}

void Particles::pause()
{
    m_paused = !m_paused;
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
    const auto r = glm::sphericalRand(1.f);
    const auto e = 0.01f * std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now()).time_since_epoch().count();

    m_velocities[index] = r * glm::linearRand(0.f, 1.f) + glm::vec3(sin(e * 0.545545), 3.f + 2.f * sin(e * 0.618709f), sin(0.121031f * e));
    m_positions[index] = glm::ballRand(0.02f) + glm::vec3(0.f, 0.5f, 0.f);
}

void Particles::prepare()
{
    m_positions.resize(m_num);
    m_velocities.resize(m_num);

    for (auto i = 0u; i < m_num; ++i)
        spawn(i);

    m_time = std::chrono::high_resolution_clock::now();
}

void Particles::process()
{
    static const auto gravity = glm::vec3(0.0, -1.80665f, 0.0); // m/s²;
    static const auto friction = 0.33f;

    const auto t0 = m_time;
    m_time = std::chrono::high_resolution_clock::now();
    
    if (m_paused)
        return;

    const auto elapsed = static_cast<float>(secs(m_time - t0).count());
    const auto elapsed2 = elapsed * elapsed;

    //std::cout << elapsed << std::endl;

    __m128 sse_gravity = _mm_set_ps(gravity.x, gravity.y, gravity.z, 0.0f);
    __m128 sse_friction = _mm_set1_ps(friction);

    #pragma omp parallel for
    for (auto i = 0; i < static_cast<std::int32_t>(m_num); ++i)
    {
        __m128 sse_position = _mm_set_ps(m_positions[i].x, m_positions[i].y, m_positions[i].z, 0.0f);
        __m128 sse_velocity = _mm_set_ps(m_velocities[i].x, m_velocities[i].y, m_velocities[i].z, 0.0f);
        __m128 sse_elapsed = _mm_set1_ps(elapsed);
        __m128 sse_elapsed2 = _mm_set1_ps(elapsed2);
        __m128 sse_f = _mm_sub_ps(sse_gravity, _mm_mul_ps(sse_velocity, sse_friction));
        __m128 sse_05 = _mm_set1_ps(0.5f);

        __m128 next_position = _mm_add_ps(sse_position, _mm_add_ps(_mm_mul_ps(sse_velocity, sse_elapsed), _mm_mul_ps(sse_05, _mm_mul_ps(sse_f, sse_elapsed2))));
        __m128 next_velocity = _mm_add_ps(sse_velocity, _mm_mul_ps(sse_f, sse_elapsed));

        m_positions[i].x = reinterpret_cast<float*>(&next_position)[3];
        m_positions[i].y = reinterpret_cast<float*>(&next_position)[2];
        m_positions[i].z = reinterpret_cast<float*>(&next_position)[1];

        m_velocities[i].x = reinterpret_cast<float*>(&next_velocity)[3];
        m_velocities[i].y = reinterpret_cast<float*>(&next_velocity)[2];
        m_velocities[i].z = reinterpret_cast<float*>(&next_velocity)[1];

        //const auto f = gravity - m_velocities[i] * friction;
        //m_positions[i] = m_positions[i] + (m_velocities[i] * elapsed) + (0.5f * f * elapsed2);
        //m_velocities[i] = m_velocities[i] + (f * elapsed);

        if (m_positions[i].y >= 0.f)
            continue;

        m_positions[i].y *= -1.f;
        m_velocities[i].y *= -1.f;

        m_velocities[i] *= (1.0 - friction);
    }

    #pragma omp parallel for
    for (auto i = 0; i < static_cast<std::int32_t>(m_num); ++i)
    {
        if (glm::length(m_velocities[i]) < 0.01f)
            spawn(i);
    }
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

    auto eye = glm::vec3(glm::vec4(0.f, 2.f, 3.f, 0.f) * glm::rotate(glm::mat4(1.f), m_angle, glm::vec3(0.f, 1.f, 0.f)));
    //auto eye = glm::vec3(0.f, 1.f, 2.f);

    const auto view = glm::lookAt(eye, glm::vec3(0.f, 0.5f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    const auto projection = glm::perspective(glm::radians(30.f), static_cast<float>(m_width) / m_height, 0.5f, 8.f);

    m_transform = projection * view;

    glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(m_transform));



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

    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_num, m_positions.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_num, m_velocities.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vaos[0]);

    //auto T = glm::scale(m_transform, glm::vec3(0.01f));
    glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(m_transform));
    
    glDrawArrays(GL_POINTS, 0, m_num);
    process();

    glBindVertexArray(0);

    glUseProgram(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    //glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

}


void Particles::execute()
{
    render();
}
