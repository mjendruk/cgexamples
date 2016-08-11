
#include "scrat.h"

#include <cmath>
#include <iostream>
#include <string>

#include <glbinding/gl32ext/gl.h>

#include <cgutils/common.h>


using namespace gl32core;


ScrAT::ScrAT()
: m_recorded(false)
, m_vaoMode(0)
, m_timeDurationMagnitude(3u)
{
}

ScrAT::~ScrAT()
{
    glDeleteBuffers(static_cast<GLsizei>(m_vbos.size()), m_vbos.data());
    glDeleteVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    for(auto i = 0; i < m_programs.size(); ++i)
        glDeleteProgram(m_programs[i]);
    for (auto i = 0; i < m_vertexShaders.size(); ++i)
        glDeleteShader(m_vertexShaders[i]);
    for (auto i = 0; i < m_fragmentShaders.size(); ++i)
        glDeleteShader(m_fragmentShaders[i]);
    
    glDeleteFramebuffers(1, &m_fbo);

    glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());

    glDeleteBuffers(1, &m_acbuffer);
    glDeleteQueries(1, &m_query);
}

void ScrAT::initialize()
{
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    glGenBuffers(2, m_vbos.data());

    static const float verticesScrAT[] = { -1.f, -3.f, -1.f, 1.f, 3.f, 1.f };
    static const float verticesScrAQ[] = { -1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f };

    glGenVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    glBindVertexArray(m_vaos[0]);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(verticesScrAT), verticesScrAT, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    glBindVertexArray(0);

    glBindVertexArray(m_vaos[1]);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(verticesScrAQ), verticesScrAQ, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    glBindVertexArray(0);

    glBindVertexArray(m_vaos[2]);
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    for (auto i = 0; i < 2; ++i)
    {
        m_programs[i] = glCreateProgram();

        m_vertexShaders[i] = glCreateShader(GL_VERTEX_SHADER);
        m_fragmentShaders[i] = glCreateShader(GL_FRAGMENT_SHADER);

        glAttachShader(m_programs[i], m_vertexShaders[i]);
        glAttachShader(m_programs[i], m_fragmentShaders[i]);

        glBindFragDataLocation(m_programs[i], 0, "out_color");
    }

    {
        m_programs[2] = glCreateProgram();

        m_vertexShaders[2] = glCreateShader(GL_VERTEX_SHADER);
        m_geometryShaders[0] = glCreateShader(GL_GEOMETRY_SHADER);

        glAttachShader(m_programs[2], m_vertexShaders[2]);
        glAttachShader(m_programs[2], m_geometryShaders[0]);
        glAttachShader(m_programs[2], m_fragmentShaders[0]);

        glBindFragDataLocation(m_programs[2], 0, "out_color");
    }

    loadShaders();


    glGenTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());

    // Fragment Index Render Target

    glBindTexture(GL_TEXTURE_2D, m_textures[0]);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_R32F), m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));


    //loadTextures();

    // configure framebuffer

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textures[0], 0);

    static const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    glGenBuffers(1, &m_acbuffer);
    glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, m_acbuffer);
    glBufferData(gl32ext::GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0);

    // setup time measurement

    glGenQueries(1, &m_query);
}

void ScrAT::cleanup()
{
}

