
#pragma once

#include "OVR_CAPI_GL.h"
#include "Extras/OVR_Math.h"

#include <array>
#include <memory>

#include <glbinding/gl/types.h>


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

std::array<ovrPosef, ovrEye_Count> queryEyePoses(ovrSession session, long long frameIndex, double * sampleTime);

OVR::Matrix4f getViewMatrixForPose(const ovrPosef & pose);
OVR::Matrix4f getProjectionMatrixForFOV(const ovrFovPort & fov);
