
#include "ovr.h"

#include <cassert>

#if defined(_WIN32)
    #include <dxgi.h> // for GetDefaultAdapterLuid
    #pragma comment(lib, "dxgi.lib")
#endif

#include <glm/glm.hpp>

#include <glbinding/gl32core/gl.h>

#include "Scene.h"

using namespace gl32core;

namespace
{

ovrGraphicsLuid GetDefaultAdapterLuid()
{
    ovrGraphicsLuid luid = ovrGraphicsLuid();

#if defined(_WIN32)
    IDXGIFactory* factory = nullptr;

    if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
    {
        IDXGIAdapter* adapter = nullptr;

        if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
        {
            DXGI_ADAPTER_DESC desc;

            adapter->GetDesc(&desc);
            memcpy(&luid, &desc.AdapterLuid, sizeof(luid));
            adapter->Release();
        }

        factory->Release();
    }
#endif

    return luid;
}

int Compare(const ovrGraphicsLuid & lhs, const ovrGraphicsLuid & rhs)
{
    return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}

glm::mat4 toGlm(const OVR::Matrix4f & mat)
{
    return glm::mat4(
        mat.M[0][0], mat.M[1][0], mat.M[2][0], mat.M[3][0],
        mat.M[0][1], mat.M[1][1], mat.M[2][1], mat.M[3][1],
        mat.M[0][2], mat.M[1][2], mat.M[2][2], mat.M[3][2],
        mat.M[0][3], mat.M[1][3], mat.M[2][3], mat.M[3][3]);
}

}

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

    const auto result = ovr_CreateTextureSwapChainGL(m_session, &desc, &m_textureChain);

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