bool ScrAT::loadShaders()
{
    static const auto sourceFiles = std::array<std::string, 6>{
        "data/screen_aligned_triangles/record.vert",
        "data/screen_aligned_triangles/record-empty.vert",
        "data/screen_aligned_triangles/record.geom",
        "data/screen_aligned_triangles/record.frag",
        "data/screen_aligned_triangles/replay.vert",
        "data/screen_aligned_triangles/replay.frag"  };

    {
        const auto vertexShaderSource = cgutils::textFromFile(sourceFiles[0].c_str());
        const auto vertexShaderSource_ptr = vertexShaderSource.c_str();
        if (vertexShaderSource_ptr)
            glShaderSource(m_vertexShaders[0], 1, &vertexShaderSource_ptr, 0);

        glCompileShader(m_vertexShaders[0]);
        bool success = cgutils::checkForCompilationError(m_vertexShaders[0], sourceFiles[0]);


        const auto fragmentShaderSource = cgutils::textFromFile(sourceFiles[3].c_str());
        const auto fragmentShaderSource_ptr = fragmentShaderSource.c_str();
        if (fragmentShaderSource_ptr)
            glShaderSource(m_fragmentShaders[0], 1, &fragmentShaderSource_ptr, 0);

        glCompileShader(m_fragmentShaders[0]);
        success &= cgutils::checkForCompilationError(m_fragmentShaders[0], sourceFiles[3]);

        if (!success)
            return false;

        gl::glLinkProgram(m_programs[0]);

        success &= cgutils::checkForLinkerError(m_programs[0], "record program");
        if (!success)
            return false;
    }

    {
        const auto vertexShaderSource = cgutils::textFromFile(sourceFiles[1].c_str());
        const auto vertexShaderSource_ptr = vertexShaderSource.c_str();
        if (vertexShaderSource_ptr)
            glShaderSource(m_vertexShaders[2], 1, &vertexShaderSource_ptr, 0);

        glCompileShader(m_vertexShaders[2]);
        bool success = cgutils::checkForCompilationError(m_vertexShaders[2], sourceFiles[1]);


        const auto geometryShaderSource = cgutils::textFromFile(sourceFiles[2].c_str());
        const auto geometryShaderSource_ptr = geometryShaderSource.c_str();
        if (geometryShaderSource_ptr)
            glShaderSource(m_geometryShaders[0], 1, &geometryShaderSource_ptr, 0);

        glCompileShader(m_geometryShaders[0]);
        success &= cgutils::checkForCompilationError(m_geometryShaders[0], sourceFiles[3]);


        /*const auto fragmentShaderSource = cgutils::textFromFile(sourceFiles[3].c_str());
        const auto fragmentShaderSource_ptr = fragmentShaderSource.c_str();
        if (fragmentShaderSource_ptr)
            glShaderSource(m_fragmentShaders[0], 1, &fragmentShaderSource_ptr, 0);

        glCompileShader(m_fragmentShaders[0]);
        success &= cgutils::checkForCompilationError(m_fragmentShaders[0], sourceFiles[3]);*/

        if (!success)
            return false;

        gl::glLinkProgram(m_programs[2]);

        success &= cgutils::checkForLinkerError(m_programs[2], "record program");
        if (!success)
            return false;
    }

    {
        const auto vertexShaderSource = cgutils::textFromFile(sourceFiles[4].c_str());
        const auto vertexShaderSource_ptr = vertexShaderSource.c_str();
        if (vertexShaderSource_ptr)
            glShaderSource(m_vertexShaders[1], 1, &vertexShaderSource_ptr, 0);

        glCompileShader(m_vertexShaders[1]);
        bool success = cgutils::checkForCompilationError(m_vertexShaders[1], sourceFiles[4]);


        const auto fragmentShaderSource = cgutils::textFromFile(sourceFiles[5].c_str());
        const auto fragmentShaderSource_ptr = fragmentShaderSource.c_str();
        if (fragmentShaderSource_ptr)
            glShaderSource(m_fragmentShaders[1], 1, &fragmentShaderSource_ptr, 0);

        glCompileShader(m_fragmentShaders[1]);
        success &= cgutils::checkForCompilationError(m_fragmentShaders[1], sourceFiles[5]);

        if (!success)
            return false;

        gl::glLinkProgram(m_programs[1]);

        success &= cgutils::checkForLinkerError(m_programs[1], "replay program");
        if (!success)
            return false;
    }


    loadUniformLocations();

    return true;
}

void ScrAT::loadUniformLocations()
{
    //glUseProgram(m_programs[0]);
    m_uniformLocations[2] = glGetUniformLocation(m_programs[0], "benchmark");

    //glUseProgram(m_programs[1]);
    m_uniformLocations[0] = glGetUniformLocation(m_programs[1], "fragmentIndex");
    m_uniformLocations[1] = glGetUniformLocation(m_programs[1], "threshold");

    //glUseProgram(m_programs[2]);
    m_uniformLocations[3] = glGetUniformLocation(m_programs[2], "benchmark");

    //glUseProgram(0);
}

bool ScrAT::loadTextures()
{
    //glBindTexture(GL_TEXTURE_2D, m_textures[0]);

    //auto raw = cgutils::rawFromFile("ScrAT/moep.raw");
    //glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA8), 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw.data());

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));

    //glGenerateMipmap(GL_TEXTURE_2D);

    return true;
}

