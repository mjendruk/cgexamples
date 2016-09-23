
#pragma once

#include <glm/fwd.hpp>


class Scene;

class AbstractRenderer
{
public:
    virtual bool init() = 0;
    virtual bool render(Scene & scene) = 0;
    virtual void setSize(const glm::ivec2 & size) = 0;
};
