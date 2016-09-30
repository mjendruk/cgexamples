
#pragma once

#include <memory>

#include <glm/fwd.hpp>

#include <glbinding/gl41core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 4.1 Core symbols

class Icosahedron;

// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class Scene
{
public:
	Scene();
    ~Scene();
    
    void init();
    bool loadShaders();

    void render(const glm::mat4 & view, const glm::mat4 & projection);

protected:
    void initCubeVertexArray();
    void loadUniformLocations();

protected:
    std::unique_ptr<Icosahedron> m_object;

    std::array<gl41core::GLuint, 1> m_vaos;
    std::array<gl41core::GLuint, 1> m_vbos;

    std::array<gl41core::GLuint, 1> m_programs;
    std::array<gl41core::GLuint, 1> m_vertexShaders;
    std::array<gl41core::GLuint, 1> m_fragmentShaders;

    std::array<gl41core::GLuint, 3> m_uniformLocations;

    int m_width;
    int m_height;
};