void ScrAT::resize(int w, int h)
{
    m_width = w;
    m_height = h;

    m_recorded = false;

    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_R32F), m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::uint64_t ScrAT::record(const bool benchmark)
{
    glViewport(0, 0, m_width, m_height);

    // clear record buffer

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClear(GL_COLOR_BUFFER_BIT);

    static const GLfloat color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, color);

    // reset atomic counter

    glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, m_acbuffer);

    //auto counter = static_cast<GLuint *>(glMapBufferRange(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
    //    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT));
    //memset(counter, 0, sizeof(GLuint));
    //glUnmapBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER);

    const auto counter = 0u;
    glBufferSubData(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &counter);
    glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0);

    glBindBufferBase(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, m_acbuffer);
    
    // draw

    auto elapsed = std::uint64_t{ 0 };

    if (m_vaoMode == 4)
    {
        glUseProgram(m_programs[2]);
        glUniform1i(m_uniformLocations[4], static_cast<GLint>(benchmark));
    }
    else
    {
        glUseProgram(m_programs[0]);
        glUniform1i(m_uniformLocations[2], static_cast<GLint>(benchmark));
    }

    if(benchmark)
        glBeginQuery(gl::GL_TIME_ELAPSED, m_query);

    switch(m_vaoMode)
    {
    case 0:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_vaos[1]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawArrays(GL_TRIANGLES, 1, 3);
        break;
    case 1:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_vaos[1]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        break;
    case 2:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_vaos[0]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        break;
    case 3:
        glPolygonMode(GL_FRONT_AND_BACK, gl::GL_FILL_RECTANGLE_NV);
        glBindVertexArray(m_vaos[1]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        break;
    case 4:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(m_vaos[2]);
        glDrawArrays(GL_POINTS, 0, 1);
        break;
    }

    if (benchmark)
    {
        glEndQuery(gl::GL_TIME_ELAPSED);       

        auto done = 0;
        while (!done)
            glGetQueryObjectiv(m_query, GL_QUERY_RESULT_AVAILABLE, &done);

        //glFinish();
        glGetQueryObjectui64v(m_query, GL_QUERY_RESULT, &elapsed);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    glBindBufferBase(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, 0);  

    if (!benchmark)
    {
        glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, m_acbuffer);

        auto data = 0u;
        gl32ext::glMemoryBarrier(gl32ext::GL_ATOMIC_COUNTER_BARRIER_BIT);
        glGetBufferSubData(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &data);

        m_threshold[2] = static_cast<float>(data);

        glBindBuffer(gl32ext::GL_ATOMIC_COUNTER_BUFFER, 0);
    }

    return elapsed;
}

void ScrAT::replay()
{
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);

    glUseProgram(m_programs[1]);
    glUniform1f(m_uniformLocations[1], m_threshold[1]);

    // use screen aligned triangle here
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(m_vaos[0]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ScrAT::render()
{
    if (!m_recorded)
    {
        std::cout << "benchmarking ... ";

        auto elapsed = std::uint64_t{ 0 };
        for(auto i = 0; i < 1000; ++i)
            elapsed += record(true); // benchmark

        static const auto modes = std::array<std::string, 5>{
            "(0) two triangles, two draw calls :         ", 
            "(1) two triangles, single draw call (quad): ", 
            "(2) single triangle, single draw call :     ",
            "(3) fill rectangle ext, single draw call:   ",
            "(4) AVC, single draw call:                  " };
        std::cout << modes[m_vaoMode] <<  cgutils::humanTimeDuration(elapsed / 1000) << std::endl;

        record(false);
        m_recorded = true;

        m_threshold[0] = 0;
        m_time = std::chrono::high_resolution_clock::now();
    }

    replay();
    updateThreshold();
}

void ScrAT::updateThreshold()
{
    m_threshold[1] = m_threshold[0] + 0.1f * powf(10.f, static_cast<float>(m_timeDurationMagnitude))
        * msecs(std::chrono::high_resolution_clock::now() - m_time).count();
}

void ScrAT::execute()
{
    render();
}

void ScrAT::resetAC()
{
    m_recorded = false;
}

void ScrAT::switchVAO()
{
    m_recorded = false;
    m_vaoMode = (++m_vaoMode) % 5;
}

void ScrAT::incrementReplaySpeed()
{
    updateThreshold();
    
    m_threshold[0] = m_threshold[1];   
    m_time = std::chrono::high_resolution_clock::now();

    ++m_timeDurationMagnitude;
}

void ScrAT::decrementReplaySpeed()
{
    if (m_timeDurationMagnitude == 0)
        return;

    updateThreshold();

    m_threshold[0] = m_threshold[1];
    m_time = std::chrono::high_resolution_clock::now();

    --m_timeDurationMagnitude;
}
