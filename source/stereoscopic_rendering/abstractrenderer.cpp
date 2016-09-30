
#include "abstractrenderer.h"


AbstractRenderer::AbstractRenderer()
:   m_zNear(0.2f)
,   m_zFar(100.0f)
{
}

void AbstractRenderer::setClippingPlanes(float zNear, float zFar)
{
    m_zNear = zNear;
    m_zFar = zFar;
}

float AbstractRenderer::zNear() const
{
    return m_zNear;
}

float AbstractRenderer::zFar() const
{
    return m_zFar;
}
