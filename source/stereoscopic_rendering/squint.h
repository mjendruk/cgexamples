
#pragma once

#include <glm/glm.hpp>

#include <glbinding/gl/types.h>

#include "abstractrenderer.h"


class Scene;

class SquintRenderer : public AbstractRenderer
{
public:
    SquintRenderer();

    bool init() override;
    bool render(Scene & scene) override;
    void setSize(const glm::ivec2 & size) override;
    
private:
    static glm::mat4 getViewMatrix(const glm::vec3 & eye, const glm::vec3 & center, 
        const glm::vec3 & up, float iod, bool isLeft);
    static glm::mat4 getProjectionMatrix(float iod, float fov, float aspectRatio,
        float viewportDepth, float zNear, float zFar, bool isLeft);

private:
    glm::ivec2 m_size;
};
