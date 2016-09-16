
#include "scene.h"

#include <cmath>
#include <iostream>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include <glbinding/gl32ext/gl.h>

#include <cgutils/common.h>

#include "icosahedron.h"


using namespace gl32core;

Scene::Scene()
{

}

Scene::~Scene()
{
    for(auto i = 0; i < m_programs.size(); ++i)
        glDeleteProgram(m_programs[i]);
    for (auto i = 0; i < m_vertexShaders.size(); ++i)
        glDeleteShader(m_vertexShaders[i]);
    for (auto i = 0; i < m_fragmentShaders.size(); ++i)
        glDeleteShader(m_fragmentShaders[i]);
}

void Scene::initialize()
{
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);

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

	m_object = std::make_unique<Icosahedron>();
}

bool Scene::loadShaders()
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
        auto success = cgutils::checkForCompilationError(m_vertexShaders[i], sourceFiles[0]);


        const auto fragmentShaderSource = cgutils::textFromFile(sourceFiles[1].c_str());
        const auto fragmentShaderSource_ptr = fragmentShaderSource.c_str();
        if (fragmentShaderSource_ptr)
            glShaderSource(m_fragmentShaders[i], 1, &fragmentShaderSource_ptr, 0);

        glCompileShader(m_fragmentShaders[i]);
        success &= cgutils::checkForCompilationError(m_fragmentShaders[i], sourceFiles[1]);

        if (!success)
            return false;

        gl::glLinkProgram(m_programs[i]);

        success &= cgutils::checkForLinkerError(m_programs[i], "Scene program");
        if (!success)
            return false;
    }

    loadUniformLocations();

    return true;
}

void Scene::loadUniformLocations()
{
    glUseProgram(m_programs[0]);
    m_uniformLocations[0] = glGetUniformLocation(m_programs[0], "modelView");
    m_uniformLocations[1] = glGetUniformLocation(m_programs[0], "projection");
    glUseProgram(0);
}

void Scene::resize(int w, int h)
{
    m_width = w;
    m_height = h;
}

void Scene::render(const glm::mat4 & view, const glm::mat4 & projection)
{
    glUseProgram(m_programs[0]);

	const auto modelView = view * glm::translate(glm::scale(glm::mat4(), glm::vec3(0.1f)), glm::vec3(0.0f, 0.0f, 0.0f));

    glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(modelView));
    glUniformMatrix4fv(m_uniformLocations[1], 1, GL_FALSE, glm::value_ptr(projection));

    // draw

	m_object->draw();

    glUseProgram(0);
}