// Avoids an error when calling SetAndClearRenderSurface during next iteration.
// Without this, during the next while loop iteration SetAndClearRenderSurface
// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
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

    const auto result = ovr_CreateMirrorTextureGL(m_session, &mirrorDesc, &m_texture);
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
    const auto w = static_cast<GLint>(m_size.w);
    const auto h = static_cast<GLint>(m_size.h);
    glBlitFramebuffer(0, h, w, 0, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

OculusRiftRenderer::OculusRiftRenderer()
:   m_session(nullptr)
,   m_frameIndex(0ll)
{
}

OculusRiftRenderer::~OculusRiftRenderer()
{
    m_eyeFramebuffers = {};
    m_mirrorFramebuffer = nullptr;

    if (m_session)
        ovr_Destroy(m_session);

    ovr_Shutdown();
}

bool OculusRiftRenderer::init()
{
    auto result = ovr_Initialize(nullptr);

    if (OVR_FAILURE(result))
        return false;

    auto luid = ovrGraphicsLuid();
    result = ovr_Create(&m_session, &luid);

    if (OVR_FAILURE(result))
        return false;

    if (Compare(luid, GetDefaultAdapterLuid())) // If luid that the Rift is on is not the default adapter LUID...
        return false; // OpenGL supports only the default graphics adapter.

    m_hmdDesc = ovr_GetHmdDesc(m_session);

    auto ok = true;
    m_eyeFramebuffers = EyeFramebuffer::createPair(m_session, m_hmdDesc, &ok);

    if (!ok)
        return false;

    const auto windowSize = ovrSizei{ m_hmdDesc.Resolution.w / 2, m_hmdDesc.Resolution.h / 2};
    m_mirrorFramebuffer = std::make_unique<MirrorFramebuffer>(m_session, windowSize);

    if (!m_mirrorFramebuffer->init())
        return false;

    ovr_SetTrackingOriginType(m_session, ovrTrackingOrigin_FloorLevel);

    return true;
}

bool OculusRiftRenderer::render(Scene & scene)
{
    auto sessionStatus = ovrSessionStatus();
    ovr_GetSessionStatus(m_session, &sessionStatus);

    if (sessionStatus.ShouldQuit)
        return false;

    if (sessionStatus.ShouldRecenter)
        ovr_RecenterTrackingOrigin(m_session);

    if (sessionStatus.IsVisible)
    {
        auto sampleTime = 0.0;

        const auto eyePoses = queryEyePoses(&sampleTime);

        for (auto eye = 0; eye < ovrEye_Count; ++eye)
        {
            const auto & framebuffer = m_eyeFramebuffers[eye];

            framebuffer->bindAndClear();

            const auto view = getViewMatrixForPose(eyePoses[eye]);
            const auto projection = getProjectionMatrixForFOV(m_hmdDesc.DefaultEyeFov[eye]);

            scene.render(toGlm(view), toGlm(projection));

            framebuffer->unbind();
            framebuffer->commit();
        }

        prepareLayerAndSubmit(eyePoses, sampleTime);

        ++m_frameIndex;
    }

    // Blit mirror texture to back buffer
    m_mirrorFramebuffer->blit(0);

    return true;
}

void OculusRiftRenderer::setSize(const glm::ivec2 & size)
{
}

std::array<ovrPosef, ovrEye_Count> OculusRiftRenderer::queryEyePoses(double * sampleTime)
{
    // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
    auto descriptors = std::array<ovrEyeRenderDesc, ovrEye_Count>();

    const auto hmdDesc = ovr_GetHmdDesc(m_session);

    for (auto eye = 0; eye < ovrEye_Count; ++eye)
        descriptors[eye] = ovr_GetRenderDesc(m_session, static_cast<ovrEyeType>(eye), hmdDesc.DefaultEyeFov[eye]);

    // Get eye poses, feeding in correct IPD offset
    auto poses = std::array<ovrPosef, ovrEye_Count>();
    const auto offsets = std::array<ovrVector3f, ovrEye_Count>{ {
            descriptors[0].HmdToEyeOffset,
                descriptors[1].HmdToEyeOffset}};

    ovr_GetEyePoses(m_session, m_frameIndex, ovrTrue, offsets.data(), poses.data(), sampleTime);

    return poses;
}

bool OculusRiftRenderer::prepareLayerAndSubmit(const std::array<ovrPosef, ovrEye_Count> & eyePoses, double sampleTime)
{
    // Do distortion rendering, present and flush/sync

    auto layer = ovrLayerEyeFov();
    layer.Header.Type = ovrLayerType_EyeFov;
    layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

    for (auto eye = 0; eye < ovrEye_Count; ++eye)
    {
        layer.ColorTexture[eye] = m_eyeFramebuffers[eye]->textureChain();
        layer.Viewport[eye] = { ovrVector2i{}, m_eyeFramebuffers[eye]->size() };
        layer.Fov[eye] = m_hmdDesc.DefaultEyeFov[eye];
        layer.RenderPose[eye] = eyePoses[eye];
        layer.SensorSampleTime = sampleTime;
    }

    const auto layers = &layer.Header;
    const auto result = ovr_SubmitFrame(m_session, m_frameIndex, nullptr, &layers, 1);

    return OVR_SUCCESS(result);
}

OVR::Matrix4f OculusRiftRenderer::getViewMatrixForPose(const ovrPosef & pose)
{
    static auto yaw = 3.141592f;

    // align OVR and OpenGL coordinate systems
    const auto rollPitchYaw = OVR::Matrix4f::RotationY(yaw);
    const auto finalRollPitchYaw = rollPitchYaw * OVR::Matrix4f(pose.Orientation);
    const auto finalUp = finalRollPitchYaw.Transform(OVR::Vector3f(0.0f, 1.0f, 0.0f));
    const auto finalForward = finalRollPitchYaw.Transform(OVR::Vector3f(0.0f, 0.0f, -1.0f));
    const auto shiftedEyePos = rollPitchYaw.Transform(pose.Position);

    const auto view = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);

    return view;
}

OVR::Matrix4f OculusRiftRenderer::getProjectionMatrixForFOV(const ovrFovPort & fov)
{
    return ovrMatrix4f_Projection(fov, 0.2f, 1000.0f, ovrProjection_None);
}
