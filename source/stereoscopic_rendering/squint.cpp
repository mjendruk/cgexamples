
#include "squint.h"

#include <glm/gtc/matrix_transform.hpp>

#include <glbinding/gl32core/gl.h>

#include "scene.h"


using namespace gl32core;

SquintRenderer::SquintRenderer() = default;

bool SquintRenderer::init()
{
    return true;
}

bool SquintRenderer::render(Scene & scene)
{   
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    const auto width = m_size.x / 2;
    const auto height = m_size.y;
    const auto aspectRatio = static_cast<float>(width) / height;

    for (auto i = 0; i < 2; ++i)
    {
        const auto xOffset = i * width;
        const auto isLeft = i == 0 ? false : true;

        glViewport(xOffset, 0, width, height);

        static const auto iod = 0.0635f;
        static const auto fov = 90.0f;
        static const auto viewportDepth = 2.0f;
        static const auto zNear = 0.02f;
        static const auto zFar = 100.0f;
        const auto projection = getProjectionMatrix(iod, fov, aspectRatio, 
            viewportDepth, zNear, zFar, isLeft);

        static const auto eye = glm::vec3(0.0f, 1.8f, 0.0f);
        static const auto center = glm::vec3(0.0f, 1.6f, 2.0f);
        static const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
        const auto view = getViewMatrix(eye, center, up, iod, isLeft);

        scene.render(view, projection);
    }

    return true;
}

void SquintRenderer::setSize(const glm::ivec2 & size)
{
    m_size = size;
}

glm::mat4 SquintRenderer::getViewMatrix(const glm::vec3 & eye, const glm::vec3 & center, 
    const glm::vec3 & up, float iod, bool isLeft)
{
    const auto shiftDirection = isLeft ? 1.0f : -1.0f;
    const auto orientedHalfIod = glm::vec3((iod * 0.5f) * shiftDirection, 0.0f, 0.0f);

    return glm::lookAt(eye + orientedHalfIod, center + orientedHalfIod, up);
}

// https://www.packtpub.com/books/content/rendering-stereoscopic-3d-models-using-opengl
// http://relativity.net.au/gaming/java/Frustum.html
glm::mat4 SquintRenderer::getProjectionMatrix(float iod, float fov, float aspectRatio,
    float viewportDepth, float zNear, float zFar, bool isLeft)
{
    const auto shiftDirection = isLeft ? 1.0f : -1.0f;
    const auto frustumShift = (iod * 0.5f) * zNear / viewportDepth;
    const auto orientedShift = frustumShift * shiftDirection;

    const auto top = glm::tan(glm::radians(fov * 0.5f)) * zNear;
    const auto bottom = -top;
    const auto right = aspectRatio * top + orientedShift;
    const auto left = -aspectRatio * top + orientedShift;

    return glm::frustum(left, right, bottom, top, zNear, zFar);
}
