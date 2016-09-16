
#pragma once

#include "OVR_CAPI_GL.h"

#include <glbinding/gl32core/types.h>


class EyeFramebuffer
{
public:
	EyeFramebuffer(ovrSession session, const ovrSizei & textureSize);
	~EyeFramebuffer();

	bool valid() const;

	void bindAndClear();
	void unbind();
	void commit();

private:
	bool m_valid;
	ovrSession m_session;
	ovrSizei m_size;
	ovrTextureSwapChain m_textureChain;
	gl32::GLuint m_depthBuffer;
	gl32::GLuint m_framebuffer;
};
