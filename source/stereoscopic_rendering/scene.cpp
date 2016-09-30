
#include "scene.h"

#include <memory>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#pragma warning(pop)

#include <glbinding/gl41core/enum.h>
#include <glbinding/gl41core/functions.h>

#include <cgutils/common.h>

#include "icosahedron.h"
#include "make_unique.h"


using namespace gl41core;

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

std::array<glm::mat4, 6u> chairMatrices()
{
    static const auto chair = std::array<glm::vec3, 12>{{
        glm::vec3(-0.32f, 0.5f, -0.32f), glm::vec3(0.32f, 0.55f, 0.32f), // Chair Set
        glm::vec3(-0.3f, 0.0f, -0.3f), glm::vec3(-0.24f, 1.0f, -0.24f), // Chair Leg 1
        glm::vec3(-0.3f, 0.5f, 0.3f), glm::vec3(-0.24f, 0.0f, 0.24f), // Chair Leg 2
        glm::vec3(0.3f, 0.0f, 0.3f), glm::vec3(0.24f, 0.5f, 0.24f), // Chair Leg 2
        glm::vec3(0.3f, 1.0f, -0.3f), glm::vec3(0.24f, 0.0f, -0.24f), // Chair Leg 2
        glm::vec3(-0.29f, 0.97f, -0.26f), glm::vec3(0.29f, 0.92f, -0.29f)}}; // Chair Back high bar

    auto matrices = std::array<glm::mat4, 6u>();

    for (auto i = 0u; i < matrices.size(); ++i)
    {
        const auto j = i * 2;
        const auto translation = (chair[j] + chair[j + 1]) * 0.5f;
        const auto scale = (chair[j + 1] - chair[j]);

        matrices[i] = glm::translate(translation) * glm::scale(scale);
    }

    return matrices;
}

std::array<glm::mat4, 5u> tableMatrices()
{
    static const auto table = std::array<glm::vec3, 10>{ {
        glm::vec3(-0.65f, 0.97f, -0.45f), glm::vec3(0.65f, 0.92f, 0.45f),
        glm::vec3(-0.6f, 0.97f, -0.4f), glm::vec3(-0.55f, 0.0f, -0.35f),
        glm::vec3(0.55f, 0.97f, -0.35f), glm::vec3(0.6f, 0.0f, -0.4f),
        glm::vec3(-0.6f, 0.97f, 0.35f), glm::vec3(-0.55f, 0.0f, 0.4f),
        glm::vec3(0.55f, 0.97f, 0.35f), glm::vec3(0.6f, 0.0f, 0.4f)}};

    auto matrices = std::array<glm::mat4, 5u>();

    for (auto i = 0u; i < matrices.size(); ++i)
    {
        const auto j = i * 2;
        const auto translation = (table[j] + table[j + 1]) * 0.5f;
        const auto scale = (table[j + 1] - table[j]);

        matrices[i] = glm::translate(translation) * glm::scale(scale);
    }

    return matrices;
}

}

Scene::Scene()
{

}

Scene::~Scene()
{
    glDeleteBuffers(m_vbos.size(), m_vbos.data());
    glDeleteVertexArrays(m_vaos.size(), m_vaos.data());

    for(auto i = 0u; i < m_programs.size(); ++i)
        glDeleteProgram(m_programs[i]);
    for (auto i = 0u; i < m_vertexShaders.size(); ++i)
        glDeleteShader(m_vertexShaders[i]);
    for (auto i = 0u; i < m_fragmentShaders.size(); ++i)
        glDeleteShader(m_fragmentShaders[i]);
}

void Scene::init()
{
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    initCubeVertexArray();

    for (auto i = 0u; i < m_programs.size(); ++i)
    {
        m_programs[i] = glCreateProgram();

        m_vertexShaders[i] = glCreateShader(GL_VERTEX_SHADER);
        m_fragmentShaders[i] = glCreateShader(GL_FRAGMENT_SHADER);

        glAttachShader(m_programs[i], m_vertexShaders[i]);
        glAttachShader(m_programs[i], m_fragmentShaders[i]);

        glBindFragDataLocation(m_programs[i], 0, "out_color");
    }

    loadShaders();

    m_object = make_unique<Icosahedron>(3);
}

