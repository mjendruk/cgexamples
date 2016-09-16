
#pragma once

#include "OVR_CAPI_GL.h"

#include <glbinding/gl/types.h>


class EyeFramebuffer
{
public:
	EyeFramebuffer(ovrSession session, const ovrSizei & textureSize);
	~EyeFramebuffer();

	bool valid() const;

	void bindAndClear();
	void unbind();
	void commit();

	ovrTextureSwapChain textureChain();
	ovrSizei size() const;

private:
	bool m_valid;
	ovrSession m_session;
	ovrSizei m_size;
	ovrTextureSwapChain m_textureChain;
	gl::GLuint m_depthBuffer;
	gl::GLuint m_framebuffer;
};
