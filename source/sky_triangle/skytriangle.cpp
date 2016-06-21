
#include "skytriangle.h"

#include <cmath>
#include <iostream>
#include <string>
#include <chrono>

#include <glbinding/gl32ext/gl.h>

#include <cgutils/common.h>


using namespace gl32core;


SkyTriangle::SkyTriangle()
{
}

SkyTriangle::~SkyTriangle()
{
    glDeleteBuffers(static_cast<GLsizei>(m_vbos.size()), m_vbos.data());
    glDeleteVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    for(auto i = 0; i < m_programs.size(); ++i)
        glDeleteProgram(m_programs[i]);
    for (auto i = 0; i < m_vertexShaders.size(); ++i)
        glDeleteShader(m_vertexShaders[i]);
    for (auto i = 0; i < m_fragmentShaders.size(); ++i)
        glDeleteShader(m_fragmentShaders[i]);
    
    glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
}

void SkyTriangle::initialize()
{
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    glGenBuffers(1, m_vbos.data());

    static const float verticesScrAT[] = { -1.f, -3.f, -1.f, 1.f, 3.f, 1.f };

    glGenVertexArrays(static_cast<GLsizei>(m_vaos.size()), m_vaos.data());

    glBindVertexArray(m_vaos[0]);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(verticesScrAT), verticesScrAT, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    for (auto i = 0; i < m_programs.size(); ++i)
    {
        m_programs[i] = glCreateProgram();

        m_vertexShaders[i] = glCreateShader(GL_VERTEX_SHADER);
        m_fragmentShaders[i] = glCreateShader(GL_FRAGMENT_SHADER);

        glAttachShader(m_programs[i], m_vertexShaders[i]);
        glAttachShader(m_programs[i], m_fragmentShaders[i]);

        glBindFragDataLocation(m_programs[i], 0, "out_color");
    }

    loadShaders();


    glGenTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());

    loadTextures();
}

void SkyTriangle::cleanup()
{
}

bool SkyTriangle::loadShaders()
{
    static const auto sourceFiles = std::array<std::string, 2>{
        "data/sky_triangle/skytriangle.vert",
        "data/sky_triangle/skytriangle.frag", };

    {   static const auto i = 0;

        const auto vertexShaderSource = cgutils::textFromFile(sourceFiles[0].c_str());
        const auto vertexShaderSource_ptr = vertexShaderSource.c_str();
        if (vertexShaderSource_ptr)
            glShaderSource(m_vertexShaders[i], 1, &vertexShaderSource_ptr, 0);

        glCompileShader(m_vertexShaders[i]);
        bool success = cgutils::checkForCompilationError(m_vertexShaders[i], sourceFiles[0]);


        const auto fragmentShaderSource = cgutils::textFromFile(sourceFiles[1].c_str());
        const auto fragmentShaderSource_ptr = fragmentShaderSource.c_str();
        if (fragmentShaderSource_ptr)
            glShaderSource(m_fragmentShaders[i], 1, &fragmentShaderSource_ptr, 0);

        glCompileShader(m_fragmentShaders[i]);
        success &= cgutils::checkForCompilationError(m_fragmentShaders[i], sourceFiles[1]);

        if (!success)
            return false;

        gl::glLinkProgram(m_programs[i]);

        success &= cgutils::checkForLinkerError(m_programs[i], "skytriangle program");
        if (!success)
            return false;
    }

    loadUniformLocations();

    return true;
}

void SkyTriangle::loadUniformLocations()
{
    glUseProgram(m_programs[0]);
    m_uniformLocations[0] = glGetUniformLocation(m_programs[0], "cubemap");

    glUseProgram(0);
}

bool SkyTriangle::loadTextures()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_textures[0]);

    auto rawPX = cgutils::rawFromFile("data/sky_triangle/cube_gen_17_px.1024.1024.rgb.ub.raw");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, static_cast<GLint>(GL_RGB8), 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, rawPX.data());

    auto rawPY = cgutils::rawFromFile("data/sky_triangle/cube_gen_17_py.1024.1024.rgb.ub.raw");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, static_cast<GLint>(GL_RGB8), 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, rawPY.data());

    auto rawPZ = cgutils::rawFromFile("data/sky_triangle/cube_gen_17_pz.1024.1024.rgb.ub.raw");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, static_cast<GLint>(GL_RGB8), 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, rawPZ.data());

    auto rawNX = cgutils::rawFromFile("data/sky_triangle/cube_gen_17_nx.1024.1024.rgb.ub.raw");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, static_cast<GLint>(GL_RGB8), 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, rawNX.data());

    auto rawNY = cgutils::rawFromFile("data/sky_triangle/cube_gen_17_ny.1024.1024.rgb.ub.raw");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, static_cast<GLint>(GL_RGB8), 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, rawNY.data());

    auto rawNZ = cgutils::rawFromFile("data/sky_triangle/cube_gen_17_nz.1024.1024.rgb.ub.raw");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, static_cast<GLint>(GL_RGB8), 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, rawNZ.data());

    // configure required min/mag filter and wrap modes
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));

    return true;
}

void SkyTriangle::resize(int w, int h)
{
    m_width = w;
    m_height = h;
}

void SkyTriangle::render()
{
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);

    glUseProgram(m_programs[0]);
    glUniform1f(m_uniformLocations[0], 0);
    
    glBindVertexArray(m_vaos[0]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


void SkyTriangle::execute()
{
    render();
}
