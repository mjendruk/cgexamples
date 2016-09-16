
#include "eyeframebuffer.h"

#include <cassert>

#include <glbinding/gl32core/gl.h>


using namespace gl32core;

EyeFramebuffer::EyeFramebuffer(ovrSession session, const ovrSizei & textureSize)
:	m_session(session)
,	m_size(textureSize)
,	m_valid(true)
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
    {
    	m_valid = false;
    	return;
    }

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
};

EyeFramebuffer::~EyeFramebuffer()
{
	ovr_DestroyTextureSwapChain(m_session, m_textureChain);
	glDeleteTextures(1, &m_depthBuffer);
}

bool EyeFramebuffer::valid() const
{
	return m_valid;
};

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
