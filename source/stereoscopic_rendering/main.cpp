
#include <iostream>

// C++ library for creating windows with OpenGL contexts and receiving 
// input and events http://www.glfw.org/ 
#include <GLFW/glfw3.h> 

// Oculus SDK for rendering to the Oculus Rift
#include "OVR_CAPI_GL.h"
#include "Extras/OVR_Math.h"

#if defined(_WIN32)
    #include <dxgi.h> // for GetDefaultAdapterLuid
    #pragma comment(lib, "dxgi.lib")
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// C++ binding for the OpenGL API. 
// https://github.com/cginternals/glbinding
#include <glbinding/Binding.h>

#include <cgutils/common.h>

#include "eyeframebuffer.h"
#include "scene.h"


// From http://en.cppreference.com/w/cpp/language/namespace:
// "Unnamed namespace definition. Its members have potential scope 
// from their point of declaration to the end of the translation
// unit, and have internal linkage."
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

glm::vec3 toGlm(const OVR::Vector3f & vec)
{
	return glm::vec3(vec.x, vec.y, vec.z);
}

glm::mat4 toGlm(const OVR::Matrix4f & mat)
{
	return glm::mat4(
		mat.M[0][0], mat.M[1][0], mat.M[2][0], mat.M[3][0],
		mat.M[0][1], mat.M[1][1], mat.M[2][1], mat.M[3][1],
		mat.M[0][2], mat.M[1][2], mat.M[2][2], mat.M[3][2],
		mat.M[0][3], mat.M[1][3], mat.M[2][3], mat.M[3][3]);
}

Scene example;

// "The size callback ... which is called when the window is resized."
// http://www.glfw.org/docs/latest/group__window.html#gaa40cd24840daa8c62f36cafc847c72b6
void resizeCallback(GLFWwindow * window, int width, int height)
{
    example.resize(width, height);
}

// "The key callback ... which is called when a key is pressed, repeated or released."
// http://www.glfw.org/docs/latest/group__input.html#ga7e496507126f35ea72f01b2e6ef6d155
void keyCallback(GLFWwindow * /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_RELEASE)
        return;

    switch (key)
    {
    case GLFW_KEY_F5:
        example.loadShaders();
        break;
    }
}


// "In case a GLFW function fails, an error is reported to the GLFW 
// error callback. You can receive these reports with an error
// callback." http://www.glfw.org/docs/latest/quick.html#quick_capture_error
void errorCallback(int errnum, const char * errmsg)
{
    std::cerr << errnum << ": " << errmsg << std::endl;
}

}

int main(int /*argc*/, char ** /*argv*/)
{
    auto result = ovr_Initialize(nullptr);

    if (OVR_FAILURE(result))
    {
        return 1;
    }

	ovrSession session;
	ovrGraphicsLuid luid;
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		ovr_Shutdown();
		return 2;
	}

	if (Compare(luid, GetDefaultAdapterLuid())) // If luid that the Rift is on is not the default adapter LUID...
	{
		ovr_Destroy(session);
		ovr_Shutdown();
		return 3; // OpenGL supports only the default graphics adapter.
	}

    if (!glfwInit())
    {
		ovr_Destroy(session);
		ovr_Shutdown();
        return 4;
    }

    glfwSetErrorCallback(errorCallback);

    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	auto hmdDesc = ovr_GetHmdDesc(session);
	auto windowSize = ovrSizei{hmdDesc.Resolution.w/2, hmdDesc.Resolution.h/2};

    GLFWwindow * window = glfwCreateWindow(windowSize.w, windowSize.h, "", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
		ovr_Destroy(session);
		ovr_Shutdown();
        return 5;
    }

    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    std::cout << "Sky Triangle (no Skybox)" << std::endl << std::endl;

    std::cout << "Key Binding: " << std::endl
        << "  [F5] reload shaders" << std::endl
        << std::endl;

    glfwMakeContextCurrent(window);

	glbinding::Binding::initialize(false);

    std::array<std::unique_ptr<EyeFramebuffer>, 2u> eyeFramebuffers;

    for (auto eye = 0u; eye < 2u; ++eye)
    {
        const auto idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
		eyeFramebuffers[eye] = std::make_unique<EyeFramebuffer>(session, idealTextureSize);

		if (!eyeFramebuffers[eye]->valid())
		{
			eyeFramebuffers = {};
			glfwTerminate();
			ovr_Destroy(session);
			ovr_Shutdown();
			return 6;
		}
    }

	// Turn off vsync to let the compositor do its magic
	// wglSwapIntervalEXT(0);

    example.resize(hmdDesc.Resolution.w, hmdDesc.Resolution.h);
    example.initialize();

    while (!glfwWindowShouldClose(window)) // main loop
    {
		auto frameIndex = 0ll;

        glfwPollEvents();

		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(session, &sessionStatus);
		if (sessionStatus.ShouldQuit)
		{
			break;
		}

		if (sessionStatus.ShouldRecenter)
			ovr_RecenterTrackingOrigin(session);

		if (sessionStatus.IsVisible)
		{
			// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
			ovrEyeRenderDesc eyeRenderDesc[2];
			eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
			eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

			// Get eye poses, feeding in correct IPD offset
			ovrPosef EyeRenderPose[2];
			ovrVector3f HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset, eyeRenderDesc[1].HmdToEyeOffset };

			auto sensorSampleTime = 0.0;    // sensorSampleTime is fed into the layer later
			ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffset, EyeRenderPose, &sensorSampleTime);

			for (auto eye = 0u; eye < 2u; ++eye)
			{
				eyeFramebuffers[eye]->bindAndClear();

				// Get view and projection matrices
				OVR::Matrix4f rollPitchYaw = OVR::Matrix4f(EyeRenderPose[eye].Orientation);
				OVR::Vector3f finalUp = rollPitchYaw.Transform(OVR::Vector3f(0, 1, 0));
				OVR::Vector3f finalForward = rollPitchYaw.Transform(OVR::Vector3f(0, 0, -1));
				OVR::Vector3f shiftedEyePos = rollPitchYaw.Transform(EyeRenderPose[eye].Position);

				const auto view = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
				const auto projection = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);

				example.render(toGlm(view), toGlm(projection));

				eyeFramebuffers[eye]->unbind();

				eyeFramebuffers[eye]->commit();
			}

			// Do distortion rendering, Present and flush/sync

			ovrLayerEyeFov ld;
			ld.Header.Type = ovrLayerType_EyeFov;
			ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

			for (int eye = 0; eye < 2; ++eye)
			{
				ld.ColorTexture[eye] = eyeFramebuffers[eye]->textureChain();
				ld.Viewport[eye] = { ovrVector2i{}, eyeFramebuffers[eye]->size() };
				ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
				ld.RenderPose[eye] = EyeRenderPose[eye];
				ld.SensorSampleTime = sensorSampleTime;
			}

			auto layers = &ld.Header;
			result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);

			if (OVR_FAILURE(result))
			{
				break;
			}

			++frameIndex;
		}

        // glfwSwapBuffers(window);
    }

	eyeFramebuffers = {};

    glfwMakeContextCurrent(nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

	ovr_Destroy(session);
    ovr_Shutdown();

    return 0;
}
