
#include "scene.h"

#include <cmath>
#include <iostream>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#pragma warning(pop)

#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>

#include <cgutils/common.h>

#include "icosahedron.h"


using namespace gl;

namespace
{

std::array<glm::vec3, 28u> cubeVertices()
{
    static const auto vertices = std::array<glm::vec3, 8u>{{
        glm::vec3(-.5f, -.5f, -.5f),
        glm::vec3(-.5f, -.5f, .5f),
        glm::vec3(-.5f, .5f, -.5f),
        glm::vec3(-.5f, .5f, .5f),
        glm::vec3(.5f, -.5f, -.5f),
        glm::vec3(.5f, -.5f, .5f),
        glm::vec3(.5f, .5f, -.5f),
        glm::vec3(.5f, .5f, .5f)}};

    static const auto normals = std::array<glm::vec3, 7u>{{
        glm::vec3(-1.f, 0.f, 0.f),
        glm::vec3(1.f, 0.f, 0.f),
        glm::vec3(0.f,-1.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(0.f, 0.f,-1.f),
        glm::vec3(0.f, 0.f, 1.f),
        glm::vec3(0.f, 0.f, 0.f)}}; // dummy

    // use an interleaved array
    return std::array<glm::vec3, 28u>{{
        vertices[7], normals[6],
        vertices[3], normals[6],
        vertices[5], normals[5],
        vertices[1], normals[5],
        vertices[0], normals[2],
        vertices[3], normals[0],
        vertices[2], normals[0],
        vertices[7], normals[3],
        vertices[6], normals[3],
        vertices[5], normals[1],
        vertices[4], normals[1],
        vertices[0], normals[2],
        vertices[6], normals[4],
        vertices[2], normals[4]}};
}

}

Scene::Scene()
{

}

Scene::~Scene()
{
    glDeleteBuffers(m_vbos.size(), m_vbos.data());
    glDeleteVertexArrays(m_vaos.size(), m_vaos.data());

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

    initCubeVertexArray();

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

    m_object = std::make_unique<Icosahedron>(1);
}

void Scene::initCubeVertexArray()
{
    const auto vertices = cubeVertices();

    glGenVertexArrays(m_vaos.size(), m_vaos.data());
    glBindVertexArray(m_vaos[0]);

    glGenBuffers(m_vbos.size(), m_vbos.data());
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribBinding(0, 0);
    glBindVertexBuffer(0, m_vbos[0], 0, sizeof(glm::vec3) * 2);
    glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribBinding(1, 1);
    glBindVertexBuffer(1, m_vbos[0], sizeof(glm::vec3), sizeof(glm::vec3) * 2);
    glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

bool Scene::loadShaders()
{
    static const auto sourceFiles = std::array<std::string, 2>{
        "data/stereoscopic_rendering/icosahedron.vert",
        "data/stereoscopic_rendering/icosahedron.frag", };

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

        glLinkProgram(m_programs[i]);

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

void Scene::render(const glm::mat4 & view, const glm::mat4 & projection)
{
    glUseProgram(m_programs[0]);

    glUniformMatrix4fv(m_uniformLocations[1], 1, GL_FALSE, glm::value_ptr(projection));

    // draw

	static const auto icosahedronTranslations = std::array<glm::vec3, 4>{ {
		glm::vec3(-4.0f, -1.0f, 0.0f),
		glm::vec3(4.0f, 0.5f, 0.0f),
		glm::vec3(-1.0f, -1.8f, -3.5f),
		glm::vec3(2.0f, -1.0f, 2.0f)}};

	for (auto i = 0; i < icosahedronTranslations.size(); ++i)
	{
		auto model = glm::translate(icosahedronTranslations[i]);
		glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(view * model));

		m_object->draw();
	}

	static const auto cubeTranslations = std::array<glm::vec3, 2>{ {
		glm::vec3(0.0f, 0.0f, 0.0),
		glm::vec3(4.0f, -1.0f, 0.0f)}};

	static const auto cubeScalings = std::array<glm::vec3, 2>{ {
		glm::vec3(20.0f, 6.0f, 16.0),
		glm::vec3(1.0f, 1.0f, 1.0f)}};

	for (auto i = 0; i < cubeTranslations.size(); ++i)
	{
		auto model = glm::translate(cubeTranslations[i]) * glm::scale(cubeScalings[i]);
		glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(view * model));

		glBindVertexArray(m_vaos[0]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
		glBindVertexArray(0);
	}

    glUseProgram(0);
}
