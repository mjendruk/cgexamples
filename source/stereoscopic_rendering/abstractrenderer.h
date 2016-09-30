
#pragma once

#include <glm/fwd.hpp>


class Scene;

class AbstractRenderer
{
public:
    AbstractRenderer();

    virtual bool init() = 0;
    virtual bool render(Scene & scene) = 0;
    virtual void setSize(const glm::ivec2 & size) = 0;
    
    void setClippingPlanes(float zNear, float zFar);

protected:
    float zNear() const;
    float zFar() const;

private:
    float m_zNear, m_zFar;
};