void Scene::initCubeVertexArray()
{
    const auto vertices = cubeVertices();

    glGenVertexArrays(m_vaos.size(), m_vaos.data());
    glBindVertexArray(m_vaos[0]);

    glGenBuffers(m_vbos.size(), m_vbos.data());
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, 0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[0]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, reinterpret_cast<const void *>(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

bool Scene::loadShaders()
{
    static const auto sourceFiles = std::array<std::string, 2>{{
        "data/stereoscopic_rendering/icosahedron.vert",
        "data/stereoscopic_rendering/icosahedron.frag", }};

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
    m_uniformLocations[2] = glGetUniformLocation(m_programs[0], "color");
    glUseProgram(0);
}

void Scene::render(const glm::mat4 & view, const glm::mat4 & projection)
{
    glUseProgram(m_programs[0]);

    glUniformMatrix4fv(m_uniformLocations[1], 1, GL_FALSE, glm::value_ptr(projection));

    // draw

    auto color = glm::vec3(0.80, 0.78, 0.27);
    glUniform3fv(m_uniformLocations[2], 1, glm::value_ptr(color));

	static const auto icosahedronTranslations = std::array<glm::vec3, 1u>{ {
		glm::vec3(2.0f, 3.0f, 4.0f)}};

	for (auto i = 0u; i < icosahedronTranslations.size(); ++i)
	{
		auto model = glm::translate(icosahedronTranslations[i]);
		glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(view * model));

		m_object->draw();
	}

    color = glm::vec3(0.60, 0.60, 0.51);
    glUniform3fv(m_uniformLocations[2], 1, glm::value_ptr(color));

	static const auto cubeTranslations = std::array<glm::vec3, 2u>{ {
		glm::vec3(0.0f, 4.0f, 0.0),
        glm::vec3(2.0f, 6.0f, 4.0f)}};

	static const auto cubeScalings = std::array<glm::vec3, 2u>{ {
		glm::vec3(16.0f, 8.0f, 20.0),
        glm::vec3(0.07f, 6.0f, 0.07f)}};

	for (auto i = 0u; i < cubeTranslations.size(); ++i)
	{
		auto model = glm::translate(cubeTranslations[i]) * glm::scale(cubeScalings[i]);
		glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(view * model));

		glBindVertexArray(m_vaos[0]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
		glBindVertexArray(0);
	}

    static const auto chair = chairMatrices();
    static const auto table = tableMatrices();

    static const auto tableChairPairs = std::array<glm::vec3, 6u>{ {
        glm::vec3(-0.0f, 0.0f, 0.0f),
        glm::vec3(-2.0f, 0.0f, 0.0f),
        glm::vec3(-4.0f, 0.0f, 0.0f),
        glm::vec3(-0.0f, 0.0f, 3.0f),
        glm::vec3(-2.0f, 0.0f, 3.0f),
        glm::vec3(-4.0f, 0.0f, 3.0f)}};

    color = glm::vec3(1.00, 0.53, 0.44);
    glUniform3fv(m_uniformLocations[2], 1, glm::value_ptr(color));

    for (auto j = 0u; j < tableChairPairs.size(); ++j)
    {
        const auto translation = glm::translate(tableChairPairs[j]);
        for (auto i = 0u; i < chair.size(); ++i)
        {
            auto model = translation * chair[i];
            glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(view * model));

            glBindVertexArray(m_vaos[0]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
            glBindVertexArray(0);
        }

        for (auto i = 0u; i < table.size(); ++i)
        {
            auto model = translation * glm::translate(glm::vec3(0.0f, 0.0f, 1.0f)) * table[i];
            glUniformMatrix4fv(m_uniformLocations[0], 1, GL_FALSE, glm::value_ptr(view * model));

            glBindVertexArray(m_vaos[0]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
            glBindVertexArray(0);
        }
    }

    glUseProgram(0);
}
