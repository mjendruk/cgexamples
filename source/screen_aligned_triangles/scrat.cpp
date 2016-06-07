
#include "scrat.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <glbinding/gl32ext/gl.h>

#include <cgutils/common.h>


using namespace gl32core;


ScrAT::ScrAT()
{
}

ScrAT::~ScrAT()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_colors);
    glDeleteProgram(m_program);
    glDeleteShader(m_vertexShader);
    glDeleteShader(m_fragmentShader);
    glDeleteVertexArrays(1, &m_vao);
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
}

void ScrAT::initialize()
{
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);


    float vertices[] = { -1.f, -1.f, -1.f, 1.f, 1.f, 1.f };

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    m_program = glCreateProgram();

    m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
    m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glAttachShader(m_program, m_vertexShader);
    glAttachShader(m_program, m_fragmentShader);

    loadShaders();


    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
    glEnableVertexAttribArray(0);


    glGenTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
    loadTextures();


    glBindFragDataLocation(m_program, 0, "out_color");
}

bool ScrAT::loadShaders()
{
    const auto vertexShaderSource = cgutils::textFromFile("data/screen_aligned_triangles/scrat.vert");
    const auto vertexShaderSource_ptr = vertexShaderSource.c_str();
    if(vertexShaderSource_ptr)
        glShaderSource(m_vertexShader, 1, &vertexShaderSource_ptr, 0);

    glCompileShader(m_vertexShader);
    bool success = cgutils::checkForCompilationError(m_vertexShader, "vertex shader");


    const auto fragmentShaderSource = cgutils::textFromFile("data/screen_aligned_triangles/scrat.frag");
    const auto fragmentShaderSource_ptr = fragmentShaderSource.c_str();
    if(fragmentShaderSource_ptr)
        glShaderSource(m_fragmentShader, 1, &fragmentShaderSource_ptr, 0);

    glCompileShader(m_fragmentShader);
    success &= cgutils::checkForCompilationError(m_fragmentShader, "fragment shader");


    if (!success)
        return false;

    glLinkProgram(m_program);

    success &= cgutils::checkForLinkerError(m_program, "program");
    if (!success)
        return false;


    loadUniformLocations();

    return true;
}

void ScrAT::loadUniformLocations()
{
    glUseProgram(m_program);

    // m_moepLocation = glGetUniformLocation(m_program, "moep");

    glUseProgram(0);
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
    m_width  = w;
    m_height = h;
}

void ScrAT::render()
{
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(m_vao);

    glUseProgram(m_program);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, m_textures[0]);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR_MIPMAP_LINEAR));
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR_MIPMAP_LINEAR));

    //glTexParameterf(GL_TEXTURE_2D, gl32ext::GL_TEXTURE_MAX_ANISOTROPY_EXT, std::pow(2.f, i));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

    //glDisable(GL_BLEND);

    glUseProgram(0);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
}

void ScrAT::execute()
{
    render();
}
