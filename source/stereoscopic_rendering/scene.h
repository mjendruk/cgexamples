
#pragma once

#include <memory>

#include <glm/fwd.hpp>

#include <glbinding/gl32core/gl.h>  // this is a OpenGL feature include; it declares all OpenGL 3.2 Core symbols

class Icosahedron;

// For more information on how to write C++ please adhere to: 
// http://cginternals.github.io/guidelines/cpp/index.html

class Scene
{
public:
	Scene();
    ~Scene();
    
    void initialize();
    bool loadShaders();

    void resize(int w, int h);
    void render(const glm::mat4 & view, const glm::mat4 & projection);

protected:
    void loadUniformLocations();

protected:
    std::array<gl::GLuint, 1> m_programs;
    std::array<gl::GLuint, 1> m_vertexShaders;
    std::array<gl::GLuint, 1> m_fragmentShaders;

    std::array<gl::GLuint, 2> m_uniformLocations;

	std::unique_ptr<Icosahedron> m_object;

    int m_width;
    int m_height;
};
