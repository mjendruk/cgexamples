
#pragma once

// Oculus SDK for rendering to the Oculus Rift
#include "OVR_CAPI_GL.h"
#include "Extras/OVR_Math.h"
#include <Extras/OVR_StereoProjection.h>

#include <array>
#include <functional>
#include <memory>

#include <glbinding/gl/types.h>

#include "abstractrenderer.h"


class Scene;

class EyeFramebuffer
{
public:
    using Pair = std::array<std::unique_ptr<EyeFramebuffer>, ovrEye_Count>;

    static Pair createPair(ovrSession session, const ovrHmdDesc & hmdDesc, bool * ok);

public:
	EyeFramebuffer(ovrSession session, const ovrSizei & textureSize);
	~EyeFramebuffer();

	bool init();

	void bindAndClear();
	void unbind();
	void commit();

	ovrTextureSwapChain textureChain();
	ovrSizei size() const;

private:
	ovrSession m_session;

	ovrSizei m_size;
	ovrTextureSwapChain m_textureChain;
	gl::GLuint m_depthBuffer;
	gl::GLuint m_framebuffer;
};


class MirrorFramebuffer
{
public:
	MirrorFramebuffer(ovrSession session, const ovrSizei & windowSize);
	~MirrorFramebuffer();

	bool init();

	void blit(gl::GLuint destFramebuffer);

private:
	ovrSession m_session;
	ovrSizei m_size;
	ovrMirrorTexture m_texture;
	gl::GLuint m_framebuffer;
};


class OculusRiftRenderer : public AbstractRenderer
{
public:
    OculusRiftRenderer();
    ~OculusRiftRenderer();

    bool init() override;
    bool render(Scene & scene) override;
    void setSize(const glm::ivec2 & size) override;

private:
    std::array<ovrPosef, ovrEye_Count> queryEyePoses(double * sampleTime);
    bool prepareLayerAndSubmit(const std::array<ovrPosef, ovrEye_Count> & eyePoses, double sampleTime);

private:
    static OVR::Matrix4f getViewMatrixForPose(const ovrPosef & pose);
    static OVR::Matrix4f getProjectionMatrixForFOV(const ovrFovPort & fov);
    
private:
    ovrSession m_session;
    ovrHmdDesc m_hmdDesc;
    long long m_frameIndex;

    EyeFramebuffer::Pair m_eyeFramebuffers;
    std::unique_ptr<MirrorFramebuffer> m_mirrorFramebuffer;
};
