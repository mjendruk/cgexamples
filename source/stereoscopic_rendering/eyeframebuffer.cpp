
#include "eyeframebuffer.h"

#include <cassert>

#include <glbinding/gl32core/gl.h>


using namespace gl32core;

EyeFramebuffer::Pair EyeFramebuffer::createPair(ovrSession session, const ovrHmdDesc & hmdDesc, bool * ok)
{
    Pair pair;

    for (auto eye = 0; eye < ovrEye_Count; ++eye)
    {
        const auto idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
        pair[eye] = std::make_unique<EyeFramebuffer>(session, idealTextureSize);

        if (!pair[eye]->init())
        {   
            *ok = false;
            return Pair();
        }
    }

    *ok = true;
    return pair;
}

EyeFramebuffer::EyeFramebuffer(ovrSession session, const ovrSizei & textureSize)
:   m_session(session)
,   m_size(textureSize)
{
};

EyeFramebuffer::~EyeFramebuffer()
{
    ovr_DestroyTextureSwapChain(m_session, m_textureChain);
    glDeleteTextures(1, &m_depthBuffer);
}

bool EyeFramebuffer::init()
{
    auto desc = ovrTextureSwapChainDesc();
    desc.Type = ovrTexture_2D;
    desc.ArraySize = 1;
    desc.Width = m_size.w;
    desc.Height = m_size.h;
    desc.MipLevels = 1;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.SampleCount = 1;
    desc.StaticImage = ovrFalse;

    auto result = ovr_CreateTextureSwapChainGL(m_session, &desc, &m_textureChain);

    if (OVR_FAILURE(result))
        return false;

    auto length = 0;
    ovr_GetTextureSwapChainLength(m_session, m_textureChain, &length);

    for (auto i = 0; i < length; ++i)
    {
        auto texture = GLuint();
        ovr_GetTextureSwapChainBufferGL(m_session, m_textureChain, i, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glGenTextures(1, &m_depthBuffer);
    glBindTexture(GL_TEXTURE_2D, m_depthBuffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_size.w, m_size.h, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthBuffer, 0);

    return true;
}

void EyeFramebuffer::bindAndClear()
{
    auto currentTexture = GLuint();
    auto currentIndex = 0;
    ovr_GetTextureSwapChainCurrentIndex(m_session, m_textureChain, &currentIndex);
    ovr_GetTextureSwapChainBufferGL(m_session ,m_textureChain, currentIndex, &currentTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currentTexture, 0);

    glViewport(0, 0, m_size.w, m_size.h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
};

void EyeFramebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EyeFramebuffer::commit()
{
    ovr_CommitTextureSwapChain(m_session, m_textureChain);
}

ovrTextureSwapChain EyeFramebuffer::textureChain()
{
    return m_textureChain;
}

ovrSizei EyeFramebuffer::size() const
{
    return m_size;
}

MirrorFramebuffer::MirrorFramebuffer(ovrSession session, const ovrSizei & windowSize)
:   m_session(session)
,   m_size(windowSize)
,   m_texture(nullptr)
,   m_framebuffer(0u)
{
}

MirrorFramebuffer::~MirrorFramebuffer()
{
    glDeleteFramebuffers(1, &m_framebuffer);
    ovr_DestroyMirrorTexture(m_session, m_texture);
}

bool MirrorFramebuffer::init()
{
    auto mirrorDesc = ovrMirrorTextureDesc();
    mirrorDesc.Width = m_size.w;
    mirrorDesc.Height = m_size.h;
    mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

    auto result = ovr_CreateMirrorTextureGL(m_session, &mirrorDesc, &m_texture);
    if (OVR_FAILURE(result))
        return false;

	auto textureID = GLuint{0u};
    ovr_GetMirrorTextureBufferGL(m_session, m_texture, &textureID);
    
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return true;
}

void MirrorFramebuffer::blit(GLuint destFramebuffer)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destFramebuffer);
    auto w = static_cast<GLint>(m_size.w);
    auto h = static_cast<GLint>(m_size.h);
    glBlitFramebuffer(0, h, w, 0, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

HeadTracking::HeadTracking(ovrSession session, const ovrHmdDesc & hmdDesc)
:   m_session(session)
,   m_hmdDesc(hmdDesc)
,   m_sampleTime(0.0)
{
}

std::array<ovrPosef, 2u> HeadTracking::queryEyePoses(long long frameIndex)
{
    // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
    auto descriptors = std::array<ovrEyeRenderDesc, ovrEye_Count>();
    
    for (auto eye = 0; eye < ovrEye_Count; ++eye)
        descriptors[eye] = ovr_GetRenderDesc(m_session, static_cast<ovrEyeType>(eye), m_hmdDesc.DefaultEyeFov[eye]);

    // Get eye poses, feeding in correct IPD offset
    auto poses = std::array<ovrPosef, ovrEye_Count>();
    const auto offsets = std::array<ovrVector3f, ovrEye_Count>{{
        descriptors[0].HmdToEyeOffset,
        descriptors[1].HmdToEyeOffset}};

    ovr_GetEyePoses(m_session, frameIndex, ovrTrue, offsets.data(), poses.data(), &m_sampleTime);
    
    return poses;
}

double HeadTracking::latestSampleTime() const
{
    return m_sampleTime;
}

OVR::Matrix4f getViewMatrixForPose(const ovrPosef & pose)
{
    static auto yaw = 3.141592f;

    // align OVR and OpenGL coordinate systems
    OVR::Matrix4f rollPitchYaw = OVR::Matrix4f::RotationY(yaw);
    OVR::Matrix4f finalRollPitchYaw = rollPitchYaw * OVR::Matrix4f(pose.Orientation);
    OVR::Vector3f finalUp = finalRollPitchYaw.Transform(OVR::Vector3f(0.0f, 1.0f, 0.0f));
    OVR::Vector3f finalForward = finalRollPitchYaw.Transform(OVR::Vector3f(0.0f, 0.0f, -1.0f));
    OVR::Vector3f shiftedEyePos = rollPitchYaw.Transform(pose.Position);

    const auto view = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);

    return view;
}

OVR::Matrix4f getProjectionMatrixForFOV(const ovrFovPort & fov)
{
    return ovrMatrix4f_Projection(fov, 0.2f, 1000.0f, ovrProjection_None);
}
